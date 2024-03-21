/*
 * mp6500_Driver.c
 *
 *  Created on: 24 Feb 2024
 *      Author: Daniel
 */

#include "mp6500_driver.h"


void init_timers(Step_Count){ //sets up timer
    //Stop both counters
    Timer_A_stop(TIMER_A0_BASE); // step count timer
    Timer_A_stop(TIMER_A1_BASE); // PWM timer


    Timer_A_initUpModeParam step_counter_params = {0};
#ifdef DEBUG
    step_counter_params.clockSource = TIMER_A_CLOCKSOURCE_ACLK; // 32kHz
    step_counter_params.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64; // 32k/64 = 500Hz
#else
    step_counter_params.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    step_counter_params.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_2; //1M/2 = 500kHz
#endif
        step_counter_params.timerPeriod = (Step_Count+1) * STEPPER_PWM_PERIOD; //0->65k
        step_counter_params.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
        step_counter_params.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
        step_counter_params.timerClear = TIMER_A_DO_CLEAR;

     Timer_A_initUpMode(TIMER_A0_BASE,&step_counter_params); //initialise timer

     Timer_A_outputPWMParam pwm_params = {0};
         pwm_params.clockSource = step_counter_params.clockSource;
         pwm_params.clockSourceDivider = step_counter_params.clockSourceDivider;
         pwm_params.timerPeriod = STEPPER_PWM_PERIOD;
         pwm_params.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
         pwm_params.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
         pwm_params.dutyCycle = 1/2*STEPPER_PWM_PERIOD;

     //start step counter
     Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
     //Start PWM output
     Timer_A_outputPWM(TIMER_A1_BASE, &pwm_params);
};

void init_pins() {
    //DIR output:
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN2);
    //STEP output
    GPIO_setAsPeripheralModuleFunctionOutputPin( //Primary function for 8.3 is CCI2A
                GPIO_PORT_P8,
                GPIO_PIN3,
                GPIO_PRIMARY_MODULE_FUNCTION
                );

}


int move_pos(current_pos,new_pos) {
    int steps_to_move = 0;

    if(current_pos == new_pos) {
        return current_pos;
    }
    else if(current_pos > new_pos) {
        direction = 1;
        steps_to_move = current_pos - new_pos;
    }
    else {
        P4_OUT |=
        steps_to_move =  new_pos - current_pos;
    };

    P4_OUT = direction << 2; //shift it two position of PIN2
    init_timers(steps_to_move);






};


