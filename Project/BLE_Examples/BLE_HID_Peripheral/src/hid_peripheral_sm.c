/******************** (C) COPYRIGHT 2018 STMicroelectronics ********************
* File Name          : hid_peripheral_sm.c
* Author             : AMS - VMA RF Application Team
* Version            : V1.1.0
* Date               : 09-April-2018
* Description        : BlueNRG-1 HID/HOGP Peripheral state machine interface
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
/** @} */
/** \cond DOXYGEN_SHOULD_SKIP_THIS
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "hid_peripheral_config.h"
#include "hid_peripheral_sm.h"
#include "hid_peripheral.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "bluenrg1_api.h"
#include "bluenrg1_events.h"
#include "hal_types.h"


/* External variables --------------------------------------------------------*/
/* External Functions --------------------------------------------------------*/
__weak void hidSetReport_CB(uint8_t ID, uint8_t type, uint8_t len, uint8_t *data);
__weak void hidGetReport_CB(uint8_t ID, uint8_t type);
__weak void hidPassKeyRequest_CB(void);
__weak void hidChangeProtocolMode_CB(uint8_t mode);
__weak void hidControlPoint_CB(uint8_t value);

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define DEFAULT_IDLE_CONNECTION_TIMEOUT 500  // 500 ms
#define NOTIFICATION_ENABLED            0x01

/* Private variables ---------------------------------------------------------*/
static hidGlobalContextType globalContextMemory = {1,};
hidGlobalContextType *hidGlobalContext = &globalContextMemory;
HID_StateType HID_State = {HID_STATE_INIT, 1};
devContext_Type devContext;

/* Private macros ------------------------------------------------------------*/
#ifdef DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void DEBUG_MESSAGE(uint8_t prefix, uint8_t next)
{
  PRINTF("State transition from %02x to %02x\n", prefix, next);
}

