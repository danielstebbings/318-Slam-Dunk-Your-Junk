//***************************************************************************************
//  MSP430 Blink the LED Demo - Software Toggle P1.0
//
//  Description; Toggle P1.0 by xor'ing P1.0 inside of a software loop.
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//                MSP430x5xx
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |             P1.0|-->LED
//
//  Texas Instruments, Inc
//  July 2013
//***************************************************************************************

#include <msp430.h>
#include "driverlib/driverlib.h"
#include "mp6500_driver.h"




    //source:
    //https://dev.ti.com/tirex/explore/node?node=A__AAIWeSHt4Nh0zyhUfSM00A__com.ti.MSP430_ACADEMY__bo90bso__1.00.05.06&search=msp430f6726a
    // Timer A0 interrupt service routine

    #pragma vector = TIMER0_A0_VECTOR
    __interrupt void Timer_A (void)
    {
        Timer_A_stop(TIMER_A1_BASE); // Stop PWM
        Timer_A_stop(TIMER_A0_BASE); // Stop step counter
        P1OUT ^= BIT0; //xor - toggle LED

        //TA0CCR0 = 0;// Add Offset to TACCR0
        //uint16_t count_val = 0;
        //count_val = Timer_A_getCounterValue(TIMER_A0_BASE);

        Timer_A_clearTimerInterrupt(TIMER_A0_BASE);
        //Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

        return;
    }

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    P1DIR |= 0x01;                          // Set P1.0 to output direction
    P1OUT  = 0x00;

    //Output parameters
    const int Step_Count = 20;
    const int PWM_Period = 200; // Period in clock cycles
    const int PWM_Duty = 50; // duty cycle %



    __enable_interrupt();

    //Counter timer setup
    Timer_A_initUpModeParam up_params = {0};
    up_params.clockSource = TIMER_A_CLOCKSOURCE_ACLK; // 1MHz
    up_params.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64; // 1M/2 = 500kHz
    up_params.timerPeriod = (Step_Count+1) * PWM_Period; //0->65k
    up_params.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    up_params.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    up_params.timerClear = TIMER_A_DO_CLEAR;


    Timer_A_initUpMode(TIMER_A0_BASE,&up_params); //init timer
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE); //start counting

    //PWM Setup
    //Based on TI example code
    //will output to second LED: P4.0



    GPIO_setAsPeripheralModuleFunctionOutputPin(
            GPIO_PORT_P4,
            GPIO_PIN0,
            GPIO_PRIMARY_MODULE_FUNCTION
            );

    Timer_A_outputPWMParam pwm_params = {0};
    pwm_params.clockSource = up_params.clockSource;
    pwm_params.clockSourceDivider = up_params.clockSourceDivider;
    pwm_params.timerPeriod = PWM_Period;
    pwm_params.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    pwm_params.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    pwm_params.dutyCycle = PWM_Period *  PWM_Duty/100;

    Timer_A_outputPWM(TIMER_A1_BASE, &pwm_params);




    //Dummy loop to break out of
    volatile uint16_t i;
    for(;;) { // smile :)
        i += 1;
        }






}
