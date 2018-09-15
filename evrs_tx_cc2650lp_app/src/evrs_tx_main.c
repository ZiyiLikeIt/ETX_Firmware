/******************************************************************************

 @file  simple_peripheral.c

 @brief This file contains the Simple BLE Peripheral sample application for use
 with the CC2650 Bluetooth Low Energy Protocol Stack.

 Group: WCS, BTS
 Target Device: CC2650, CC2640

 ******************************************************************************

 Copyright (c) 2013-2018, Texas Instruments Incorporated
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
#include <string.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

#include "hci_tl.h"
#include "gatt.h"
#include "linkdb.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "etx_gatt_prof.h"

#include "peripheral.h"
#include "gapbondmgr.h"

#include "osal_snv.h"
#include "icall_apimsg.h"

#include "util.h"

#ifdef USE_RCOSC
#include "rcosc_calibration.h"
#endif //USE_RCOSC

#include "board_key.h"
#include "board_led.h"
#include "board_display.h"
#include "ti/drivers/ADC.h"

#include "board.h"

#include "evrs_tx_main.h"

/*********************************************************************
 * CONSTANTS
 */

// Advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic
// parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     16

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic
// parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     400

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter
// update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// Whether to enable automatic parameter update request when a connection is
// formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         GAPROLE_LINK_PARAM_UPDATE_INITIATE_BOTH_PARAMS

// Connection Pause Peripheral time value (in seconds)
#define CONN_PAUSE_PERIPHERAL         6

// How often to perform periodic event (in msec)
#define ETX_PERIODIC_EVT_PERIOD               5000

// Task configuration
#define ETX_TASK_PRIORITY                     1

#ifndef ETX_TASK_STACK_SIZE
#define ETX_TASK_STACK_SIZE                   1024
#endif

#define ETX_ADTYPE_DEST				0xAF
#define ETX_ADTYPE_DEVID			0xAE

// Application state
typedef enum AppStates_t {
	APP_STATE_INIT,
	APP_STATE_IDLE,
	APP_STATE_ADVERT
} AppStates_t;

// Internal Events for RTOS application
#define ETX_STATE_CHANGE_EVT        0x0001
#define ETX_CHAR_CHANGE_EVT         0x0002
#define ETX_CHAR_ENQUIRE_EVT        0x0004
#define ETX_PERIODIC_EVT            0x0008
#define ETX_CONN_EVT_END_EVT        0x0010
#define ETX_KEY_CHANGE_EVT          0x0020

#define ETX_DEVID_LEN 			4
#define ETX_DEVID_NV_ID			0x80
#define ETX_DEVID_PREFIX		0x95

/*********************************************************************
 * TYPEDEFS
 */

// App event passed from profiles.
typedef struct EtxEvt_t {
	appEvtHdr_t hdr;  // event header.
} EtxEvt_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Entity ID globally used to check for source and/or destination of messages
static ICall_EntityID selfEntity;

// Semaphore globally used to post events to the application thread
static ICall_Semaphore sem;

// Clock instances for internal periodic events.
//static Clock_Struct periodicClock;

// Queue object used for app messages
static Queue_Struct appMsg;
static Queue_Handle appMsgQueue;

// events flag for internal application events.
static uint16_t events = 0;

// Task configuration
Task_Struct sbpTask;
Char sbpTaskStack[ETX_TASK_STACK_SIZE];

