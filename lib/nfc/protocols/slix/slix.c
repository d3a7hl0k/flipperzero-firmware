#include "slix_i.h"
#include "slix_device_defs.h"

#include <furi.h>
#include <nfc/nfc_common.h>

#define SLIX_PROTOCOL_NAME "SLIX"
#define SLIX_DEVICE_NAME "SLIX"

#define SLIX_TYPE_SLIX_SLIX2 (0x01U)
#define SLIX_TYPE_SLIX_S (0x02U)
#define SLIX_TYPE_SLIX_L (0x03U)

#define SLIX_TYPE_INDICATOR_SLIX (0x02U)
#define SLIX_TYPE_INDICATOR_SLIX2 (0x01U)

#define SLIX_PASSWORD_READ_KEY "Password Read"
#define SLIX_PASSWORD_WRITE_KEY "Password Write"
#define SLIX_PASSWORD_PRIVACY_KEY "Password Privacy"
#define SLIX_PASSWORD_DESTROY_KEY "Password Destroy"
#define SLIX_PASSWORD_EAS_KEY "Password EAS"
#define SLIX_SIGNATURE_KEY "Signature"
#define SLIX_PRIVACY_MODE_KEY "Privacy Mode"
#define SLIX_PROTECTION_POINTER_KEY "Protection Pointer"
#define SLIX_PROTECTION_CONDITION_KEY "Protection Condition"

typedef struct {
    uint8_t iso15693_3[2];
    uint8_t icode_type;
    union {
        struct {
            uint8_t unused_1 : 3;
            uint8_t type_indicator : 2;
            uint8_t unused_2 : 3;
        };
        uint8_t serial_num[5];
    };
} SlixUidLayout;

const NfcDeviceBase nfc_device_slix = {
    .protocol_name = SLIX_PROTOCOL_NAME,
    .alloc = (NfcDeviceAlloc)slix_alloc,
    .free = (NfcDeviceFree)slix_free,
    .reset = (NfcDeviceReset)slix_reset,
    .copy = (NfcDeviceCopy)slix_copy,
    .verify = (NfcDeviceVerify)slix_verify,
    .load = (NfcDeviceLoad)slix_load,
    .save = (NfcDeviceSave)slix_save,
    .is_equal = (NfcDeviceEqual)slix_is_equal,
    .get_name = (NfcDeviceGetName)slix_get_device_name,
    .get_uid = (NfcDeviceGetUid)slix_get_uid,
    .set_uid = (NfcDeviceSetUid)slix_set_uid,
    .get_base_data = (NfcDeviceGetBaseData)slix_get_base_data,
};

static const char* slix_nfc_device_name[] = {
    [SlixTypeSlix] = SLIX_DEVICE_NAME,
    [SlixTypeSlixS] = SLIX_DEVICE_NAME "-S",
    [SlixTypeSlixL] = SLIX_DEVICE_NAME "-L",
    [SlixTypeSlix2] = SLIX_DEVICE_NAME "2",
};

SlixData* slix_alloc() {
    SlixData* data = malloc(sizeof(SlixData));

    data->iso15693_3_data = iso15693_3_alloc();

    return data;
}

void slix_free(SlixData* data) {
    furi_assert(data);

    iso15693_3_free(data->iso15693_3_data);

    free(data);
}

void slix_reset(SlixData* data) {
    furi_assert(data);

    iso15693_3_reset(data->iso15693_3_data);

    memset(&data->passwords, 0, sizeof(SlixPasswords));
    memset(&data->signature, 0, sizeof(SlixSignature));
    memset(&data->privacy_mode, 0, sizeof(SlixPrivacy));
    memset(&data->protection, 0, sizeof(SlixProtection));
}

void slix_copy(SlixData* data, const SlixData* other) {
    furi_assert(data);
    furi_assert(other);

    iso15693_3_copy(data->iso15693_3_data, other->iso15693_3_data);

    data->passwords = other->passwords;
    data->signature = other->signature;
    data->privacy_mode = other->privacy_mode;
    data->protection = other->protection;
}

bool slix_verify(SlixData* data, const FuriString* device_type) {
    UNUSED(data);
    UNUSED(device_type);
    // No backward compatibility, unified format only
    return false;
}

