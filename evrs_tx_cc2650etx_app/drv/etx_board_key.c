/******************************************************************************

 @file  board_key.c

 @brief This file contains the interface to the SRF06EB Key Service.

 Group: WCS, BTS
 Target Device: CC2650, CC2640

 ******************************************************************************
 
 Copyright (c) 2014-2018, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: ble_sdk_2_02_02_25
 Release Date: 2018-04-02 18:03:35
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

#include <ti/drivers/pin/PINCC26XX.h>

#ifdef USE_ICALL
#include <icall.h>
#endif

#include <inc/hw_ints.h>

#include "util.h"
#include "etx_board_key.h"
#include "etx_board.h"

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Board_keyChangeHandler(UArg a0);
static void Board_keyCallback(PIN_Handle hPin, PIN_Id pinId);

/*******************************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Value of keys Pressed
static uint8_t keysPressed;

// Key debounce clock
static Clock_Struct keyChangeClock;

// Pointer to application callback
keysPressedCB_t appKeyChangeHandler = NULL;

// Memory for the GPIO module to construct a Hwi
Hwi_Struct callbackHwiKeys;

// PIN configuration structure to set all KEY pins as inputs with pullups enabled
PIN_Config keyPinsCfg[] =
{
     Board_BTN0   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN1   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN2   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN3   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN4   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN5   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN6   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */
     Board_BTN7   | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,    /* Button is active low          */

     PIN_TERMINATE
};

PIN_State  keyPins;
PIN_Handle hKeyPins;

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/*********************************************************************
 * @fn      Board_initKeys
 *
 * @brief   Enable interrupts for keys on GPIOs.
 *
 * @param   appKeyCB - application key pressed callback
 *
 * @return  none
 */
void Board_initKeys(keysPressedCB_t appKeyCB)
{
  // Initialize KEY pins. Enable int after callback registered
    hKeyPins = PIN_open(&keyPins, keyPinsCfg);
    PIN_registerIntCb(hKeyPins, Board_keyCallback);

    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN0 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN1 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN2 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN3 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN4 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN5 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN6 | PIN_IRQ_NEGEDGE);
    PIN_setConfig(hKeyPins, PIN_BM_IRQ, Board_BTN7 | PIN_IRQ_NEGEDGE);

#ifdef POWER_SAVING
  //Enable wakeup
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN0 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN1 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN2 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN3 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN4 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN5 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN6 | PINCC26XX_WAKEUP_NEGEDGE);
    PIN_setConfig(hKeyPins, PINCC26XX_BM_WAKEUP, Board_BTN7 | PINCC26XX_WAKEUP_NEGEDGE);
#endif //POWER_SAVING

  // Setup keycallback for keys
    Util_constructClock(&keyChangeClock, Board_keyChangeHandler,
                      KEY_DEBOUNCE_TIMEOUT, 0, false, 0);

  // Set the application callback
    appKeyChangeHandler = appKeyCB;
}

/*********************************************************************
 * @fn      Board_keyCallback
 *
 * @brief   Interrupt handler for Keys
 *
 * @param   none
 *
 * @return  none
 */
#include "board_key.h"
static void Board_keyCallback(PIN_Handle hPin, PIN_Id pinId)
{
    keysPressed = 0;

    if ( PIN_getInputValue(Board_BTN0) == 0 )
        keysPressed |= S1;
    if ( PIN_getInputValue(Board_BTN1) == 0 )
        keysPressed |= S2;
    if ( PIN_getInputValue(Board_BTN2) == 0 )
        keysPressed |= S3;
    if ( PIN_getInputValue(Board_BTN3) == 0 )
        keysPressed |= S4;
    if ( PIN_getInputValue(Board_BTN4) == 0 )
        keysPressed |= S5;
    if ( PIN_getInputValue(Board_BTN5) == 0 )
        keysPressed |= S6;
    if ( PIN_getInputValue(Board_BTN6) == 0 )
        keysPressed |= S7;
    if ( PIN_getInputValue(Board_BTN7) == 0 )
        keysPressed |= S8;

    Util_startClock(&keyChangeClock);
}

/*********************************************************************
 * @fn      Board_keyChangeHandler
 *
 * @brief   Handler for key change
 *
 * @param   UArg a0 - ignored
 *
 * @return  none
 */
static void Board_keyChangeHandler(UArg a0)
{
  if (appKeyChangeHandler != NULL)
  {
    // Notify the application
    (*appKeyChangeHandler)(keysPressed);
  }
}
/*********************************************************************
*********************************************************************/