// Profile state and parameters
static AppStates_t appState = APP_STATE_INIT;

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8_t advertData[] = {
// Flags; this sets the device to use limited discoverable
// mode (advertises for 30 seconds at a time) instead of general
// discoverable mode (advertises indefinitely)
		0x02,// length of this data
		GAP_ADTYPE_FLAGS,
		DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

		// service UUID, to notify central devices what services are included
		// in this peripheral
		0x03,// length of this data
		GAP_ADTYPE_16BIT_MORE,      // some of the UUID's, but not all
		LO_UINT16(ETXPROFILE_SERV_UUID), HI_UINT16(ETXPROFILE_SERV_UUID),

		0x02,
		ETX_ADTYPE_DEST, 0x00 };

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[15] = {
// complete name
//0x14,// length of this data
//GAP_ADTYPE_LOCAL_NAME_COMPLETE, 'S', 'i', 'm', 'p', 'l', 'e', 'B', 'L',
//'E', 'P', 'e', 'r', 'i', 'p', 'h', 'e', 'r', 'a', 'l',

// connection interval range
		0x05,// length of this data
		GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE, LO_UINT16(
				DEFAULT_DESIRED_MIN_CONN_INTERVAL),   // 100ms
		HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL), LO_UINT16(
				DEFAULT_DESIRED_MAX_CONN_INTERVAL),   // 1s
		HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

		// Tx power level
		0x02,// length of this data
		GAP_ADTYPE_POWER_LEVEL, 0x00,       // 0dBm

		// Device ID rsp
		0x05,
		ETX_ADTYPE_DEVID, 0x00, 0x00, 0x00, 0x00 };

// GAP GATT Attributes
static const uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "EVRS Transmitter";

// Globals used for ATT Response retransmission
static gattMsgEvent_t *pAttRsp = NULL;
static uint8_t rspTxRetry = 0;

// Destiny base station ID
static uint8_t destBsID = 0x00;

// device ID params about Flash
static uint8_t devID[ETX_DEVID_LEN] = { 0 };

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void ETX_init(void);
static void ETX_taskFxn(UArg a0, UArg a1);

static uint8_t ETX_processStackMsg(ICall_Hdr *pMsg);
static uint8_t ETX_processGATTMsg(gattMsgEvent_t *pMsg);
static void ETX_processAppMsg(EtxEvt_t *pMsg);

static void ETX_processStateChangeEvt(gaprole_States_t newState);
static void ETX_processCharValueChangeEvt(uint8_t paramID);
static void ETX_processCharValueEnquireEvt(uint8_t paramID);
//static void ETX_performPeriodicTask(void);
//static void ETX_clockHandler(UArg arg);

static void ETX_sendAttRsp(void);
static void ETX_freeAttRsp(uint8_t status);

static void ETX_stateChangeCB(gaprole_States_t newState);
static void ETX_charValueChangeCB(uint8_t paramID);
static void ETX_charValueEnquireCB(uint8_t paramID);

static void ETX_enqueueMsg(uint8_t event, uint8_t state);

void ETX_keyChangeHandler(uint8_t keys);
static void ETX_handleKeys(uint8_t shift, uint8_t keys);

//device id
static void ETX_DevId_Find(uint8_t* nvBuf);
static void ETX_DevId_Refresh(uint8_t IdPrefix, uint8_t* nvBuf);
static void ETX_ScanRsp_UpdateDeviceID();
static void ETX_Advert_UpdateDestinyBS();

/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t ETX_gapRoleCBs = { ETX_stateChangeCB // Profile State Change Callbacks
		};

// GAP Bond Manager Callbacks
static gapBondCBs_t ETX_BondMgrCBs = {
NULL, // Passcode callback (not used by application)
		NULL  // Pairing / Bonding state Callback (not used by application)
		};

// Simple GATT Profile Callbacks
static ETXProfileCBs_t ETX_ProfileCBs = { ETX_charValueChangeCB, // Characteristic value change callback
		ETX_charValueEnquireCB // char enquired callback
		};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      ETX_createTask
 *
 * @brief   Task creation function for the Simple BLE Peripheral.
 *
 * @param   None.
 *
 * @return  None.
 */
void ETX_createTask(void) {
	Task_Params taskParams;

	// Configure task
	Task_Params_init(&taskParams);
	taskParams.stack = sbpTaskStack;
	taskParams.stackSize = ETX_TASK_STACK_SIZE;
	taskParams.priority = ETX_TASK_PRIORITY;

	Task_construct(&sbpTask, ETX_taskFxn, &taskParams, NULL);
}

/*********************************************************************
 * @fn      ETX_init
 *
 * @brief   Called during initialization and contains application
 *          specific initialization (ie. hardware initialization/setup,
 *          table initialization, power up notification, etc), and
 *          profile initialization/setup.
 *
 * @param   None.
 *
 * @return  None.
 */
