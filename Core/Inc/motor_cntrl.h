/*
 * motor_cntrl.h
 *
 *  Created on: Aug 5, 2026
 *      Author: maxda
 */

#ifndef INC_MOTOR_CNTRL_H_
#define INC_MOTOR_CNTRL_H_

#include <stdbool.h>
#include <stdint.h>

void motor_cntrl_init(void);

bool motor_cntrl_move_steps(uint8_t direction, uint32_t steps);

bool motor_cntrl_is_busy(void);
void motor_cntrl_abort(void);

#endif /* INC_MOTOR_CNTRL_H_ */
