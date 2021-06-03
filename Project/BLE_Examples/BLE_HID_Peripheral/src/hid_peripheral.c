/******************** (C) COPYRIGHT 2018 STMicroelectronics ********************
* File Name          : hid_peripheral.c
* Author             : AMS - VMA RF Application Team
* Version            : V1.1.0
* Date               : 09-April-2018
* Description        : BlueNRG-1 HID functions interface
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
#include "hid_peripheral.h"
#include "hid_peripheral_config.h"
#include "hid_peripheral_sm.h"
#include "ble_const.h"
#include "bluenrg1_api.h"
#include "bluenrg1_events.h"
#include "hal_types.h"

/* External variables --------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define PERIPHERAL_PREFERRED_CONN_PARAMETER_HANDLE 0x0A
#define MAX_NUMBER_BYTES_TO_UPDATE 50
#define NOTIFICATION_ENABLED       0x01

/* Private variables ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#ifdef DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

extern void DEBUG_MESSAGE(uint8_t prefix, uint8_t next);
#define STATE_TRANSITION(prefix, prevState, nextState) { HID_State.state = prefix##_##nextState; HID_State.subState = prefix##_##nextState | 0x01; DEBUG_MESSAGE(prefix##_##prevState, HID_State.state);}
#define GET_CURRENT_STATE() HID_State.state
#define GET_CURRENT_SUBSTATE() HID_State.subState

/* Private Functions ----------------------------------------------------------*/

static uint8_t numbAttrRecordsHID(hidService_Type* hid)
{
  uint8_t nmbRec=0, i, reportType;

  nmbRec++; // HID Primary Service
  nmbRec += INCLUDED_SERVICE_NUMBER; // Included Service Number
  nmbRec += 2; // HID Information
  nmbRec += 2; // HID Control Point
  if (hid->bootSupport) {
    nmbRec += 2; // Protocol Mode 2
    if (hid->isBootDevKeyboard)
      nmbRec += 5; // Boot Keyboard 3 Input + 2 Output
    if (hid->isBootDevMouse)
      nmbRec += 3; // Boot Mouse 3 Input
  }
  nmbRec += 2; //Report Map
  if (hid->externalReportEnabled)
    nmbRec += EXTERNAL_REPORT_NUMBER; // External Report Number 
  if (hid->reportSupport) {
    for (i=0; i< REPORT_NUMBER; i++) {
      reportType = hid->reportReferenceDesc[i].type;
      switch(reportType) {
      case INPUT_REPORT:
	nmbRec += 4;
	break;
      case OUTPUT_REPORT:
      case FEATURE_REPORT:
	nmbRec += 3;
	break;
      }
    } 
  }
  return nmbRec;
}

static uint8_t addScanParameterService(void)
{
  uint8_t ret, security;
  Service_UUID_t scan_ser;
  Char_UUID_t scan_char;
  
  /* Added Scan Parameter Service. See the Scan Parameter Service profile
     for more details
  */
  scan_ser.Service_UUID_16 = SCAN_PARAMETER_SERVICE_UUID;
  ret = aci_gatt_add_service(UUID_TYPE_16, &scan_ser, PRIMARY_SERVICE, 6, &devContext.scanParameterServHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Scan Parameter Service failed\n");
    return ret;
  }
  
  security = ATTR_PERMISSION_ENCRY_READ;
  if (devContext.mitmEnabled)
    security |= ATTR_PERMISSION_AUTHEN_READ;

  scan_char.Char_UUID_16 = SCAN_INTERVAL_WINDOW_CHARAC;
  ret =  aci_gatt_add_char(devContext.scanParameterServHandle, UUID_TYPE_16, &scan_char, 4,
                           CHAR_PROP_WRITE_WITHOUT_RESP, security, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           16, 0, &devContext.scanIntervalWindowHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Scan Interval Window Characteristics failed\n");
    return ret;
  }

  scan_char.Char_UUID_16 = SCAN_REFRESH_CHARAC;
  ret =  aci_gatt_add_char(devContext.scanParameterServHandle, UUID_TYPE_16, &scan_char, 1,
                           CHAR_PROP_NOTIFY, security, 0,
                           16, 0, &devContext.scanRefreshHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Scan Refresh Characteristics failed\n");
    return ret;
  }
  PRINTF("Add Scan Parameter Service and Characteristics\n");        
    
  return ret; 
}

