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

/* Same RF Configuration as 7x7 EM */
#define CC2650EM_5XD

/* Mapping of pins to board signals using general board aliases
 *  <board signal alias>  <pin mapping>
 */

/* Discrete outputs */
#define Board_RLED          IOID_3
#define Board_GLED          IOID_2
#define Board_LED_ON        1
#define Board_LED_OFF       0

/* Discrete inputs */
#define Board_BTN0          IOID_10
#define Board_BTN1          IOID_9
#define Board_BTN2          IOID_4
#define Board_BTN3          IOID_8
#define Board_BTN4          IOID_11
#define Board_BTN5          IOID_12
#define Board_BTN6          IOID_13
#define Board_BTN7          IOID_14

/* UART Board */
#define Board_UART_RX       IOID_1          /* RXD  */
#define Board_UART_TX       IOID_0          /* TXD  */

/* ADC Vin */
#define Board_ADC           IOID_7

#endif /* ETXBOARD_H */
