/**
 * @file ns_lib_protocol.c
 * @brief OUT-report command parsing and input/command report helpers (NS-LIB-HID; migrated from HOJA switch_commands).
 *
 * Copyright (c) 2026 Mitchell Cairns, Hand Held Legend, LLC.
 *
 * Licensed under the Creative Commons Attribution-NonCommercial 4.0 International License
 * (CC BY-NC 4.0). Non-commercial use with attribution; commercial licensing from Hand Held Legend.
 * Full terms: https://creativecommons.org/licenses/by-nc/4.0/legalcode — see LICENSING.md in this folder
 *
 * SPDX-License-Identifier: CC-BY-NC-4.0
 *
 * TRADEMARK AND AFFILIATION DISCLAIMER:
 * This library is not affiliated, associated, authorized, endorsed by, or in any way officially
 * connected with Nintendo Co., Ltd., or any of its subsidiaries or its affiliates. Nintendo and
 * related marks are trademarks of their respective owners.
 */

#include "ns_lib.h"
#include "ns_lib_protocol.h"
#include "ns_lib_config.h"
#include "ns_lib_spi.h"
#include "ns_lib_types.h"
#include "ns_lib_haptics.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Common report/packet offsets */
#define NS_PROTOCOL_IN_IDX_REPORT_ID 0u
#define NS_PROTOCOL_IN_IDX_TIMER 1u
#define NS_PROTOCOL_IN_IDX_BATTCONN 2u
#define NS_PROTOCOL_IN_IDX_BUTTONS 3u
#define NS_PROTOCOL_IN_IDX_STICKS 6u

#define NS_PROTOCOL_IN_IDX_VIBRATOR 12u
#define NS_PROTOCOL_IN_IDX_ACK 13u
#define NS_PROTOCOL_IN_IDX_ACK_SUBCMD 14u
#define NS_PROTOCOL_IN_IDX_PAYLOAD 15u

#define NS_PROTOCOL_IDX_IMU_DATA_START 13u

#define NS_PROTOCOL_OUT_IDX_REPORT_ID 0u
#define NS_PROTOCOL_OUT_IDX_INFO 1u
#define NS_PROTOCOL_OUT_IDX_PACKETNUM 1u
#define NS_PROTOCOL_OUT_HAPTIC_DATA_START 2u
#define NS_PROTOCOL_OUT_IDX_SUBCMD 10u
#define NS_PROTOCOL_OUT_IDX_SUBCMD_ARG 11u

#define NS_LIB_PROTOCOL_IN_REPORT_ID_30 0x30u
#define NS_LIB_PROTOCOL_REPLY_ID_21 0x21u
#define NS_LIB_PROTOCOL_REPLY_ID_81 0x81u
#define NS_LIB_PROTOCOL_MAX_PACKET_BYTES 64u
#define NS_LIB_PROTOCOL_CMD_FIFO_DEPTH 8u

typedef struct
{
    uint16_t len;
    uint8_t data[NS_LIB_PROTOCOL_MAX_PACKET_BYTES];
} ns_protocol_pending_packet_s;

typedef struct
{
    uint8_t imu_mode;
    bool init_sent;
    uint8_t reporting_mode;
    uint8_t link_key[16];
} ns_protocol_sm_s;

static ns_protocol_sm_s _protocol_sm = {.imu_mode = NS_IMU_OFF, .init_sent = false, .reporting_mode = NS_LIB_PROTOCOL_IN_REPORT_ID_30};

static ns_protocol_pending_packet_s s_ns_lib_protocol_queue[NS_LIB_PROTOCOL_CMD_FIFO_DEPTH];
static uint8_t s_ns_lib_protocol_queue_head = 0u;
static uint8_t s_ns_lib_protocol_queue_tail = 0u;
static uint8_t s_ns_lib_protocol_queue_count = 0u;

static void _ns_protocol_generate_linkkey(void)
{
    for (uint8_t i = 0; i < 16; i++)
    {
        _protocol_sm.link_key[i] = ns_api_hook_get_random_u8();
    }
}

static void _ns_protocol_set_ack(uint8_t ack, uint8_t *target)
{
    target[NS_PROTOCOL_IN_IDX_ACK] = ack;
}

static void _ns_protocol_set_command(uint8_t command, uint8_t *target)
{
    target[NS_PROTOCOL_IN_IDX_ACK_SUBCMD] = command;
}

