//
// Copyright (c) 2020 dushin.net
// All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// References
// https://docs.espressif.com/projects/esp-idf/en/v4.4.4/api-reference/peripherals/adc.html
//

#include "atomvm_adc.h"

#include <context.h>
#include <defaultatoms.h>
#include <interop.h>
#include <nifs.h>
#include <term.h>

// #define ENABLE_TRACE
#include <trace.h>

#include <driver/adc.h>
#include <esp32_sys.h>
#include <esp_adc_cal.h>
#include <esp_log.h>
#include <sdkconfig.h>

#include <stdlib.h>

#define TAG "atomvm_adc"
#define DEFAULT_SAMPLES 64
#define DEFAULT_VREF 1100


static const AtomStringIntPair bit_width_table[] = {
    { ATOM_STR("\x7", "bit_max"), (ADC_WIDTH_BIT_DEFAULT) },
#if SOC_ADC_MAX_BITWIDTH == 13
    { ATOM_STR("\x6", "bit_13"), ADC_WIDTH_BIT_13 },
#elif SOC_ADC_MAX_BITWIDTH == 12
    { ATOM_STR("\x6", "bit_12"), ADC_WIDTH_BIT_12 },
#elif CONFIG_IDF_TARGET_ESP32
    { ATOM_STR("\x6", "bit_11"), ADC_WIDTH_BIT_11 },
    { ATOM_STR("\x6", "bit_10"), ADC_WIDTH_BIT_10 },
    { ATOM_STR("\x5", "bit_9"), ADC_WIDTH_BIT_9 },
#endif
    SELECT_INT_DEFAULT(ADC_WIDTH_MAX)
};

static const AtomStringIntPair attenuation_table[] = {
    { ATOM_STR("\x4", "db_0"), ADC_ATTEN_DB_0 },
    { ATOM_STR("\x6", "db_2_5"), ADC_ATTEN_DB_2_5 },
    { ATOM_STR("\x4", "db_6"), ADC_ATTEN_DB_6 },
    { ATOM_STR("\x6", "db_11"), ADC_ATTEN_DB_11 },
    SELECT_INT_DEFAULT(ADC_ATTEN_MAX)
};

static const char *const invalid_pin_atom   = ATOM_STR("\xb", "invalid_pin");
static const char *const invalid_width_atom = ATOM_STR("\xd", "invalid_width");
static const char *const invalid_db_atom    = ATOM_STR("\xa", "invalid_db");
#ifdef CONFIG_AVM_ADC2_ENABLE
static const char *const timeout_atom = ATOM_STR("\x7", "timeout");
#endif

static adc_unit_t adc_unit_from_pin(int pin_val)
{
    switch (pin_val) {
#if CONFIG_IDF_TARGET_ESP32
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
#elif CONFIG_IDF_TARGET_ESP32C3
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
#endif
            return ADC_UNIT_1;
#ifdef CONFIG_AVM_ADC2_ENABLE
#if CONFIG_IDF_TARGET_ESP32
        case 0:
        case 2:
        case 4:
        case 12:
        case 13:
        case 14:
        case 15:
        case 25:
        case 26:
        case 27:
#elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
#elif CONFIG_IDF_TARGET_ESP32C3
        case 5:
#endif
            return ADC_UNIT_2;
#endif
        default:
            return ADC_UNIT_MAX;
    }
}

