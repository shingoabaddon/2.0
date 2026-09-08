#pragma once

#include "subghz_types.h"

#include "subghz_garage_worker.h"
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/protocols/raw.h>
#include <lib/subghz/devices/devices.h>
#include "../protocols/subghz_garage_protocol_plugin.h"
#include "../protocols/protocol_groups.h"

typedef struct SubGhzTxRx SubGhzTxRx;

typedef void (*SubGhzTxRxNeedSaveCallback)(void* context);

typedef enum {
    SubGhzTxRxStartTxStateOk,
    SubGhzTxRxStartTxStateErrorOnlyRx,
    SubGhzTxRxStartTxStateErrorParserOthers,
} SubGhzTxRxStartTxState;

/**
 * Allocate SubGhzTxRx
 * 
 * @return SubGhzTxRx* pointer to SubGhzTxRx
 */
SubGhzTxRx* subghz_txrx_alloc(void);

/**
 * Free SubGhzTxRx
 * 
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_free(SubGhzTxRx* instance);

/**
 * Check if the database is loaded
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @return bool True if the database is loaded
 */
bool subghz_txrx_is_database_loaded(SubGhzTxRx* instance);

/**
 * Set preset 
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param preset_name Name of preset
 * @param frequency Frequency in Hz
 * @param preset_data Data of preset
 * @param preset_data_size Size of preset data
 */
void subghz_txrx_set_preset(
    SubGhzTxRx* instance,
    const char* preset_name,
    uint32_t frequency,
    uint8_t* preset_data,
    size_t preset_data_size);

/**
 * Set TX Power
 * 
 * @param preset_data Data of preset
 * @param preset_data_size Size of preset data
 * @param tx_power Menu Index of TX Power Setting. (Saves iterating in Config enter)
 */
uint8_t* subghz_txrx_set_tx_power(uint8_t* preset_data, size_t preset_data_size, uint8_t tx_power);

/**
 * Get name of preset
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param preset String of preset 
 * @return const char*  Name of preset
 */
const char* subghz_txrx_get_preset_name(SubGhzTxRx* instance, const char* preset);

/**
 * Get of preset
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @return SubGhzRadioPreset Preset
 */
SubGhzRadioPreset subghz_txrx_get_preset(SubGhzTxRx* instance);

/**
 * Get string frequency and modulation
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param frequency Pointer to a string frequency
 * @param modulation Pointer to a string modulation
 */
void subghz_txrx_get_frequency_and_modulation(
    SubGhzTxRx* instance,
    FuriString* frequency,
    FuriString* modulation,
    bool long_name);

/**
 * Start TX CC1101
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param flipper_format Pointer to a FlipperFormat
 * @return SubGhzTxRxStartTxState 
 */
SubGhzTxRxStartTxState subghz_txrx_tx_start(SubGhzTxRx* instance, FlipperFormat* flipper_format);

/**
 * Start RX CC1101
 *
 * @param instance Pointer to a SubGhzTxRx
 * @return false if the active protocol group's plugin could not be loaded
 *         (after retrying) - RX was not actually armed in that case, since
 *         there's nothing loaded to decode against. Callers that care
 *         (currently just Receiver's on_enter) should surface this to the
 *         user rather than silently sitting in a no-protocols RX state.
 */
bool subghz_txrx_rx_start(SubGhzTxRx* instance);

/**
 * Stop TX/RX CC1101
 * 
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_stop(SubGhzTxRx* instance);

/**
 * Set sleep mode CC1101
 * 
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_sleep(SubGhzTxRx* instance);

/**
 * Update frequency CC1101 in automatic mode (hopper)
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param stay_threshold RSSI theshold over which to stay before hopping
 */
void subghz_txrx_hopper_update(SubGhzTxRx* instance, float stay_threshold);

/**
 * Get state hopper
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @return SubGhzHopperState 
 */
SubGhzHopperState subghz_txrx_hopper_get_state(SubGhzTxRx* instance);

/**
 * Set state hopper
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param state State hopper
 */