static uint8_t addBatteryService(uint8_t reportMapPresent, uint8_t reportID)
{
  uint8_t ret, maxAttrRec, reportRefValue[2], security, level;
  uint16_t connHandle;
  Service_UUID_t battery_ser;
  Char_UUID_t battery_char; 
  Char_Desc_Uuid_t battery_desc; 
  
  /* Added the Battery Service. See the Battery Service profile
     for more details
  */
  battery_ser.Service_UUID_16 = BATTERY_SERVICE_UUID;
  if (reportMapPresent)
    maxAttrRec = 5;
  else
    maxAttrRec = 4;
  ret = aci_gatt_add_service(UUID_TYPE_16, &battery_ser, PRIMARY_SERVICE, maxAttrRec, &devContext.batteryServHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Battery Service failed\n");
    return ret;
  }
  
  security = ATTR_PERMISSION_ENCRY_READ;
  if (devContext.mitmEnabled)
    security |= ATTR_PERMISSION_AUTHEN_READ;

  battery_char.Char_UUID_16 = BATTERY_LEVEL_CHARAC_UUID;
  ret =  aci_gatt_add_char(devContext.batteryServHandle, UUID_TYPE_16, &battery_char, 1,
                           CHAR_PROP_NOTIFY|CHAR_PROP_READ, security, 0,
                           16, 0, &devContext.batteryLevelHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Battery Characteristics failed\n");
    return ret;
  }

  if (reportMapPresent) {
    devContext.batteryReportID = reportID;
    battery_desc.Char_UUID_16 = REPORT_REFERENCE_UUID;
    reportRefValue[0] = reportID;
    reportRefValue[1] = INPUT_REPORT;
    ret = aci_gatt_add_char_desc(devContext.batteryServHandle, devContext.batteryLevelHandle,
                                 UUID_TYPE_16, &battery_desc, 2, 2, reportRefValue, security, 0x01,
                                 0, 16, 0, &connHandle);
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Add Battery Report Reference failed\n");
      return ret;
    }
  }
  
  level = 100;
  
  ret = aci_gatt_update_char_value_ext(0, devContext.batteryServHandle, 
                                       devContext.batteryLevelHandle,0,1,0, 
                                       1, &level);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update Battery Level failed\n");
    return ret;
  }
  PRINTF("Add Battery Service and Characteristics\n"); 
  
  return ret;
}

static uint8_t addDeviceInformationService(devInfService_Type* devInf)
{
  uint8_t ret, security;
  uint16_t characHandle;
  Service_UUID_t devInf_ser;
  Char_UUID_t devInf_char; 

  devInf_ser.Service_UUID_16  = DEVICE_INFORMATION_SERVICE_UUID;
  ret = aci_gatt_add_service(UUID_TYPE_16, &devInf_ser, PRIMARY_SERVICE, 11, &devContext.devInfServHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("ADD Device Information Service failed\n");
    return ret;
  }

  security = ATTR_PERMISSION_ENCRY_READ;
  if (devContext.mitmEnabled)
    security |= ATTR_PERMISSION_AUTHEN_READ;

  devInf_char.Char_UUID_16 = MANUF_NAME_STRING_CHARAC;
  ret = aci_gatt_add_char(devContext.devInfServHandle, UUID_TYPE_16, &devInf_char, MANUFAC_NAME_LEN,
                          CHAR_PROP_READ, security, 0,
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Manufacter Name String Charac failed\n");
    return ret;
  }
 
  ret = aci_gatt_update_char_value_ext(0, devContext.devInfServHandle, characHandle, 0,MANUFAC_NAME_LEN, 0, 
                                       MANUFAC_NAME_LEN, devInf->manufacName);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update Manuf Value failed\n");
    return ret;
  }
  
  devInf_char.Char_UUID_16 = MODEL_NUMBER_STRING_CHARAC;
  ret = aci_gatt_add_char(devContext.devInfServHandle, UUID_TYPE_16, &devInf_char, MODEL_NUMB_LEN,
                          CHAR_PROP_READ, security, 0,
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add Model Number String Charac failed\n");
    return ret;
  }
  
  ret = aci_gatt_update_char_value_ext(0,devContext.devInfServHandle, characHandle, 0, MODEL_NUMB_LEN, 0, 
                                   MODEL_NUMB_LEN, devInf->modelNumber);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update Model Number Value failed\n");
    return ret;
  }

  devInf_char.Char_UUID_16 = FW_REVISION_STRING_CHARAC;
  ret = aci_gatt_add_char(devContext.devInfServHandle, UUID_TYPE_16, &devInf_char, FW_REV_LEN,
                          CHAR_PROP_READ, security, 0,
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add FW Revision String Charac failed\n");
    return ret;
  }
  
  ret = aci_gatt_update_char_value_ext(0,devContext.devInfServHandle, characHandle, 0,FW_REV_LEN, 0, 
                                   FW_REV_LEN, devInf->fwRevision);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update FW Revision Value failed\n");
    return ret;
  }

  devInf_char.Char_UUID_16 = SW_REVISION_STRING_CHARAC;
  ret = aci_gatt_add_char(devContext.devInfServHandle, UUID_TYPE_16, &devInf_char, SW_REV_LEN,
                          CHAR_PROP_READ, security, 0,
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add SW Revision String Charac failed\n");
    return ret;
  }
  
  ret = aci_gatt_update_char_value_ext(0,devContext.devInfServHandle, characHandle, 0, SW_REV_LEN,0, 
                                   SW_REV_LEN, devInf->swRevision);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update SW Revision Value failed\n");
    return ret;
  }

  devInf_char.Char_UUID_16 = PNP_ID_CHARAC;
  ret = aci_gatt_add_char(devContext.devInfServHandle, UUID_TYPE_16, &devInf_char, PNP_ID_LEN,
                          CHAR_PROP_READ, security, 0,
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add PNP ID Charac failed\n");
    return ret;
  }
    
  ret = aci_gatt_update_char_value_ext(0,devContext.devInfServHandle, characHandle,0,PNP_ID_LEN ,0, 
                                       PNP_ID_LEN, devInf->pnpID);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update PNP ID Value failed\n");
    return ret;
  }
  PRINTF("ADD Device Information Service and Characteristics\n");
  
  return ret;
}