static void ETX_init(void) {
	// ******************************************************************
	// N0 STACK API CALLS CAN OCCUR BEFORE THIS CALL TO ICall_registerApp
	// ******************************************************************
	// Register the current thread as an ICall dispatcher application
	// so that the application can send and receive messages.
	ICall_registerApp(&selfEntity, &sem);

#ifdef USE_RCOSC
	RCOSC_enableCalibration();
#endif // USE_RCOSC

	// Create an RTOS queue for message from profile to be sent to app.
	appMsgQueue = Util_constructQueue(&appMsg);

	Board_initKeys(ETX_keyChangeHandler);
	Board_initLEDs();
	Board_Display_Init();
	Board_initADC();

	// Device ID check
	{
		ETX_DevId_Find(devID);
		if (devID[3] != ETX_DEVID_PREFIX) // no valid device id found
			ETX_DevId_Refresh(ETX_DEVID_PREFIX, devID);
		ETX_ScanRsp_UpdateDeviceID();
	}

	// Setup the GAP
	GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, CONN_PAUSE_PERIPHERAL);

	// Setup the GAP Peripheral Role Profile
	{
		// For all hardware platforms, device starts advertising upon initialization
		uint8_t initialAdvertEnable = TRUE;

		// By setting this to zero, the device will go into the waiting state after
		// being discoverable for 30.72 second, and will not being advertising again
		// until the enabler is set back to TRUE
		uint16_t advertOffTime = 0;

		uint8_t enableUpdateRequest = DEFAULT_ENABLE_UPDATE_REQUEST;
		uint16_t desiredMinInterval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
		uint16_t desiredMaxInterval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
		uint16_t desiredSlaveLatency = DEFAULT_DESIRED_SLAVE_LATENCY;
		uint16_t desiredConnTimeout = DEFAULT_DESIRED_CONN_TIMEOUT;

		// Set the GAP Role Parameters
		GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
				&initialAdvertEnable);
		GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16_t),
				&advertOffTime);

		GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData),
				scanRspData);
		GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData),
				advertData);

		GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8_t),
				&enableUpdateRequest);
		GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t),
				&desiredMinInterval);
		GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t),
				&desiredMaxInterval);
		GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16_t),
				&desiredSlaveLatency);
		GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16_t),
				&desiredConnTimeout);
	}

	// Set the GAP Characteristics
	GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN,
			(void*) attDeviceName);

	// Set advertising interval
	{
		uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

		GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
		GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
		GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
		GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
	}

	// Setup the GAP Bond Manager
	{
		uint32_t passkey = 0; // passkey "000000"
		uint8_t pairMode = GAPBOND_PAIRING_MODE_NO_PAIRING;
		uint8_t mitm = FALSE;
		uint8_t ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
		uint8_t bonding = FALSE;

		GAPBondMgr_SetParameter(GAPBOND_DEFAULT_PASSCODE, sizeof(uint32_t),
				&passkey);
		GAPBondMgr_SetParameter(GAPBOND_PAIRING_MODE, sizeof(uint8_t),
				&pairMode);
		GAPBondMgr_SetParameter(GAPBOND_MITM_PROTECTION, sizeof(uint8_t),
				&mitm);
		GAPBondMgr_SetParameter(GAPBOND_IO_CAPABILITIES, sizeof(uint8_t),
				&ioCap);
		GAPBondMgr_SetParameter(GAPBOND_BONDING_ENABLED, sizeof(uint8_t),
				&bonding);
	}

	// Initialize GATT attributes
	GGS_AddService(GATT_ALL_SERVICES);           // GAP
	GATTServApp_AddService(GATT_ALL_SERVICES);   // GATT attributes
	DevInfo_AddService();                        // Device Information Service

	ETXProfile_AddService(GATT_ALL_SERVICES); // EVRS GATT Profile

	// Setup the EVRSProfile Characteristic Values
	{
		uint8_t cmdVal = 0xA3;
		uint8_t dataVal = 0xA4;

		ETXProfile_SetParameter(ETXPROFILE_CMD, sizeof(cmdVal), &cmdVal);
		ETXProfile_SetParameter(ETXPROFILE_DATA, sizeof(dataVal), &dataVal);
	}

	// Register callback with SimpleGATTprofile
	ETXProfile_RegisterAppCBs(&ETX_ProfileCBs);

	// Start the Device
	VOID GAPRole_StartDevice(&ETX_gapRoleCBs);

	// Start Bond Manager
	VOID GAPBondMgr_Register(&ETX_BondMgrCBs);

	// Register with GAP for HCI/Host messages
	GAP_RegisterForMsgs(selfEntity);

	// Register for GATT local events and ATT Responses pending for transmission
	GATT_RegisterForMsgs(selfEntity);

	HCI_LE_ReadMaxDataLenCmd();
	{
	    ADC_Handle   adc;
	    ADC_Params   params;
	    int_fast16_t res;
	    uint16_t adcValue;

	    ADC_Params_init(&params);
	    adc = ADC_open(Board_ADC1, &params);

	    if (adc == NULL) {
	    	uout0("ADC init error");
	    }
	    res = ADC_convert(adc, &adcValue);
	    if (res == ADC_STATUS_SUCCESS) {
	        uout1("ADC channel 0 convert result: 0x%x", adcValue);
	    }
	    else {
	        uout0("ADC channel 0 convert failed");
	    }

	    ADC_close(adc);
	}
	uout0("EVRS TX initialized");
	Board_ledControl(BOARD_LED_ID_G, BOARD_LED_STATE_FLASH, 300);
	//Board_ledControl(BOARD_LED_ID_R, BOARD_LED_STATE_ON, 0);
}

