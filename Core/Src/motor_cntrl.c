/*
 * motor_cntrl.c
 *
 *  Created on: Aug 5, 2026
 *      Author: maxda
 */

#include "motor_cntrl.h"
#include "main.h"
#include "tim.h"

#define MTR_TIM_CHANNEL TIM_CHANNEL_1

static TIM_HandleTypeDef *mtr_tim_handle = &htim3;

static volatile uint32_t steps_remaining = 0U;
static volatile bool motor_busy = false;

/*
 * Write EN low to enable output to motor
 */
static void motor_cntrl_enable(void){
	HAL_GPIO_WritePin(MotorEN_GPIO_Port, MotorEN_Pin, GPIO_PIN_RESET);
}

/*
 * Write EN high to disable output to motor
 */
static void motor_cntrl_disable(void){
	HAL_GPIO_WritePin(MotorEN_GPIO_Port, MotorEN_Pin, GPIO_PIN_SET);
}

/*
 * Set the direction of the motor
 * direction: 1 = clockwise, 0 = counterclockwise
 */
static void motor_cntrl_set_direction(uint8_t direction) {
  HAL_GPIO_WritePin(MotorDIR_GPIO_Port, MotorDIR_Pin, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/*
 * Initialize the motor control system.
 * Stops any ongoing PWM signals, resets the step counter,
 * and disables the motor.
 */
void motor_cntrl_init(void) {
  HAL_TIM_PWM_Stop_IT(mtr_tim_handle, MTR_TIM_CHANNEL);

  steps_remaining = 0U;
  motor_busy = false;
  motor_cntrl_disable();
}

/*
 * Non-blocking function to move the motor a specified number of steps.
 * direction: 1 = clockwise, 0 = counterclockwise
 * steps: number of steps to move
 * Returns true if the movement was successfully started, false otherwise.
 */
bool motor_cntrl_move_steps(uint8_t direction, uint32_t steps){
  	if (motor_busy || (steps == 0U)) {
    	return false;
  	}

	motor_cntrl_set_direction(direction);

	// Reset the timer counter and clear the capture/compare flag before starting the movement
	__HAL_TIM_SET_COUNTER(mtr_tim_handle, 0U);
	__HAL_TIM_CLEAR_FLAG(mtr_tim_handle, TIM_FLAG_CC1);
	steps_remaining = steps;
	motor_busy = true;

	motor_cntrl_enable();

	// Start the PWM signal to move the motor
	if (HAL_TIM_PWM_Start_IT(mtr_tim_handle, MTR_TIM_CHANNEL) != HAL_OK) {
		steps_remaining = 0U;
		motor_busy = false;
		motor_cntrl_disable();
		return false;
	}

	return true;
}

/*
 * Check if the motor is currently busy.
 * Returns true if the motor is moving, false otherwise.
 */
bool motor_cntrl_is_busy(void) { return motor_busy; }

/*
 * Abort any ongoing motor movement.
 * Stops the PWM signal, resets the step counter,
 * and disables the motor.
 */
void motor_cntrl_abort(void) {
  HAL_TIM_PWM_Stop_IT(mtr_tim_handle, MTR_TIM_CHANNEL);

  steps_remaining = 0U;
  motor_busy = false;
  motor_cntrl_disable();
}

/*
 * Callback function called when a PWM pulse is finished.
 * Decrements the step counter and stops the motor if all steps are completed.
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
  if ((htim->Instance != TIM3) || (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)) {
    return;
  }

  if (steps_remaining > 0U) {
    steps_remaining--;
  }

  if (steps_remaining == 0U) {
    HAL_TIM_PWM_Stop_IT(htim, MTR_TIM_CHANNEL);

    motor_cntrl_disable();
    motor_busy = false;
  }
}