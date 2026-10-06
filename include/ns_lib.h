/**
 * @file ns_lib.h
 * @brief User-facing NS-LIB-HID API: configuration, host weak callbacks, platform hooks, and protocol
 *        entry points.
 */

#ifndef NS_LIB_API_H
#define NS_LIB_API_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ns_lib_types.h"
#include "ns_lib_analog.h"
#include "ns_lib_config.h"
#include "ns_lib_haptics.h"
#include "ns_lib_motion.h"
#include "ns_lib_spi.h"
#include "ns_lib_hid.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- API Functions --- */

/**
 * @brief One-shot initialize: apply device config, then initialize runtime state.
 *
 * @param cfg Device configuration to apply.
 * @return `NS_CONFIG_OK` on success; validation error otherwise.
 */
ns_config_status_t ns_api_init(const ns_device_config_s *cfg);

/**
 * @brief Build one outgoing input report payload in-place.
 *
 * Writes report id to `data[0]` and payload to `data[1..]`.
 *
 * @param data[64] Output buffer of 64 bytes size.*/
bool ns_api_generate_inputreport(uint8_t data[64]);

/**
 * @brief Send raw output tunnel data to the USB/HID transport layer.
 *
 * @param data Pointer to the output packet payload.
 * @param len Length of the packet in bytes.
 */
void ns_api_output_tunnel(const uint8_t *data, uint16_t len);

/**
 * @brief Convert one decoded haptics packet from table indices into float values.
 *
 * The raw packet delivered by @ref ns_api_hook_set_haptic_packet_raw contains
 * lookup-table indices only. This helper resolves those indices into the
 * library's reference amplitude and frequency values, which is useful for
 * debug tooling, software synthesis, or actuator backends that prefer float
 * units over raw indices.
 *
 * @param in Source raw packet containing decoded indices.
 * @param out Destination packet receiving processed values.
 */
void ns_api_convert_haptic_packet(ns_haptics_packet_raw_s *in, ns_haptics_packet_processed_s *out);

/**
 * @brief Generate fixed-point hi/lo frequency step tables for hardware playback.
 *
 * This converts the library's built-in frequency reference tables into phase
 * increments for a DDS or wavetable-style synthesizer. @p shift is the
 * fixed-point scale factor used by the playback engine (for example
 * `1u << 12` for Q12), @p sine_table_width is the wavetable length, and
 * @p sample_rate_hz is the PCM update rate.
 *
 * @param shift Fixed-point scale factor applied to each phase increment.
 * @param sine_table_width Number of samples in the oscillator wavetable.
 * @param sample_rate_hz Output sample rate used by the playback engine.
 * @param hi_out Output table for the high-band frequency indices.
 * @param lo_out Output table for the low-band frequency indices.
 */
void ns_api_generate_fp_haptic_frequency_tables(uint16_t shift, uint16_t sine_table_width, uint16_t sample_rate_hz,
    uint16_t hi_out[128], uint16_t lo_out[128]);

/**
 * @brief Generate a fixed-point amplitude multiplier table for haptic playback.
 *
 * The returned values are scaled by @p shift so firmware can map a decoded
 * amplitude index directly into a mixer or PWM gain without doing float math
 * during the real-time audio/haptics path.
 *
 * @param shift Fixed-point scale factor, typically `1u << Q`.
 * @param out Output table indexed by decoded amplitude row.
 */
void ns_api_generate_fp_amplitude_multiplier_table(uint16_t shift, uint16_t out[256]);

/* --- API Platform Hook Functions --- */

/**
 * @brief Monotonic time in milliseconds (weak).
 *
 * Firmware must set `*ms` based on the MCUs system time. Example: Time since boot. 
 *
 * @param[out] ms Elapsed milliseconds; ignored if NULL.
 */
void ns_api_hook_get_time_ms(uint64_t *ms);

/**
 * @brief Non-cryptographic random byte (weak).
 *
 * Used for Bluetooth Link Key placeholder generation during pairing. Replace with a hardware RNG or CSPRNG
 * if your product requires it.
 *
 * @return Uniform-ish byte in 0..255.
 */
uint8_t ns_api_hook_get_random_u8(void);

/**
 * @brief Receive the most recent decoded HD-rumble packet (weak, user-overridable).
 *
 * Called immediately when a host OUT report containing rumble data is
 * ingested. @p packet->state holds the current cumulative hi/lo index state
 * and @p packet->samples[0..sample_count-1] contains up to three sequential
 * sub-samples from the current host packet.
 *
 * The values are lookup-table indices, not Hz or linear gain. Firmware may
 * either consume them directly with precomputed fixed-point tables or call
 * @ref ns_api_convert_haptic_packet when float values are more convenient.
 *
 * @param packet Decoded raw haptics packet; ignored if NULL.
 */
void ns_api_hook_set_haptic_packet_raw(ns_haptics_packet_raw_s *packet);

/**
 * @brief Motor-status nibble for input-report byte 12 (weak).
 *
 * Official firmware packs `fill | (strobe << 3)` into one nibble
 * (`PrepareInputReport` @ 0021a6b0). fill is used_pcm/5 capped at 7,
 * captured when a rumble word arrives, before its pairs are queued.
 * strobe is set whenever the ring is topped up because used < 5, and
 * cleared only by a non-empty rumble word. The Switch uses this byte to
 * decide whether the next vibration pattern should be sent.
 *
 * Return the 4-bit nibble (0..15). Protocol writes it to the high nibble
 * of byte 12 and leaves the low nibble 0, matching Pro Controller captures.
 */
uint8_t ns_api_hook_get_vibrator_nibble(void);

/**
 * @brief Host-set player LED callback (weak, user-overridable).
 *
 * @param player_leds Host player slot `1`–`8` from SET_PLAYER decoding, or `-1` when disconnected /
 *                    no valid assignment (unknown bitmask / cleared LEDs).
 */
void ns_api_hook_set_led(int player_leds);

/** @brief Host power-state request callback (weak, user-overridable). */
void ns_api_hook_set_power(uint8_t shutdown);

/** @brief USB pairing data callback (weak, user-overridable). */
void ns_api_hook_set_usbpair(ns_usbpair_s pairing_data);

/**
 * @brief Fill battery / USB status for input report byte @c 1 (weak, user-overridable).
 *
 * Set @ref ns_powerstatus_s bitfields (or assign @c out->val directly). Layout matches HOJA
 * `bat_status_u` / Switch Pro Controller wire format.
 *
 * @param out Output status; ignored if NULL.
 */
void ns_api_hook_get_powerstatus(ns_powerstatus_s *out);

/** @brief IMU Mode Setter.
 *
 * Set the IMU report mode.
 * 0 = disabled, 1 = raw IMU samples, 2 = quaternion.
 */
void ns_api_hook_set_imu_mode(ns_imu_mode_t imu_mode);

/**
 * @brief Fill IMU sample for standard mode reports (weak, user-overridable).
 * @param out Output gyro/accel sample.
 */
void ns_api_hook_get_imu(ns_gyrodata_s *out);

/**
 * @brief Fill quaternion state for IMU mode-2 reports (weak, user-overridable).
 */
void ns_api_hook_get_quaternion(ns_quaternion_s *out);

/**
 * @brief Fill logical button/stick state for report generation (weak, user-overridable).
 *
 * Provide button bits and unpacked stick positions in @ref ns_input_s. The
 * library packs those values into the Switch report layout internally.
 *
 * @param out Output input state structure.
 */
void ns_api_hook_get_input(ns_input_s *out);

#ifdef __cplusplus
}
#endif

#endif /* NS_LIB_API_H */