#define SUBSTATE_TRANSITION(prefix, prevState, nextState)  \
  { if  (prefix##_##prevState != prefix##_##nextState)	   \
      { \
	DEBUG_MESSAGE(prefix##_##prevState, prefix##_##nextState); \
      } \
      newState = prefix##_##nextState;			   \
  }

#define STATE_TRANSITION(prefix, prevState, nextState) \
  { if  (prefix##_##prevState != prefix##_##nextState)			\
      { \
	DEBUG_MESSAGE(prefix##_##prevState, prefix##_##nextState);	\
      } \
    newState.state = prefix##_##nextState;				\
    if (prefix##_##prevState != prefix##_##nextState) {			\
      newState.subState = prefix##_##nextState | 0x0001;		\
    }									\
  }

#define GET_CURRENT_STATE() HID_State.state
#define GET_CURRENT_SUBSTATE() HID_State.subState

/* Private Functions ---------------------------------------------------------*/

static uint8_t getReportInfo(uint16_t attr_handle, uint8_t *id, uint8_t *type)
{
  uint8_t endReportList, i;
  
  *type = 0xff;

  if (attr_handle == (devContext.hidBootKeyboardInputReportHandle+1)) {
    *type = BOOT_KEYBOARD_INPUT_REPORT;
  } 
 
  if (attr_handle == (devContext.hidBootKeyboardOutputReportHandle+1)) {
    *type = BOOT_KEYBOARD_OUTPUT_REPORT;
  }
  
  if (attr_handle == (devContext.hidBootMouseInputReportHandle+1)) {
    *type = BOOT_MOUSE_INPUT_REPORT;
  }

  if (*type != 0xff) {
    *id = 0xff;
    return BLE_STATUS_SUCCESS;
  } 

  if (attr_handle == (devContext.batteryLevelHandle+1)) {
    *id = devContext.batteryReportID;
    *type = INPUT_REPORT;
    return BLE_STATUS_SUCCESS;
  }

  endReportList = FALSE;
  i = 0;
  while (!endReportList) {
    if ((devContext.hidReportHandle[i]+1) == attr_handle) {
      *id = devContext.hidReportID[i];
      *type = devContext.hidReportType[i];
    }
    i++;
    if (i == REPORT_NUMBER)
      endReportList = TRUE;
  }
  
  if (endReportList && (*type == 0xff))
    return BLE_STATUS_INVALID_PARAMS;
 
  return BLE_STATUS_SUCCESS;
}

/* Functions -----------------------------------------------------------------*/

/***** HID Bonding State machine *****/
uint8_t bondingStateMachine(uint8_t subState)
{
  uint8_t newState = subState;
  devBondableContextType *bondableContext = (devBondableContextType *) hidGlobalContext;
  uint8_t ret = BLE_STATUS_SUCCESS;

  switch(subState) {
  case  HID_SUBSTATE_DEVICE_BONDABLE_INIT:
    { 
      uint8_t i;

      SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_BONDABLE, INIT, WAITING); // Host Start Bonding procedure
      
      PRINTF("Connection Handle 0x%04x Master Address:", devContext.connHandle);
      for (i=0; i<6; i++) {
	PRINTF("%02x", devContext.addrMaster[i]);
      }
      PRINTF("\n");
    }
    break;
  case HID_SUBSTATE_DEVICE_BONDABLE_WAITING:
    {
      if (bondableContext->completed) {
	if (devContext.bonded) {
	  ret = hci_le_add_device_to_white_list(devContext.addrMasterType, devContext.addrMaster); // ??? [FC]
	  //ret = aci_gap_configure_whitelist();
	  if (ret == BLE_STATUS_SUCCESS) {
	    devContext.whiteListConfigured = TRUE;
	  } else {
	    PRINTF("Error during whitelist configuration\n");
	  }
	} else {
	  if (devContext.connected) {
	    ret = aci_gap_terminate(devContext.connHandle, 0x13); /* 0x13: Remote User Terminated Connection */
	    if (ret != BLE_STATUS_SUCCESS) {
	      PRINTF("Error during the GAP terminate connection\n");
	    }
	  }
	}
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_BONDABLE, WAITING, DONE); // Bondable proc ended
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_BONDABLE, WAITING, WAITING); // Procedure ongoning
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_BONDABLE_DONE:
    /* Nothing to be done */
  default:
    /* Invalid sub state */
    break;
  }

  return newState;
}

/***** HID Reconnection State machine *****/
uint8_t reconnectionStateMachine(uint8_t subState)
{
  uint8_t newState = subState;
  devReconnectionContextType *reconnContext = (devReconnectionContextType *) hidGlobalContext;
  uint8_t ret = 0;
  uint8_t serviceUUID[3] = {AD_TYPE_16_BIT_SERV_UUID};
  uint8_t local_name[21];

  switch(subState) {
  case  HID_SUBSTATE_DEVICE_RECONNECTION_INIT:
    {
      /* Disable the deep sleep flag */
      devContext.readyToDeepSleep = FALSE;
      reconnContext->completed = FALSE;
      reconnContext->connected = FALSE;
      reconnContext->bondCompleted = FALSE;
      
      serviceUUID[1] = (uint8_t)(HID_SERVICE_UUID & 0x00ff);
      serviceUUID[2] = (uint8_t)(HID_SERVICE_UUID>>8);
      
      if (devContext.normallyConnectable) {
	local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
	memcpy(&local_name[1], devContext.localName, devContext.localNameLen);
	ret = aci_gap_set_limited_discoverable(ADV_IND, DISC_ADV_REDUCED_POWER_MIN, DISC_ADV_REDUCED_POWER_MAX, 
                                               PUBLIC_ADDR, NO_WHITE_LIST_USE,
                                               (devContext.localNameLen+1), local_name, sizeof(serviceUUID), serviceUUID,
                                               0, 0);
      } else {
	ret = aci_gap_set_direct_connectable(PUBLIC_ADDR, HIGH_DUTY_CYCLE_DIRECTED_ADV, 
                                             devContext.addrMasterType, devContext.addrMaster, 0, 0);
      }
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Error during the Init reconnection procedure\n");
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, INIT, DONE); // Error during reconnection
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, INIT, WAITING); // Start reconnection procedure
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_RECONNECTION_WAITING:
    {
      if (reconnContext->completed) {
	if (reconnContext->connected) {
          SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAITING, WAIT_END_SECURITY); // Host Start Bonding procedure
	} else {
	  if (devContext.normallyConnectable) {
	    SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAITING, INIT); // Normally Connectable = ADV permanent
	  } else {
	    SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAITING, LOW_POWER_ADV); // Direct ADV failed
	  }
	}
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAITING, WAITING); // Procedure ongoing
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_RECONNECTION_LOW_POWER_ADV:
    {
      reconnContext->completed = FALSE;
      reconnContext->connected = FALSE;

      serviceUUID[1] = (uint8_t)(HID_SERVICE_UUID & 0x00ff);
      serviceUUID[2] = (uint8_t)(HID_SERVICE_UUID>>8);
      
      // Discoverable Parameter
      local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
      memcpy(&local_name[1], devContext.localName, devContext.localNameLen);
      ret = aci_gap_set_limited_discoverable(ADV_IND, DISC_ADV_HIGH_LATENCY_MIN, DISC_ADV_HIGH_LATENCY_MAX, 
                                             PUBLIC_ADDR, NO_WHITE_LIST_USE,
                                             (devContext.localNameLen+1), local_name, sizeof(serviceUUID), serviceUUID,
                                             0, 0);
      if (ret == BLE_STATUS_SUCCESS) {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, LOW_POWER_ADV, WAIT_LOW_POWER_ADV); // Start Reconn procedure
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, LOW_POWER_ADV, DONE); // Error in Reconnection procedure
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_RECONNECTION_WAIT_LOW_POWER_ADV:
    {
      if (reconnContext->completed) {
	if (reconnContext->connected) {
          SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAITING, WAIT_END_SECURITY); // Host Start Bonding procedure
	} else {
	  ret = aci_gap_set_non_discoverable();
	  if (ret != BLE_STATUS_SUCCESS) {
	    PRINTF("Error during the non discoverable set in reconnection\n");
	  }
	  SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAIT_LOW_POWER_ADV, DONE); //Low Power ADV failed
	}
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAIT_LOW_POWER_ADV, WAIT_LOW_POWER_ADV); //Procedure ongoing
      }

    }
    break;
  case HID_SUBSTATE_DEVICE_RECONNECTION_WAIT_END_SECURITY:
    {
      if (reconnContext->bondCompleted|| !reconnContext->connected) {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAIT_END_SECURITY, DONE); // Bond completed
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_RECONNECTION, WAIT_END_SECURITY, WAIT_END_SECURITY); // Procedure ongoing
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_RECONNECTION_DONE:
    /* Nothing to be done */
  default:
    /* Invalid sub state */
    break;
  }

  return newState;
}

