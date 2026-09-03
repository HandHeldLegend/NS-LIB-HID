/**
 * @file ns_lib_haptics.h
 * @brief HD rumble decode helpers plus float/fixed-point lookup-table utilities.
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

#ifndef NS_LIB_HAPTICS_H
#define NS_LIB_HAPTICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NS_LIB_HAPTIC_CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

typedef enum
{
    NS_LIB_HAPTIC_ACTION_IGNORE = 0x0,
    NS_LIB_HAPTIC_ACTION_DEFAULT = 0x1,
    NS_LIB_HAPTIC_ACTION_SUBSTITUTE = 0x2,
    NS_LIB_HAPTIC_ACTION_SUM = 0x3,
} ns_lib_haptic_action_t;

typedef struct
{
    ns_lib_haptic_action_t am_action : 8;
    ns_lib_haptic_action_t fm_action : 8;
    int16_t am_offset;
    int16_t fm_offset;
} ns_lib_haptic_5bit_cmd_s;

typedef union
{
    struct
    {
        uint32_t data : 30;
        /* PackFormat (bits 31:30). Equals sample count only for 5-bit encodings. */
        uint32_t frame_count : 2;
    };

    struct
    {
        uint32_t cmd_hi_2 : 5;
        uint32_t cmd_lo_2 : 5;
        uint32_t cmd_hi_1 : 5;
        uint32_t cmd_lo_1 : 5;
        uint32_t cmd_hi_0 : 5;
        uint32_t cmd_lo_0 : 5;
        uint32_t frame_count : 2;
    } type1;

    struct
    {
        uint32_t padding : 2;
        uint32_t freq_hi : 7;
        uint32_t amp_hi : 7;
        uint32_t freq_lo : 7;
        uint32_t amp_lo : 7;
        uint32_t frame_count : 2;
    } type2;

    struct
    {
        uint32_t high_select : 1;
        uint32_t freq_xx_0 : 7;
        uint32_t cmd_hi_1 : 5;
        uint32_t cmd_lo_1 : 5;
        uint32_t cmd_xx_0 : 5;
        uint32_t amp_xx_0 : 7;
        uint32_t frame_count : 2;
    } type3;

    struct
    {
        uint32_t high_select : 1;
        uint32_t blank : 1; /* IsThree7bit; official firmware requires 1 */
        uint32_t freq_select : 1;
        uint32_t cmd_hi_2 : 5;
        uint32_t cmd_lo_2 : 5;
        uint32_t cmd_hi_1 : 5;
        uint32_t cmd_lo_1 : 5;
        uint32_t xx_xx_0 : 7;
        uint32_t frame_count : 2;
    } type4;
} __attribute__((packed)) ns_lib_haptic_wire_u;

/** Rows in the exp₂ amplitude envelope LUT. */
#define NS_LIB_HAPTICS_AMP_LUT_LEN 256u

/** Entries per rumble frequency axis (high / low band share index range 0–127). */
#define NS_LIB_HAPTICS_FREQ_LUT_LEN 128u

/**
 * @brief Reference lookup tables (floats).
 *
 * Fill with @ref ns_haptics_build_raw_tables (same curves as @ref ns_haptics_init loads internally).
 *
 * PCM phase increments and Q-format amplitudes are not stored here — generate those in your PCM driver
 * from frequency_hz_* and amplitude_linear if needed.
 */
typedef struct
{
    /** Unitless exp₂ envelope sample per row (0 below library cutoff). Indexed by decoded amplitude index. */
    float amplitude_linear[NS_LIB_HAPTICS_AMP_LUT_LEN];
    /** Synthesized sine frequency (Hz) for the high haptic band. */
    float frequency_hz_hi[NS_LIB_HAPTICS_FREQ_LUT_LEN];
    /** Synthesized sine frequency (Hz) for the low haptic band. */
    float frequency_hz_lo[NS_LIB_HAPTICS_FREQ_LUT_LEN];
} ns_haptics_tables_s;

/**
 * @brief Forward a decoded raw haptics packet to user space.
 *
 * This is the low-level dispatch point used by the decoder once a host rumble
 * word has been expanded into one library packet. Most firmware should
 * override @ref ns_api_hook_set_haptic_packet_raw instead of calling this
 * function directly.
 */
void ns_haptics_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet);

/**
 * @brief Convert a raw haptic packet with table indices into processed values.
 *
 * Translates amplitude and frequency indices from the raw packet into linear
 * amplitude and frequency values using the library's lookup tables.
 *
 * @param in   Source raw packet containing amplitude/frequency indices.
 * @param out  Destination processed packet to receive converted values.
 */
void ns_haptics_convert_raw_to_processed(ns_haptics_packet_raw_s *in, ns_haptics_packet_processed_s *out);

/**
 * @brief Build fixed-point phase-step lookup tables for decoded frequency rows.
 *
 * Each output entry is derived from the library's internal Hz reference table
 * and scaled for a DDS / wavetable playback engine.
 *
 * @param shift Fixed-point scale factor, typically `1u << Q`.
 * @param sine_table_width Number of samples in the oscillator wavetable.
 * @param sample_rate_hz PCM update rate used by the playback engine.
 * @param hi_out Output table for the high-band frequency rows.
 * @param lo_out Output table for the low-band frequency rows.
 */
void ns_haptics_generate_fixedpoint_frequency_step_tables(uint16_t shift, uint16_t sine_table_width, uint16_t sample_rate_hz,
    uint16_t hi_out[NS_LIB_HAPTICS_FREQ_LUT_LEN], uint16_t lo_out[NS_LIB_HAPTICS_FREQ_LUT_LEN]);

/**
 * @brief Build a fixed-point amplitude multiplier table for decoded amplitude rows.
 *
 * This is intended for playback engines that want to index directly from the
 * decoded amplitude row to a pre-scaled fixed-point gain value.
 *
 * @param shift Fixed-point scale factor, typically `1u << Q`.
 * @param out Output table indexed by decoded amplitude row.
 */
void ns_haptics_generate_fixedpoint_amplitude_multiplier_table(uint16_t shift, uint16_t out[NS_LIB_HAPTICS_AMP_LUT_LEN]);

/** @brief Initialize the built-in haptics lookup tables and raw decode state. */
void ns_haptics_init(void);

/**
 * @brief Decode one 4-byte host rumble word into raw haptics indices.
 *
 * The decoder preserves running hi/lo state between packets and dispatches the
 * resulting raw packet through the haptics callback path.
 *
 * @param data Pointer to the 4-byte rumble payload from the host report.
 */
void ns_haptics_rumble_translate(const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* NS_LIB_HAPTICS_H */
