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
typedef enum BoardLedId_t {
    	BOARD_RLED,
		BOARD_BLED
} BoardLedId_t;


/** LED States */
typedef enum BoardLedState_t {
    	BOARD_LED_STATE_OFF,
		BOARD_LED_STATE_ON,
		BOARD_LED_STATE_FLASH
} BoardLedState_t;


/*****************************************************************************
 * @fn      Board_initLEDs
 *
 * @brief   Initial the LED services.
 *
 * @param   void
 *
 * @return  none
 */
void Board_initLEDs(void);

/*****************************************************************************
 * @fn      Board_ledControl
 *
 * @brief   Change the lED state between off, on and flashing
 *
 * @param   boardLedId_t ledId to indicate the LED being control
 			boardLedState_t state to indicate the state turning to
 			uint32_t period was used if the state is flashing
 *
 * @return  none
 */
void Board_ledControl(BoardLedId_t ledId, BoardLedState_t state, uint32_t period);




#ifdef __cplusplus
}
#endif

#endif /* BOARD_LED_H */