/***** HID Discovery State machine *****/
uint8_t discoverableStateMachine(uint8_t subState)
{
  uint8_t newState = subState;
  devDiscoverableContextType *devDiscContext = (devDiscoverableContextType *) hidGlobalContext;

  switch(subState) {
  case  HID_SUBSTATE_DEVICE_DISCOVERABLE_INIT:
    {
      SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_DISCOVERABLE, INIT, WAIT_HOST); // Device in Discoverable mode
    }
    break;
  case HID_SUBSTATE_DEVICE_DISCOVERABLE_WAIT_HOST:
    {
      if (devDiscContext->complete) {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_DISCOVERABLE, WAIT_HOST, DONE); // Device Discoverable completed
      } else {
	SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_DISCOVERABLE, WAIT_HOST, WAIT_HOST); // Ongoing
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_DISCOVERABLE_DONE:
    /* Nothing to be done */
  default:
    /* Invalid sub state */
    break;
  }

  return newState;
}

/***** HID Report Configuration State machine *****/
uint8_t reportConfigurationStateMachine(uint8_t subState)
{
  uint8_t newState = subState;
  devReportConfigContextType *devReportConfig = (devReportConfigContextType *) hidGlobalContext;
  uint8_t ret;

  switch(subState) {
  case  HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_INIT:
    {
      devReportConfig->batteryReportConfigured = FALSE;
      devReportConfig->indexReport = 0;
      devReportConfig->numberOfRepetition = 0;
      SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION, INIT, STATUS); // Device in Discoverable mode
    }
    break;
  case HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_STATUS:
    {
      if (devContext.hidReportType[devReportConfig->indexReport] == INPUT_REPORT) {
        if (hidInputReportReady(devReportConfig->indexReport)) {
	    devReportConfig->indexReport++;
	    devReportConfig->numberOfRepetition = 0;
        } else {
          devReportConfig->numberOfRepetition++;
        }
      } else {
        devReportConfig->indexReport++;
      }
      if ((devReportConfig->indexReport >= REPORT_NUMBER) || (devReportConfig->numberOfRepetition >= 10)) {
        if (devReportConfig->numberOfRepetition >= 10) {
          uint8_t configDesc[2]={NOTIFICATION_ENABLED, 0x00};
          uint8_t i;
          for (i=0; i<REPORT_NUMBER; i++) {
            if (!hidInputReportReady(i) && (devContext.hidReportType[i] == INPUT_REPORT)) {
              ret = aci_gatt_set_desc_value(devContext.hidServHandle,
                                            devContext.hidReportHandle[i],
                                            (devContext.hidReportHandle[i]+2),
                                            0, 2, configDesc);
              if (ret != BLE_STATUS_SUCCESS) {
                PRINTF("Error during the aci_gatt_set_desc_value(hidreportNumber[%d]) = 0x%02x\r\n",
                       i, ret);
              }
            }
          }
        }
        devReportConfig->numberOfRepetition = 0;
        SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION, STATUS, BATTERY_STATUS); // Read Battery Report status
      } else {
        SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION, STATUS, STATUS); // Read Input Report status
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_BATTERY_STATUS:
    {
      uint8_t configDesc[2];
      uint16_t data_len, len;
      if (aci_gatt_read_handle_value((devContext.batteryLevelHandle+2), 0, 2, &len, &data_len, configDesc) != BLE_STATUS_SUCCESS) {
        PRINTF("Error during the gatt read handle value\n");
      }
      if (configDesc[0] == NOTIFICATION_ENABLED) {
        devReportConfig->batteryReportConfigured = TRUE;
      } else {
        devReportConfig->numberOfRepetition++;
      }
      if ((devReportConfig->numberOfRepetition >= 10) || devReportConfig->batteryReportConfigured) {
        if (!devReportConfig->batteryReportConfigured) {
          configDesc[0] = NOTIFICATION_ENABLED;
          ret = aci_gatt_set_desc_value(devContext.batteryServHandle,
                                        devContext.batteryLevelHandle,
                                        (devContext.batteryLevelHandle+2),
                                        0, 2, configDesc);
          if (ret != BLE_STATUS_SUCCESS) {
            PRINTF("Error during the aci_gatt_set_desc_value(Battery) = 0x%02x\r\n", ret);
          }
        }
        SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION, BATTERY_STATUS, DONE); // Read Report status done
      } else {
        SUBSTATE_TRANSITION(HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION, BATTERY_STATUS, BATTERY_STATUS); // Read Battery Input Report status
      }
    }
    break;
  case HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_DONE:
    /* Nothing to be done */
  default:
    /* Invalid sub state */
    break;
  }

  return newState;
}