static uint8_t addhidService(hidService_Type* hid)
{
  uint8_t ret, security, i, characProp, reportLen, eventMask; //, protMode;
  uint16_t includedHandle, characHandle, characDescHandle, startHandle, endHandle;
  uint8_t reportRef[2], maxAttrRecords, offset, numbers_offset, descLen_offset_module;
  Service_UUID_t hid_ser;
  Char_UUID_t hid_char; 
  Char_Desc_Uuid_t hid_desc;
  Include_UUID_t hid_incl;

  maxAttrRecords = numbAttrRecordsHID(hid);

  hid_ser.Service_UUID_16 = HID_SERVICE_UUID;
  ret = aci_gatt_add_service(UUID_TYPE_16, &hid_ser, PRIMARY_SERVICE, maxAttrRecords, &devContext.hidServHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("ADD HID Service failed\n");
    return ret;
  }

  if (hid->includedServiceEnabled) {
    for (i=0; i<INCLUDED_SERVICE_NUMBER; i++) {
      hid_incl.Include_UUID_16 = hid->includedService[i].uuid;
      if (hid->includedService[i].uuid == BATTERY_SERVICE_UUID) {
	startHandle = devContext.batteryServHandle;
	endHandle = devContext.batteryServHandle + 4;
      } else {
	startHandle = hid->includedService[i].start_handle;
	endHandle = hid->includedService[i].end_handle;
      }
      ret = aci_gatt_include_service(devContext.hidServHandle, startHandle,
                                     endHandle, UUID_TYPE_16, &hid_incl,
                                     &includedHandle);
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Add included service failed\n");
	return ret;
      }
    }
  }

  security = ATTR_PERMISSION_ENCRY_READ;
  if (devContext.mitmEnabled)
    security |= ATTR_PERMISSION_AUTHEN_READ;

  hid_char.Char_UUID_16 = HID_INFORMATION_CHARAC;
  ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, 4,
                          CHAR_PROP_READ, security, 0,
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add HID Information Charac failed\n");
    return ret;
  }
  
  ret = aci_gatt_update_char_value_ext(0,devContext.hidServHandle, characHandle, 0, 4,0, 
                                   4, hid->informationCharac);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Update HID Information Value failed\n");
    return ret;
  }
  devContext.normallyConnectable = (hid->informationCharac[3] & 0x02) >> 1;

  hid_char.Char_UUID_16 = HID_CONTROL_POINT_CHARAC;
  ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, 1,
                          CHAR_PROP_WRITE_WITHOUT_RESP, security, GATT_NOTIFY_ATTRIBUTE_WRITE,
                          16, 0, &devContext.hidControlPointHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add HID Control Point Charac failed\n");
    return ret;
  }

  if (hid->bootSupport) {
    hid_char.Char_UUID_16 = HID_PROTOCOL_MODE_CHARAC;
    ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, 1,
                            CHAR_PROP_READ|CHAR_PROP_WRITE_WITHOUT_RESP, security,
                            GATT_NOTIFY_ATTRIBUTE_WRITE, 16, 0, &devContext.hidProtModeHandle);
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Add HID Protocol Mode Charac failed\n");
      return ret;
    }
