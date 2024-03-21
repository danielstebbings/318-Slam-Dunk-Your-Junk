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


#ifndef STEPPER_PWM_PERIOD
#define STEPPER_PWM_PERIOD 50;
#endif




void init_motor(); //sets up timer
int move_pos(int current_pos, int new_pos); //returns either new_pos or an error.

bool calibrate(); //Manual calibration



#endif /* MP6500_DRIVER_H_ */
