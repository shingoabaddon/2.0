#include "subghz_last_settings.h"
#include "subghz_i.h"
#include "helpers/subghz_tx_allowed_compat.h"
#include <float_tools.h>

#define TAG "SubGhzGarageLastSettings"

#define SUBGHZ_LAST_SETTING_FILE_TYPE    "Flipper SubGhz Last Setting File"
#define SUBGHZ_LAST_SETTING_FILE_VERSION 3
#define SUBGHZ_LAST_SETTINGS_PATH        EXT_PATH("subghz/assets/last_subghz.settings")

#define SUBGHZ_LAST_SETTING_FIELD_FREQUENCY                         "Frequency"
#define SUBGHZ_LAST_SETTING_FIELD_PRESET                            "Preset" // AKA Modulation
#define SUBGHZ_LAST_SETTING_FIELD_FREQUENCY_ANALYZER_FEEDBACK_LEVEL "FeedbackLevel"
#define SUBGHZ_LAST_SETTING_FIELD_FREQUENCY_ANALYZER_TRIGGER        "FATrigger"
#define SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_FILE_NAMES               "ProtocolNames"
#define SUBGHZ_LAST_SETTING_FIELD_HOPPING_ENABLE                    "Hopping"
#define SUBGHZ_LAST_SETTING_FIELD_FILTER                            "Filter"
#define SUBGHZ_LAST_SETTING_FIELD_RSSI_THRESHOLD                    "RSSI"
#define SUBGHZ_LAST_SETTING_FIELD_HOPPING_THRESHOLD                 "HoppingThreshold"
#define SUBGHZ_LAST_SETTING_FIELD_LED_AND_POWER_AMP                 "LedAndPowerAmp"
#define SUBGHZ_LAST_SETTING_FIELD_TX_POWER                          "TXPower"
#define SUBGHZ_LAST_SETTING_FIELD_VISUALIZER_MODE        "VizMode"
#define SUBGHZ_LAST_SETTING_FIELD_RAW_ZOOM_LEVEL      "RawZoom"
#define SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_FILTER      "ProtoFilter"
#define SUBGHZ_LAST_SETTING_FIELD_MOD_FILTER           "ModFilter"
#define SUBGHZ_LAST_SETTING_FIELD_BYPASS_REGION_LOCK   "BypassRegionLock"
#define SUBGHZ_LAST_SETTING_FIELD_FILE_PREFIX          "FilePrefix"
#define SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_GROUP       "ProtocolGroup"

SubGhzGarageLastSettings* subghz_garage_last_settings_alloc(void) {
    SubGhzGarageLastSettings* instance = malloc(sizeof(SubGhzGarageLastSettings));
    return instance;
}

void subghz_garage_last_settings_free(SubGhzGarageLastSettings* instance) {
    furi_assert(instance);
    free(instance);
}

