#pragma once

#include "types.h"
#include "protocols/base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SubGhzReceiver SubGhzReceiver;

typedef void (*SubGhzReceiverCallback)(
    SubGhzReceiver* decoder,
    SubGhzProtocolDecoderBase* decoder_base,
    void* context);

/**
 * Per-protocol enable check, consulted for every registered decoder on
 * every call to subghz_receiver_decode() (in addition to the coarser
 * flag-based filter set via subghz_receiver_set_filter()). Lets an app
 * implement an individual protocol ON/OFF list without SubGhzReceiver
 * needing to know anything about how that app stores its filter state.
 * @param context App-supplied context
 * @param registry_index This decoder's index in the SubGhzProtocolRegistry
 *        it was built from - stable for the lifetime of one receiver, cheap
 *        to use as an array index (unlike a name-based lookup).
 * @param protocol_name The protocol's name, e.g. "KeeLoq"
 * @return true if this protocol should still be fed data, false to skip it
 */
typedef bool (*SubGhzReceiverProtocolEnabledCallback)(
    void* context,
    size_t registry_index,
    const char* protocol_name);

/**
 * Allocate and init SubGhzReceiver.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzReceiver* pointer to a SubGhzReceiver instance
 */
SubGhzReceiver* subghz_receiver_alloc_init(SubGhzEnvironment* environment);

/**
 * Free SubGhzReceiver.
 * @param instance Pointer to a SubGhzReceiver instance
 */
void subghz_receiver_free(SubGhzReceiver* instance);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param instance Pointer to a SubGhzReceiver instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_receiver_decode(SubGhzReceiver* instance, bool level, uint32_t duration);

/**
 * Reset decoder SubGhzReceiver.
 * @param instance Pointer to a SubGhzReceiver instance
 */
void subghz_receiver_reset(SubGhzReceiver* instance);

/**
 * Set a callback upon completion of successful decoding of one of the protocols.
 * @param instance Pointer to a SubGhzReceiver instance
 * @param callback Callback, SubGhzReceiverCallback
 * @param context Context
 */
void subghz_receiver_set_rx_callback(
    SubGhzReceiver* instance,
    SubGhzReceiverCallback callback,
    void* context);

/**
 * Set the filter of receivers that will work at the moment.
 * @param instance Pointer to a SubGhzReceiver instance
 * @param filter Filter, SubGhzProtocolFlag
 */
void subghz_receiver_set_filter(SubGhzReceiver* instance, SubGhzProtocolFlag filter);

/**
 * Set (or clear, with NULL) a per-protocol enable callback. When set, it is
 * consulted before feeding data to each registered decoder, letting an app
 * turn individual protocols off without rebuilding the receiver/registry.
 * @param instance Pointer to a SubGhzReceiver instance
 * @param callback Callback, SubGhzReceiverProtocolEnabledCallback, or NULL
 * @param context Context passed to the callback
 */
void subghz_receiver_set_protocol_enabled_callback(
    SubGhzReceiver* instance,
    SubGhzReceiverProtocolEnabledCallback callback,
    void* context);

/**
 * Search for a cattery by his name.
 * @param instance Pointer to a SubGhzReceiver instance
 * @param decoder_name Receiver name
 * @return SubGhzProtocolDecoderBase* pointer to a SubGhzProtocolDecoderBase instance
 */
SubGhzProtocolDecoderBase*
    subghz_receiver_search_decoder_base_by_name(SubGhzReceiver* instance, const char* decoder_name);

#ifdef __cplusplus
}
#endif