bool slix_load(SlixData* data, FlipperFormat* ff, uint32_t version) {
    furi_assert(data);

    bool loaded = false;

    do {
        if(!iso15693_3_load(data->iso15693_3_data, ff, version)) break;

        SlixPasswords* passwords = &data->passwords;

        passwords->read.is_present = flipper_format_key_exist(ff, SLIX_PASSWORD_READ_KEY);

        if(passwords->read.is_present) {
            if(!flipper_format_read_hex(
                   ff, SLIX_PASSWORD_READ_KEY, passwords->read.data, SLIX_PASSWORD_SIZE))
                break;
        }

        passwords->write.is_present = flipper_format_key_exist(ff, SLIX_PASSWORD_WRITE_KEY);

        if(passwords->write.is_present) {
            if(!flipper_format_read_hex(
                   ff, SLIX_PASSWORD_WRITE_KEY, passwords->write.data, SLIX_PASSWORD_SIZE))
                break;
        }

        passwords->privacy.is_present = flipper_format_key_exist(ff, SLIX_PASSWORD_PRIVACY_KEY);

        if(passwords->privacy.is_present) {
            if(!flipper_format_read_hex(
                   ff, SLIX_PASSWORD_PRIVACY_KEY, passwords->privacy.data, SLIX_PASSWORD_SIZE))
                break;
        }

        passwords->destroy.is_present = flipper_format_key_exist(ff, SLIX_PASSWORD_DESTROY_KEY);

        if(passwords->destroy.is_present) {
            if(!flipper_format_read_hex(
                   ff, SLIX_PASSWORD_DESTROY_KEY, passwords->destroy.data, SLIX_PASSWORD_SIZE))
                break;
        }

        passwords->eas.is_present = flipper_format_key_exist(ff, SLIX_PASSWORD_EAS_KEY);

        if(passwords->eas.is_present) {
            if(!flipper_format_read_hex(
                   ff, SLIX_PASSWORD_EAS_KEY, passwords->eas.data, SLIX_PASSWORD_SIZE))
                break;
        }

        SlixSignature* signature = &data->signature;
        signature->is_present = flipper_format_key_exist(ff, SLIX_SIGNATURE_KEY);

        if(signature->is_present) {
            if(!flipper_format_read_hex(
                   ff, SLIX_SIGNATURE_KEY, signature->data, SLIX_SIGNATURE_SIZE))
                break;
        }

        SlixPrivacy* privacy_mode = &data->privacy_mode;
        privacy_mode->is_present = flipper_format_key_exist(ff, SLIX_PRIVACY_MODE_KEY);

        if(privacy_mode->is_present) {
            if(!flipper_format_read_bool(ff, SLIX_PRIVACY_MODE_KEY, &privacy_mode->mode, 1)) break;
        }

        SlixProtection* protection = &data->protection;
        protection->is_present = flipper_format_key_exist(ff, SLIX_PROTECTION_POINTER_KEY) &&
                                 flipper_format_key_exist(ff, SLIX_PROTECTION_CONDITION_KEY);

        if(protection->is_present) {
            if(!flipper_format_read_hex(ff, SLIX_PROTECTION_POINTER_KEY, &protection->pointer, 1))
                break;
            if(!flipper_format_read_hex(
                   ff, SLIX_PROTECTION_CONDITION_KEY, &protection->condition, 1))
                break;
        }

        loaded = true;
    } while(false);

    return loaded;
}

