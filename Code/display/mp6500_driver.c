/*
 * mp6500_Driver.c
 *
 *  Created on: 24 Feb 2024
 *      Author: Daniel
 */

#include "mp6500_driver.h"
#include "driverlib/driverlib.h"
#include <msp430.h>


void step_init_timers(uint16_t Step_Count){ //sets up timer
    //consts
    const uint16_t PWM_period = 50;
    const uint16_t duty = 25;
    const uint16_t clock_source = TIMER_A_CLOCKSOURCE_ACLK;
    const uint16_t clock_div = TIMER_A_CLOCKSOURCE_DIVIDER_1;

    uint16_t timer_period_clks = (Step_Count+1) * PWM_period;

    //Stop both counters
    Timer_A_stop(TIMER_A0_BASE); // step count timer
    Timer_A_stop(TIMER_A1_BASE); // PWM timer

     Timer_A_outputPWMParam pwm_params = {
                                      clock_source,  //source
                                      clock_div,     //divider
                                      PWM_period,                        //period of wave
                                      TIMER_A_CAPTURECOMPARE_REGISTER_2, //register to store compare
                                      TIMER_A_OUTPUTMODE_RESET_SET,      //PWM mode
                                      duty};

     Timer_A_initUpModeParam timer_params = {
                                       clock_source,   //source
                                       clock_div,      //divider
                                       timer_period_clks,                  //number to count to
                                       TIMER_A_TAIE_INTERRUPT_DISABLE,     //Timer A Interrupt (triggers after it resets)
                                       TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, //CCR0 interrupt (triggers on last)
                                       TIMER_A_DO_CLEAR };                  //reset the timer before starting

     Timer_A_initUpMode(TIMER_A0_BASE,&timer_params); //configure timer

     //Start PWM and timer
     Timer_A_outputPWM(TIMER_A1_BASE, &pwm_params);
     Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
     return;
};

void step_init_pins() {
    //DIR output: Pin 8.2 GPIO out
    P8DIR |= BIT2;
    P8SEL0 &= ~BIT0; //set bit0 to 0 without affecting others

    //Step output: Pin 8.3 is TA1.2, CCR2
    P8DIR |= BIT3; //sel 0
    P8SEL0 |= BIT3; //sel 1

}


void step_move_pos(uint16_t current_pos,uint16_t new_pos) {
        //uint16_t direction;
        uint16_t steps_to_move = 0;

        if(current_pos == new_pos) {
            return;
        }
        else if(current_pos > new_pos) {
            //direction = 1;
            //Pin 8.2
            P8OUT |= BIT2;
            steps_to_move = current_pos - new_pos;
        }
        else { //current_pos < new_pos
            //direction = 0;
            P8OUT &= ~BIT2; //set bit 2 to 0, leave others;
            steps_to_move =  new_pos - current_pos;
        };


        step_init_timers(steps_to_move);
        return;

};


