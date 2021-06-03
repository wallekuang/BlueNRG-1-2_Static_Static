/**
 * @file    hid_peripheral_sm.h
 * @author  AMS - VMA RF Application Team
 * @version V1.0.0
 * @date    October 5, 2015
 * @brief   This file provides all the functions to manage the HID/HOGP peripheral role state machines
 * @details
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
 *
 * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HID_PERIPHERAL_SM_H
#define __HID_PERIPHERAL_SM_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Private macros ------------------------------------------------------------*/
/* Defines -------------------------------------------------------------------*/
#define DIRECT_CONNECTION_PROCEDURE     0x40
#define AUTO_CONNECTION_PROCEDURE       0x08
#define REPORT_REFERENCE_UUID           0x2908
#define EXTERNAL_REPORT_REFERENCE_UUID  0X2907
#define CONN_PARAM_TIMEOUT              30000
#define HID_SLEEP_TIMER_NUMBER          1

#define APPEARANCE_AD_TYPE              0x19
#define HID_GENERIC_TYPE_APPEARANCE     0x03C0
#define HID_GENERIC_TYPE_KEYBOARD       0x03C1

/* HID Service */
#define HID_SERVICE_UUID                0x1812
#define HID_INFORMATION_CHARAC          0x2A4A
#define HID_CONTROL_POINT_CHARAC        0x2A4C
#define HID_PROTOCOL_MODE_CHARAC        0x2A4E
#define HID_BOOT_KEYBOARD_INPUT_CHARAC  0x2A22
#define HID_BOOT_KEYBOARD_OUTPUT_CHARAC 0x2A32
#define HID_BOOT_MOUSE_INPUT_CHARAC     0x2A33
#define HID_REPORT_MAP_CHARAC           0x2A4B
#define HID_REPORT_CHARAC               0x2A4D
/* Battery Service */
#define BATTERY_SERVICE_UUID            0x180F
#define BATTERY_LEVEL_CHARAC_UUID       0x2A19
/* Device Information Service */
#define DEVICE_INFORMATION_SERVICE_UUID 0x180A
#define MANUF_NAME_STRING_CHARAC        0x2A29
#define MODEL_NUMBER_STRING_CHARAC      0x2A24
#define FW_REVISION_STRING_CHARAC       0x2A26
#define SW_REVISION_STRING_CHARAC       0x2A28
#define PNP_ID_CHARAC                   0x2A50
/* Scan Parameter Service */
#define SCAN_PARAMETER_SERVICE_UUID     0x1813
#define SCAN_INTERVAL_WINDOW_CHARAC     0x2A4F
#define SCAN_REFRESH_CHARAC             0x2A31


/* HID INIT states */
#define HID_STATE_INIT 0x00

/* HID IDLE states */
#define HID_STATE_IDLE 0x10
#define HID_SUBSTATE_IDLE_CONN_UPD_PARAM     (HID_STATE_IDLE | 0x01)
#define HID_SUBSTATE_IDLE_START_IDLE_TIMER   (HID_STATE_IDLE | 0x02)
#define HID_SUBSTATE_IDLE_WAIT_TIMEOUT       (HID_STATE_IDLE | 0x03)
#define HID_SUBSTATE_IDLE_WAIT_DISCONNECTION (HID_STATE_IDLE | 0x04)
#define HID_SUBSTATE_IDLE_DONE               (HID_STATE_IDLE | 0x05)

/* HID DEVICE DISCOVERABLE states */
#define HID_STATE_DEVICE_DISCOVERABLE 0x20
#define HID_SUBSTATE_DEVICE_DISCOVERABLE_INIT        (HID_STATE_DEVICE_DISCOVERABLE | 0x01)
#define HID_SUBSTATE_DEVICE_DISCOVERABLE_WAIT_HOST   (HID_STATE_DEVICE_DISCOVERABLE | 0x02)
#define HID_SUBSTATE_DEVICE_DISCOVERABLE_DONE        (HID_STATE_DEVICE_DISCOVERABLE | 0x03)

/* HID DEVICE BONDABLE states */
#define HID_STATE_DEVICE_BONDABLE 0x30
#define HID_SUBSTATE_DEVICE_BONDABLE_INIT          (HID_STATE_DEVICE_BONDABLE | 0x01)
#define HID_SUBSTATE_DEVICE_BONDABLE_WAITING       (HID_STATE_DEVICE_BONDABLE | 0x02)
#define HID_SUBSTATE_DEVICE_BONDABLE_DONE          (HID_STATE_DEVICE_BONDABLE | 0x03)

