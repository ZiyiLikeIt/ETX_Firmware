/*****************************************************************************

 file	etx_board_led.c

 brief	This file contains the functions and params to implement CC2650
 LaunchXL LED service.

 proj	EVRS

 date	1056pm 29 Jul 2018

 author	Ziyi

 *****************************************************************************/

/*********************************************************************
 * Includes
 */

#include <ti/sysbios/knl/Clock.h>
#include <ti/drivers/PIN.h>

#include "util.h"
#include "etx_board_led.h"
#include "etx_board.h"

/*********************************************************************
 * Typedefs
 */
#define DEFAULT_FLASH_PERIOD 500

#define IDPARSER(x) (x == BOARD_RLED)?(Board_RLED):(Board_BLED)


/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config ledPinTable[] = {
	Board_RLED | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	Board_BLED | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE };

/*********************************************************************
 * Local Varibles
 */

/* flash control timer */
static Clock_Struct ledFlashClk[2];

/* LED pin state */
static PIN_State ledPinState;


/* LED Pin Handle */
static PIN_Handle ledPinHandle;

/* LED state table */
static BoardLedState_t ledState[2] = {BOARD_LED_STATE_OFF};
static uint32_t ledPeriod[2] = {DEFAULT_FLASH_PERIOD};

/*********************************************************************
 * Local Functions
 */
//static uint32_t Board_ledIDParser(boardLedId_t id);
static void Board_ledFlashTimeoutCB(UArg ledID);

/*********************************************************************
 * Public Functions
 */
/*****************************************************************************
 * @fn      Board_initLEDs
 *
 * @brief   Initial the LED services.
 *
 * @param   void
 *
 * @return  none
 */
void Board_initLEDs(void) {
	/* Open LED PIN driver */
	ledPinHandle = PIN_open(&ledPinState, ledPinTable);

	/* Set default value */
	PIN_setOutputValue(ledPinHandle, Board_RLED, BOARD_LED_STATE_OFF);
	PIN_setOutputValue(ledPinHandle, Board_BLED, BOARD_LED_STATE_OFF);

	/* construct clock to control the flashing */
	Util_constructClock(&ledFlashClk[BOARD_RLED], Board_ledFlashTimeoutCB,
			ledPeriod[BOARD_RLED], 0, false, BOARD_RLED);
	Util_constructClock(&ledFlashClk[BOARD_BLED], Board_ledFlashTimeoutCB,
			ledPeriod[BOARD_BLED], 0, false, BOARD_BLED);
}

/*****************************************************************************
 * @fn      Board_ledControl
 *
 * @brief   Change the lED state between off, on and flashing
 *
 * @param   uint32_t index is to indicate the LED being control
 boardLedState_t to indicate the state turning to
 uint32_t period is to set the flashing period
 *
 * @return  none
 */
void Board_ledControl(BoardLedID_t ledID, BoardLedState_t state,
		uint32_t period) {
	switch (state) {
		case BOARD_LED_STATE_OFF:
			PIN_setOutputValue(ledPinHandle, IDPARSER(ledID), 0);
			if (ledState[ledID] == BOARD_LED_STATE_FLASH)
				Util_stopClock(&ledFlashClk[ledID]);
		break;

		case BOARD_LED_STATE_ON:
			PIN_setOutputValue(ledPinHandle, IDPARSER(ledID), 1);
			if (ledState[ledID] == BOARD_LED_STATE_FLASH)
				Util_stopClock(&ledFlashClk[ledID]);
		break;

		case BOARD_LED_STATE_TOGGLE:
			if (ledState[ledID] == BOARD_LED_STATE_FLASH)
				Util_stopClock(&ledFlashClk[ledID]);
			PIN_setOutputValue(ledPinHandle, IDPARSER(ledID),
					!PIN_getOutputValue(IDPARSER(ledID)));
		break;

		case BOARD_LED_STATE_FLASH:
			PIN_setOutputValue(ledPinHandle, IDPARSER(ledID), 0);
			Util_restartClock(&ledFlashClk[ledID], period);
		break;

		case BOARD_LED_STATE_LOWFLASH:
			PIN_setOutputValue(ledPinHandle, IDPARSER(ledID), 0);
			Util_restartClock(&ledFlashClk[ledID], period);
		break;

		default:
		break;
	}
	ledState[ledID] = state;
	ledPeriod[ledID] = period;
}

/*
 * timer timeout callback
 */
static void Board_ledFlashTimeoutCB(UArg ledID) {
	ledID = (BoardLedID_t) ledID;
	uint8_t currState = 0;
	currState = PIN_getOutputValue(IDPARSER(ledID));
	PIN_setOutputValue(ledPinHandle, IDPARSER(ledID), !currState);

	if (ledState[ledID] == BOARD_LED_STATE_FLASH)
		Util_startClock(&ledFlashClk[ledID]);
	else if (ledState[ledID] == BOARD_LED_STATE_LOWFLASH)
		Util_restartClock(&ledFlashClk[ledID],
				(currState == 0)?(50):(ledPeriod[ledID]));
}
