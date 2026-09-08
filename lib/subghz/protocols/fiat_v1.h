#pragma once

#include <furi.h>
#include <lib/subghz/protocols/base.h>
#include <lib/subghz/types.h>
#include <lib/subghz/blocks/const.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include <lib/subghz/blocks/generic.h>
#include <lib/subghz/blocks/math.h>
#include <flipper_format/flipper_format.h>

#define FIAT_V1_PROTOCOL_NAME "Fiat V1"
#define FIAT_V1_HITAG2_KEY_FIELD    "Hitag2 Key"
#define FIAT_V1_HITAG2_EPOCH_FIELD  "Hitag2 Epoch"

typedef struct SubGhzProtocolDecoderFiatV1 SubGhzProtocolDecoderFiatV1;
typedef struct SubGhzProtocolEncoderFiatV1 SubGhzProtocolEncoderFiatV1;

extern const SubGhzProtocol fiat_v1_protocol;

void* subghz_protocol_decoder_fiat_v1_alloc(SubGhzEnvironment* environment);
void subghz_protocol_decoder_fiat_v1_reset(void* context);
void subghz_protocol_decoder_fiat_v1_feed(void* context, bool level, uint32_t duration);
uint8_t subghz_protocol_decoder_fiat_v1_get_hash_data(void* context);
SubGhzProtocolStatus subghz_protocol_decoder_fiat_v1_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);
SubGhzProtocolStatus
    subghz_protocol_decoder_fiat_v1_deserialize(void* context, FlipperFormat* flipper_format);
void subghz_protocol_decoder_fiat_v1_get_string(void* context, FuriString* output);

/**
 * @brief Total number of (known key, epoch) combinations the Hitag2 key
 * recovery search covers.
 */
uint32_t fiat_v1_hitag2_recovery_total(void);

// progress_cb: called periodically with 0-100 progress and combos tested so
// far; return false to cancel the search early (can be NULL).
typedef bool (
    *FiatV1RecoverProgressCallback)(uint8_t progress, uint32_t combos_tested, void* context);

/**
 * @brief Run the full Hitag2 key recovery search (blocking).
 *
 * Searches every (known key, epoch) combination for one that reproduces the
 * captured authenticator. Intended to be run from a background thread
 * rather than the RF decode path, since a full search can take from several
 * seconds up to a few minutes.
 *
 * @param out_key filled with the recovered key on success
 * @param out_epoch filled with the recovered epoch on success
 * @return true if a match was found
 */
bool fiat_v1_hitag2_recover(
    uint32_t uid,
    uint8_t button,
    uint16_t control,
    uint32_t hop,
    uint8_t out_key[6],
    uint32_t* out_epoch,
    FiatV1RecoverProgressCallback progress_cb,
    void* progress_ctx);

void* subghz_protocol_encoder_fiat_v1_alloc(SubGhzEnvironment* environment);
void subghz_protocol_encoder_fiat_v1_free(void* context);
SubGhzProtocolStatus
    subghz_protocol_encoder_fiat_v1_deserialize(void* context, FlipperFormat* flipper_format);
void subghz_protocol_encoder_fiat_v1_stop(void* context);
LevelDuration subghz_protocol_encoder_fiat_v1_yield(void* context);