/*********************************************************************
 * @fn      ETX_taskFxn
 *
 * @brief   Application task entry point for the Simple BLE Peripheral.
 *
 * @param   a0, a1 - not used.
 *
 * @return  None.
 */
static void ETX_taskFxn(UArg a0, UArg a1) {
	// Initialize application
	ETX_init();

	// Application main loop
	for (;;) {
		// Waits for a signal to the semaphore associated with the calling thread.
		// Note that the semaphore associated with a thread is signaled when a
		// message is queued to the message receive queue of the thread or when
		// ICall_signal() function is called onto the semaphore.
		ICall_Errno errno = ICall_wait(ICALL_TIMEOUT_FOREVER);

		if (errno == ICALL_ERRNO_SUCCESS) {
			ICall_EntityID dest;
			ICall_ServiceEnum src;
			ICall_HciExtEvt *pMsg = NULL;

			if (ICall_fetchServiceMsg(&src, &dest,
					(void **) &pMsg) == ICALL_ERRNO_SUCCESS) {
				uint8 safeToDealloc = TRUE;

				if ((src == ICALL_SERVICE_CLASS_BLE) && (dest == selfEntity)) {
					ICall_Stack_Event *pEvt = (ICall_Stack_Event *) pMsg;

					// Check for BLE stack events first
					if (pEvt->signature == 0xffff) {
						if (pEvt->event_flag & ETX_CONN_EVT_END_EVT) {
							// Try to retransmit pending ATT Response (if any)
							ETX_sendAttRsp();
						}
					} else {
						// Process inter-task message
						safeToDealloc = ETX_processStackMsg((ICall_Hdr *) pMsg);
					}
				}

				if (pMsg && safeToDealloc) {
					ICall_freeMsg(pMsg);
				}
			}

			// If RTOS queue is not empty, process app message.
			while (!Queue_empty(appMsgQueue)) {
				EtxEvt_t *pMsg = (EtxEvt_t *) Util_dequeueMsg(appMsgQueue);
				if (pMsg) {
					// Process message.
					ETX_processAppMsg(pMsg);

					// Free the space from the message.
					ICall_free(pMsg);
				}
			}
		}

		if (events & ETX_PERIODIC_EVT) {
			events &= ~ETX_PERIODIC_EVT;

			//Util_startClock(&periodicClock);

			// Perform periodic application task
			//ETX_performPeriodicTask();
		}

	}
}