static adc_channel_t get_channel(avm_int_t pin_val)
{
    switch (pin_val) {
#if CONFIG_IDF_TARGET_ESP32
        case 32:
            return ADC1_CHANNEL_4;
        case 33:
            return ADC1_CHANNEL_5;
        case 34:
            return ADC1_CHANNEL_6;
        case 35:
            return ADC1_CHANNEL_7;
        case 36:
            return ADC1_CHANNEL_0;
        case 37:
            return ADC1_CHANNEL_1;
        case 38:
            return ADC1_CHANNEL_2;
        case 39:
            return ADC1_CHANNEL_3;
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
        case 1:
            return ADC1_CHANNEL_0;
        case 2:
            return ADC1_CHANNEL_1;
        case 3:
            return ADC1_CHANNEL_2;
        case 4:
            return ADC1_CHANNEL_3;
        case 5:
            return ADC1_CHANNEL_4;
        case 6:
            return ADC1_CHANNEL_5;
        case 7:
            return ADC1_CHANNEL_6;
        case 8:
            return ADC1_CHANNEL_7;
        case 9:
            return ADC1_CHANNEL_8;
        case 10:
            return ADC1_CHANNEL_9;
#elif CONFIG_IDF_TARGET_ESP32C3
        case 0:
            return ADC1_CHANNEL_0;
        case 1:
            return ADC1_CHANNEL_1;
        case 2:
            return ADC1_CHANNEL_2;
        case 3:
            return ADC1_CHANNEL_3;
        case 4:
            return ADC1_CHANNEL_4;
#endif
#ifdef CONFIG_AVM_ADC2_ENABLE
#if CONFIG_IDF_TARGET_ESP32
        case 0:
            return ADC2_CHANNEL_1;
        case 2:
            return ADC2_CHANNEL_2;
        case 4:
            return ADC2_CHANNEL_0;
        case 12:
            return ADC2_CHANNEL_5;
        case 13:
            return ADC2_CHANNEL_4;
        case 14:
            return ADC2_CHANNEL_6;
        case 15:
            return ADC2_CHANNEL_3;
        case 25:
            return ADC2_CHANNEL_8;
        case 26:
            return ADC2_CHANNEL_9;
        case 27:
            return ADC2_CHANNEL_7;
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
        case 11:
            return ADC2_CHANNEL_0;
        case 12:
            return ADC2_CHANNEL_1;
        case 13:
            return ADC2_CHANNEL_2;
        case 14:
            return ADC2_CHANNEL_3;
        case 15:
            return ADC2_CHANNEL_4;
        case 16:
            return ADC2_CHANNEL_5;
        case 17:
            return ADC2_CHANNEL_6;
        case 18:
            return ADC2_CHANNEL_7;
        case 19:
            return ADC2_CHANNEL_8;
        case 20:
            return ADC2_CHANNEL_9;
#elif CONFIG_IDF_TARGET_ESP32C3
        case 5:
            return ADC2_CHANNEL_0;
#endif
#endif
        default:
            return ADC_CHANNEL_MAX;
    }
}

static term create_pair(Context *ctx, term term1, term term2)
{
    term ret = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(ret, 0, term1);
    term_put_tuple_element(ret, 1, term2);

    return ret;
}

static void log_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        TRACE("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        TRACE("Characterized using eFuse Vref\n");
    } else {
        TRACE("Characterized using Default Vref\n");
    }
}

static term nif_adc_config_width(Context *ctx, int argc, term argv[])
{
    UNUSED(argc);

    term pin = argv[0];
    VALIDATE_VALUE(pin, term_is_integer);
    adc_unit_t adc_unit = adc_unit_from_pin(term_to_int(pin));
    if (UNLIKELY(adc_unit == ADC_UNIT_MAX)) {
#ifdef ENABLE_TRACE
        int Pin = term_to_int(pin);
#endif
        TRACE("Pin %i is not a valid adc pin.\n", Pin);
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_pin_atom));
        }
    }

    term width = argv[1];
    VALIDATE_VALUE(width, term_is_atom);
    adc_bits_width_t bit_width = interop_atom_term_select_int(bit_width_table, width, ctx->global);
    if (UNLIKELY(bit_width == ADC_WIDTH_MAX)) {
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_width_atom));
        }
    }

    if (adc_unit == ADC_UNIT_1) {
        esp_err_t err = adc1_config_width(bit_width);
        if (err != ESP_OK) {
            if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
                RAISE_ERROR(OUT_OF_MEMORY_ATOM);
            } else {
                return create_pair(ctx, ERROR_ATOM, term_from_int(err));
            }
        }
        TRACE("Width set to %u\n", bit_width);
    }
#ifdef CONFIG_AVM_ADC2_ENABLE
    else {
        TRACE("ADC2 read option bit_width set to %u\n", bit_width);
    }
#endif

    return OK_ATOM;
}