/***** HID Idle State machine *****/
uint8_t idleStateMachine(uint8_t subState)
{
  uint8_t newState = subState;
  devIdleContextType *devIdleContext = (devIdleContextType *) hidGlobalContext;
  uint8_t ret;

  switch(subState) {
  case HID_SUBSTATE_IDLE_CONN_UPD_PARAM:
    {
      if (!devIdleContext->updateConnParamEnded) {
        SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, CONN_UPD_PARAM, CONN_UPD_PARAM); // Wait end of procedure
      } else {
        SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, CONN_UPD_PARAM, START_IDLE_TIMER); // Conn update proc end
      }
    }
    break;
  case  HID_SUBSTATE_IDLE_START_IDLE_TIMER:
    {
      devIdleContext->idleTimeout = FALSE;
      //HAL_VTimerStart_ms(HID_SLEEP_TIMER_NUMBER, devContext.idleConnectionTimeout);
      SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, START_IDLE_TIMER, WAIT_TIMEOUT); // Start idle timer
    }
    break;
  case HID_SUBSTATE_IDLE_WAIT_TIMEOUT:
    {
      if (devContext.notificationPending) {
        HAL_VTimer_Stop(HID_SLEEP_TIMER_NUMBER);
        devContext.notificationPending = FALSE;
        SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, WAIT_TIMEOUT, START_IDLE_TIMER); // Restart idle timer
      } else {
        if (!devIdleContext->idleTimeout) {
          SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, WAIT_TIMEOUT, WAIT_TIMEOUT); // idle cycle
        } else {
          ret = hidCloseConnection();
          if (ret != BLE_STATUS_SUCCESS) {
            PRINTF("Disconnection failed with status = 0x%02x\n", ret);
          }    
          SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, WAIT_TIMEOUT, WAIT_DISCONNECTION); // Disconnection
        }
      }
    }
    break;
    case HID_SUBSTATE_IDLE_WAIT_DISCONNECTION:
      {
        if (devContext.connected) {
          SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, WAIT_DISCONNECTION, WAIT_DISCONNECTION); // wait end procedure
        } else {
          SUBSTATE_TRANSITION(HID_SUBSTATE_IDLE, WAIT_DISCONNECTION, DONE); // end procedure
        }
      }
      break;
  case HID_SUBSTATE_IDLE_DONE:
    /* Nothing to be done */
  default:
    /* Invalid sub state */
    break;
  }

  return newState;
}

