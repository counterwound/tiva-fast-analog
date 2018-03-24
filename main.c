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
#include "driverlib/udma.h"
#include "pinmux.h"

#include "utils/uartstdio.h"

#define ADC_SAMPLE_BUF_SIZE 1024

#define UDMA_CHCTL_XFERMODE_M   0x00000007  // uDMA Transfer Mode

//*****************************************************************************
// Variables
//*****************************************************************************

uint32_t status = 0;

uint16_t DMAControlTable[1024/sizeof(uint32_t)] __attribute__(( aligned(1024) ));

uint16_t ADCValues2[ADC_SAMPLE_BUF_SIZE];
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
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_EXTERNAL, 0);
    GPIOADCTriggerEnable(GPIO_PORTB_BASE,GPIO_PIN_4);

    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 9, ADC_CTL_CH3);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH4);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH5);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH6);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH7 |ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 0);
    ADCIntClear(ADC0_BASE, 0);
}

void ADCprocess(uint32_t ch)
{
    if ((((tDMAControlTable *) DMAControlTable)[ch].ui32Control & UDMA_CHCTL_XFERMODE_M) != UDMA_MODE_STOP) return;
   // uDMAChannelModeGet(ch)
    // store the next buffer in the uDMA transfer descriptor
    // the ADC is read directly into the correct emacBufTx to be transmitted
    uDMAChannelTransferSet(ch, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), ADCValues2, ADC_SAMPLE_BUF_SIZE);
}

void ADC0IntHandler()
{
    // this interrupt handler is optimized to use as few CPU cycles as possible
    // it must handle ADC_SAMPLE_BUF_SIZE samples in 1/125000 seconds
    //
    // optimizations include:
    // 1. choosing ADC_SAMPLE_BUF_SIZE to minimize overhead
    // 2. replacing calls to ROM_* functions with raw memory accesses (less portable)

    status = ADCIntStatus(ADC0_BASE, 0, true);
    ADCIntClear(ADC0_BASE, status);
    *(uint32_t *) (ADC0_BASE + ADC_O_ISC) = ADC_INT_DMA_SS0;    // optimized form of ROM_ADCIntClearEx(ADC0_BASE, ADC_INT_DMA_SS0);

    //ADCprocess(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT);
    //ADCprocess(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);
}

//*******************************************************************************
// uDMA Setup
//*****************************************************************************
void ConfigureUDMA(void){

    // Enable the UDMA peripheral
    //
//    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    //
    // Wait for the UDMA module to be ready.
    //
//    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
    {
    }
    //
    // Enable the uDMA controller.
    //
//    uDMAEnable();
    //
    // Set the base for the channel control table.
    //
//    uDMAControlBaseSet(&DMAControlTable[0]);
//    ADCSequenceDMAEnable(ADC0_BASE, 0);

//    uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0, UDMA_ATTR_ALTSELECT /*start with ping-pong PRI side*/ |
//            UDMA_ATTR_HIGH_PRIORITY /*low priority*/ | UDMA_ATTR_REQMASK /*unmask*/);
        // enable some bits
//    uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0, UDMA_ATTR_USEBURST /*only allow burst transfers*/);

//    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1024);
//    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1024);
    //uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), ADCValues2, ADC_SAMPLE_BUF_SIZE);
    //uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void *)(ADC0_BASE + ADC_O_SSFIFO0), ADCValues2, ADC_SAMPLE_BUF_SIZE);
    //IntEnable(INT_ADC0SS0);

    //ADCIntEnableEx(ADC0_BASE, ADC_INT_DMA_SS0);
//    uDMAChannelEnable(UDMA_CHANNEL_ADC0);

}



//*****************************************************************************
// GPIO Setup
//*****************************************************************************

//*****************************************************************************
// Configure the interrupts
//*****************************************************************************
void ConfigureInterrupts(void)
{
    // Enable processor interrupts.
    IntMasterEnable();

    GPIOPadConfigSet(GPIO_PORTB_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPD);
    GPIOIntTypeSet(GPIO_PORTB_BASE,GPIO_PIN_4,GPIO_FALLING_EDGE);
    //GPIOADCTriggerEnable(GPIO_PORTF_BASE, GPIO_PIN_4);

    ADCIntEnable(ADC0_BASE, 0);
    ADCIntRegister(ADC0_BASE,0,ADC0IntHandler);

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
    ConfigureUDMA();
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
        status = ADCIntStatus(ADC0_BASE, 0, true);
    }
}