static term nif_adc_config_channel_attenuation(Context *ctx, int argc, term argv[])
{
    UNUSED(argc);

    term pin = argv[0];
    VALIDATE_VALUE(pin, term_is_integer);
    adc_channel_t channel = get_channel(term_to_int(pin));
    if (UNLIKELY(channel == ADC_CHANNEL_MAX)) {
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_pin_atom));
        }
    }

    term attenuation = argv[1];
    VALIDATE_VALUE(attenuation, term_is_atom);
    adc_atten_t atten = interop_atom_term_select_int(attenuation_table, attenuation, ctx->global);
    if (UNLIKELY(atten == ADC_ATTEN_MAX)) {
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_db_atom));
        }
    }

    adc_unit_t adc_unit = adc_unit_from_pin(term_to_int(pin));

    if (adc_unit == ADC_UNIT_1) {
        esp_err_t err = adc1_config_channel_atten((adc1_channel_t) channel, atten);
        if (UNLIKELY(err != ESP_OK)) {
            if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
                RAISE_ERROR(OUT_OF_MEMORY_ATOM);
            } else {
                return create_pair(ctx, ERROR_ATOM, term_from_int(err));
            }
        }
    }
#ifdef CONFIG_AVM_ADC2_ENABLE
    else if (adc_unit == ADC_UNIT_2) {
        esp_err_t err = adc2_config_channel_atten((adc2_channel_t) channel, atten);
        if (UNLIKELY(err != ESP_OK)) {
            if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
                RAISE_ERROR(OUT_OF_MEMORY_ATOM);
            } else {
                return create_pair(ctx, ERROR_ATOM, term_from_int(err));
            }
        }
    }
#endif

    TRACE("Attenuation on channel %u set to %u\n", channel, atten);
    return OK_ATOM;
}

static term nif_adc_take_reading(Context *ctx, int argc, term argv[])
{
    UNUSED(argc);

    term pin = argv[0];
    VALIDATE_VALUE(pin, term_is_integer);
    adc_channel_t channel = get_channel(term_to_int(pin));
    TRACE("take_reading channel: %u\n", channel);
    if (UNLIKELY(channel == ADC_CHANNEL_MAX)) {
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_pin_atom));
        }
    }

    term read_options = argv[1];
    VALIDATE_VALUE(read_options, term_is_list);
    term samples = interop_kv_get_value_default(read_options, ATOM_STR("\x7", "samples"), DEFAULT_SAMPLES, ctx->global);
    avm_int_t samples_val = term_to_int(samples);
    TRACE("take_reading samples: %i\n", samples_val);
    term raw = interop_kv_get_value_default(read_options, ATOM_STR("\x3", "raw"), FALSE_ATOM, ctx->global);
    term voltage = interop_kv_get_value_default(read_options, ATOM_STR("\x7", "voltage"), FALSE_ATOM, ctx->global);

    term width = argv[2];
    VALIDATE_VALUE(width, term_is_atom);
    adc_bits_width_t bit_width = interop_atom_term_select_int(bit_width_table, width, ctx->global);
    TRACE("take_reading bit width: %i\n", bit_width);
    if (UNLIKELY(bit_width == ADC_WIDTH_MAX)) {
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_width_atom));
        }
    }

    term attenuation = argv[3];
    VALIDATE_VALUE(attenuation, term_is_atom);
    adc_atten_t atten = interop_atom_term_select_int(attenuation_table, attenuation, ctx->global);
    TRACE("take_reading attenuation: %i\n", atten);
    if (UNLIKELY(atten == ADC_ATTEN_MAX)) {
        if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
            RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        } else {
            return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_db_atom));
        }
    }

    adc_unit_t adc_unit = adc_unit_from_pin(term_to_int(pin));

    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    if (UNLIKELY(IS_NULL_PTR(adc_chars))) {
        RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }

    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(adc_unit, atten, bit_width, DEFAULT_VREF, adc_chars);
    TRACE("take_reading calibration type: %i\n", val_type);
    log_char_val_type(val_type);

    uint32_t adc_reading = 0;
    if (adc_unit == ADC_UNIT_1) {
        // adc1_config_width() is used here in case the last adc1 pin to be configured was of a different width.
        // this will ensure the calibration characteristics and reading match the desired bit width for the channel.
        esp_err_t err = adc1_config_width(bit_width);
        if (UNLIKELY(err != ESP_OK)) {
            if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
                RAISE_ERROR(OUT_OF_MEMORY_ATOM);
            } else {
                return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, invalid_width_atom));
            }
        }
        for (avm_int_t i = 0; i < samples_val; ++i) {
            adc_reading += adc1_get_raw((adc1_channel_t) channel);
        }
    }