static void _ns_protocol_set_timer(uint8_t *target)
{
    static uint64_t this_ms = 0;
    ns_api_hook_get_time_ms(&this_ms);

    static uint64_t ns_lib_protocol_timer = 0;
    ns_lib_protocol_timer += this_ms;
    if (ns_lib_protocol_timer > 0xFF)
    {
        ns_lib_protocol_timer %= 0xFF;
    }
    target[NS_PROTOCOL_IN_IDX_TIMER] = (uint8_t)ns_lib_protocol_timer;
}

static void _ns_protocol_set_battconn(uint8_t *target)
{
    ns_powerstatus_s ps = {0};
    ns_api_hook_get_powerstatus(&ps);

    ns_batstat_s stat = {0};
    stat.bat_lvl = ps.battery_level > NS_BATLVL_FULL ? NS_BATLVL_FULL : ps.battery_level;
    stat.charging = ps.charging_status > NS_CHARGING_ACTIVE ? NS_CHARGING_ACTIVE : ps.charging_status;
    stat.power_source = ps.power_source > NS_POWERSRC_EXTERNAL ? NS_POWERSRC_EXTERNAL : ps.power_source;

    target[NS_PROTOCOL_IN_IDX_BATTCONN] = stat.val;
}

static void _ns_protocol_set_devinfo(uint8_t *target)
{
    uint8_t fw_hi, fw_lo;
    ns_config_get_fw_version(&fw_hi, &fw_lo);

    /* Factory configuration and calibration */
    uint8_t factory_id_hi, factory_id_lo, factory_color_byte, factory_snes_region_byte;
    ns_config_get_devtype_bytes(&factory_id_hi, &factory_id_lo, &factory_color_byte,
                            &factory_snes_region_byte);

    target[NS_PROTOCOL_IN_IDX_PAYLOAD + 0] = fw_hi;
    target[NS_PROTOCOL_IN_IDX_PAYLOAD + 1] = fw_lo;
    target[NS_PROTOCOL_IN_IDX_PAYLOAD + 2] = factory_id_hi;
    target[NS_PROTOCOL_IN_IDX_PAYLOAD + 3] = factory_id_lo;

    ns_config_get_device_mac(&target[NS_PROTOCOL_IN_IDX_PAYLOAD + 4]);

    // I don't know what these do :)
    target[NS_PROTOCOL_IN_IDX_PAYLOAD + 10] = 0x01;
    target[NS_PROTOCOL_IN_IDX_PAYLOAD + 11] = 0x02;
}

static void _ns_protocol_set_subtriggertime(uint16_t time_10_ms, uint8_t *target)
{
    uint8_t upper_ms = 0xFF & time_10_ms;
    uint8_t lower_ms = (uint8_t)((0xFF00 & time_10_ms) >> 8);

    for (uint8_t i = 0; i < 14; i += 2)
    {
        target[NS_PROTOCOL_IN_IDX_PAYLOAD + i] = upper_ms;
        target[NS_PROTOCOL_IN_IDX_PAYLOAD + i] = lower_ms;
    }
}

static void _ns_protocol_set_inputdata(ns_input_s *in, uint8_t *target)
{
    ns_input_packed_s packed = {
        .left_buttons = in->left_buttons,
        .shared_buttons = in->shared_buttons,
        .right_buttons = in->right_buttons,
    };

    ns_analog_pack_xy12(in->ls_x, in->ls_y, packed.left_stick);
    ns_analog_pack_xy12(in->rs_x, in->rs_y, packed.right_stick);

    memcpy(&target[NS_PROTOCOL_IN_IDX_BUTTONS], &packed, sizeof(packed));
}

static void _ns_protocol_info_set_mac(uint8_t *target)
{
    target[1] = 1u;
    target[2] = 0u;
    target[3] = 3u;

    uint8_t mac[6];
    ns_config_get_device_mac(mac);

    target[4] = mac[5];
    target[5] = mac[4];
    target[6] = mac[3];
    target[7] = mac[2];
    target[8] = mac[1];
    target[9] = mac[0];
}

static void _ns_protocol_info_handler(uint8_t *in, uint8_t *target)
{
    target[0] = NS_LIB_PROTOCOL_REPLY_ID_81;

    uint8_t info_code = in[1];

    switch (info_code)
    {
    // First stage, device info request
    case 0x01:
        _ns_protocol_info_set_mac(target);
        break;

    // Host is checking if gamepad is ready/fully initialized
    case 0x02:
    // Seems to be sent by Steam/Windows
    case 0x03:
        // Mirror back the info code
        target[1] = info_code;
        break;

    // We do not reply to 0x04
    case 0x04:
        break;
    }
}