void subghz_txrx_hopper_set_state(SubGhzTxRx* instance, SubGhzHopperState state);

/**
 * Unpause hopper
 * 
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_hopper_unpause(SubGhzTxRx* instance);

/**
 * Set pause hopper
 * 
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_hopper_pause(SubGhzTxRx* instance);

/**
 * Speaker on
 *
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_speaker_on(SubGhzTxRx* instance);

/**
 * Speaker off
 * 
 * @param instance Pointer to a SubGhzTxRx 
 */
void subghz_txrx_speaker_off(SubGhzTxRx* instance);

/**
 * Speaker mute
 * 
 * @param instance Pointer to a SubGhzTxRx 
 */
void subghz_txrx_speaker_mute(SubGhzTxRx* instance);

/**
 * Speaker unmute
 * 
 * @param instance Pointer to a SubGhzTxRx 
 */
void subghz_txrx_speaker_unmute(SubGhzTxRx* instance);

/**
 * Set state speaker
 * 
 * @param instance Pointer to a SubGhzTxRx 
 * @param state State speaker
 */
void subghz_txrx_speaker_set_state(SubGhzTxRx* instance, SubGhzSpeakerState state);

/**
 * Get state speaker
 * 
 * @param instance Pointer to a SubGhzTxRx 
 * @return SubGhzSpeakerState 
 */
SubGhzSpeakerState subghz_txrx_speaker_get_state(SubGhzTxRx* instance);

/**
 * load decoder by name protocol
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param name_protocol Name protocol
 * @return bool True if the decoder is loaded 
 */
bool subghz_txrx_load_decoder_by_name_protocol(SubGhzTxRx* instance, const char* name_protocol);

/**
 * Get decoder
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @return SubGhzProtocolDecoderBase* Pointer to a SubGhzProtocolDecoderBase
 */
SubGhzProtocolDecoderBase* subghz_txrx_get_decoder(SubGhzTxRx* instance);

/**
 * Set callback for save data
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param callback Callback for save data
 * @param context Context for callback
 */
void subghz_txrx_set_need_save_callback(
    SubGhzTxRx* instance,
    SubGhzTxRxNeedSaveCallback callback,
    void* context);

/**
 * Get pointer to a load data key
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @return FlipperFormat* 
 */
FlipperFormat* subghz_txrx_get_fff_data(SubGhzTxRx* instance);

/**
 * Get pointer to a SugGhzSetting
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @return SubGhzSetting* 
 */
SubGhzSetting* subghz_txrx_get_setting(SubGhzTxRx* instance);

/**
 * Is it possible to save this protocol
 * 
 * @param instance Pointer to a SubGhzTxRx 
 * @return bool True if it is possible to save this protocol
 */
bool subghz_txrx_protocol_is_serializable(SubGhzTxRx* instance);

/**
 * Is it possible to send this protocol
 * 
 * @param instance Pointer to a SubGhzTxRx 
 * @return bool True if it is possible to send this protocol
 */
bool subghz_txrx_protocol_is_transmittable(SubGhzTxRx* instance, bool check_type);

/**
 * Set filter, what types of decoder to use 
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param filter Filter
 */
void subghz_txrx_receiver_set_filter(SubGhzTxRx* instance, SubGhzProtocolFlag filter);

/**
 * Set callback for receive data
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param callback Callback for receive data
 * @param context Context for callback
 */
void subghz_txrx_set_rx_callback(
    SubGhzTxRx* instance,
    SubGhzReceiverCallback callback,
    void* context);

/**
 * Set callback for Raw decoder, end of data transfer  
 * 
 * @param instance Pointer to a SubGhzTxRx
 * @param callback Callback for Raw decoder, end of data transfer 
 * @param context Context for callback
 */
void subghz_txrx_set_raw_file_encoder_worker_callback_end(
    SubGhzTxRx* instance,
    SubGhzProtocolEncoderRAWCallbackEnd callback,
    void* context);