//    protMode = REPORT_PROTOCOL_MODE;
    
    ret = aci_gatt_update_char_value_ext(0,devContext.hidServHandle, characHandle, 0,4, 0, 
                                   4, hid->informationCharac);
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Update HID Protocol Mode Value failed\n");
      return ret;
    }

    if (hid->isBootDevKeyboard) {
      hid_char.Char_UUID_16 = HID_BOOT_KEYBOARD_INPUT_CHARAC;
      ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, 8,
                              CHAR_PROP_READ|CHAR_PROP_NOTIFY|CHAR_PROP_WRITE, security,
                              GATT_NOTIFY_ATTRIBUTE_WRITE|GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, 
                              16, 0, &devContext.hidBootKeyboardInputReportHandle);
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Add HID Boot keyboard Input Report Charac failed\n");
	return ret;
      }     
      hid_char.Char_UUID_16 = HID_BOOT_KEYBOARD_OUTPUT_CHARAC;
      ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, 1,
                              CHAR_PROP_READ|CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RESP, security,
                              GATT_NOTIFY_ATTRIBUTE_WRITE|GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, 
                              16, 0, &devContext.hidBootKeyboardOutputReportHandle);
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Add HID Boot keyboard Output Charac failed\n");
	return ret;
      }     
    }

    if (hid->isBootDevMouse) {
      hid_char.Char_UUID_16 = HID_BOOT_MOUSE_INPUT_CHARAC;
      ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, 3,
                              CHAR_PROP_READ|CHAR_PROP_WRITE|CHAR_PROP_NOTIFY, security,
                              GATT_NOTIFY_ATTRIBUTE_WRITE|GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, 
                              16, 0, &devContext.hidBootMouseInputReportHandle);
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Add HID Boot keyboard Output Charac failed\n");
	return ret;
      }     
    }
  }

  hid_char.Char_UUID_16 = HID_REPORT_MAP_CHARAC;
  ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, hid->reportDescLen,
                          CHAR_PROP_READ, security, 0, 
                          16, 0, &characHandle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Add HID Report MAP Charac failed\n");
    return ret;
  }
  
  numbers_offset = hid->reportDescLen / MAX_NUMBER_BYTES_TO_UPDATE;
  descLen_offset_module = hid->reportDescLen % MAX_NUMBER_BYTES_TO_UPDATE;
  for (i=0; i<numbers_offset; i++) {
    offset = i * MAX_NUMBER_BYTES_TO_UPDATE;
    
    ret = aci_gatt_update_char_value_ext(0,devContext.hidServHandle, characHandle, 0, MAX_NUMBER_BYTES_TO_UPDATE, offset, 
                                         MAX_NUMBER_BYTES_TO_UPDATE, &hid->reportDesc[offset]);
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Update HID Report MAP Value failed\n");
      return ret;
    }
  }
  if (descLen_offset_module != 0) {
    offset = numbers_offset * MAX_NUMBER_BYTES_TO_UPDATE;
 
    ret = aci_gatt_update_char_value_ext(0,devContext.hidServHandle, characHandle, 0,descLen_offset_module,offset, 
                                         descLen_offset_module, &hid->reportDesc[offset]);
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Update HID Report MAP Value failed\n");
      return ret;
    }
  }

  if (hid->externalReportEnabled) {
    hid_desc.Char_UUID_16 = EXTERNAL_REPORT_REFERENCE_UUID;
    reportRef[0] = BATTERY_LEVEL_CHARAC_UUID & 0x00ff;
    reportRef[1] = BATTERY_LEVEL_CHARAC_UUID >> 8;
    ret = aci_gatt_add_char_desc(devContext.hidServHandle, characHandle,
                                 UUID_TYPE_16, &hid_desc, 2, 2, reportRef, security, 0x01,
                                 0, 16, 0, &characDescHandle);
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Add External Report Reference failed\n");
      return ret;
    }
  }
  if (hid->reportSupport) {
    for (i=0; i<REPORT_NUMBER; i++) {
      if (hid->reportReferenceDesc[i].type == INPUT_REPORT) {
	characProp = CHAR_PROP_READ|CHAR_PROP_NOTIFY|CHAR_PROP_WRITE;
	reportLen = hid->reportReferenceDesc[i].length;
	reportRef[0] = hid->reportReferenceDesc[i].ID;
	reportRef[1] = INPUT_REPORT;
	eventMask = GATT_NOTIFY_ATTRIBUTE_WRITE|GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP;
	devContext.hidReportType[i] = INPUT_REPORT;
      }
      if (hid->reportReferenceDesc[i].type == OUTPUT_REPORT) {
	characProp = CHAR_PROP_READ|CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RESP;
	reportLen = hid->reportReferenceDesc[i].length;
	reportRef[0] = hid->reportReferenceDesc[i].ID;
	reportRef[1] = OUTPUT_REPORT;
	eventMask = GATT_NOTIFY_ATTRIBUTE_WRITE|GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP;
	devContext.hidReportType[i] = OUTPUT_REPORT;
      }
      if (hid->reportReferenceDesc[i].type == FEATURE_REPORT) {
	characProp = CHAR_PROP_READ|CHAR_PROP_WRITE;
	reportLen = hid->reportReferenceDesc[i].length;
	reportRef[0] = hid->reportReferenceDesc[i].ID;
	reportRef[1] = FEATURE_REPORT;
	devContext.hidReportType[i] = FEATURE_REPORT;
	eventMask = GATT_NOTIFY_ATTRIBUTE_WRITE|GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP;
      }
      hid_char.Char_UUID_16 = HID_REPORT_CHARAC;
      ret = aci_gatt_add_char(devContext.hidServHandle, UUID_TYPE_16, &hid_char, reportLen,
                              characProp, security, eventMask, 
                              16, 0, &devContext.hidReportHandle[i]);
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Add HID Report Charac failed\n");
	return ret;
      }
      devContext.hidReportID[i] = reportRef[0]; 
      hid_desc.Char_UUID_16 = REPORT_REFERENCE_UUID;
      ret = aci_gatt_add_char_desc(devContext.hidServHandle, devContext.hidReportHandle[i],
                                   UUID_TYPE_16, &hid_desc, 2, 2, reportRef, security, 0x01,
                                   0, 16, 0, &characDescHandle);
      if (ret != BLE_STATUS_SUCCESS) {
	PRINTF("Add Report Reference failed\n");
	return ret;
      }
    }
  }
  PRINTF("ADD HID Service and Characteristics\n");
  
  return ret;
}


