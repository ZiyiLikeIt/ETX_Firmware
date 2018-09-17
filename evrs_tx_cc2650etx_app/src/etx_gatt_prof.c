/*****************************************************************************
 * 
 * @filepath 	/evrs_tx_cc2650lp_app/src/etx_gatt_prof.c
 * 
 * @project 	evrs_tx_cc2650lp_app
 * 
 * @brief 		TODO
 * 
 * @date 		12 Sep. 2018
 * 
 * @author		Ziyi@outlook.com.au
 *
 ****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include "bcomdef.h"
#include "osal.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "etx_gatt_prof.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        17

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// ETX GATT Profile Service UUID: 0xAFF0
CONST uint8 ETXProfileServUUID[ATT_BT_UUID_SIZE] =
        { LO_UINT16(ETXPROFILE_SERV_UUID), HI_UINT16(ETXPROFILE_SERV_UUID) };

// Command number UUID: 0xAFF2
CONST uint8 ETXProfileCmdUUID[ATT_BT_UUID_SIZE] =
        { LO_UINT16(ETXPROFILE_CMD_UUID), HI_UINT16(ETXPROFILE_CMD_UUID) };

// User data UUID: 0xAFF4
CONST uint8 ETXProfileDataUUID[ATT_BT_UUID_SIZE] =
        { LO_UINT16(ETXPROFILE_DATA_UUID), HI_UINT16(ETXPROFILE_DATA_UUID) };

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static ETXProfileCBs_t *ETXProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// ETX Profile Service attribute
static CONST gattAttrType_t ETXProfileService =
        { ATT_BT_UUID_SIZE, ETXProfileServUUID };

// ETX Profile Command BS Properties
static uint8 ETXProfileCmdProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Command BS Value
static uint8 ETXProfileCmd = 0;

// ETX Profile BS Command User Description
static uint8 ETXProfileCmdUserDesp[11] = "BS Command";

// ETX Profile User Data Properties
static uint8 ETXProfileDataProps = GATT_PROP_READ;

// User Data Value
static uint8 ETXProfileData = 0;

// ETX Profile User Data User Description
static uint8 ETXProfileDataUserDesp[10] = "User Data";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t ETXProfileAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
// ETX Profile Service
        { { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
        GATT_PERMIT_READ, /* permissions */
        0, /* handle */
        (uint8 *) &ETXProfileService /* pValue */
        },

        // BS Command Declaration
        { { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ, 0, &ETXProfileCmdProps },

        // BS Command Value
        { { ATT_BT_UUID_SIZE, ETXProfileCmdUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, &ETXProfileCmd },

        // BS Command User Description
        { { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 0, ETXProfileCmdUserDesp },

        // User Data Declaration
        { { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ, 0, &ETXProfileDataProps },

        // User Data Value
        { { ATT_BT_UUID_SIZE, ETXProfileDataUUID },
        GATT_PERMIT_READ, 0, &ETXProfileData },

        // User Data User Description
        { { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 0, ETXProfileDataUserDesp }, };

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t ETXProfile_ReadAttrCB(uint16_t connHandle,
        gattAttribute_t *pAttr, uint8_t *pValue, uint16_t *pLen,
        uint16_t offset, uint16_t maxLen, uint8_t method);

static bStatus_t ETXProfile_WriteAttrCB(uint16_t connHandle,
        gattAttribute_t *pAttr, uint8_t *pValue, uint16_t len, uint16_t offset,
        uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// ETX Profile Service Callbacks
// Note: When an operation on a characteristic requires authorization and
// pfnAuthorizeAttrCB is not defined for that characteristic's service, the
// Stack will report a status of ATT_ERR_UNLIKELY to the client.  When an
// operation on a characteristic requires authorization the Stack will call
// pfnAuthorizeAttrCB to check a client's authorization prior to calling
// pfnReadAttrCB or pfnWriteAttrCB, so no checks for authorization need to be
// made within these functions.
CONST gattServiceCBs_t ETXProfileCBs = {
        ETXProfile_ReadAttrCB, // Read callback function pointer
        ETXProfile_WriteAttrCB, // Write callback function pointer
        NULL                     // Authorization callback function pointer
        };

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      ETXProfile_AddService
 *
 * @brief   Initializes the ETX Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t ETXProfile_AddService(uint32 services) {
    uint8 status;

    // Allocate Client Characteristic Configuration table

    // Initialize Client Characteristic Configuration attributes
    //GATTServApp_InitCharCfg( INVALID_CONNHANDLE, ETXProfileDataConfig );

    if (services & ETXPROFILE_SERVICE)
    {
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(ETXProfileAttrTbl,
                GATT_NUM_ATTRS(ETXProfileAttrTbl),
                GATT_MAX_ENCRYPT_KEY_SIZE, &ETXProfileCBs);
    } else
    {
        status = SUCCESS;
    }

    return (status);
}

/*********************************************************************
 * @fn      ETXProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t ETXProfile_RegisterAppCBs(ETXProfileCBs_t *appCallbacks) {
    if (appCallbacks)
    {
        ETXProfile_AppCBs = appCallbacks;

        return ( SUCCESS);
    } else
    {
        return ( bleAlreadyInRequestedMode);
    }
}

/*********************************************************************
 * @fn      ETXProfile_SetParameter
 *
 * @brief   Set a ETX Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t ETXProfile_SetParameter(uint8 param, uint8 len, void *value) {
    bStatus_t rtn = SUCCESS;
    switch (param)
    {
        case ETXPROFILE_CMD:
            if (len == sizeof(uint8))
            {
                ETXProfileCmd = *((uint8*) value);
            } else
            {
                rtn = bleInvalidRange;
            }
            break;

        case ETXPROFILE_DATA:
            if (len == sizeof(uint8))
            {
                ETXProfileData = *((uint8*) value);
            } else
            {
                rtn = bleInvalidRange;
            }
            break;

        default:
            rtn = INVALIDPARAMETER;
            break;
    }

    return (rtn);
}

/*********************************************************************
 * @fn      ETXProfile_GetParameter
 *
 * @brief   Get a ETX Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t ETXProfile_GetParameter(uint8 param, void *value) {
    bStatus_t rtn = SUCCESS;
    switch (param)
    {
        case ETXPROFILE_CMD:
            *((uint8*) value) = ETXProfileCmd;
            break;

        case ETXPROFILE_DATA:
            *((uint8*) value) = ETXProfileData;
            break;

        default:
            rtn = INVALIDPARAMETER;
            break;
    }

    return (rtn);
}

/*********************************************************************
 * @fn          ETXProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t ETXProfile_ReadAttrCB(uint16_t connHandle,
        gattAttribute_t *pAttr, uint8_t *pValue, uint16_t *pLen,
        uint16_t offset, uint16_t maxLen, uint8_t method) {
    bStatus_t status = SUCCESS;
    uint8 notifyApp = 0xFF;

    // Make sure it's not a blob operation (no attributes in the profile are long)
    if (offset > 0)
    {
        return ( ATT_ERR_ATTR_NOT_LONG);
    }

    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16 uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch (uuid)
        {
            // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
            // gattserverapp handles those reads

            case ETXPROFILE_CMD_UUID:
            case ETXPROFILE_DATA_UUID:
                *pLen = 1;
                pValue[0] = *pAttr->pValue;
                notifyApp = (uuid == ETXPROFILE_CMD_UUID)?(ETXPROFILE_CMD):(ETXPROFILE_DATA);
                break;

            default:
                // Should never get here! (characteristics 3 and 4 do not have read permissions)
                *pLen = 0;
                status = ATT_ERR_ATTR_NOT_FOUND;
                break;
        }
    } else
    {
        // 128-bit UUID
        *pLen = 0;
        status = ATT_ERR_INVALID_HANDLE;
    }

    // If a characteristic value enquired then callback function to notify application
    if ((notifyApp != 0xFF) && ETXProfile_AppCBs
            && ETXProfile_AppCBs->pfnETXProfileEnquire)
    {
        ETXProfile_AppCBs->pfnETXProfileEnquire(notifyApp);
    }

    return (status);
}

/*********************************************************************
 * @fn      ETXProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t ETXProfile_WriteAttrCB(uint16_t connHandle,
        gattAttribute_t *pAttr, uint8_t *pValue, uint16_t len, uint16_t offset,
        uint8_t method) {
    bStatus_t status = SUCCESS;
    uint8 notifyApp = 0xFF;

    if (pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16 uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch (uuid)
        {
            case ETXPROFILE_CMD_UUID:

                //Validate the value
                // Make sure it's not a blob oper
                if (offset == 0)
                {
                    if (len != 1)
                    {
                        status = ATT_ERR_INVALID_VALUE_SIZE;
                    }
                } else
                {
                    status = ATT_ERR_ATTR_NOT_LONG;
                }

                //Write the value
                if (status == SUCCESS)
                {
                    uint8 *pCurValue = (uint8 *)pAttr->pValue;
                    *pCurValue = pValue[0];

                    notifyApp = ETXPROFILE_CMD;
                }
                break;

            case ETXPROFILE_DATA_UUID:
            default:
                // Should never get here! (characteristics 2 and 4 do not have write permissions)
                status = ATT_ERR_ATTR_NOT_FOUND;
                break;
        }
    } else
    {
        // 128-bit UUID
        status = ATT_ERR_INVALID_HANDLE;
    }

    // If a characteristic value changed then callback function to notify application
    if ((notifyApp != 0xFF) && ETXProfile_AppCBs
            && ETXProfile_AppCBs->pfnETXProfileChange)
    {
        ETXProfile_AppCBs->pfnETXProfileChange(notifyApp);
    }

    return (status);
}