/* Checking if an external radio device is connected
* 
* @param instance Pointer to a SubGhzTxRx
* @param name Name of external radio device
* @return bool True if is connected to the external radio device
*/
bool subghz_txrx_radio_device_is_external_connected(SubGhzTxRx* instance, const char* name);

/* Set the selected radio device to use
*
* @param instance Pointer to a SubGhzTxRx
* @param radio_device_type Radio device type
* @return SubGhzRadioDeviceType Type of installed radio device
*/
SubGhzRadioDeviceType
    subghz_txrx_radio_device_set(SubGhzTxRx* instance, SubGhzRadioDeviceType radio_device_type);

/* Get the selected radio device to use
*
* @param instance Pointer to a SubGhzTxRx
* @return SubGhzRadioDeviceType Type of installed radio device
*/
SubGhzRadioDeviceType subghz_txrx_radio_device_get(SubGhzTxRx* instance);

/* Get RSSI the selected radio device to use
*
* @param instance Pointer to a SubGhzTxRx
* @return float RSSI
*/
float subghz_txrx_radio_device_get_rssi(SubGhzTxRx* instance);

/* Get name the selected radio device to use
*
* @param instance Pointer to a SubGhzTxRx
* @return const char* Name of installed radio device
*/
const char* subghz_txrx_radio_device_get_name(SubGhzTxRx* instance);

/* Get intelligence whether frequency the selected radio device to use
*
* @param instance Pointer to a SubGhzTxRx
* @return bool True if the frequency is valid
*/
bool subghz_txrx_radio_device_is_frequency_valid(SubGhzTxRx* instance, uint32_t frequency);

bool subghz_txrx_radio_device_is_tx_allowed(SubGhzTxRx* instance, uint32_t frequency);

void subghz_txrx_set_debug_pin_state(SubGhzTxRx* instance, bool state);
bool subghz_txrx_get_debug_pin_state(SubGhzTxRx* instance);

void subghz_txrx_reset_dynamic_and_custom_btns(SubGhzTxRx* instance);

SubGhzReceiver* subghz_txrx_get_receiver(SubGhzTxRx* instance); // TODO use only in DecodeRaw

/**
 * Make sure the currently-selected protocol group's plugin is loaded
 * (loading it on first use if necessary) and return its descriptor. Safe
 * to call repeatedly - a no-op once already loaded. See
 * subghz_txrx_set_protocol_group() to change which group is active.
 *
 * @param instance Pointer to a SubGhzTxRx
 * @return const SubGhzGarageProtocolPlugin* Plugin descriptor, or NULL if
 *         the plugin failed to load
 */
const SubGhzGarageProtocolPlugin* subghz_txrx_ensure_protocol_plugin(SubGhzTxRx* instance);

/**
 * Make sure the ~27KB KeeLoq manufacturer-code keystore is parsed into
 * RAM. Only 4 protocols (FAAC SLH, Beninca ARC, Jarolift, KingGates
 * Stylo 4K) ever read it, and only for "Add Manually" TX generation or
 * enriching a captured signal's info-screen text - not for plain RX
 * decode. Call this explicitly right before one of those two actions,
 * not as part of starting RX. No-op once already loaded.
 *
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_ensure_keystore(SubGhzTxRx* instance);

/**
 * Make sure a specific protocol group's plugin is loaded, swapping out
 * whatever group is currently loaded if it's a different one. Used by
 * "Add Manually"/emulate (each protocol lives in exactly one group) as
 * well as by subghz_txrx_set_protocol_group().
 *
 * @param instance Pointer to a SubGhzTxRx
 * @param group Which protocol group to load
 * @return const SubGhzGarageProtocolPlugin* Plugin descriptor, or NULL if
 *         the plugin failed to load
 */
const SubGhzGarageProtocolPlugin*
    subghz_txrx_ensure_protocol_group(SubGhzTxRx* instance, SubGhzGarageProtocolGroup group);