bool slix_save(const SlixData* data, FlipperFormat* ff) {
    furi_assert(data);

    bool saved = false;

    do {
        if(!iso15693_3_save(data->iso15693_3_data, ff)) break;
        if(!flipper_format_write_comment_cstr(ff, SLIX_PROTOCOL_NAME " specific data")) break;

        if(!flipper_format_write_comment_cstr(
               ff, "Passwords are optional. If a password is omitted, any password is accepted"))
            break;

        const SlixPasswords* passwords = &data->passwords;

        if(passwords->read.is_present) {
            if(!flipper_format_write_hex(
                   ff, SLIX_PASSWORD_READ_KEY, passwords->read.data, SLIX_PASSWORD_SIZE))
                break;
        }
        if(passwords->write.is_present) {
            if(!flipper_format_write_hex(
                   ff, SLIX_PASSWORD_WRITE_KEY, passwords->write.data, SLIX_PASSWORD_SIZE))
                break;
        }
        if(passwords->privacy.is_present) {
            if(!flipper_format_write_hex(
                   ff, SLIX_PASSWORD_PRIVACY_KEY, passwords->privacy.data, SLIX_PASSWORD_SIZE))
                break;
        }
        if(passwords->destroy.is_present) {
            if(!flipper_format_write_hex(
                   ff, SLIX_PASSWORD_DESTROY_KEY, passwords->destroy.data, SLIX_PASSWORD_SIZE))
                break;
        }
        if(passwords->eas.is_present) {
            if(!flipper_format_write_hex(
                   ff, SLIX_PASSWORD_EAS_KEY, passwords->eas.data, SLIX_PASSWORD_SIZE))
                break;
        }

        const SlixSignature* signature = &data->signature;

        if(signature->is_present) {
            if(!flipper_format_write_comment_cstr(
                   ff,
                   "This is the card's secp128r1 elliptic curve signature. It can not be calculated without knowing NXP's private key."))
                break;
            if(!flipper_format_write_hex(
                   ff, SLIX_SIGNATURE_KEY, signature->data, SLIX_SIGNATURE_SIZE))
                break;
        }

        const SlixPrivacy* privacy_mode = &data->privacy_mode;

        if(privacy_mode->is_present) {
            if(!flipper_format_write_bool(ff, SLIX_PRIVACY_MODE_KEY, &privacy_mode->mode, 1))
                break;
        }

        const SlixProtection* protection = &data->protection;

        if(protection->is_present) {
            if(!flipper_format_write_comment_cstr(ff, "Protection pointer configuration")) break;
            if(!flipper_format_write_hex(
                   ff, SLIX_PROTECTION_POINTER_KEY, &protection->pointer, sizeof(uint8_t)))
                break;
            if(!flipper_format_write_hex(
                   ff, SLIX_PROTECTION_CONDITION_KEY, &protection->condition, sizeof(uint8_t)))
                break;
        }

        saved = true;
    } while(false);

    return saved;
}

bool slix_is_equal(const SlixData* data, const SlixData* other) {
    return iso15693_3_is_equal(data->iso15693_3_data, other->iso15693_3_data) &&
           memcmp(&data->passwords, &other->passwords, sizeof(SlixPasswords)) == 0 &&
           memcmp(&data->signature, &other->signature, sizeof(SlixSignature)) == 0 &&
           memcmp(&data->privacy_mode, &other->privacy_mode, sizeof(SlixPrivacy)) == 0 &&
           memcmp(&data->protection, &other->protection, sizeof(SlixProtection)) == 0;
}

const char* slix_get_device_name(const SlixData* data, NfcDeviceNameType name_type) {
    UNUSED(name_type);

    const SlixType slix_type = slix_get_type(data);
    furi_assert(slix_type < SlixTypeNum);

    return slix_nfc_device_name[slix_type];
}

const uint8_t* slix_get_uid(const SlixData* data, size_t* uid_len) {
    return iso15693_3_get_uid(data->iso15693_3_data, uid_len);
}

bool slix_set_uid(SlixData* data, const uint8_t* uid, size_t uid_len) {
    furi_assert(data);

    return iso15693_3_set_uid(data->iso15693_3_data, uid, uid_len);
}

const Iso15693_3Data* slix_get_base_data(const SlixData* data) {
    furi_assert(data);

    return data->iso15693_3_data;
}

SlixType slix_get_type(const SlixData* data) {
    SlixType type = SlixTypeNum;

    do {
        if(iso15693_3_get_manufacturer_id(data->iso15693_3_data) != SLIX_NXP_MANUFACTURER_CODE)
            break;

        const SlixUidLayout* uid = (const SlixUidLayout*)data->iso15693_3_data->uid;

        if(uid->icode_type == SLIX_TYPE_SLIX_SLIX2) {
            if(uid->type_indicator == SLIX_TYPE_INDICATOR_SLIX) {
                type = SlixTypeSlix;
            } else if(uid->type_indicator == SLIX_TYPE_INDICATOR_SLIX2) {
                type = SlixTypeSlix2;
            }
        } else if(uid->icode_type == SLIX_TYPE_SLIX_S) {
            type = SlixTypeSlixS;
        } else if(uid->icode_type == SLIX_TYPE_SLIX_L) {
            type = SlixTypeSlixL;
        }

    } while(false);

    return type;
}