/*********************************************************************
 * @fn      ETX_processStackMsg
 *
 * @brief   Process an incoming stack message.
 *
 * @param   pMsg - message to process
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t ETX_processStackMsg(ICall_Hdr *pMsg) {
	uint8_t safeToDealloc = TRUE;

	switch (pMsg->event) {
		case GATT_MSG_EVENT:
			// Process GATT message
			safeToDealloc = ETX_processGATTMsg((gattMsgEvent_t *) pMsg);
		break;

		case HCI_GAP_EVENT_EVENT: {
			// Process HCI message
			switch (pMsg->status) {
				case HCI_COMMAND_COMPLETE_EVENT_CODE:
					// Process HCI Command Complete Event
				break;

				case HCI_BLE_HARDWARE_ERROR_EVENT_CODE: {
					AssertHandler(HAL_ASSERT_CAUSE_HARDWARE_ERROR, 0);
				}
				break;

				default:
				break;
			}
		}
		break;

		default:
			// do nothing
		break;
	}

	return (safeToDealloc);
}

/*********************************************************************
 * @fn      ETX_processGATTMsg
 *
 * @brief   Process GATT messages and events.
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t ETX_processGATTMsg(gattMsgEvent_t *pMsg) {
	// See if GATT server was unable to transmit an ATT response
	if (pMsg->hdr.status == blePending) {
		// No HCI buffer was available. Let's try to retransmit the response
		// on the next connection event.
		if (HCI_EXT_ConnEventNoticeCmd(pMsg->connHandle, selfEntity,
		ETX_CONN_EVT_END_EVT) == SUCCESS) {
			// First free any pending response
			ETX_freeAttRsp(FAILURE);

			// Hold on to the response message for retransmission
			pAttRsp = pMsg;

			// Don't free the response message yet
			return (FALSE);
		}
	} else if (pMsg->method == ATT_FLOW_CTRL_VIOLATED_EVENT) {
		// ATT request-response or indication-confirmation flow control is
		// violated. All subsequent ATT requests or indications will be dropped.
		// The app is informed in case it wants to drop the connection.

		// Display the opcode of the message that caused the violation.
		uout1("FC Violated: %d", pMsg->msg.flowCtrlEvt.opcode);
	} else if (pMsg->method == ATT_MTU_UPDATED_EVENT) {
		// MTU size updated
		uout1("MTU Size: $d", pMsg->msg.mtuEvt.MTU);
	}

	// Free message payload. Needed only for ATT Protocol messages
	GATT_bm_free(&pMsg->msg, pMsg->method);

	// It's safe to free the incoming message
	return (TRUE);
}

/*********************************************************************
 * @fn      ETX_sendAttRsp
 *
 * @brief   Send a pending ATT response message.
 *
 * @param   none
 *
 * @return  none
 */
static void ETX_sendAttRsp(void) {
	// See if there's a pending ATT Response to be transmitted
	if (pAttRsp != NULL) {
		uint8_t status;

		// Increment retransmission count
		rspTxRetry++;

		// Try to retransmit ATT response till either we're successful or
		// the ATT Client times out (after 30s) and drops the connection.
		status = GATT_SendRsp(pAttRsp->connHandle, pAttRsp->method,
				&(pAttRsp->msg));
		if ((status != blePending) && (status != MSG_BUFFER_NOT_AVAIL)) {
			// Disable connection event end notice
			HCI_EXT_ConnEventNoticeCmd(pAttRsp->connHandle, selfEntity, 0);

			// We're done with the response message
			ETX_freeAttRsp(status);
		} else {
			// Continue retrying
			uout1("Rsp send retry: %d", rspTxRetry);
		}
	}
}

/*********************************************************************
 * @fn      ETX_freeAttRsp
 *
 * @brief   Free ATT response message.
 *
 * @param   status - response transmit status
 *
 * @return  none
 */
static void ETX_freeAttRsp(uint8_t status) {
	// See if there's a pending ATT response message
	if (pAttRsp != NULL) {
		// See if the response was sent out successfully
		if (status == SUCCESS) {
			uout1("Rsp sent retry: %d", rspTxRetry);
		} else { // Free response payload
			GATT_bm_free(&pAttRsp->msg, pAttRsp->method);
			uout1("Rsp retry failed: %d", rspTxRetry);
		}

		// Free response message
		ICall_freeMsg(pAttRsp);

		// Reset our globals
		pAttRsp = NULL;
		rspTxRetry = 0;
	}
}

