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
#include "pinmux.h"

#include "utils/uartstdio.h"

#define ADC_SAMPLE_BUF_SIZE 1024


//*****************************************************************************
// Variables
//*****************************************************************************

uint32_t ADCValues[ADC_SAMPLE_BUF_SIZE];
//*****************************************************************************
// Timers Setup
//*****************************************************************************

//*****************************************************************************
// UART and UART Buffer setup
//*****************************************************************************

//*****************************************************************************
// ADC Setup
//*****************************************************************************
void ConfigureADC(void)
{
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 9, ADC_CTL_CH3);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH4);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH5);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH6);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH7 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntClear(ADC0_BASE, 0);
}

//*******************************************************************************
// uDMA Setup
//*****************************************************************************

//*****************************************************************************
// GPIO Setup
//*****************************************************************************

volatile bool g_bEventGPIOFlag = 0;         // External Event input
bool g_bEventStatus = 0;

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

    GPIOPadConfigSet(GPIO_PORTC_BASE,GPIO_PIN_5,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPD);
    GPIOIntTypeSet(GPIO_PORTC_BASE,GPIO_PIN_5,GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_5);

    GPIOIntRegister(GPIO_PORTC_BASE,PortCIntHandler);

}

//*****************************************************************************
// Process the incoming data
//*****************************************************************************

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
    ConfigureADC();
    ConfigureInterrupts();

    //*****************************************************************************
    // UART Setup
    //*****************************************************************************

    // Loop forever while the timers run.
    while(1)
    {
        //*****************************************************************************
        // Event
        //*****************************************************************************

        if(g_bEventGPIOFlag && !g_bEventStatus){
            IntMasterDisable();
            GPIOIntDisable(GPIO_PORTC_BASE, GPIO_INT_PIN_5);
            g_bEventGPIOFlag = 0;
            int i = 0;
            for(i = 0; i < 1024; i += 8){
                ADCProcessorTrigger(ADC0_BASE, 0);
                while(!ADCIntStatus(ADC0_BASE, 0, false)) {}
                ADCIntClear(ADC0_BASE, 0);
                ADCSequenceDataGet(ADC0_BASE, 0, &ADCValues[i]);
            }
            g_bEventStatus = 1;
            GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_5);
            IntMasterEnable();
            GPIOIntEnable(GPIO_PORTC_BASE, GPIO_INT_PIN_5);
        }
        else{
            g_bEventGPIOFlag = 0;
        }
    }
}