/***** HID Global State machine *****/
HID_StateType hidStateMachine(HID_StateType state)
{
  HID_StateType newState = state;
  uint8_t ret;

  switch (state.state) {
  case HID_STATE_INIT:
    {
      devContext.idleConnectionTimeout = DEFAULT_IDLE_CONNECTION_TIMEOUT;
      devContext.protocolMode = REPORT_PROTOCOL_MODE;
      devContext.readyToDeepSleep = FALSE;
    }
    break;
  case HID_STATE_DEVICE_DISCOVERABLE:
    {
      devDiscoverableContextType *devDiscContext = (devDiscoverableContextType *) hidGlobalContext;

      newState.subState = discoverableStateMachine(state.subState);
      if (newState.subState == HID_SUBSTATE_DEVICE_DISCOVERABLE_DONE) { 
	if (devDiscContext->connected) {
          devBondableContextType *bondableContext = (devBondableContextType *) hidGlobalContext;
          bondableContext->completed = FALSE;
	  STATE_TRANSITION(HID_STATE, DEVICE_DISCOVERABLE, DEVICE_BONDABLE); // Device Discoverable completed
	} else {
	  STATE_TRANSITION(HID_STATE, DEVICE_DISCOVERABLE, IDLE); // Device Discoverable completed
	}
      }
    }
    break;
  case  HID_STATE_DEVICE_BONDABLE:
    {
      newState.subState = bondingStateMachine(state.subState);
      if (newState.subState == HID_SUBSTATE_DEVICE_BONDABLE_DONE) { 
	STATE_TRANSITION(HID_STATE, DEVICE_BONDABLE, DEVICE_REPORT_CONFIGURATION); // Bonding procedure completed
      }
    }
    break;
  case HID_STATE_DEVICE_RECONNECTION:
    {
      newState.subState = reconnectionStateMachine(state.subState);
      if (newState.subState == HID_SUBSTATE_DEVICE_RECONNECTION_DONE) { 
	STATE_TRANSITION(HID_STATE, DEVICE_RECONNECTION, DEVICE_REPORT_CONFIGURATION); // Reconnection procedure completed
      }
    }
    break;
  case HID_STATE_DEVICE_REPORT_CONFIGURATION:
    {
      newState.subState = reportConfigurationStateMachine(state.subState);
      if (newState.subState == HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_DONE) { 
        devIdleContextType *devIdleContext = (devIdleContextType *) hidGlobalContext;
        devIdleContext->updateConnParamEnded = FALSE;
        ret = aci_l2cap_connection_parameter_update_req(devContext.connHandle, 
                                                        devContext.interval_min, devContext.interval_max,
                                                        devContext.slave_latency, devContext.timeout_multiplier);
        if (ret == BLE_STATUS_SUCCESS) {
          PRINTF("Conn Update Param Ongoing\n");
        } else {
          PRINTF("Conn Update Param Error\n");
        }
	STATE_TRANSITION(HID_STATE, DEVICE_REPORT_CONFIGURATION, IDLE); // Report Config procedure completed
      }
    }
    break;
  case HID_STATE_IDLE:
    {
      newState.subState = idleStateMachine(state.subState);
      if ((newState.subState == HID_SUBSTATE_IDLE_DONE) || !devContext.connected) {
        devContext.notificationPending = FALSE;
        STATE_TRANSITION(HID_STATE, IDLE, DEVICE_BONDED); // Device Bonded and not connected
      }
    }
    break;
  case HID_STATE_DEVICE_BONDED:
    {
      devContext.readyToDeepSleep = TRUE;
      STATE_TRANSITION(HID_STATE, DEVICE_BONDED, DEVICE_RECONNECTION); // Start Reconnection procedure
    }
    break;
  default:
    /* Error state not defined */
    break;
  }
  return newState;
}

