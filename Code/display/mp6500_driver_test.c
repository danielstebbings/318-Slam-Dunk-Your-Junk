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
#include <stdbool.h>
#include "mp6500_driver.h"

#define DEBUG


volatile char interrupt_flag = 0;


typedef enum test_state {
    move_1,
    waiting_1,
    move_2,
    waiting_2,
    move_3,
    waiting_3
}test_state;

//Motor ISR
//source:
//https://dev.ti.com/tirex/explore/node?node=A__AAIWeSHt4Nh0zyhUfSM00A__com.ti.MSP430_ACADEMY__bo90bso__1.00.05.06&search=msp430f6726a
//interrupts
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void) {

        Timer_A_stop(TIMER_A1_BASE); // Stop counter
        Timer_A_stop(TIMER_A0_BASE); // Stop PWM

        int j = 0;
        for (j = 0; j< 10; j++) {
             __delay_cycles(65000);
             P4OUT ^= BIT0;
             __delay_cycles(65000);
             P1OUT ^= BIT0;
        };

        P4OUT &= ~BIT0; //turn off
        P1OUT &= ~BIT0; //turn off
        interrupt_flag = 1;

        Timer_A_clearTimerInterrupt(TIMER_A0_BASE);

        return;
    };

   int _system_pre_init(void)
   {
       WDTCTL = WDTPW | WDTHOLD;
       return 1;
   };

void main(void) {
    //WDTCTL = WDTPW | WDTHOLD;             // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    __enable_interrupt();
    //DEBUG LED SETUP
    P1DIR |= 0x01;                          // Set P1.0 to output direction
    P1OUT  = 0x00;                          // P1.0 off
    P4DIR |= 0x01;                          // Set P4.0 to output direction
    P4OUT  = 0x00;


    test_state state = move_1;
    test_state next_state;
    int k =0;

    uint16_t bin_positions[3] = {0, 1000, 2000};
    uint16_t current_position = 500;
    volatile char b = 0;
    step_init_pins();

    //step_init_timers(1);

    while(1) {
        switch (state) {
        case move_1:
            interrupt_flag = 0;
            step_move_pos(current_position, 0);
            next_state = waiting_1;
            break;
        case waiting_1:
            b = interrupt_flag;
            if (interrupt_flag == 1) {
                current_position = bin_positions[0];
                interrupt_flag = 0;
                next_state = move_2;
            }

            break;
        case move_2:
            step_move_pos(current_position, 1000);
            next_state = waiting_2;
            break;
        case waiting_2:
            b = interrupt_flag;
            if (interrupt_flag == 1) {
                current_position = bin_positions[1];
                interrupt_flag = 0;
                next_state  = move_3;
            }
            break;
        case move_3:
            step_move_pos(current_position, bin_positions[2]);
            next_state = waiting_3;
            break;
        case waiting_3:
            if (interrupt_flag == 1) {
                current_position = bin_positions[2];
                interrupt_flag = 0;
                next_state = move_1;
            }
            break;
        default:
                           for (k = 0; k< 10; k++) {
                                __delay_cycles(65000);
                                P4OUT ^= BIT0;
                                __delay_cycles(65000);
                                P1OUT ^= BIT0;
                           };
           break;
        };

        state = next_state;
    }





}