/* Functions -----------------------------------------------------------------*/

uint8_t hidDevice_Init(uint8_t IO_Capability, connParam_Type connParam, 
                       uint8_t dev_name_len, uint8_t *dev_name, uint8_t *addr)
{ 
  uint8_t ret = BLE_STATUS_SUCCESS;
  uint8_t appearance[2] = {0xC0, 0x03};
  uint8_t prefConnParam[8];
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle, pref_conn_par_handle;


  if (IO_Capability > IO_CAP_KEYBOARD_DISPLAY)
    return BLE_STATUS_INVALID_PARAMS;

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, 
                                  CONFIG_DATA_PUBADDR_LEN, addr);
  if(ret != BLE_STATUS_SUCCESS){
    PRINTF("Setting BD_ADDR failed.\n");
    return ret;
  }
    
  ret = aci_gatt_init();    
  if(ret != BLE_STATUS_SUCCESS){
    PRINTF("GATT_Init failed.\n");
    return ret;
  }
    
  ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0, dev_name_len, &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if(ret != BLE_STATUS_SUCCESS){
    PRINTF("GAP_Init failed.\n");
    return ret;
  }

  // Set the Peripheral Preferred Connection Parameters
  pref_conn_par_handle = PERIPHERAL_PREFERRED_CONN_PARAMETER_HANDLE;
  memcpy(prefConnParam, &connParam.interval_min, 2);
  memcpy(&prefConnParam[2], &connParam.interval_max, 2);
  memcpy(&prefConnParam[4], &connParam.slave_latency, 2);
  memcpy(&prefConnParam[6], &connParam.timeout_multiplier, 2);
  
  ret = aci_gatt_update_char_value_ext(0,service_handle, pref_conn_par_handle, 0, sizeof(prefConnParam), 0, sizeof(prefConnParam), prefConnParam);
  if (ret != 0) {
    PRINTF("Error during update the Preferred Connection Parameters\n");
  }

  ret =  aci_gatt_update_char_value_ext(0, service_handle, dev_name_char_handle, 0, dev_name_len, 
                                        0, dev_name_len, dev_name);
  if (ret != 0) {
    PRINTF("Error during update the gap name\n");
  }

  ret =  aci_gatt_update_char_value_ext(0,service_handle, appearance_char_handle,0, sizeof(appearance), 0, sizeof(appearance), appearance);
  if (ret != 0) {
    PRINTF("Error durign update the gap appearance\n");
  }    

  /* -2 dBm output power */
  ret = aci_hal_set_tx_power_level(1,4);    
  if(ret != BLE_STATUS_SUCCESS) {
    PRINTF("Set Output Power failed.\n");
    return ret;
  }

  ret = aci_gap_set_io_capability(IO_Capability);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Set Device IO capability failed\n");
  }

  /* Reset the device context variable */
  memset(&devContext, 0, sizeof(devContext));

  /* To use during the warm start */
  devContext.IO_Capability = IO_Capability;

  /* Connection Parameters which best suit application use case */
  devContext.interval_min = connParam.interval_min;
  devContext.interval_max = connParam.interval_max;
  devContext.slave_latency = connParam.slave_latency;
  devContext.timeout_multiplier = connParam.timeout_multiplier;

  PRINTF("BLE Stack Initialized.\n");

  HID_State.state = HID_STATE_INIT;
  HID_State.subState = 0;
  hidDevice_Tick();
  STATE_TRANSITION(HID_STATE, INIT, IDLE); // Init complete

  return ret;
}

