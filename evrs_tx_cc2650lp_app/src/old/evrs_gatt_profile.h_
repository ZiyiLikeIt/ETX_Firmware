/*****************************************************************************

file	evrs_gatt_profile.h

brief	This is the header file of gatt profile of the transmitter using 
		cc2650lp

proj	EVRS

date	0350pm 14 Aug 2018

author	Ziyi

*****************************************************************************/

#ifndef EVRSGATTPROFILE_H
#define EVRSGATTPROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Profile Parameters
#define EVRSPROFILE_SYSID				0x00  // RW uint8
#define EVRSPROFILE_DEVID				0x01  // RW uint8
#define EVRSPROFILE_CMD				0x02  // RW uint8
#define EVRSPROFILE_DATA				0x03  // RW uint8

// EVRS Profile Service UUID
#define EVRSPROFILE_SERV_UUID       	0xAFF0

// Key Pressed UUID
#define EVRSPROFILE_SYSID_UUID         	0xAFF2
#define EVRSPROFILE_DEVID_UUID      	0xAFF4
#define EVRSPROFILE_CMD_UUID         	0xAFF8
#define EVRSPROFILE_DATA_UUID        	0xAFFE

// EVRS Keys Profile Services bit fields
#define EVRSPROFILE_SERVICE         	0x00000001

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// Callback when a characteristic value has changed
typedef void (*EVRSProfileChange_t)( uint8 paramID );

typedef struct
{
  EVRSProfileChange_t        pfnEVRSProfileChange;  // Called when characteristic value changes
} EVRSProfileCBs_t;



/*********************************************************************
 * API FUNCTIONS
 */


/*
 * EVRSProfile_AddService- Initializes the EVRS GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t EVRSProfile_AddService( uint32 services );

/*
 * EVRSProfile_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
extern bStatus_t EVRSProfile_RegisterAppCBs( EVRSProfileCBs_t *appCallbacks );

/*
 * EVRSProfile_SetParameter - Set a EVRS GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t EVRSProfile_SetParameter( uint8 param, uint8 len, void *value );

/*
 * EVRSProfile_GetParameter - Get a EVRS GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t EVRSProfile_GetParameter( uint8 param, void *value );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* EVRSGATTPROFILE_H */