static void _ns_protocol_pairing_set(const uint8_t *in, uint8_t *target)
{
    // This is a const value that is used in the pairing response data
    const uint8_t pro_controller_string[25] = {0x00, 0x25, 0x08, 0x50, 0x72, 0x6F, 0x20, 0x43, 0x6F,
                                               0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C, 0x65, 0x72, 0x00,
                                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68};

    static uint8_t host_mac[6] = {0};

    ns_device_config_s cfg = {0};
    ns_config_get(&cfg);

    uint8_t in_code = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG];

    bool console_has_gamepad_in_db  = false;
    bool message_has_console_mac    = false;
    bool gamepad_host_mac_matches   = true;

    // set ACK
    _ns_protocol_set_ack(0x81, target);

    switch(in_code)
    {
        // Console does not have a matching
        // DB entry for this gamepad's MAC address
        case 1:
        console_has_gamepad_in_db = false;
        message_has_console_mac = true;
        break;

        // Console has a matching DB entry
        // for this gamepad's MAC address
        case 4:
        console_has_gamepad_in_db = true;
        message_has_console_mac = true;
        break;

        // Console is requesting a new
        // randomly generated Link Key
        // The gamepad generates the Link Key
        // and provides it to the Console
        case 2:
        target[NS_PROTOCOL_IN_IDX_PAYLOAD] = 2;
        // Generate link key
        _ns_protocol_generate_linkkey();
        // Copy XOR'd link key
        for (int i = 0; i < 16; i++)
        {
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 1 + i] = _protocol_sm.link_key[i] ^ 0xAA;
        }

        // Update runtime config host mac
        ns_config_set_host_mac(host_mac);

        // Save our data to gamepad
        ns_usbpair_s pair = {0};
        memcpy(pair.host_mac, host_mac, 6);
        memcpy(pair.link_key, _protocol_sm.link_key, 16);

        // Send user-space callback
        // To save link key for 
        // Host MAC address
        ns_api_hook_set_usbpair(pair);
        return;

        // Console indicates that it received
        // the Link Key and it's stored
        default:
        case 3:
        target[NS_PROTOCOL_IN_IDX_PAYLOAD] = 3;
        return;
    }

    if(message_has_console_mac)
    {
        // Extract HOST address
        host_mac[0] = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 6];
        host_mac[1] = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 5];
        host_mac[2] = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 4];
        host_mac[3] = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 3];
        host_mac[4] = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 2];
        host_mac[5] = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 1];

        // Compare host address to our stored host address
        for(int i = 0; i < 6; i++)
        {
            if(host_mac[i] != cfg.host_mac[i])
            {
               gamepad_host_mac_matches = false;
            }
        }

        if(!gamepad_host_mac_matches || !console_has_gamepad_in_db)
        {
            target[NS_PROTOCOL_IN_IDX_PAYLOAD] = 1;
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 1] = cfg.device_mac[5];
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 2] = cfg.device_mac[4];
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 3] = cfg.device_mac[3];
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 4] = cfg.device_mac[2];
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 5] = cfg.device_mac[1];
            target[NS_PROTOCOL_IN_IDX_PAYLOAD + 6] = cfg.device_mac[0];
            memcpy(&target[NS_PROTOCOL_IN_IDX_PAYLOAD + 7], pro_controller_string, sizeof(pro_controller_string));
        }
        else
        {
            target[NS_PROTOCOL_IN_IDX_PAYLOAD] = 3;
        }
        return;
    }
}

static void _ns_protocol_set_vibrator(uint8_t *out)
{
    // Official Procon captures put fill/strobe in the HIGH nibble (type 1).
    // Leave the low nibble 0 so this matches 0x70 / 0xB0 / 0xC0 traces.
    uint8_t nibble = (uint8_t)(ns_api_hook_get_vibrator_nibble() & 0x0Fu);
    out[NS_PROTOCOL_IN_IDX_VIBRATOR] = (uint8_t)(nibble << 4);
}