/* HID DEVICE RECONNECTION states */
#define HID_STATE_DEVICE_RECONNECTION 0x40
#define HID_SUBSTATE_DEVICE_RECONNECTION_INIT               (HID_STATE_DEVICE_RECONNECTION | 0x01)
#define HID_SUBSTATE_DEVICE_RECONNECTION_WAITING            (HID_STATE_DEVICE_RECONNECTION | 0x02)
#define HID_SUBSTATE_DEVICE_RECONNECTION_LOW_POWER_ADV      (HID_STATE_DEVICE_RECONNECTION | 0x03)
#define HID_SUBSTATE_DEVICE_RECONNECTION_WAIT_LOW_POWER_ADV (HID_STATE_DEVICE_RECONNECTION | 0x04)
#define HID_SUBSTATE_DEVICE_RECONNECTION_WAIT_END_SECURITY  (HID_STATE_DEVICE_RECONNECTION | 0x05)
#define HID_SUBSTATE_DEVICE_RECONNECTION_DONE               (HID_STATE_DEVICE_RECONNECTION | 0x06)

/* HID DEVICE REPORT CONFIGURATION states */
#define HID_STATE_DEVICE_REPORT_CONFIGURATION 0x50
#define HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_INIT              (HID_STATE_DEVICE_REPORT_CONFIGURATION | 0x01)
#define HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_STATUS            (HID_STATE_DEVICE_REPORT_CONFIGURATION | 0x02)
#define HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_BATTERY_STATUS    (HID_STATE_DEVICE_REPORT_CONFIGURATION | 0x03)
#define HID_SUBSTATE_DEVICE_REPORT_CONFIGURATION_DONE              (HID_STATE_DEVICE_REPORT_CONFIGURATION | 0x04)

/* HID BONDED states */
#define HID_STATE_DEVICE_BONDED 0x60

/* Global State machine Information */
typedef struct HID_StateS {
  /** Macro State */
  uint8_t state;
  /** Substate */
  uint8_t subState;
} HID_StateType;

/* Device Discoverable Context Type */
typedef struct devDiscoverableContextS {
  uint8_t complete;
  uint8_t modeUsed;
  uint8_t connected;
} devDiscoverableContextType;

/* Device Bondable Context Type */
typedef struct devBondableContextS {
  uint8_t completed;
} devBondableContextType;

/* Device Reconnection Context Type */
typedef struct devReconnectionContextS {
  uint8_t completed;
  uint8_t connected;
  uint8_t bondCompleted;
} devReconnectionContextType;

typedef struct devReportConfigContextS {
  uint8_t indexReport;
  uint8_t batteryReportConfigured;
  uint8_t numberOfRepetition;
} devReportConfigContextType;

typedef struct devIdleContextS {
  uint8_t updateConnParamEnded;
  uint8_t idleTimeout;
} devIdleContextType; 

/** Dummy type just to compute the largest of all contexts */
typedef union hidGlobalContextU {
  devDiscoverableContextType a;
  devBondableContextType b;
  devReconnectionContextType c;
  devReportConfigContextType d;
  devIdleContextType e;
} hidGlobalContextType;

typedef struct devContextS {
  uint8_t  IO_Capability;
  uint16_t batteryServHandle;
  uint16_t batteryLevelHandle;
  uint8_t  batteryReportID;
  uint16_t devInfServHandle;
  uint8_t  protocolMode;
  uint16_t hidServHandle;
  uint16_t hidControlPointHandle;
  uint16_t hidProtModeHandle;
  uint16_t hidBootKeyboardInputReportHandle;
  uint16_t hidBootKeyboardOutputReportHandle;
  uint16_t hidBootMouseInputReportHandle;
  uint16_t hidReportHandle[REPORT_NUMBER];
  uint8_t  hidReportID[REPORT_NUMBER];
  uint8_t  hidReportType[REPORT_NUMBER];
  uint16_t connHandle;
  uint8_t  mitmEnabled;
  uint8_t  connected;
  uint8_t  bonded;
  uint32_t idleConnectionTimeout;
  uint8_t  whiteListConfigured;
  uint8_t  notificationPending;
  uint8_t  localNameLen;
  uint8_t  localName[20];
  uint8_t  addrMasterType;
  uint8_t  addrMaster[6];  
  uint8_t  normallyConnectable;
  uint16_t interval_min;
  uint16_t interval_max;
  uint16_t slave_latency;
  uint16_t timeout_multiplier;
  uint16_t scanParameterServHandle;
  uint16_t scanIntervalWindowHandle;
  uint16_t scanRefreshHandle;
  volatile uint8_t readyToDeepSleep;
} devContext_Type;

extern hidGlobalContextType *hidGlobalContext;
extern HID_StateType HID_State;
extern devContext_Type devContext;

HID_StateType hidStateMachine(HID_StateType state);

#endif
