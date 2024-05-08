/*
 * mp6500_driver.h
 *
 *  Created on: 24 Feb 2024
 *      Author: Daniel
 */

#include <msp430.h>
#include "driverlib/driverlib.h"

#ifndef MP6500_DRIVER_H_
#define MP6500_DRIVER_H_


void step_init_pins(); //sets up pins for PWM output
void step_init_timers(uint16_t Step_Count); //initialises timers

void step_move_pos(uint16_t current_pos,uint16_t new_pos); //returns either new_pos or an error.
bool calibrate(uint16_t bin0, uint16_t bin1, uint16_t bin3); //Manual calibration



#endif /* MP6500_DRIVER_H_ */