void subghz_garage_last_settings_load(SubGhzGarageLastSettings* instance, size_t preset_count) {
    UNUSED(preset_count);
    furi_assert(instance);

    // Default values (all others set to 0, if read from file fails these are used)
    instance->frequency = SUBGHZ_LAST_SETTING_DEFAULT_FREQUENCY;
    instance->preset_index = SUBGHZ_LAST_SETTING_DEFAULT_PRESET;
    instance->frequency_analyzer_feedback_level =
        SUBGHZ_LAST_SETTING_FREQUENCY_ANALYZER_FEEDBACK_LEVEL;
    instance->frequency_analyzer_trigger = SUBGHZ_LAST_SETTING_FREQUENCY_ANALYZER_TRIGGER;
    // See bin_raw_value in scenes/subghz_scene_receiver_config.c
    instance->filter = SubGhzProtocolFlag_Decodable;
    instance->rssi = -65.0f;
    instance->hopping_threshold = -90.0f;
    instance->leds_and_amp = true;
    instance->visualizer_display_mode = SUBGHZ_LAST_SETTING_DEFAULT_VISUALIZER_MODE;
    instance->raw_playback_zoom_level = SUBGHZ_LAST_SETTING_DEFAULT_RAW_ZOOM_LEVEL;
    instance->bypass_region_lock = false;
    instance->file_prefix[0] = '\0';
    instance->protocol_group = SubGhzGarageProtocolGroup1;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff_data_file = flipper_format_file_alloc(storage);

    FuriString* temp_str = furi_string_alloc();
    uint32_t config_version = 0;

    if(FSE_OK == storage_sd_status(storage) &&
       flipper_format_file_open_existing(fff_data_file, SUBGHZ_LAST_SETTINGS_PATH)) {
        do {
            if(!flipper_format_read_header(fff_data_file, temp_str, &config_version)) break;
            if((strcmp(furi_string_get_cstr(temp_str), SUBGHZ_LAST_SETTING_FILE_TYPE) != 0) ||
               (config_version != SUBGHZ_LAST_SETTING_FILE_VERSION)) {
                break;
            }

            if(!flipper_format_read_uint32(
                   fff_data_file, SUBGHZ_LAST_SETTING_FIELD_FREQUENCY, &instance->frequency, 1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_uint32(
                   fff_data_file, SUBGHZ_LAST_SETTING_FIELD_PRESET, &instance->preset_index, 1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_uint32(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_FREQUENCY_ANALYZER_FEEDBACK_LEVEL,
                   &instance->frequency_analyzer_feedback_level,
                   1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_float(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_FREQUENCY_ANALYZER_TRIGGER,
                   &instance->frequency_analyzer_trigger,
                   1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_bool(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_FILE_NAMES,
                   &instance->protocol_file_names,
                   1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_bool(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_HOPPING_ENABLE,
                   &instance->enable_hopping,
                   1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_uint32(
                   fff_data_file, SUBGHZ_LAST_SETTING_FIELD_FILTER, &instance->filter, 1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_float(
                   fff_data_file, SUBGHZ_LAST_SETTING_FIELD_RSSI_THRESHOLD, &instance->rssi, 1)) {
                flipper_format_rewind(fff_data_file);
            }
            uint32_t tx_power = 0;
            if(!flipper_format_read_uint32(
                   fff_data_file, SUBGHZ_LAST_SETTING_FIELD_TX_POWER, &tx_power, 1)) {
                flipper_format_rewind(fff_data_file);
            }
            instance->tx_power = (uint8_t)(tx_power & 0xFF);
            if(!flipper_format_read_uint32(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_VISUALIZER_MODE,
                   &instance->visualizer_display_mode,
                   1)) {
                instance->visualizer_display_mode =
                    SUBGHZ_LAST_SETTING_DEFAULT_VISUALIZER_MODE;
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_uint32(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_RAW_ZOOM_LEVEL,
                   &instance->raw_playback_zoom_level,
                   1)) {
                instance->raw_playback_zoom_level =
                    SUBGHZ_LAST_SETTING_DEFAULT_RAW_ZOOM_LEVEL;
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_float(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_HOPPING_THRESHOLD,
                   &instance->hopping_threshold,
                   1)) {
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_bool(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_LED_AND_POWER_AMP,
                   &instance->leds_and_amp,
                   1)) {
                flipper_format_rewind(fff_data_file);
            }
            /* Load filter arrays — silently skip if key is missing (older file) */
            instance->protocol_filter_present = flipper_format_read_hex(
                fff_data_file, SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_FILTER,
                instance->protocol_filter_data, sizeof(instance->protocol_filter_data));
            if(!instance->protocol_filter_present)
                flipper_format_rewind(fff_data_file);
            instance->mod_filter_present = flipper_format_read_hex(
                fff_data_file, SUBGHZ_LAST_SETTING_FIELD_MOD_FILTER,
                instance->mod_filter_data, sizeof(instance->mod_filter_data));
            if(!instance->mod_filter_present)
                flipper_format_rewind(fff_data_file);
            if(!flipper_format_read_bool(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_BYPASS_REGION_LOCK,
                   &instance->bypass_region_lock,
                   1)) {
                instance->bypass_region_lock = false;
                flipper_format_rewind(fff_data_file);
            }
            if(!flipper_format_read_uint32(
                   fff_data_file,
                   SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_GROUP,
                   &instance->protocol_group,
                   1)) {
                instance->protocol_group = SubGhzGarageProtocolGroup1;
                flipper_format_rewind(fff_data_file);
            }
            furi_string_reset(temp_str);
            if(flipper_format_read_string(
                   fff_data_file, SUBGHZ_LAST_SETTING_FIELD_FILE_PREFIX, temp_str)) {
                strncpy(
                    instance->file_prefix,
                    furi_string_get_cstr(temp_str),
                    sizeof(instance->file_prefix) - 1);
                instance->file_prefix[sizeof(instance->file_prefix) - 1] = '\0';
            } else {
                instance->file_prefix[0] = '\0';
            }
            flipper_format_rewind(fff_data_file);

        } while(0);
    } else {
        FURI_LOG_E(TAG, "Error open file %s", SUBGHZ_LAST_SETTINGS_PATH);
    }

    /* One-time migration for existing SD cards: the settings file persists
     * across firmware reflashes independent of firmware version, so a file
     * saved before RSSI Threshold got a sensible default (see instance->rssi
     * above) still has the old disabled sentinel (SUBGHZ_RAW_THRESHOLD_MIN)
     * written into it, and the version-gated read above happily loads that
     * straight over the new default. Nobody has a real reason to
     * deliberately pick the dropdown's "-----" (disabled) option on
     * purpose - treat a saved value still sitting on the sentinel as
     * "never configured" and apply the new default instead of the old one. */
    if(float_is_equal(instance->rssi, SUBGHZ_RAW_THRESHOLD_MIN)) {
        instance->rssi = -65.0f;
    }

    furi_string_free(temp_str);

    flipper_format_file_close(fff_data_file);
    flipper_format_free(fff_data_file);
    furi_record_close(RECORD_STORAGE);

    if(instance->frequency == 0 || !subghz_garage_is_tx_allowed(instance->frequency)) {
        instance->frequency = SUBGHZ_LAST_SETTING_DEFAULT_FREQUENCY;
    }

    if(instance->preset_index > 4) {
        instance->preset_index = SUBGHZ_LAST_SETTING_DEFAULT_PRESET;
    }

    if(instance->protocol_group >= SUBGHZ_GARAGE_PROTOCOL_GROUP_COUNT) {
        instance->protocol_group = SubGhzGarageProtocolGroup1;
    }
}

bool subghz_garage_last_settings_save(SubGhzGarageLastSettings* instance) {
    furi_assert(instance);

    bool saved = false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* file = flipper_format_file_alloc(storage);

    do {
        if(FSE_OK != storage_sd_status(storage)) {
            break;
        }

        // Open file
        if(!flipper_format_file_open_always(file, SUBGHZ_LAST_SETTINGS_PATH)) break;

        // Write header
        if(!flipper_format_write_header_cstr(
               file, SUBGHZ_LAST_SETTING_FILE_TYPE, SUBGHZ_LAST_SETTING_FILE_VERSION))
            break;
        if(!flipper_format_write_uint32(
               file, SUBGHZ_LAST_SETTING_FIELD_FREQUENCY, &instance->frequency, 1)) {
            break;
        }
        if(!flipper_format_write_uint32(
               file, SUBGHZ_LAST_SETTING_FIELD_PRESET, &instance->preset_index, 1)) {
            break;
        }
        if(!flipper_format_write_uint32(
               file,
               SUBGHZ_LAST_SETTING_FIELD_FREQUENCY_ANALYZER_FEEDBACK_LEVEL,
               &instance->frequency_analyzer_feedback_level,
               1)) {
            break;
        }
        if(!flipper_format_write_float(
               file,
               SUBGHZ_LAST_SETTING_FIELD_FREQUENCY_ANALYZER_TRIGGER,
               &instance->frequency_analyzer_trigger,
               1)) {
            break;
        }
        if(!flipper_format_write_bool(
               file,
               SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_FILE_NAMES,
               &instance->protocol_file_names,
               1)) {
            break;
        }
        if(!flipper_format_write_bool(
               file, SUBGHZ_LAST_SETTING_FIELD_HOPPING_ENABLE, &instance->enable_hopping, 1)) {
            break;
        }
        if(!flipper_format_write_uint32(
               file, SUBGHZ_LAST_SETTING_FIELD_FILTER, &instance->filter, 1)) {
            break;
        }
        if(!flipper_format_write_float(
               file, SUBGHZ_LAST_SETTING_FIELD_RSSI_THRESHOLD, &instance->rssi, 1)) {
            break;
        }
        uint32_t tx_power = instance->tx_power;
        if(!flipper_format_write_uint32(file, SUBGHZ_LAST_SETTING_FIELD_TX_POWER, &tx_power, 1)) {
            break;
        }
        if(!flipper_format_write_uint32(
               file,
               SUBGHZ_LAST_SETTING_FIELD_VISUALIZER_MODE,
               &instance->visualizer_display_mode,
               1)) {
            break;
        }
        if(!flipper_format_write_uint32(
               file,
               SUBGHZ_LAST_SETTING_FIELD_RAW_ZOOM_LEVEL,
               &instance->raw_playback_zoom_level,
               1)) {
            break;
        }
        if(!flipper_format_write_float(
               file,
               SUBGHZ_LAST_SETTING_FIELD_HOPPING_THRESHOLD,
               &instance->hopping_threshold,
               1)) {
            break;
        }
        if(!flipper_format_write_bool(
               file, SUBGHZ_LAST_SETTING_FIELD_LED_AND_POWER_AMP, &instance->leds_and_amp, 1)) {
            break;
        }

        /* Save filter arrays when present */
        if(instance->protocol_filter_present) {
            flipper_format_write_hex(
                file, SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_FILTER,
                instance->protocol_filter_data,
                (uint16_t)sizeof(instance->protocol_filter_data));
        }
        if(instance->mod_filter_present) {
            flipper_format_write_hex(
                file, SUBGHZ_LAST_SETTING_FIELD_MOD_FILTER,
                instance->mod_filter_data,
                (uint16_t)sizeof(instance->mod_filter_data));
        }
        if(!flipper_format_write_bool(
               file,
               SUBGHZ_LAST_SETTING_FIELD_BYPASS_REGION_LOCK,
               &instance->bypass_region_lock,
               1)) {
            break;
        }
        if(!flipper_format_write_string_cstr(
               file, SUBGHZ_LAST_SETTING_FIELD_FILE_PREFIX, instance->file_prefix)) {
            break;
        }
        if(!flipper_format_write_uint32(
               file, SUBGHZ_LAST_SETTING_FIELD_PROTOCOL_GROUP, &instance->protocol_group, 1)) {
            break;
        }
        saved = true;
    } while(0);

    if(!saved) {
        FURI_LOG_E(TAG, "Error save file %s", SUBGHZ_LAST_SETTINGS_PATH);
    }

    flipper_format_file_close(file);
    flipper_format_free(file);
    furi_record_close(RECORD_STORAGE);

    return saved;
}

void subghz_garage_last_settings_set_protocol_filter(
    SubGhzGarageLastSettings* s, const uint8_t* data, size_t count) {
    size_t n = count < sizeof(s->protocol_filter_data) ? count : sizeof(s->protocol_filter_data);
    memcpy(s->protocol_filter_data, data, n);
    s->protocol_filter_present = true;
}

void subghz_garage_last_settings_set_mod_filter(
    SubGhzGarageLastSettings* s, const uint8_t* data, size_t count) {
    size_t n = count < sizeof(s->mod_filter_data) ? count : sizeof(s->mod_filter_data);
    memcpy(s->mod_filter_data, data, n);
    s->mod_filter_present = true;
}
