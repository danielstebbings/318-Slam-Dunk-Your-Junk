/* Testing Both Sg90 and STEPPER
 *
 *
 *
 *
 */

#include <msp430.h>
#include "driverlib/driverlib.h"
#include <stdbool.h>
#include "mp6500_driver.h"
#include "sg90.h"

volatile char stepper_interrupt_flag = 0;
volatile char sg90_interrupt_flag    = 0;


typedef enum test_state {
    move_step,
    wait_move_step,
    open_sg90,
    wait_open_sg90,
    close_sg90,
    wait_close_sg90
}test_state;

//Motor ISR
//source:
//https://dev.ti.com/tirex/explore/node?node=A__AAIWeSHt4Nh0zyhUfSM00A__com.ti.MSP430_ACADEMY__bo90bso__1.00.05.06&search=msp430f6726a
//interrupts
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0 (void) {

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

        P4OUT &= ~BIT0; //turn off
        P1OUT &= ~BIT0; //turn off
        sg90_interrupt_flag = 1;

        Timer_A_clearTimerInterrupt(TIMER_A0_BASE);

        return;
    };

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer_A1 (void) {

        Timer_A_stop(TIMER_A1_BASE); // Stop counter
        Timer_A_stop(TIMER_A0_BASE); // Stop PWM

        int j = 0;
        for (j = 0; j< 10; j++) {
             __delay_cycles(65000);
             P4OUT ^= BIT0;
             __delay_cycles(65000);
             P1OUT ^= BIT0;
        };

        P4OUT &= !BIT0; //turn off
        P1OUT &= !BIT0; //turn off
        stepper_interrupt_flag = 1;

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
    test_state state = move_step;
    test_state next_state;

    uint16_t bin_positions[3] = {0, 1000, 2000};
    uint16_t current_position = 500;
    uint16_t next_position = 0;

    step_init_pins();
    sg90_init_pins();
    //sg90_move(CLOSED);
    stepper_interrupt_flag = 0;
    sg90_interrupt_flag = 0;

    int led = 0;

    while (1) {
        switch (state) {
            case move_step:
                Timer_A_stop(TIMER_A1_BASE); // Stop counter
                Timer_A_stop(TIMER_A0_BASE); // Stop PWM
                step_move_pos(current_position, next_position);
                next_state = wait_move_step;
                break;

            case wait_move_step:
                if (stepper_interrupt_flag == 1) {
                    Timer_A_stop(TIMER_A1_BASE); // Stop counter
                    Timer_A_stop(TIMER_A0_BASE); // Stop PWM
                    //flash indicators
                    for (led = 0; led< 10; led++) {
                                 __delay_cycles(65000);
                                 P4OUT ^= BIT0;
                                 __delay_cycles(65000);
                                 P1OUT ^= BIT0;
                            };
                    stepper_interrupt_flag = 0;
                    current_position = next_position;
                    next_state = open_sg90;
                };
                break;
            case open_sg90:

                sg90_move(OPEN);
                next_state = wait_open_sg90;

                break;

            case wait_open_sg90:
                if (sg90_interrupt_flag == 1) {
                    for (led = 0; led< 10; led++) {
                                                     __delay_cycles(65000);
                                                     P4OUT ^= BIT0;
                                                     __delay_cycles(65000);
                                                     P1OUT ^= BIT0;
                                                };
                    sg90_interrupt_flag = 0;
                    next_state = close_sg90;
                };
                break;
            case close_sg90:
                //sg90_interrupt_flag = 0;
                sg90_move(CLOSED);
                next_state = wait_close_sg90;
                break;
            case wait_close_sg90:
                if (sg90_interrupt_flag == 1) {
                    for (led = 0; led< 10; led++) {
                                                     __delay_cycles(65000);
                                                     P4OUT ^= BIT0;
                                                     __delay_cycles(65000);
                                                     P1OUT ^= BIT0;
                                                };
                    sg90_interrupt_flag = 0;
                    next_position = 1000;
                    next_state = move_step;
                };

                break;
        };

        state = next_state;

    };

};

