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

#include "sg90.h"
#include <stdbool.h>

//DEBUG
#include <stdio.h>

// TYPES
typedef enum test_state {
    open,
    wait_open,
    close,
    wait_close
} test_state;

//GLOBAL VARS
volatile bool interrupt_flag = 0;

//interrupts
#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer_A (void) {

        Timer_A_stop(TIMER_A1_BASE); // Stop counter
        Timer_A_stop(TIMER_A0_BASE); // Stop PWM
        int j = 0;
        for (j = 0; j< 100; j++) {
             __delay_cycles(65000);
             P4OUT ^= BIT0;
             __delay_cycles(65000);
             P1OUT ^= BIT0;
        };
        P4OUT &= !BIT0; //turn off
        P1OUT &= !BIT0; //turn off
        interrupt_flag = 1;

        Timer_A_clearTimerInterrupt(TIMER_A1_BASE);

        return;
    };


int _system_pre_init(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    return 1;
};

void main(void) {

    //WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    __enable_interrupt();

    //DEBUG LED SETUP
    P1DIR |= 0x01;                          // Set P1.0 to output direction
    P1OUT  = 0x00;                          // P1.0 off
    P4DIR |= 0x01;                          // Set P4.0 to output direction
    P4OUT  = 0x00;                          // P4.0 off
    //P4OUT ^= BIT0; //xor - toggle LED at P1.0
    //setup state machine
    test_state state = open;
    test_state next_state;
    sg90_state servo_state = CLOSED;
    //sg90_init_pins();
    //printf("Init finished \n");
    //Dummy loop to break out of

    //setup P1.7 as timer A0 CCR1 output
    P1DIR  |= BIT7;
    P1SEL0 |= BIT7;

    __delay_cycles(10);

    uint8_t angle = 60;
    const uint16_t PWM_period = 1280; //gives 50Hz at 64kHz
    const uint8_t PWM_1ms = 0; //64 clocks
    const uint16_t timer_period_seconds = 5; //max 64!
    const uint16_t timer_period_clks = timer_period_seconds*1000;

    uint16_t duty = PWM_1ms + angle; //from 64-128 clocks.

    //setup timers
    Timer_A_outputPWMParam pwm_params = {
                                         TIMER_A_CLOCKSOURCE_ACLK,          //64kHz source
                                         TIMER_A_CLOCKSOURCE_DIVIDER_1,     //divider
                                         PWM_period,                        //period of wave
                                         TIMER_A_CAPTURECOMPARE_REGISTER_1, //register to store compare
                                         TIMER_A_OUTPUTMODE_RESET_SET,      //PWM mode
                                         duty};                             //duty cycle
     Timer_A_initUpModeParam timer_params = {
                                         TIMER_A_CLOCKSOURCE_ACLK,           //64kHz
                                         TIMER_A_CLOCKSOURCE_DIVIDER_64,     //1kHz
                                         timer_period_clks,                  //number to count to
                                         TIMER_A_TAIE_INTERRUPT_DISABLE,     //Timer A Interrupt (triggers after it resets)
                                         TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, //CCR0 interrupt (triggers on last)
                                         TIMER_A_DO_CLEAR};                  //reset the timer before starting

      //Timer_A_initUpMode(TIMER_A1_BASE,&timer_params); //configure timer
      __delay_cycles(10);
      //Start PWM and timer
      Timer_A_outputPWM(TIMER_A0_BASE, &pwm_params);
      //Timer_A_outputPWM(TIMER_A1_BASE, &pwm_params);
      //Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
      int i =0;
      for (i = 0; i < 10; i++) {
      __delay_cycles(65000);
      P4OUT ^= BIT0;
      __delay_cycles(65000);
      P4OUT ^= BIT0;
      __delay_cycles(65000);
      P4OUT ^= BIT0;
      __delay_cycles(65000);
      P4OUT ^= BIT0;
      };
      P4OUT &= !BIT0; //turn off
      Timer_A_stop(TIMER_A1_BASE); // Stop counter
      Timer_A_stop(TIMER_A0_BASE); // Stop PWM
      sg90_init_timers(20);

      for (i = 0; i < 10; i++) {
            __delay_cycles(65000);
            P4OUT ^= BIT0;
            __delay_cycles(65000);
            P4OUT ^= BIT0;
            __delay_cycles(65000);
            P4OUT ^= BIT0;
            __delay_cycles(65000);
            P4OUT ^= BIT0;
            };
      P4OUT &= !BIT0; //turn off
      sg90_move(servo_state);
      for (i = 0; i < 10; i++) {
                  __delay_cycles(65000);
                  P1OUT ^= BIT0;
                  __delay_cycles(65000);
                  P1OUT ^= BIT0;
                  __delay_cycles(65000);
                  P1OUT ^= BIT0;
                  __delay_cycles(65000);
                  P1OUT ^= BIT0;
                  };
      P1OUT &= !BIT0; //turn off
      servo_state = OPEN;
      sg90_move(servo_state);



    //volatile uint16_t i;
    while (1) {
        if (interrupt_flag) {
                  P1OUT ^= BIT0; //xor - toggle LED at P1.0
                  sg90_init_timers(125);
              }
    };


} //end main


//source:
//https://dev.ti.com/tirex/explore/node?node=A__AAIWeSHt4Nh0zyhUfSM00A__com.ti.MSP430_ACADEMY__bo90bso__1.00.05.06&search=msp430f6726a
// Timer A0 interrupt service routine
