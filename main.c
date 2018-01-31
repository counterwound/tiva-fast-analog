/******************************************************************
 * Armor Panel Controller (APC) for Power Brick
 * Developed by Sevun Scientific, Inc.
 * http://sevunscientific.com
 * *****************************************************************
 *
 *    _____/\\\\\\\\\\\_______/\\\\\\\\\\\____/\\\\\\\\\\\_
 *     ___/\\\/////////\\\___/\\\/////////\\\_\/////\\\///__
 *      __\//\\\______\///___\//\\\______\///______\/\\\_____
 *       ___\////\\\___________\////\\\_____________\/\\\_____
 *        ______\////\\\___________\////\\\__________\/\\\_____
 *         _________\////\\\___________\////\\\_______\/\\\_____
 *          __/\\\______\//\\\___/\\\______\//\\\______\/\\\_____
 *           _\///\\\\\\\\\\\/___\///\\\\\\\\\\\/____/\\\\\\\\\\\_
 *            ___\///////////_______\///////////_____\///////////__
 *
 * *****************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_adc.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "pinmux.h"

#include "utils/uartstdio.h"

//*****************************************************************************
// Variables
//*****************************************************************************

// Heartbeat signals
uint64_t g_ui64Heartbeat;
uint64_t g_ui64Clock100ms;

uint16_t g_ui16MicroTemp;
uint16_t g_u16Current;

//*****************************************************************************
// Timers Setup
//*****************************************************************************
volatile bool g_bTimer0Flag = 0;        // Timer 0 occurred flag
volatile bool g_bTimer1Flag = 0;        // Timer 1 occurred flag

// Configure Timers
void ConfigureTimers(void)
{
    // Enable the peripherals used by this example.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    // Configure the two 32-bit periodic timers.
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 1);      // 1 Hz
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 10);      // 10 Hz

    // Setup the interrupts for the timer timeouts.
    IntEnable(INT_TIMER0A);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    // Enable the timers.
    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER1_BASE, TIMER_A);
}

// The interrupt handler for the first timer interrupt. 1 Hz
void Timer0IntHandler(void)
{
    // Clear the timer interrupt.
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    g_bTimer0Flag = 1;      // Set flag to indicate Timer 0 interrupt
}

// The interrupt handler for the second timer interrupt. 10 Hz
void Timer1IntHandler(void)
{
    // Clear the timer interrupt.
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    g_bTimer1Flag = 1;      // Set flag to indicate Timer 1 interrupt
}

//*****************************************************************************
// UART and UART Buffer setup
//*****************************************************************************

// Configure UART interrupts
void ConfigureUART(void)
{
    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
}

//*****************************************************************************
// ADC Setup
//*****************************************************************************
void ConfigureADC(void)
{
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 |ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);

    ADCSequenceConfigure(ADC1_BASE, 3, ADC_TRIGGER_PROCESSOR, 1);
    ADCSequenceStepConfigure(ADC1_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC1_BASE, 3);
    ADCIntClear(ADC1_BASE, 3);
}

//*****************************************************************************
// GPIO Setup
//*****************************************************************************
volatile bool g_bBatteryGPIOFlag = 0;       // External Battery Fault input
volatile bool g_bEventGPIOFlag = 0;         // External Event input

// Immediately trigger a message burst if the External fault occurs
void PortCIntHandler(void)
{
    uint32_t ui32StatusGPIOC;
    ui32StatusGPIOC = GPIOIntStatus(GPIO_PORTC_BASE,true);
    GPIOIntClear(GPIO_PORTC_BASE, ui32StatusGPIOC);

    if ( GPIO_INT_PIN_5 == (ui32StatusGPIOC & GPIO_INT_PIN_5) )
    {
        g_bEventGPIOFlag = 1;
    }
}

//*****************************************************************************
// Configure the interrupts
//*****************************************************************************
void ConfigureInterrupts(void)
{
    // Enable processor interrupts.
    IntMasterEnable();

    // Event monitor interrupt
    GPIOPadConfigSet(GPIO_PORTC_BASE,GPIO_PIN_5,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD);
    GPIOIntTypeSet(GPIO_PORTC_BASE,GPIO_PIN_5,GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_5);

    GPIOIntRegister(GPIO_PORTC_BASE,PortCIntHandler);
}

//*****************************************************************************
// Process the incoming data
//*****************************************************************************
void processCommands()
{
    uint32_t ADCValues2[1];

    // Get Temperature value
    ADCProcessorTrigger(ADC1_BASE, 3);
    while(!ADCIntStatus(ADC1_BASE, 3, false)) {}
    ADCIntClear(ADC1_BASE, 3);
    ADCSequenceDataGet(ADC1_BASE, 3, ADCValues2);
    g_ui16MicroTemp = (uint16_t)ADCValues2[0];
}

//*****************************************************************************
// Main code starts here
//*****************************************************************************
int main(void)
{
    // Set the clocking to run directly from the crystal.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
            SYSCTL_XTAL_16MHZ);

    // Initialize and setup the ports
    PortFunctionInit();
    ConfigureTimers();
    ConfigureUART();
    ConfigureADC();
    ConfigureInterrupts();

    //*****************************************************************************
    // UART Setup
    //*****************************************************************************

    UARTStdioConfig(0, 115200, 16000000);

    // Loop forever while the timers run.
    while(1)
    {
        //*****************************************************************************
        // Event
        //*****************************************************************************

        if ( g_bEventGPIOFlag )
        {
            g_bEventGPIOFlag = 0;

            uint32_t ADCValues0[1];

            // Capture rogo value
            ADCProcessorTrigger(ADC0_BASE, 3);
            while(!ADCIntStatus(ADC0_BASE, 3, false)) {}
            ADCIntClear(ADC0_BASE, 3);
            ADCSequenceDataGet(ADC0_BASE, 3, ADCValues0);
            g_u16Current = (uint16_t)ADCValues0[0];
        }

        //*****************************************************************************
        // Timers
        //*****************************************************************************

        // Timer 0
        if ( g_bTimer0Flag )
        {
            g_bTimer0Flag = 0;      // Clear flag
            g_ui64Clock100ms++;     // Bump 100 ms clock counter

            switch ( g_ui64Clock100ms & 0b11)
            {
                case 0b00:
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
                    break;
                case 0b01:
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
                    break;
                case 0b10:
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
                    break;
                case 0b11:
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                    break;
                default:
                    break;
            }
        }

        // Timer 1
        if ( g_bTimer1Flag )
        {
            g_bTimer1Flag = 0;
            g_ui64Heartbeat++;      // Bump the heartbeat counter

            processCommands();

            UARTprintf("\033[2J");
            UARTprintf("\033[0;0H");

            UARTprintf("Temp:    0x%04x\n", g_ui16MicroTemp);
            UARTprintf("Current: 0x%04x\n", g_u16Current);
        }
    }
}