/*********************************************************************
 * @fn      ETX_processAppMsg
 *
 * @brief   Process an incoming callback from a profile.
 *
 * @param   pMsg - message to process
 *
 * @return  None.
 */
static void ETX_processAppMsg(EtxEvt_t *pMsg) {
	switch (pMsg->hdr.event) {
		case ETX_STATE_CHANGE_EVT:
			ETX_processStateChangeEvt((gaprole_States_t) pMsg->hdr.state);
		break;

		case ETX_CHAR_CHANGE_EVT:
			ETX_processCharValueChangeEvt(pMsg->hdr.state);
		break;

		case ETX_CHAR_ENQUIRE_EVT:
			ETX_processCharValueEnquireEvt(pMsg->hdr.state);
		break;

		case ETX_KEY_CHANGE_EVT:
			ETX_handleKeys(0, pMsg->hdr.state);

		default:
			// Do nothing.
		break;
	}
}

/*********************************************************************
 * @fn      ETX_stateChangeCB
 *
 * @brief   Callback from GAP Role indicating a role state change.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void ETX_stateChangeCB(gaprole_States_t newState) {
	ETX_enqueueMsg(ETX_STATE_CHANGE_EVT, newState);
}

/*********************************************************************
 * @fn      ETX_processStateChangeEvt
 *
 * @brief   Process a pending GAP Role state change event.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void ETX_processStateChangeEvt(gaprole_States_t newState) {
#ifdef PLUS_BROADCASTER
	static bool firstConnFlag = false;
#endif // PLUS_BROADCASTER

	switch (newState) {
		case GAPROLE_STARTED: {
			uint8_t ownAddress[B_ADDR_LEN];
			uint8_t systemId[DEVINFO_SYSTEM_ID_LEN];

			GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

			// use 6 bytes of device address for 8 bytes of system ID value
			systemId[0] = ownAddress[0];
			systemId[1] = ownAddress[1];
			systemId[2] = ownAddress[2];

			// set middle bytes to zero
			systemId[4] = 0x00;
			systemId[3] = 0x00;

			// shift three bytes up
			systemId[7] = ownAddress[5];
			systemId[6] = ownAddress[4];
			systemId[5] = ownAddress[3];

			DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN,
					systemId);

			appState = APP_STATE_IDLE;

			// Display device address
			uout0(Util_convertBdAddr2Str(ownAddress));
			uout0("Initialized");
			Board_ledControl(BOARD_LED_ID_R, BOARD_LED_STATE_OFF, 0);

		}
		break;

		case GAPROLE_ADVERTISING:
			appState = APP_STATE_IDLE;
			uout0("Advertising");
			Board_ledControl(BOARD_LED_ID_R, BOARD_LED_STATE_FLASH, 100);
		break;

#ifdef PLUS_BROADCASTER
			/* After a connection is dropped a device in PLUS_BROADCASTER will continue
			 * sending non-connectable advertisements and shall sending this change of
			 * state to the application.  These are then disabled here so that sending
			 * connectable advertisements can resume.
			 */
			case GAPROLE_ADVERTISING_NONCONN:
			{
				uint8_t advertEnabled = FALSE;

				// Disable non-connectable advertising.
				GAPRole_SetParameter(GAPROLE_ADV_NONCONN_ENABLED, sizeof(uint8_t),
						&advertEnabled);

				advertEnabled = TRUE;

				// Enabled connectable advertising.
				GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
						&advertEnabled);

				// Reset flag for next connection.
				firstConnFlag = false;

				ETX_freeAttRsp(bleNotConnected);
			}
			break;
