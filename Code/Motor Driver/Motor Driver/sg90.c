/*
 * mp6500_Driver.c
 *
 *  Created on: 19 March 2024
 *      Author: Daniel
 */

#include "sg90.h"
#include "stdbool.h"
#include "driverlib/driverlib.h"

void sg90_init_pins() {//setup the pins
    //setup P1.6 as timer A0 CCR1 output
        P1DIR  |= BIT6;
        P1SEL0 |= BIT6;
};

void sg90_init_timers(uint16_t angle) {
    const uint16_t PWM_period = 20000; //gives 50Hz at 1MHz
    const uint16_t PWM_min = 0; //900 micromiseconds
    //const uint16_t timer_period_seconds = 1; //max 64!
    const uint16_t timer_period_clks = 500;

    uint16_t duty = PWM_min + angle;
    Timer_A_stop(TIMER_A1_BASE); // Stop counter
    Timer_A_stop(TIMER_A0_BASE); // Stop PWM
        //setup timers
        Timer_A_outputPWMParam pwm_params = {
                                         TIMER_A_CLOCKSOURCE_SMCLK,         //1MHz source
                                         TIMER_A_CLOCKSOURCE_DIVIDER_1,     //divider
                                         PWM_period,                        //period of wave
                                         TIMER_A_CAPTURECOMPARE_REGISTER_2, //register to store compare
                                         TIMER_A_OUTPUTMODE_RESET_SET,      //PWM mode
                                         duty};                             //duty cycle
        Timer_A_initUpModeParam timer_params = {
                                         TIMER_A_CLOCKSOURCE_ACLK,           //64kHz
                                         TIMER_A_CLOCKSOURCE_DIVIDER_64,      //1kHz
                                         timer_period_clks,                  //number to count to
                                         TIMER_A_TAIE_INTERRUPT_DISABLE,     //Timer A Interrupt (triggers after it resets)
                                         TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, //CCR0 interrupt (triggers on last)
                                         TIMER_A_DO_CLEAR};                  //reset the timer before starting

        Timer_A_initUpMode(TIMER_A1_BASE,&timer_params); //configure timer

        //Start PWM and timer
        Timer_A_outputPWM(TIMER_A0_BASE, &pwm_params);
        Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
        return;
};


void sg90_move(const enum sg90_state state) {
    if (state == CLOSED) {
        sg90_init_timers(910);
        return;
    }
    else if (state == OPEN) {
        sg90_init_timers(1490);
        return;
    } else {
        //ERROR
        return;
    }

};
