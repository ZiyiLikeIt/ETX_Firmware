/*****************************************************************************

file	etx_board_led.h

brief	This file contains the CC2650 LaunchXL LED service definitions and 
		prototypes

proj	EVRS

date	1023pm 29 Jul 2018

author	Ziyi

*****************************************************************************/

#ifndef ETXBOARD_LED_H
#define ETXBOARD_LED_H

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
 * Typedefs 
 */

/** LED Identifications **/
typedef enum BoardLedID_t {
    	BOARD_RLED,
		BOARD_BLED
} BoardLedID_t;

/** LED state list **/
typedef enum BoardLedState_t {
    	BOARD_LED_STATE_OFF,
		BOARD_LED_STATE_ON,
		BOARD_LED_STATE_LOW,
		BOARD_LED_STATE_HIGH,
		BOARD_LED_STATE_FLASH,
		BOARD_LED_STATE_LOWFLASH
} BoardLedState_t;

/*
 * External functions
 */

/** Initial the LED services **/
void Board_initLEDs(void);

/** Set the board led into corresponding state **/
void Board_ledControl(BoardLedID_t ledID, BoardLedState_t state, uint32_t period);

/** For external call **/
#define Board_ledOFF(ledID) \
	Board_ledControl((BoardLedID_t) ledID, BOARD_LED_STATE_OFF, 0)

#define Board_ledON(ledID) \
	Board_ledControl((BoardLedID_t) ledID, BOARD_LED_STATE_ON, 0)

#define Board_ledLOW(ledID) \
	Board_ledControl((BoardLedID_t) ledID, BOARD_LED_STATE_LOW, 0)

#define Board_ledHIGH(ledID) \
	Board_ledControl((BoardLedID_t) ledID, BOARD_LED_STATE_HIGH, 0)

#define Board_ledFlash(ledID, prd) \
	Board_ledControl((BoardLedID_t) ledID, BOARD_LED_STATE_FLASH, (uint32_t) prd)

#define Board_ledLowFlash(ledID, prd) \
	Board_ledControl((BoardLedID_t) ledID, BOARD_LED_STATE_LOWFLASH, (uint32_t) prd)

#ifdef __cplusplus
}
#endif

#endif /* BOARD_LED_H */