#endif //PLUS_BROADCASTER

		case GAPROLE_CONNECTED: {
			linkDBInfo_t linkInfo;
			uint8_t numActive = 0;

			//Util_startClock(&periodicClock);

			numActive = linkDB_NumActive();

			// Use numActive to determine the connection handle of the last
			// connection
			if (linkDB_GetInfo(numActive - 1, &linkInfo) == SUCCESS) {
				uout1("Num Conns: %d", (uint16_t )numActive);
				uout0(Util_convertBdAddr2Str(linkInfo.addr));
			} else {
				uint8_t peerAddress[B_ADDR_LEN];

				GAPRole_GetParameter(GAPROLE_CONN_BD_ADDR, peerAddress);

				uout0("Connected");
				uout0(Util_convertBdAddr2Str(peerAddress));
			}
			Board_ledControl(BOARD_LED_ID_R, BOARD_LED_STATE_FLASH, 500);

#ifdef PLUS_BROADCASTER
			// Only turn advertising on for this state when we first connect
			// otherwise, when we go from connected_advertising back to this state
			// we will be turning advertising back on.
			if (firstConnFlag == false)
			{
				uint8_t advertEnabled = FALSE; // Turn on Advertising

				// Disable connectable advertising.
				GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
						&advertEnabled);

				// Set to true for non-connectabel advertising.
				advertEnabled = TRUE;

				// Enable non-connectable advertising.
				GAPRole_SetParameter(GAPROLE_ADV_NONCONN_ENABLED, sizeof(uint8_t),
						&advertEnabled);
				firstConnFlag = true;
			}
#endif // PLUS_BROADCASTER
		}
		break;

		case GAPROLE_CONNECTED_ADV:
			uout0("Connected Advertising");
		break;

		case GAPROLE_WAITING:
			//Util_stopClock(&periodicClock);
			ETX_freeAttRsp(bleNotConnected);

			uout0("Disconnected");
			Board_ledControl(BOARD_LED_ID_R, BOARD_LED_STATE_OFF, 0);

			// Clear remaining lines
			//Display_clearLines(dispHandle, 3, 5);
		break;

		case GAPROLE_WAITING_AFTER_TIMEOUT:
			ETX_freeAttRsp(bleNotConnected);

			uout0("Timed Out");

			// Clear remaining lines
			//Display_clearLines(dispHandle, 3, 5);

#ifdef PLUS_BROADCASTER
			// Reset flag for next connection.
			firstConnFlag = false;
#endif //#ifdef (PLUS_BROADCASTER)
		break;

		case GAPROLE_ERROR:
			uout0("Error");
		break;

		default:
			//Display_clearLine(dispHandle, 2);
		break;
	}

}

/*********************************************************************
 * @fn      ETX_charValueChange/EnquireCB
 *
 * @brief   Callback from Simple Profile indicating a characteristic
 *          value change.
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  None.
 */
static void ETX_charValueChangeCB(uint8_t paramID) {
	ETX_enqueueMsg(ETX_CHAR_CHANGE_EVT, paramID);
}

static void ETX_charValueEnquireCB(uint8_t paramID) {
	ETX_enqueueMsg(ETX_CHAR_ENQUIRE_EVT, paramID);
}

/*********************************************************************
 * @fn      ETX_processCharValueChangeEvt
 *
 * @brief   Process a pending Simple Profile characteristic value change
 *          event.
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  None.
 */
static void ETX_processCharValueChangeEvt(uint8_t paramID) {
	uint8_t newValue;

	switch (paramID) {

		case ETXPROFILE_CMD:
			ETXProfile_GetParameter(ETXPROFILE_CMD, &newValue);

			uout1("BS Command: 0x%02x", (uint8_t )newValue);
		break;

		case ETXPROFILE_DATA:
			ETXProfile_GetParameter(ETXPROFILE_DATA, &newValue);

			uout1("User Data: 0x%02x", (uint8_t )newValue);
		break;

		default:
			// should not reach here!
		break;
	}
}

static void ETX_processCharValueEnquireEvt(uint8_t paramID) {

	uint8_t newValue;
	switch (paramID) {

		case ETXPROFILE_CMD:
			ETXProfile_GetParameter(ETXPROFILE_CMD, &newValue);
			uout1("BS Command Submitted: 0x%02x", (uint8_t )newValue);

			newValue = 0;
			ETXProfile_SetParameter(ETXPROFILE_CMD, sizeof(newValue),
					&newValue);
		break;

		case ETXPROFILE_DATA:
			ETXProfile_GetParameter(ETXPROFILE_DATA, &newValue);
			uout1("User Data Submitted: 0x%02x", (uint8_t )newValue);

			newValue = 0;
			ETXProfile_SetParameter(ETXPROFILE_DATA, sizeof(newValue),
					&newValue);
		break;

		default:
			// should not reach here!
		break;
	}
}