#ifdef CONFIG_AVM_ADC2_ENABLE
    else if (adc_unit == ADC_UNIT_2) {
        int read_raw;
        for (avm_int_t i = 0; i < samples_val; ++i) {
            esp_err_t r = adc2_get_raw((adc2_channel_t) channel, bit_width, &read_raw);
            if (UNLIKELY(r == ESP_ERR_TIMEOUT)) {
                ESP_LOGW(TAG, "ADC2 in use by Wi-Fi! Use adc:wifi_release/0 to stop wifi and free adc2 for reading.\n");
                if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
                    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
                } else {
                    return create_pair(ctx, ERROR_ATOM, globalcontext_make_atom(ctx->global, timeout_atom));
                }
            }
            adc_reading += read_raw;
        }
    }
#endif
    adc_reading /= samples_val;
    TRACE("take_reading adc_reading: %i\n", adc_reading);

    raw = raw == TRUE_ATOM ? term_from_int32(adc_reading) : UNDEFINED_ATOM;
    if (voltage == TRUE_ATOM) {
        voltage = term_from_int32(esp_adc_cal_raw_to_voltage(adc_reading, adc_chars));
    } else {
        voltage = UNDEFINED_ATOM;
    };

    free(adc_chars);

    if (UNLIKELY(memory_ensure_free(ctx, 3) != MEMORY_GC_OK)) {
        RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    } else {
        return create_pair(ctx, raw, voltage);
    }
}

static term nif_adc_pin_is_adc2(Context *ctx, int argc, term argv[])
{
    UNUSED(argc);
#ifdef CONFIG_AVM_ADC2_ENABLE
    term pin = argv[0];
    VALIDATE_VALUE(pin, term_is_integer);
    adc_unit_t Adc_Unit = adc_unit_from_pin(term_to_int(pin));
    switch (Adc_Unit) {
        case ADC_UNIT_2:
            return TRUE_ATOM;
        default:
            return FALSE_ATOM;
    }
#else
    UNUSED(argv);
    return FALSE_ATOM;
#endif
}

static const struct Nif adc_config_width_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = nif_adc_config_width
};
static const struct Nif adc_config_channel_attenuation_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = nif_adc_config_channel_attenuation
};
static const struct Nif adc_take_reading_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = nif_adc_take_reading
};
static const struct Nif adc_pin_is_adc2_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = nif_adc_pin_is_adc2
};

//
// Component Nif Entrypoints
//

void atomvm_adc_init(GlobalContext *global)
{
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
    } else {
        ESP_LOGI(TAG, "eFuse Two Point: NOT supported");
    }

    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported");
    } else {
        ESP_LOGI(TAG, "eFuse Vref: NOT supported");
    }
}

const struct Nif *atomvm_adc_get_nif(const char *nifname)
{
    TRACE("Locating nif %s ...", nifname);
    if (strcmp("adc:config_width/2", nifname) == 0) {
        TRACE("Resolved platform nif %s ...\n", nifname);
        return &adc_config_width_nif;
    }
    if (strcmp("adc:config_channel_attenuation/2", nifname) == 0) {
        TRACE("Resolved platform nif %s ...\n", nifname);
        return &adc_config_channel_attenuation_nif;
    }
    if (strcmp("adc:take_reading/4", nifname) == 0) {
        TRACE("Resolved platform nif %s ...\n", nifname);
        return &adc_take_reading_nif;
    }
    if (strcmp("adc:pin_is_adc2/1", nifname) == 0) {
        TRACE("Resolved platform nif %s ...\n", nifname);
        return &adc_pin_is_adc2_nif;
    }
    return NULL;
}

#include <sdkconfig.h>
#ifdef CONFIG_AVM_ADC_ENABLE
REGISTER_NIF_COLLECTION(atomvm_adc, atomvm_adc_init, NULL, atomvm_adc_get_nif)
#endif
