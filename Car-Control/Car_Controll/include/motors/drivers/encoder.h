/*
 * encoder.h
 *
 *  Created on: 18 iun. 2024
 *      Author: racov
 */

#ifndef INCLUDE_MOTORS_ENCODER_H_
#define INCLUDE_MOTORS_ENCODER_H_

#include "motors_common.h"

/*Encoder*/
#define QUEUE_SIZE_ENCODER_PULSE 		   20
#define QUEUE_SIZE_DATATYPE_ENCODER_PULSE (sizeof(int))
#define PULSE_COUNT_HIGH_LIMIT 		   4 // at every 4 pulses forward the car has moved half a wheel and the callback is called
#define PULSE_COUNT_LOW_LIMIT  		  -4// at every 4 pulses backward the car has moved half a wheel and the callback is called
#define encoderGPIO_A 			    	   35
#define encoderGPIO_B 					   14
bool encoder_pcnt_isr(pcnt_unit_handle_t unit,const pcnt_watch_event_data_t *edata, void *user_context);
void initializeEncoder();


#endif /* INCLUDE_MOTORS_ENCODER_H_ */
