/*****************************************************************************
 * 
 * @filepath 	/evrs_tx_cc2650etx_app/drv/etx_board.h
 * 
 * @project 	evrs_tx_cc2650etx_app
 * 
 * @brief 		Header file for the cc2650etx board definitions
 * 
 * @date 		5 Sep. 2018
 * 
 * @author		Ziyi@outlook.com.au
 *
 ****************************************************************************/

#ifndef ETXBOARD_H
#define ETXBOARD_H

/** ============================================================================
 *  Includes
 *  ==========================================================================*/
#include <ti/drivers/PIN.h>
#include <driverlib/ioc.h>

/** ============================================================================
 *  Externs
 *  ==========================================================================*/
extern const PIN_Config BoardGpioInitTable[];

/** ============================================================================
 *  Defines
 *  ==========================================================================*/

/* Same RF Configuration as 5XD */
#define CC2650EM_5XD

/* Mapping of pins to board signals using general board aliases
 *  <board signal alias>  <pin mapping>
 */

/* Discrete outputs */
#define Board_RLED         	IOID_13
#define Board_BLED         	IOID_14
#define Board_LED_ON       	1
#define Board_LED_OFF      	0
#define Board_SOFT_PWR			IOID_9

/* Discrete inputs */
#define Board_KEY1			IOID_12
#define Board_KEY2			IOID_5
#define Board_KEY3         	IOID_2
#define Board_KEY4         	IOID_11
#define Board_KEY5         	IOID_6
#define Board_KEY6         	IOID_3
#define Board_KEY7         	IOID_10
#define Board_KEY8         	IOID_7
#define Board_KEY9         	IOID_4
#define Board_KEY_OK       	IOID_1

/* UART Board */
#define Board_UART_RX      	PIN_UNASSIGNED
#define Board_UART_TX      	IOID_0

/* ADC Vin */
#define Board_ADCIN       	IOID_8

#endif /* ETXBOARD_H */
