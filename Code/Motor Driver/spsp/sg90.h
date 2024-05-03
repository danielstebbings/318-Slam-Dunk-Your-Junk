/*
 * SG90.h
 *
 *  Created on: 19 March 2024
 *      Author: Daniel
 */

#include <msp430.h>
#include "driverlib/driverlib.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef SG90_H_
#define SG90_H_

typedef enum sg90_state {
    OPEN,
    CLOSED
} sg90_state;

void sg90_init_pins(void); //setup the pins and timers

void sg90_init_timers(uint8_t angle); //angle 0-64

void sg90_move(const enum sg90_state state);



#endif

