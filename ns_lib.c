/**
 * @file ns_lib.c
 * @brief Weak default implementations of NS-LIB-HID console callback hooks.
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

ns_config_status_t ns_api_init(const ns_device_config_s *cfg)
{
    ns_config_status_t st = ns_config_set(cfg);
    if (st != NS_CONFIG_OK)
    {
        return st;
    }
    ns_analog_calibration_init();
    ns_haptics_init();
    return NS_CONFIG_OK;
}

bool ns_api_generate_inputreport(uint8_t data[64])
{
    if (data == NULL)
    {
        return false;
    }
    return ns_protocol_generate_inputreport(data);
}

void ns_api_output_tunnel(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0u)
    {
        return;
    }
    ns_protocol_process_outputreport(data, len);
}

void ns_api_convert_haptic_packet(ns_haptics_packet_raw_s *in, ns_haptics_packet_processed_s *out)
{
    ns_haptics_convert_raw_to_processed(in, out);
}

void ns_api_generate_fp_haptic_frequency_tables(uint16_t shift, uint16_t sine_table_width, uint16_t sample_rate_hz,
    uint16_t hi_out[128], uint16_t lo_out[128])
{
    ns_haptics_generate_fixedpoint_frequency_step_tables(shift, sine_table_width, sample_rate_hz, hi_out, lo_out);
}

void ns_api_generate_fp_amplitude_multiplier_table(uint16_t shift, uint16_t out[256])
{
    ns_haptics_generate_fixedpoint_amplitude_multiplier_table(shift, out);
}

void ns_api_motion_update_quaternion(ns_quaternion_s *state, ns_gyrodata_s *sample)
{
    static ns_motion_quat_integrator_s integrator = {0};
    
}

/*
 * Platform hooks (declared in ns_lib_api.h): weak definitions — firmware should supply strong replacements.
 */
__attribute__((weak)) void ns_api_hook_get_time_ms(uint64_t *ms)
{
    static uint64_t t;
    if (!ms)
    {
        return;
    }
    t++;
    *ms = t;
}

__attribute__((weak)) uint8_t ns_api_hook_get_random_u8(void)
{
    static uint16_t s = 0xACE1u;
    s = (uint16_t)(s * 1103515245u + 12345u);
    return (uint8_t)(s >> 8);
}

__attribute__((weak)) void ns_api_hook_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet)
{
    (void)&packet;
}

__attribute__((weak)) uint8_t ns_api_hook_get_vibrator_nibble(void)
{
    return 0;
}

__attribute__((weak)) void ns_api_hook_set_led(int player_leds)
{
    (void)player_leds;
}

__attribute__((weak)) void ns_api_hook_set_power(uint8_t shutdown)
{
    (void)shutdown;
}

__attribute__((weak)) void ns_api_hook_set_usbpair(ns_usbpair_s pairing_data)
{
    (void)pairing_data;
}

__attribute__((weak)) void ns_api_hook_get_powerstatus(ns_powerstatus_s *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->battery_level = NS_BATLVL_FULL;
    out->power_source = NS_POWERSRC_EXTERNAL;
    out->charging_status = NS_CHARGING_IDLE;
}

__attribute__((weak)) void ns_api_hook_set_imu_mode(ns_imu_mode_t imu_mode)
{
    (void)imu_mode;
}

__attribute__((weak)) void ns_api_hook_get_imu(ns_gyrodata_s *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
}

__attribute__((weak)) void ns_api_hook_get_quaternion(ns_quaternion_s *out)
{
    if (!out)
    {
        return;
    }
    static ns_quaternion_s quat_state = {.raw = {0.f, 0.f, 0.f, 1.f}};
    *out = quat_state;
}

__attribute__((weak)) void ns_api_hook_get_input(ns_input_s *out)
{
    if (!out)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->ls_x = 2048;
    out->ls_y = 2048;
    out->rs_x = 2048;
    out->rs_y = 2048;
}
