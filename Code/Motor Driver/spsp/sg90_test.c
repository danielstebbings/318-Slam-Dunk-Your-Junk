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
        /*
        int j = 0;
        for (j = 0; j< 10; j++) {
             __delay_cycles(65000);
             P4OUT ^= BIT0;
             __delay_cycles(65000);
             P1OUT ^= BIT0;
        };
        */
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

    //setup state machine
    test_state state = open;
    test_state next_state;
    sg90_state servo_state;

    //setup P1.7 as timer A0 CCR1 output
    P1DIR  |= BIT7;
    P1SEL0 |= BIT7;

    //Sg90 interrupt flag
    interrupt_flag = 0;

    int k = 0;
    for (k = 0; k< 10; k++) {
       __delay_cycles(65000);
       P1OUT ^= BIT0;
       __delay_cycles(65000);
       P1OUT ^= BIT0;
     };

    sg90_move(CLOSED);


    //volatile uint16_t i;
    while (1) {
        switch (state) {
            case open:
                //printf("open \n");
                interrupt_flag = 0;
                sg90_move(OPEN);
                next_state = wait_open;
                break;
            case wait_open: //wait for interrupt

                if (interrupt_flag ==1) {
                    //printf("interrupt fired \n");
                    next_state = close;
                } else {
                    //pass
                };

                break;
            case close:
                interrupt_flag = 0;
                sg90_move(CLOSED);
                next_state = wait_close;

                break;
            case wait_close:
                if (interrupt_flag ==1) {
                                next_state = open;
                            } else {
                                //pass
                            };
                break;
            }; //end swiitch
        state = next_state;

     }; //end while


} //end main


//source:
//https://dev.ti.com/tirex/explore/node?node=A__AAIWeSHt4Nh0zyhUfSM00A__com.ti.MSP430_ACADEMY__bo90bso__1.00.05.06&search=msp430f6726a
// Timer A0 interrupt service routine
