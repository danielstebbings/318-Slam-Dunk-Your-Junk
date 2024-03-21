/*
 * SG90.h
 *
 *  Created on: 19 March 2024
 *      Author: Daniel
 */

#include <msp430.h>
#include "driverlib/driverlib.h"

#ifndef SG90_H_
#define SG90_H_

void sg90_setup(); //setup the pins and timers

void sg90_move(int angle); // angle 0->360