/*********************************************************************
 * @fn      ETX_enqueueMsg
 *
 * @brief   Creates a message and puts the message in RTOS queue.
 *
 * @param   event - message event.
 * @param   state - message state.
 *
 * @return  None.
 */
static void ETX_enqueueMsg(uint8_t event, uint8_t state) {
	EtxEvt_t *pMsg;

	// Create dynamic pointer to message.
	if ((pMsg = ICall_malloc(sizeof(EtxEvt_t)))) {
		pMsg->hdr.event = event;
		pMsg->hdr.state = state;

		// Enqueue the message.
		Util_enqueueMsg(appMsgQueue, sem, (uint8*) pMsg);
	}
}

/*********************************************************************
 * @fn      ETX_keyChangeHandler
 *
 * @brief   Key event handler function
 *
 * @param   a0 - ignored
 *
 * @return  none
 */
void ETX_keyChangeHandler(uint8_t keys) {
	ETX_enqueueMsg(ETX_KEY_CHANGE_EVT, keys);
}

/*********************************************************************
 * @fn      ETX_handleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void ETX_handleKeys(uint8_t shift, uint8_t keys) {
	//uout0("handleKey() called");
	uint8_t advertEnable = FALSE;
	switch (appState) {
		case APP_STATE_INIT:
		break;

		case APP_STATE_IDLE:
			if (keys & KEY_RIGHT) {
				ETX_Advert_UpdateDestinyBS();
				bStatus_t rtn = 0;
				rtn = GAPRole_SetParameter(GAPROLE_ADVERT_DATA,
						sizeof(advertData), advertData);
				if (rtn == SUCCESS)
				uout1("BS set to 0x%02x", destBsID);
				appState = APP_STATE_ADVERT;
			}
		break;
		case APP_STATE_ADVERT:
			if (keys & KEY_LEFT) {
				advertEnable = FALSE;
				GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
						&advertEnable);
				appState = APP_STATE_IDLE;
			}
			if (keys & KEY_RIGHT) {
				uint32 newValue = 0;
				ETXProfile_GetParameter(ETXPROFILE_DATA, &newValue);
				newValue += 1;
				ETXProfile_SetParameter(ETXPROFILE_DATA, sizeof(uint32),
						&newValue);
				appState = APP_STATE_IDLE;
			}
		break;
		default:
		break;

	}
}

static void ETX_DevId_Find(uint8_t* nvBuf) {
	uint8_t rtn = osal_snv_read(ETX_DEVID_NV_ID, ETX_DEVID_LEN,
			(uint8 *) nvBuf);
	if (rtn == SUCCESS)
		uout1("Device ID found: 0x%08x",
				BUILD_UINT32(nvBuf[0], nvBuf[1], nvBuf[2], nvBuf[3]));
	return;
}

static void ETX_DevId_Refresh(uint8_t IdPrefix, uint8_t* nvBuf) {
	uint32_t rnd = Util_GetTRNG() % 0xFFFFFF;
	nvBuf[0] = rnd % 0xFF;
	nvBuf[1] = (rnd >> 8) % 0xFF;
	nvBuf[2] = (rnd >> 16) % 0xFF;
	nvBuf[3] = IdPrefix;
	uint8_t rtn = osal_snv_write(ETX_DEVID_NV_ID, ETX_DEVID_LEN,
			(uint8 *) nvBuf);
	if (rtn == SUCCESS) {
		rtn = osal_snv_read(ETX_DEVID_NV_ID, ETX_DEVID_LEN, (uint8 *) nvBuf);
		if (rtn == SUCCESS)
			uout1("Device ID refreshed: 0x%08x",
					BUILD_UINT32(nvBuf[0], nvBuf[1], nvBuf[2], nvBuf[3]));
	}
	return;
}

static void ETX_ScanRsp_UpdateDeviceID() {

	scanRspData[11] = devID[0];
	scanRspData[12] = devID[1];
	scanRspData[13] = devID[2];
	scanRspData[14] = devID[3];
}

static void ETX_Advert_UpdateDestinyBS() {
	destBsID = 0x02;
	advertData[9] = destBsID;
}

/*********************************************************************
 *********************************************************************/