static void _ns_protocol_set_standardreport(uint8_t *out)
{
    // Send standard 0x30 Input Report
    out[NS_PROTOCOL_IN_IDX_REPORT_ID] = NS_LIB_PROTOCOL_IN_REPORT_ID_30;
    _ns_protocol_set_timer(out);
    _ns_protocol_set_battconn(out);

    static ns_gyrodata_s gyro = {0};
    static ns_mode_2_s mode_2_data = {0};
    static ns_quaternion_s quat = {0};

    static ns_input_s input = {0};
    ns_api_hook_get_input(&input);
    _ns_protocol_set_inputdata(&input, out);
    _ns_protocol_set_vibrator(out);

    switch (_protocol_sm.imu_mode)
    {
    case NS_IMU_RAW:
        // Callback to get user gyro data
        ns_api_hook_get_imu(&gyro);
        // Group 1
        out[NS_PROTOCOL_IDX_IMU_DATA_START] = gyro.ay_8l; // Y-axis
        out[14] = gyro.ay_8h;
        out[15] = gyro.ax_8l; // X-axis
        out[16] = gyro.ax_8h;
        out[17] = gyro.az_8l; // Z-axis
        out[18] = gyro.az_8h;
        out[19] = gyro.gy_8l;
        out[20] = gyro.gy_8h;
        out[21] = gyro.gx_8l;
        out[22] = gyro.gx_8h;
        out[23] = gyro.gz_8l;
        out[24] = gyro.gz_8h;
        // Group 2
        memcpy(&out[25], &out[NS_PROTOCOL_IDX_IMU_DATA_START], 12);
        // Group 3
        memcpy(&out[37], &out[NS_PROTOCOL_IDX_IMU_DATA_START], 12);
        break;
    case NS_IMU_QUAT:
    {
        // Callback to get user quaternion data
        ns_api_hook_get_quaternion(&quat);
        // Pack our quaternion data
        ns_motion_pack_quat(&quat, &mode_2_data);
        // Copy to output buffer
        memcpy(&(out[NS_PROTOCOL_IDX_IMU_DATA_START]), &mode_2_data, sizeof(ns_mode_2_s));
    }
    break;

    case NS_IMU_OFF:
    default:
        break;
    }
}

static void _ns_protocol_command_handler(const uint8_t *in, uint8_t *out)
{
    out[0] = NS_LIB_PROTOCOL_REPLY_ID_21;
    uint8_t command = in[NS_PROTOCOL_OUT_IDX_SUBCMD];

    _ns_protocol_set_timer(out);
    _ns_protocol_set_battconn(out);

    static ns_input_s input = {0};
    ns_api_hook_get_input(&input);
    _ns_protocol_set_inputdata(&input, out);
    _ns_protocol_set_vibrator(out);

    _ns_protocol_set_command(command, out);

    switch (command)
    {
    case NS_LIB_PROTOCOL_SET_NFC:
        _ns_protocol_set_ack(0x80, out);
        break;
    case NS_LIB_PROTOCOL_ENABLE_IMU:
        _protocol_sm.imu_mode = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG];
        ns_api_hook_set_imu_mode(_protocol_sm.imu_mode);
        _ns_protocol_set_ack(0x80, out);
        break;
    case NS_LIB_PROTOCOL_SET_PAIRING:
        // ACK is set within the pairing set handler
        // because of the varieties of responses used
        _ns_protocol_pairing_set(in, out);
        break;
    case NS_LIB_PROTOCOL_SET_INPUTMODE:
        _ns_protocol_set_ack(0x80, out);
        _protocol_sm.reporting_mode = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG];
        break;
    case NS_LIB_PROTOCOL_GET_DEVICEINFO:
        _ns_protocol_set_ack(0x82, out);
        _ns_protocol_set_devinfo(out);
        break;
    case NS_LIB_PROTOCOL_SET_SHIPMODE:
        _ns_protocol_set_ack(0x80, out);
        // Don't handle this
        break;
    case NS_LIB_PROTOCOL_GET_SPI:
        _ns_protocol_set_ack(0x90, out);
        ns_spi_get(in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 1], in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG],
                   in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG + 4], out);
        break;
    case NS_LIB_PROTOCOL_SET_HCI:
        // We handle this pre-emptively, no need to deal with this here
        break;
    case NS_LIB_PROTOCOL_SET_SPI:
        _ns_protocol_set_ack(0x80, out);
        // We do not write to our virtual SPI device
        break;
    case NS_LIB_PROTOCOL_GET_TRIGGERET:
        _ns_protocol_set_ack(0x83, out);
        _ns_protocol_set_subtriggertime(100, out);
        break;
    case NS_LIB_PROTOCOL_ENABLE_VIBRATE:
        _ns_protocol_set_ack(0x80, out);
        // Unhandled right now. Presumed to be enabled always.
        break;
    case NS_LIB_PROTOCOL_SET_PLAYER:
    {
        _ns_protocol_set_ack(0x80, out);
        uint8_t player = in[NS_PROTOCOL_OUT_IDX_SUBCMD_ARG] & 0xF;
        uint8_t set_num = 0;
        switch (player)
        {
        default:
            break;
        case 0b1:
            set_num = 1;
            break;
        case 0b11:
            set_num = 2;
            break;
        case 0b111:
            set_num = 3;
            break;
        case 0b1111:
            set_num = 4;
            break;
        case 0b1001:
            set_num = 5;
            break;
        case 0b1010:
            set_num = 6;
            break;
        case 0b1011:
            set_num = 7;
            break;
        case 0b0110:
            set_num = 8;
            break;
        }

        ns_api_hook_set_led(set_num);
    }
    break;
    default:
        _ns_protocol_set_ack(0x80, out);
        break;
    }
}