uint8_t hidSetDeviceDiscoverable(uint8_t mode, uint8_t nameLen, uint8_t *name)
{
  uint8_t ret = BLE_STATUS_SUCCESS;
  devDiscoverableContextType *devDiscContext = (devDiscoverableContextType *) hidGlobalContext; 
  uint8_t local_name[21];
  uint8_t serviceUUID[3] = {AD_TYPE_16_BIT_SERV_UUID};

  printf("hidSetDeviceDiscoverable\n");

  memcpy(devContext.localName, name, nameLen);
  devContext.localNameLen = nameLen;

  // Discoverable Parameter
  local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
  memcpy(&local_name[1], name, devContext.localNameLen);
  serviceUUID[1] = (uint8_t)(HID_SERVICE_UUID & 0x00ff);
  serviceUUID[2] = (uint8_t)(HID_SERVICE_UUID>>8);
  
  if (GET_CURRENT_STATE() != HID_STATE_IDLE) {
    ret = BLE_ERROR_COMMAND_DISALLOWED;
  } else {
    devDiscContext->complete = FALSE;
    devDiscContext->modeUsed = mode;
    devDiscContext->connected = FALSE;
    if (mode == LIMITED_DISCOVERABLE_MODE) {
      ret = aci_gap_set_limited_discoverable(ADV_IND, DISC_ADV_MIN, DISC_ADV_MAX, PUBLIC_ADDR,
                                             NO_WHITE_LIST_USE, (devContext.localNameLen+1), 
                                             local_name, sizeof(serviceUUID), serviceUUID,
                                             0, 0);
    } else {
      ret = aci_gap_set_discoverable(ADV_IND, DISC_ADV_MIN, DISC_ADV_MAX, PUBLIC_ADDR, 
                                     NO_WHITE_LIST_USE, (devContext.localNameLen+1),
                                     local_name, sizeof(serviceUUID), serviceUUID,
                                     0, 0);
    }
    
    if (ret == BLE_STATUS_SUCCESS) {
      set_default_adv_data();
    
      STATE_TRANSITION(HID_STATE, IDLE, DEVICE_DISCOVERABLE); // Start Device Discoverable Mode
    }
  }

  return ret;
}

uint8_t hidStopDiscoverableMode(void)
{
  uint8_t ret = BLE_STATUS_SUCCESS;
  devDiscoverableContextType *devDiscContext = (devDiscoverableContextType *) hidGlobalContext; 

  if (devDiscContext->modeUsed == GENERAL_DISCOVERABLE_MODE)
    devDiscContext->complete = TRUE;
  devDiscContext->connected = FALSE;
  ret = aci_gap_set_non_discoverable();

  return ret;
}

uint8_t hidDeleteBondedDevice(void)
{
  uint8_t ret;

  if (devContext.connected) {
    ret = hidCloseConnection();
    if (ret != BLE_STATUS_SUCCESS) {
      PRINTF("Error during the Close Connection\n");
    }
    devContext.connected = FALSE;
  }

  ret = aci_gap_clear_security_db();
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error during the clear security database\n");
  }

  devContext.bonded = FALSE;

  return ret;
}

