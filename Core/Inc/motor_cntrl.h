/*
 * motor_cntrl.h
 *
 *  Created on: Aug 5, 2026
 *      Author: maxda
 */

#ifndef INC_MOTOR_CNTRL_H_
#define INC_MOTOR_CNTRL_H_

#include "tim.h"

void motor_cntrl_enable(void);
void motor_cntrl_disable(void);
void motor_cntrl_start(void);
void motor_cntrl_stop(void);

#endif /* INC_MOTOR_CNTRL_H_ */
