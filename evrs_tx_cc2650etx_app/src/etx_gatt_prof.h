/*****************************************************************************
 * 
 * @filepath 	/evrs_tx_cc2650lp_app/src/etx_gatt_prof.h
 * 
 * @project 	evrs_tx_cc2650lp_app
 * 
 * @brief 		This is the header file of gatt profile of the transmitter
 *              using cc2650lp
 * 
 * @date 		12 Sep. 2018
 * 
 * @author		Ziyi@outlook.com.au
 *
 ****************************************************************************/

#ifndef ETXGATTPROFILE_H
#define ETXGATTPROFILE_H


/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Profile Parameters
#define ETXPROFILE_CMD         0x00  // RW uint8
#define ETXPROFILE_DATA        0x01  // RW uint8

// ETX Profile Service UUID
#define ETXPROFILE_SERV_UUID   0xAFF0

// Key Pressed UUID
#define ETXPROFILE_CMD_UUID    0xAFF2
#define ETXPROFILE_DATA_UUID   0xAFF4

// ETX Keys Profile Services bit fields
#define ETXPROFILE_SERVICE     0x00000001

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// Callback when a characteristic value has changed or enquired
typedef void (*ETXProfileChange_t)( uint8 paramID );
typedef void (*ETXProfileEnquire_t)( uint8 paramID );

typedef struct ETXProfileCBs_t
{
  ETXProfileChange_t        pfnETXProfileChange;  // Called when char value changed
  ETXProfileEnquire_t       pfnETXProfileEnquire; // Called when char has been enquired
} ETXProfileCBs_t;



/*********************************************************************
 * API FUNCTIONS
 */


/*
 * ETXProfile_AddService- Initializes the ETX GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t ETXProfile_AddService( uint32 services );

/*
 * ETXProfile_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
extern bStatus_t ETXProfile_RegisterAppCBs( ETXProfileCBs_t *appCallbacks );

/*
 * ETXProfile_SetParameter - Set a ETX GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t ETXProfile_SetParameter( uint8 param, uint8 len, void *value );

/*
 * ETXProfile_GetParameter - Get a ETX GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t ETXProfile_GetParameter( uint8 param, void *value );


/*********************************************************************
*********************************************************************/


#endif /* ETXGATTPROFILE_H */