static bool _ns_protocol_outputqueue_pop(ns_protocol_pending_packet_s *out_packet)
{
    if (out_packet == NULL || s_ns_lib_protocol_queue_count == 0u)
    {
        return 0u;
    }

    *out_packet = s_ns_lib_protocol_queue[s_ns_lib_protocol_queue_head];

    s_ns_lib_protocol_queue_head = (uint8_t)((s_ns_lib_protocol_queue_head + 1u) % NS_LIB_PROTOCOL_CMD_FIFO_DEPTH);
    s_ns_lib_protocol_queue_count--;
    return true;
}

bool _ns_protocol_generate_init(uint8_t out[64])
{
    if (out == NULL)
    {
        return false;
    }

    memset(out, 0, 64);
    out[0] = NS_LIB_PROTOCOL_REPLY_ID_81;
    _ns_protocol_info_set_mac(out);

    return true;
}

bool ns_protocol_generate_inputreport(uint8_t out[64])
{
    memset(out, 0, 64);

    // First packet sent should be the init packet
    // If we are using Bluetooth this is transparent
    if (!_protocol_sm.init_sent)
    {
        // Check our protocol. We only generate an INIT message
        // if we are using USB
        if (ns_config_get_transport() == NS_TRANSPORT_USB)
        {
            if (_ns_protocol_generate_init(out))
            {
                _protocol_sm.init_sent = true;
                return true;
            }
            else
                return false;
        }
        else
        {
            _protocol_sm.init_sent = true;
        }
    }

    ns_protocol_pending_packet_s pending = {.len = 0};

    if (_ns_protocol_outputqueue_pop(&pending))
    {
        // Process pending command
        switch (pending.data[NS_PROTOCOL_OUT_IDX_REPORT_ID])
        {
        // No haptics are handled here.
        // The haptic data is ingested at reception time.
        case NS_LIB_PROTOCOL_OUT_ID_RUMBLE_CMD:
            _ns_protocol_command_handler(pending.data, out);
            break;

        
        case NS_LIB_PROTOCOL_OUT_ID_INFO:
            // Do not reply to 0x04
            if(pending.data[1] == 0x04)
            {
                goto send_standard_report;
            }
            _ns_protocol_info_handler(pending.data, out);
            break;

        default:
            goto send_standard_report;
        }
        return true;
    }

send_standard_report:
    _ns_protocol_set_standardreport(out);
    return true;
}

void ns_protocol_process_outputreport(const uint8_t *in, uint16_t len)
{
    if (in == NULL || len == 0u)
    {
        return;
    }

    if (len > NS_LIB_PROTOCOL_MAX_PACKET_BYTES)
    {
        len = NS_LIB_PROTOCOL_MAX_PACKET_BYTES;
    }

    // Process any possible haptic data here
    switch (in[NS_PROTOCOL_OUT_IDX_REPORT_ID])
    {
    case NS_LIB_PROTOCOL_OUT_ID_RUMBLE:
        // Process haptics
        ns_haptics_rumble_translate(&in[NS_PROTOCOL_OUT_HAPTIC_DATA_START]);
        break;

    case NS_LIB_PROTOCOL_OUT_ID_RUMBLE_CMD:

        // Check for shutdown case
        // For such a command, a shutdown should occur before
        // anything else
        if (in[NS_PROTOCOL_OUT_IDX_SUBCMD] == NS_LIB_PROTOCOL_SET_HCI)
        {
            ns_api_hook_set_power(true);
            return;
        }

        // Process haptics
        ns_haptics_rumble_translate(&in[NS_PROTOCOL_OUT_HAPTIC_DATA_START]);
        break;

    default:
    case NS_LIB_PROTOCOL_OUT_ID_INFO:
        // Ingest command passthrough
        break;
    }

    if (s_ns_lib_protocol_queue_count >= NS_LIB_PROTOCOL_CMD_FIFO_DEPTH)
    {
        return;
    }

    ns_protocol_pending_packet_s *slot = &s_ns_lib_protocol_queue[s_ns_lib_protocol_queue_tail];
    slot->len = len;
    memcpy(slot->data, in, len);

    s_ns_lib_protocol_queue_tail = (uint8_t)((s_ns_lib_protocol_queue_tail + 1u) % NS_LIB_PROTOCOL_CMD_FIFO_DEPTH);
    s_ns_lib_protocol_queue_count++;
}