/******* HID/HOGP Event Callback *******/

void HID_Lib_hci_disconnection_complete_event(uint8_t Status,
                                              uint16_t Connection_Handle,
                                              uint8_t Reason)
{
  devReconnectionContextType *reconnContext = (devReconnectionContextType *) hidGlobalContext;
  
  if (Status == BLE_STATUS_SUCCESS) {
    devContext.connected = FALSE;
    if (GET_CURRENT_STATE() == HID_STATE_DEVICE_RECONNECTION) {
      reconnContext->connected = FALSE;
    }
  }
}

void HID_Lib_hci_le_connection_complete_event(uint8_t Status,
                                              uint16_t Connection_Handle,
                                              uint8_t Role,
                                              uint8_t Peer_Address_Type,
                                              uint8_t Peer_Address[6],
                                              uint16_t Conn_Interval,
                                              uint16_t Conn_Latency,
                                              uint16_t Supervision_Timeout,
                                              uint8_t Master_Clock_Accuracy)
{
  if (GET_CURRENT_STATE() == HID_STATE_DEVICE_DISCOVERABLE) {
    devDiscoverableContextType *devDiscContext = (devDiscoverableContextType *) hidGlobalContext;
    
    if (Status == BLE_STATUS_SUCCESS) {
      if (devDiscContext->modeUsed == GENERAL_DISCOVERABLE_MODE)
        devDiscContext->complete = TRUE;
      devDiscContext->connected = TRUE;
      devContext.connected = TRUE;
    } else {
      devDiscContext->connected = FALSE;
    }
  }
  
  if (GET_CURRENT_STATE() == HID_STATE_DEVICE_RECONNECTION) {
    devReconnectionContextType *reconnContext = (devReconnectionContextType *) hidGlobalContext;
    
    if (Status == BLE_STATUS_SUCCESS) {
      reconnContext->connected = TRUE;
      devContext.connected = TRUE;
    } else {
      reconnContext->connected = FALSE;
    }
    reconnContext->completed = TRUE;
  }
  
  if (Status == BLE_STATUS_SUCCESS) {
    devContext.addrMasterType = Peer_Address_Type;
    devContext.connHandle = Connection_Handle;
    memcpy(devContext.addrMaster, Peer_Address, 6);
  }  
}

void HID_Lib_aci_gap_limited_discoverable_event()
{
  if (GET_CURRENT_STATE() == HID_STATE_DEVICE_DISCOVERABLE) {
    devDiscoverableContextType *devDiscContext = (devDiscoverableContextType *) hidGlobalContext;
    devDiscContext->complete = TRUE;
  }
  
  if (GET_CURRENT_STATE() == HID_STATE_DEVICE_RECONNECTION) {
    devReconnectionContextType *reconnContext = (devReconnectionContextType *) hidGlobalContext;
    reconnContext->completed = TRUE;
  }
  
  PRINTF("Limited Discoverable Timeout\n");
}

void HID_Lib_aci_gap_pass_key_req_event(uint16_t Connection_Handle)
{
  hidPassKeyRequest_CB();
}

void HID_Lib_aci_gap_pairing_complete_event(uint16_t Connection_Handle,
                                            uint8_t Status)
{
  if (Status == BLE_STATUS_SUCCESS) {
    devContext.connHandle = Connection_Handle;
    devContext.bonded = TRUE;
  } else {
    devContext.bonded = FALSE;
  }
	  
  if (GET_CURRENT_STATE() == HID_STATE_DEVICE_BONDABLE) {
    devBondableContextType *bondableContext = (devBondableContextType *) hidGlobalContext;
    bondableContext->completed = TRUE;
  }
	  
  if (GET_CURRENT_STATE() == HID_STATE_DEVICE_RECONNECTION) {
    devReconnectionContextType *reconnContext = (devReconnectionContextType *) hidGlobalContext;
    reconnContext->bondCompleted = TRUE;
  }
}

