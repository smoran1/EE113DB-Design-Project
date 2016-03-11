/**
 * \file  transmit_timer.c
 *
 * \desc. example of application of using an interrupt to create a square wave
 *		  OR transmit bits via GPIO signal (one bit per clock cycle / interrupt)
 *
 * \brief application of timerCounter.c example provided by TI. 
 *
 * Coded by Steven Moran and Cheuk Yu (2016, Mar.).
 */

/*
* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "interrupt.h"
#include "soc_OMAPL138.h"
#include "hw_syscfg0_OMAPL138.h"
#include "timer.h"
#include "lcdkOMAPL138.h"
#include "L138_LCDK_aic3106_init.h"
#include "L138_LCDK_switch_led.h"

/******************************************************************************
**                      INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define TMR_PERIOD_LSB32               (0x0005FFF) //set frequency here, determine using oscilloscope
#define TMR_PERIOD_MSB32               (0x0)

/******************************************************************************
**                      INTERNAL FUNCTION PROTOTYPES
*******************************************************************************/
static void TimerIsr(void);
static void TimerSetUp64Bit(void);
static void TimerIntrSetUp(void);

/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/
int state = 0;
int transmit = 0; //flag to start transmission
int LEN;
int header[9];
int message[1024];
int headCount = 0;
int messageCount = 0;
int squareON = 0; //flag to turn on/off square wave

/******************************************************************************
**                          FUNCTION DEFINITIONS
*******************************************************************************/


int main(void)
{

     LCDK_GPIO812_init();
     LCDK_GPIO810_init();
	 LCDK_GPIO812_on();


     TimerSetUp64Bit(); // Set up the Timer2 peripheral
     TimerIntrSetUp();  // Set up the AINTC to generate Timer2 interrupts

	 /*INSERT EXAMPLE CODE...*/
     /*Transmission intialization*/
     int x = 0;

     //Create transmission header
     for (x = 0; x < 9; x++){
     	header[x] = (x+1) % 2;
     }

     //Create message to transmit
     LEN = 500;
     int alt = 0;
     message[0]=0;
     message[1]=0;
     message[2]=0;
     message[3]=0;
     for (x = 4; x < LEN; x++){
    	 if (alt < 2)
    		 message[x] = 0;
    	 else if (alt < 3)
    		 message[x] = 1;
    	 else{
    		 alt = -1;
    		 message[x] = 1;
    	 }
    	 alt++;
     }
	 /* END OF EXAMPLE CODE... see TimerISR for interrupt code... */

    /* Enable the timer interrupt and start the timer */
    TimerIntEnable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);
    TimerEnable(SOC_TMR_2_REGS, TMR_TIMER12, TMR_ENABLE_CONT);

    while(1);

    /* Disable the timer. No more timer interrupts */
    TimerDisable(SOC_TMR_2_REGS, TMR_TIMER12);

    /* Halt the program */
    while(1);
}

/*
** Timer Interrupt Service Routine
*/
static void TimerIsr(void)
{
    /* Disable the timer interrupt */
    TimerIntDisable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);

    /* Clear interrupt status in DSPINTC */
    IntEventClear(SYS_INT_T64P2_TINTALL);

    TimerIntStatusClear(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);

	/* INTERRUPT CODE STARTS HERE... */
	// Via GUI, allow user to turn a square wave on and off
    if (squareON == 1){
        if(state == 0)
   		{
       		LCDK_GPIO812_off();
   			state = 1;
   		}
        else
        {
        	LCDK_GPIO812_on();
        	state = 0;
       	}
       	//reset states
       	transmit = 0;
  		headCount = 0;
        messageCount = 0;
    }else if (transmit == 0)
    	LCDK_GPIO812_off();

    /*Transmission*/
    if(transmit == 1){
    	squareON = 0;
    	// Send 8 bit header first
    	if (headCount < 9){
    		if(header[headCount] == 1)
    			LCDK_GPIO812_on();
    		else
    			LCDK_GPIO812_off();
    		headCount++;
    	// Transmit message
    	}else if (messageCount < LEN){
    		if (message[messageCount] == 1)
    			LCDK_GPIO812_on();
    		else
    			LCDK_GPIO812_off();
    		messageCount++;
    	// Transmit END message
    	}else{
    		LCDK_GPIO812_off();
    		transmit = 0;
    		headCount = 0;
    		messageCount = 0;
    	}

    }
	/* INTERRUPT CODE ENDS HERE... */

    /* Enable the timer interrupt */
    TimerIntEnable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE); //must re-enable to create next interrupt cycle
}


/*
** Setup the timer for 64 bit mode
*/
static void TimerSetUp64Bit(void)
{
    /* Configuration of Timer */
    TimerConfigure(SOC_TMR_2_REGS, TMR_CFG_64BIT_CLK_INT);

    /* Set the 64 bit timer period */
    TimerPeriodSet(SOC_TMR_2_REGS, TMR_TIMER12, TMR_PERIOD_LSB32);
    TimerPeriodSet(SOC_TMR_2_REGS, TMR_TIMER34, TMR_PERIOD_MSB32);
}

/*
** Set up the ARM Interrupt Controller for generating timer interrupt
*/
static void TimerIntrSetUp(void)
{
    /* Initialize the DSPINTC */
    IntDSPINTCInit();

    /* Register the Timer ISR */
    IntRegister(C674X_MASK_INT4, TimerIsr);

    /* Map Timer interrupts to DSP maskable interrupt */
    IntEventMap(C674X_MASK_INT4, SYS_INT_T64P2_TINTALL);

    /* Enable DSP interrupt in DSPINTC */
    IntEnable(C674X_MASK_INT4);

    /* Enable DSP interrupts */
    IntGlobalEnable();
}
/***************************** End Of File ***********************************/
