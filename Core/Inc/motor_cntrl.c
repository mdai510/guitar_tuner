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
//motor direction: 1 = clockwise, 0 = counterclockwise
static uint_8 mtr_dir = 0;

/*
 * Write EN low to enable output to motor
 */
void motor_cntrl_enable(void){
	HAL_GPIO_WritePin(MotorEN_GPIO_Port, MotorEN_Pin, GPIO_PIN_RESET);
}

/*
 * Write EN high to disable output to motor
 */
void motor_cntrl_disable(void){
	HAL_GPIO_WritePin(MotorEN_GPIO_Port, MotorEN_Pin, GPIO_PIN_SET);
}

/*
 * Begin pulsing to STEP pin to move the motor
 */
void motor_cntrl_start(void){
	//start tim3 800Hz STEP pulses
	HAL_TIM_PWM_Start(mtr_tim_handle, MTR_TIM_CHANNEL);
}

/*
 * Stop pulsing to STEP pin
 */
void motor_cntrl_stop(void){
	HAL_TIM_PWM_Stop(mtr_tim_handle, MTR_TIM_CHANNEL);
}