void HID_Lib_aci_gap_bond_lost_event()
{
  static uint8_t ret;
  
  devContext.bonded = FALSE;
  ret = aci_gap_allow_rebond(devContext.connHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error during allow rebond\n");
  }
  PRINTF("BOND LOST!!!!!!\n");
}

void HID_Lib_aci_gatt_attribute_modified_event(uint16_t Connection_Handle,
                                               uint16_t Attr_Handle,
                                               uint16_t Offset,
                                               uint16_t Attr_Data_Length,
                                               uint8_t Attr_Data[])
{
  uint8_t id, type; 

  if (Attr_Handle == (devContext.hidProtModeHandle+1)) {
    if (Attr_Data_Length == 1) {
      hidChangeProtocolMode_CB(Attr_Data[0]);
    } else {
      PRINTF("ERROR during change the PROTOCOL MODE\n");
    }
  } else {
    if (Attr_Handle == (devContext.hidControlPointHandle+1)) {
      if (Attr_Data_Length == 1) {
        hidControlPoint_CB(Attr_Data[0]);
      } else {
        PRINTF("ERROR during change the CONTROL POINT\n");
      }
    } else {
      if (getReportInfo(Attr_Handle, &id, &type) == BLE_STATUS_SUCCESS) {
        hidSetReport_CB(id, type, Attr_Data_Length, Attr_Data);
      } else {
        PRINTF("Enable Charac Notification: 0x%02x%02x\n", Attr_Data[1], Attr_Data[0]);
      }
    }
  }
}

void HID_Lib_aci_gatt_read_permit_req_event(uint16_t Connection_Handle,
                                            uint16_t Attribute_Handle,
                                            uint16_t Offset)
{
  uint8_t id, type;
  
  if (getReportInfo(Attribute_Handle, &id, &type) == BLE_STATUS_SUCCESS)
    hidGetReport_CB(id, type);
}

void HID_Lib_aci_l2cap_connection_update_resp_event(uint16_t Connection_Handle,
                                                    uint16_t Result)
{
  devIdleContextType *devIdleContext = (devIdleContextType *) hidGlobalContext;

  if (devContext.connected) {
    if (Result != 0) {
      if (!devIdleContext->updateConnParamEnded) {
        //HAL_VTimerStart_ms(HID_SLEEP_TIMER_NUMBER, CONN_PARAM_TIMEOUT);
      }
      PRINTF("Conn Update Param Rejected\n");
    } else {
      devIdleContext->updateConnParamEnded = TRUE;
      PRINTF("Conn Upadate Param Accepted\n");
    }
  }
}

void HID_Lib_aci_l2cap_proc_timeout_event(uint16_t Connection_Handle,
                                          uint8_t Data_Length,
                                          uint8_t Data[])
{
  PRINTF("CONN UPDATE TIMEOUT\n");
}

void HID_Lib_HAL_VTimerTimeoutCallback(uint8_t timerNum)
{
  uint8_t ret;
  devIdleContextType *devIdleContext = (devIdleContextType *) hidGlobalContext;

  if ((timerNum == HID_SLEEP_TIMER_NUMBER) && devContext.connected) {
    if (!devIdleContext->updateConnParamEnded) {
      ret = aci_l2cap_connection_parameter_update_req(devContext.connHandle, 
                                                      devContext.interval_min, devContext.interval_max,
                                                      devContext.slave_latency, devContext.timeout_multiplier);
      if (ret == BLE_STATUS_SUCCESS) {
        PRINTF("Conn Update Param Ongoing\n");
      } else {
        PRINTF("Conn Update Param Error\n");
      }
      devIdleContext->updateConnParamEnded = TRUE;  
    } else {
      devIdleContext->idleTimeout = TRUE;
      PRINTF("Idle Connection Timeout\n");
    }
  } 
}


/******************* (C) COPYRIGHT 2015 STMicroelectronics *****END OF FILE****/
/** \endcond
 */