void hidDevice_Tick(void)
{
  HID_State = hidStateMachine(HID_State);
}

uint8_t hidSetDeviceSecurty(uint8_t MITM_Mode, uint8_t fixedPinUsed, uint32_t fixedPin)
{
  uint8_t ret, pinMode;

  devContext.mitmEnabled = MITM_Mode;
  if (fixedPinUsed)
    pinMode = USE_FIXED_PIN_FOR_PAIRING;
  else
    pinMode = DONOT_USE_FIXED_PIN_FOR_PAIRING;
 
    ret = aci_gap_set_authentication_requirement(BONDING,
                                                 MITM_Mode,
                                                 SC_IS_NOT_SUPPORTED,
                                                 KEYPRESS_IS_NOT_SUPPORTED,
                                                 7, 
                                                 16,
                                                 pinMode,
                                                 fixedPin,
                                                 0x00);
  
  
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Set Authentication failed\n");
  }
  return ret;
}

uint8_t hidAddServices(batteryService_Type* battery, devInfService_Type* devInf, hidService_Type* hid)
{
  uint8_t ret;

  ret = addBatteryService(battery->inReportMap, battery->reportID);
  if (ret != BLE_STATUS_SUCCESS)
    return ret;
  
  ret = addDeviceInformationService(devInf);
  if (ret != BLE_STATUS_SUCCESS)
    return ret;

  ret = addScanParameterService();
  if (ret != BLE_STATUS_SUCCESS)
    return ret;

  ret = addhidService(hid);
  if (ret != BLE_STATUS_SUCCESS)
    return ret;

  return ret;
}

void hidSetIdleTimeout(uint32_t timeout)
{
  devContext.idleConnectionTimeout = timeout;
}

void hidSetNotificationPending(uint8_t enabled)
{
  devContext.notificationPending = enabled;
}

uint8_t hidDeviceStatus(void)
{
  uint8_t devStatus;

  devStatus = HID_DEVICE_UNKNOWN_STATUS;

  if (GET_CURRENT_SUBSTATE() == HID_SUBSTATE_DEVICE_RECONNECTION_LOW_POWER_ADV)
    return HID_DEVICE_UNKNOWN_STATUS;

  if ((GET_CURRENT_STATE() != HID_STATE_IDLE) && (GET_CURRENT_STATE()!= HID_STATE_DEVICE_BONDED) && !devContext.readyToDeepSleep)
    devStatus |= HID_DEVICE_BUSY;

  if (devContext.readyToDeepSleep)
     devStatus |= HID_DEVICE_READY_TO_DEEP_SLEEP;
  
  if ((GET_CURRENT_STATE() == HID_STATE_IDLE) && devContext.bonded && devContext.connected)
     devStatus |= HID_DEVICE_READY_TO_NOTIFY;

  if (((GET_CURRENT_STATE() == HID_STATE_IDLE) && devContext.bonded && !devContext.connected) ||
      (GET_CURRENT_STATE() == HID_STATE_DEVICE_BONDED))
    devStatus |= HID_DEVICE_BONDED;

  if (devContext.connected && !devContext.bonded)
    devStatus |= HID_DEVICE_CONNECTED;

  if ((GET_CURRENT_STATE() == HID_STATE_IDLE) && !devContext.connected && !devContext.bonded)
    devStatus |= HID_DEVICE_NOT_CONNECTED;

  return devStatus;
}

uint8_t hidSendPassKey(uint32_t key)
{
  uint8_t ret;

  ret = aci_gap_pass_key_resp(devContext.connHandle, key);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error during the pass key response\n");
  }  
  return ret;
}

uint8_t hidCloseConnection(void)
{
  uint8_t ret;

  ret = aci_gap_terminate(devContext.connHandle,0x13); /* 0x13: Remote User Terminated Connection */
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error during the GAP terminate connection\n");
  }
  return ret;
}

uint8_t hidInputReportReady(uint8_t index)
{
  uint8_t ret;
  uint8_t configDesc[2];
  uint16_t len, data_len, configDescHandle;
  
  configDescHandle = devContext.hidReportHandle[index] + 2;
  ret = aci_gatt_read_handle_value(configDescHandle, 0, 2, &len, &data_len, configDesc);
  if (ret != BLE_STATUS_SUCCESS){
    PRINTF("Error during the gatt read handle value %02x\n", ret);
    return FALSE;
  }
  
  if (configDesc[0] == NOTIFICATION_ENABLED) {
    return TRUE;
  } else {
    return FALSE;
  }
}

