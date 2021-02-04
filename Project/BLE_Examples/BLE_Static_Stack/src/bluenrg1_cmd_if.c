

/**
  ******************************************************************************
  * @file    bluenrg1_cmd_if.c
  * @author  AMG - RF Application team
  * @version V1.0.0
  * @date    21 February 2020
  * @brief   Autogenerated files, do not edit!!
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT STMicroelectronics</center></h2>
  ******************************************************************************
  */
#include "bluenrg1_stack.h"
#include "bluenrg1_api.h"
#include "compiler.h"

SECTION(".cmd_call_table")
REQUIRED(void * const cmd_call_table[]) = {
  (void *) BTLE_StackTick,
  (void *) BlueNRG_Stack_Initialization,
  (void *) BlueNRG_Stack_Perform_Deep_Sleep_Check,
  (void *) RAL_Isr,
  (void *) HAL_VTimerStart_ms,
  (void *) HAL_VTimer_Stop,
  (void *) HAL_VTimerGetCurrentTime_sysT32,
  (void *) HAL_VTimerAcc_sysT32_ms,
  (void *) HAL_VTimerDiff_ms_sysT32,
  (void *) HAL_VTimerStart_sysT32,
  (void *) HAL_VTimerExpiry_sysT32,
  (void *) hci_disconnect,
  (void *) hci_read_remote_version_information,
  (void *) hci_set_event_mask,
  (void *) hci_reset,
  (void *) hci_read_transmit_power_level,
  (void *) hci_read_local_version_information,
  (void *) hci_read_local_supported_commands,
  (void *) hci_read_local_supported_features,
  (void *) hci_read_bd_addr,
  (void *) hci_read_rssi,
  (void *) hci_le_set_event_mask,
  (void *) hci_le_read_buffer_size,
  (void *) hci_le_read_local_supported_features,
  (void *) hci_le_set_random_address,
  (void *) hci_le_set_advertising_parameters,
  (void *) hci_le_read_advertising_channel_tx_power,
  (void *) hci_le_set_advertising_data,
  (void *) hci_le_set_scan_response_data,
  (void *) hci_le_set_advertise_enable,
  (void *) hci_le_set_scan_parameters,
  (void *) hci_le_set_scan_enable,
  (void *) hci_le_create_connection,
  (void *) hci_le_create_connection_cancel,
  (void *) hci_le_read_white_list_size,
  (void *) hci_le_clear_white_list,
  (void *) hci_le_add_device_to_white_list,
  (void *) hci_le_remove_device_from_white_list,
  (void *) hci_le_connection_update,
  (void *) hci_le_set_host_channel_classification,
  (void *) hci_le_read_channel_map,
  (void *) hci_le_read_remote_used_features,
  (void *) hci_le_encrypt,
  (void *) hci_le_rand,
  (void *) hci_le_start_encryption,
  (void *) hci_le_long_term_key_request_reply,
  (void *) hci_le_long_term_key_requested_negative_reply,
  (void *) hci_le_read_supported_states,
  (void *) hci_le_receiver_test,
  (void *) hci_le_transmitter_test,
  (void *) hci_le_test_end,
  (void *) hci_le_set_data_length,
  (void *) hci_le_read_suggested_default_data_length,
  (void *) hci_le_write_suggested_default_data_length,
  (void *) hci_le_read_local_p256_public_key,
  (void *) hci_le_generate_dhkey,
  (void *) hci_le_add_device_to_resolving_list,
  (void *) hci_le_remove_device_from_resolving_list,
  (void *) hci_le_clear_resolving_list,
  (void *) hci_le_read_resolving_list_size,
  (void *) hci_le_read_peer_resolvable_address,
  (void *) hci_le_read_local_resolvable_address,
  (void *) hci_le_set_address_resolution_enable,
  (void *) hci_le_set_resolvable_private_address_timeout,
  (void *) hci_le_read_maximum_data_length,
  (void *) hci_le_set_privacy_mode,
  (void *) aci_hal_get_fw_build_number,
  (void *) aci_hal_write_config_data,
  (void *) aci_hal_read_config_data,
  (void *) aci_hal_set_tx_power_level,
  (void *) aci_hal_le_tx_test_packet_number,
  (void *) aci_hal_tone_start,
  (void *) aci_hal_tone_stop,
  (void *) aci_hal_get_link_status,
  (void *) aci_hal_set_radio_activity_mask,
  (void *) aci_hal_get_anchor_period,
  (void *) aci_hal_set_event_mask,
  (void *) aci_gap_set_non_discoverable,
  (void *) aci_gap_set_limited_discoverable,
  (void *) aci_gap_set_discoverable,
  (void *) aci_gap_set_direct_connectable,
  (void *) aci_gap_set_io_capability,
  (void *) aci_gap_set_authentication_requirement,
  (void *) aci_gap_set_authorization_requirement,
  (void *) aci_gap_pass_key_resp,
  (void *) aci_gap_authorization_resp,
  (void *) aci_gap_init,
  (void *) aci_gap_set_non_connectable,
  (void *) aci_gap_set_undirected_connectable,
  (void *) aci_gap_slave_security_req,
  (void *) aci_gap_update_adv_data,
  (void *) aci_gap_delete_ad_type,
  (void *) aci_gap_get_security_level,
  (void *) aci_gap_set_event_mask,
  (void *) aci_gap_configure_whitelist,
  (void *) aci_gap_terminate,
  (void *) aci_gap_clear_security_db,
  (void *) aci_gap_allow_rebond,
  (void *) aci_gap_start_limited_discovery_proc,
  (void *) aci_gap_start_general_discovery_proc,
  (void *) aci_gap_start_name_discovery_proc,
  (void *) aci_gap_start_auto_connection_establish_proc,
  (void *) aci_gap_start_general_connection_establish_proc,
  (void *) aci_gap_start_selective_connection_establish_proc,
  (void *) aci_gap_create_connection,
  (void *) aci_gap_terminate_gap_proc,
  (void *) aci_gap_start_connection_update,
  (void *) aci_gap_send_pairing_req,
  (void *) aci_gap_resolve_private_addr,
  (void *) aci_gap_set_broadcast_mode,
  (void *) aci_gap_start_observation_proc,
  (void *) aci_gap_get_bonded_devices,
  (void *) aci_gap_is_device_bonded,
  (void *) aci_gap_numeric_comparison_value_confirm_yesno,
  (void *) aci_gap_passkey_input,
  (void *) aci_gap_get_oob_data,
  (void *) aci_gap_set_oob_data,
  (void *) aci_gap_add_devices_to_resolving_list,
  (void *) aci_gap_remove_bonded_device,
  (void *) aci_gatt_init,
  (void *) aci_gatt_add_service,
  (void *) aci_gatt_include_service,
  (void *) aci_gatt_add_char,
  (void *) aci_gatt_add_char_desc,
  (void *) aci_gatt_update_char_value,
  (void *) aci_gatt_del_char,
  (void *) aci_gatt_del_service,
  (void *) aci_gatt_del_include_service,
  (void *) aci_gatt_set_event_mask,
  (void *) aci_gatt_exchange_config,
  (void *) aci_att_find_info_req,
  (void *) aci_att_find_by_type_value_req,
  (void *) aci_att_read_by_type_req,
  (void *) aci_att_read_by_group_type_req,
  (void *) aci_att_prepare_write_req,
  (void *) aci_att_execute_write_req,
  (void *) aci_gatt_disc_all_primary_services,
  (void *) aci_gatt_disc_primary_service_by_uuid,
  (void *) aci_gatt_find_included_services,
  (void *) aci_gatt_disc_all_char_of_service,
  (void *) aci_gatt_disc_char_by_uuid,
  (void *) aci_gatt_disc_all_char_desc,
  (void *) aci_gatt_read_char_value,
  (void *) aci_gatt_read_using_char_uuid,
  (void *) aci_gatt_read_long_char_value,
  (void *) aci_gatt_read_multiple_char_value,
  (void *) aci_gatt_write_char_value,
  (void *) aci_gatt_write_long_char_value,
  (void *) aci_gatt_write_char_reliable,
  (void *) aci_gatt_write_long_char_desc,
  (void *) aci_gatt_read_long_char_desc,
  (void *) aci_gatt_write_char_desc,
  (void *) aci_gatt_read_char_desc,
  (void *) aci_gatt_write_without_resp,
  (void *) aci_gatt_signed_write_without_resp,
  (void *) aci_gatt_confirm_indication,
  (void *) aci_gatt_write_resp,
  (void *) aci_gatt_allow_read,
  (void *) aci_gatt_set_security_permission,
  (void *) aci_gatt_set_desc_value,
  (void *) aci_gatt_read_handle_value,
  (void *) aci_gatt_update_char_value_ext,
  (void *) aci_gatt_deny_read,
  (void *) aci_gatt_set_access_permission,
  (void *) aci_l2cap_connection_parameter_update_req,
  (void *) aci_l2cap_connection_parameter_update_resp
};