/**
 * Make sure a specific protocol's dedicated TX plugin is loaded, swapping
 * out whatever TX plugin is currently loaded if it's a different one.
 * Entirely separate from the RX group plugin loaded by
 * subghz_txrx_ensure_protocol_group() - only used by "Add Manually"/emulate
 * for the handful of protocols with a real dedicated encoder (Alutech
 * AT-4N, Somfy Telis, Jarolift - see SubGhzGarageTxProtocol).
 *
 * @param instance Pointer to a SubGhzTxRx
 * @param tx_protocol Which single-protocol TX plugin to load
 * @return const SubGhzGarageProtocolPlugin* Plugin descriptor, or NULL if
 *         the plugin failed to load
 */
const SubGhzGarageProtocolPlugin* subghz_txrx_ensure_tx_protocol_plugin(
    SubGhzTxRx* instance,
    SubGhzGarageTxProtocol tx_protocol);

/**
 * Point the environment's protocol registry back at the active RX group's
 * registry. Call this right after subghz_txrx_ensure_tx_protocol_plugin()
 * and whatever needed its single-protocol registry (encoder alloc, a
 * gen_ call) are done - see subghz_txrx_ensure_tx_protocol_plugin()'s doc
 * comment for why this matters.
 *
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_restore_rx_protocol_registry(SubGhzTxRx* instance);

/**
 * Select which protocol group is active for RX/listen. Takes effect the
 * next time the plugin is actually needed (lazy, same as the rest of this
 * loading mechanism) - doesn't force an immediate load.
 *
 * @param instance Pointer to a SubGhzTxRx
 * @param group Which protocol group to make active
 */
void subghz_txrx_set_protocol_group(SubGhzTxRx* instance, SubGhzGarageProtocolGroup group);

/**
 * @param instance Pointer to a SubGhzTxRx
 * @return SubGhzGarageProtocolGroup The currently-selected protocol group
 */
SubGhzGarageProtocolGroup subghz_txrx_get_protocol_group(SubGhzTxRx* instance);

/**
 * Clear the "gave up loading this group" latch (see
 * subghz_txrx_ensure_protocol_group()) so the next load attempt gets a
 * genuine retry instead of being short-circuited to an immediate failure.
 * Normally this only clears when the active group actually changes, which
 * is deliberate (avoids hammering an already-fragile heap on every
 * re-entry into a screen that keeps re-attempting the same group) - but a
 * caller that has already independently confirmed conditions have
 * improved (e.g. free heap has recovered past a safe threshold) can use
 * this to give a previously-failed group a fair second chance rather than
 * being stuck on a stale failure from whenever it first happened. Safe to
 * call even if the group never failed to load - a no-op in that case.
 *
 * @param instance Pointer to a SubGhzTxRx
 */
void subghz_txrx_reset_protocol_load_failed(SubGhzTxRx* instance);

/**
 * Ensure the currently-selected protocol group's plugin is loaded and
 * return its protocol registry (subghz_garage_empty_protocol_registry if
 * loading failed).
 *
 * @param instance Pointer to a SubGhzTxRx
 * @return const SubGhzProtocolRegistry*
 */
const SubGhzProtocolRegistry* subghz_txrx_get_protocol_registry(SubGhzTxRx* instance);

/**
 * @brief Set current preset AM650 without additional params
 * 
 * @param instance - instance Pointer to a SubGhzTxRx
 * @param frequency - frequency of preset, if pass 0 then taking default frequency 433.92MHz
 */
void subghz_txrx_set_default_preset(SubGhzTxRx* instance, uint32_t frequency);

/**
 * @brief Set current preset by index
 * 
 * @param instance  - instance Pointer to a SubGhzTxRx
 * @param frequency - frequency of new preset
 * @param index - index of preset taken from SubGhzSetting
 * @param tx_power - index of TX Power menu index option to use.
 * @return const char* -  name of preset
 */
const char* subghz_txrx_set_preset_internal(
    SubGhzTxRx* instance,
    uint32_t frequency,
    uint8_t index,
    uint8_t tx_power);