uint8_t hidSendReport(uint8_t id, uint8_t type, uint8_t reportLen, uint8_t *reportData)
{
  uint8_t endReportList, ret, i;
   
  PRINTF("Report Data: ");         
  for (i=0; i<reportLen-1; i++){
    PRINTF("%0x - ", reportData[i]);
  }
  PRINTF("%0x\n", reportData[reportLen -1]);
  
  if (type == BOOT_KEYBOARD_INPUT_REPORT) {
    return aci_gatt_update_char_value_ext(devContext.connHandle, devContext.hidServHandle,
                                          devContext.hidBootKeyboardInputReportHandle, 1, reportLen,
                                          0, reportLen, reportData);
  }

  if (type == BOOT_MOUSE_INPUT_REPORT) {
    return aci_gatt_update_char_value_ext(devContext.connHandle,devContext.hidServHandle,
                                        devContext.hidBootMouseInputReportHandle, 1,reportLen,
                                        0, reportLen, reportData);
  }

  endReportList = FALSE;
  i = 0;
  while (!endReportList) {
    if ((id == devContext.hidReportID[i]) &&
	(devContext.hidReportType[i] == INPUT_REPORT)) {
      ret = aci_gatt_update_char_value_ext(devContext.connHandle,devContext.hidServHandle, 
                                           devContext.hidReportHandle[i], 1, reportLen,
                                           0, reportLen, reportData);
      return ret;
    }
    i++;
    if (i == REPORT_NUMBER)
      endReportList = TRUE;
  }
  
  return BLE_STATUS_INVALID_PARAMS;
}

uint8_t hidUpdateReportValue(uint8_t ID, uint8_t type, uint8_t len, uint8_t *data)
{
  uint8_t endReportList, ret, i;

  if (type == BOOT_KEYBOARD_INPUT_REPORT) {
    ret = aci_gatt_update_char_value_ext(devContext.connHandle, devContext.hidServHandle,
                                         devContext.hidBootKeyboardInputReportHandle, 1, len, 
                                         0, len, data);
  }

  if (type == BOOT_KEYBOARD_OUTPUT_REPORT) {
    ret = aci_gatt_update_char_value_ext(devContext.connHandle,devContext.hidServHandle,
                                         devContext.hidBootKeyboardOutputReportHandle, 1, len,
                                         0, len, data);
    
    
  }

  if (type == BOOT_MOUSE_INPUT_REPORT) { 
    ret = aci_gatt_update_char_value_ext(devContext.connHandle,devContext.hidServHandle,
                                         devContext.hidBootMouseInputReportHandle, 1, len, 
                                         0, len, data);
  }

  if (ret == BLE_STATUS_SUCCESS)
    return aci_gatt_allow_read(devContext.connHandle);

  endReportList = FALSE;
  i = 0;
  while (!endReportList) {
    if ((ID == devContext.hidReportID[i]) &&
	(devContext.hidReportType[i] == type)) {
      ret = aci_gatt_update_char_value_ext(devContext.connHandle,devContext.hidServHandle, 
                                           devContext.hidReportHandle[i], 1, len,
                                           0, len, data);
      
      if (ret == BLE_STATUS_SUCCESS)
	ret = aci_gatt_allow_read(devContext.connHandle);

      return ret;
    }
    i++;
    if (i == REPORT_NUMBER)
      endReportList = TRUE;
  }
  
  return BLE_STATUS_INVALID_PARAMS;
}

uint8_t sendBatteryLevel(uint8_t level)
{
  uint8_t ret;

  ret = aci_gatt_update_char_value_ext(devContext.connHandle,devContext.batteryServHandle, 
                                        devContext.batteryLevelHandle, 1, 1, 0, 
                                        1, &level);
  return ret;

}

uint8_t hidSetTxPower(uint8_t level)
{
  uint8_t ret;

  if (level > 7)
    return BLE_STATUS_INVALID_PARAMS;

  ret = aci_hal_set_tx_power_level(1, level);    
  if(ret != BLE_STATUS_SUCCESS) {
    PRINTF("Set Output Power failed.\n");
  }
  return ret;
}

/******************* (C) COPYRIGHT 2015 STMicroelectronics *****END OF FILE****/
/** \endcond
 */
