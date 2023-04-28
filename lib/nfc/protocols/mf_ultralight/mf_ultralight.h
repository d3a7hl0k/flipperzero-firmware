#pragma once

#include <lib/nfc/protocols/nfca/nfca.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MF_ULTRALIGHT_CMD_GET_VERSION (0x60)
#define MF_ULTRALIGHT_CMD_READ_PAGE (0x30)
#define MF_ULTRALIGHT_CMD_WRITE_PAGE (0xA2)
#define MF_ULTRALIGTH_CMD_READ_SIG (0x3C)
#define MF_ULTRALIGHT_CMD_READ_CNT (0x39)
#define MF_ULTRALIGHT_CMD_CHECK_TEARING (0x3E)
#define MF_ULTRALIGHT_CMD_AUTH (0x1B)

#define MF_ULTRALIGHT_CMD_ACK (0x0A)
#define MF_ULTRALIGHT_CMD_NACK (0x00)

#define MF_ULTRALIGHT_MAX_PAGE_NUM (510)
#define MF_ULTRALIGHT_PAGE_SIZE (4U)
#define MF_ULTRALIGHT_SIGNATURE_SIZE (32)
#define MF_ULTRALIGHT_COUNTER_SIZE (3)
#define MF_ULTRALIGHT_COUNTER_NUM (3)
#define MF_ULTRALIGHT_TEARING_FLAG_SIZE (1)
#define MF_ULTRALIGHT_TEARING_FLAG_NUM (3)
#define MF_ULTRALIGHT_AUTH_PASSWORD_SIZE (4)
#define MF_ULTRALIGHT_AUTH_PACK_SIZE (2)

typedef enum {
    MfUltralightErrorNone,
    MfUltralightErrorNotPresent,
    MfUltralightErrorProtocol,
    MfUltralightErrorAuth,
    MfUltralightErrorTimeout,
} MfUltralightError;

typedef enum {
    MfUltralightTypeUnknown,
    MfUltralightTypeNTAG203,
    MfUltralightTypeUL11,
    MfUltralightTypeUL21,
    MfUltralightTypeNTAG213,
    MfUltralightTypeNTAG215,
    MfUltralightTypeNTAG216,
    MfUltralightTypeNTAGI2C1K,
    MfUltralightTypeNTAGI2C2K,
    MfUltralightTypeNTAGI2CPlus1K,
    MfUltralightTypeNTAGI2CPlus2K,

    MfUltralightTypeNum,
} MfUltralightType;

typedef enum {
    MfUltralightFeatureSupportReadVersion = (1U << 0),
    MfUltralightFeatureSupportReadSignature = (1U << 1),
    MfUltralightFeatureSupportReadCounter = (1U << 2),
    MfUltralightFeatureSupportCheckTearingFlag = (1U << 3),
    MfUltralightFeatureSupportFastRead = (1U << 4),
    MfUltralightFeatureSupportIncCounter = (1U << 5),
    MfUltralightFeatureSupportFastWrite = (1U << 6),
    MfUltralightFeatureSupportCompatibleWrite = (1U << 7),
    MfUltralightFeatureSupportAuthentication = (1U << 8),
    MfUltralightFeatureSupportVcsl = (1U << 9),
    MfUltralightFeatureSupportSectorSelect = (1U << 10),
    MfUltralightFeatureSupportSingleCounter = (1U << 11),
    MfUltralightFeatureSupportAsciiMirror = (1U << 12),
    MfUltralightFeatureSupportCounterInMemory = (1U << 13),
} MfUltralightFeatureSupport;

typedef struct {
    uint8_t data[MF_ULTRALIGHT_PAGE_SIZE];
} MfUltralightPage;

typedef struct {
    uint8_t header;
    uint8_t vendor_id;
    uint8_t prod_type;
    uint8_t prod_subtype;
    uint8_t prod_ver_major;
    uint8_t prod_ver_minor;
    uint8_t storage_size;
    uint8_t protocol_type;
} MfUltralightVersion;

typedef struct {
    uint8_t data[MF_ULTRALIGHT_SIGNATURE_SIZE];
} MfUltralightSignature;

typedef struct {
    uint8_t data[MF_ULTRALIGHT_COUNTER_SIZE];
} MfUltralightCounter;

typedef struct {
    uint8_t data[MF_ULTRALIGHT_TEARING_FLAG_SIZE];
} MfUltralightTearingFlag;

typedef union {
    uint8_t data[MF_ULTRALIGHT_AUTH_PASSWORD_SIZE];
    uint32_t pass;
} MfUltralightAuthPassword;

typedef union {
    uint8_t data[MF_ULTRALIGHT_AUTH_PACK_SIZE];
    uint16_t pack;
} MfUltralightAuthPack;

typedef enum {
    MfUltralightMirrorNone,
    MfUltralightMirrorUid,
    MfUltralightMirrorCounter,
    MfUltralightMirrorUidCounter,
} MfUltralightMirrorConf;

typedef struct __attribute__((packed)) {
    union {
        uint8_t value;
        struct {
            uint8_t rfui1 : 2;
            bool strg_mod_en : 1;
            bool rfui2 : 1;
            uint8_t mirror_byte : 2;
            MfUltralightMirrorConf mirror_conf : 2;
        };
    } mirror;
    uint8_t rfui1;
    uint8_t mirror_page;
    uint8_t auth0;
    union {
        uint8_t value;
        struct {
            uint8_t authlim : 3;
            bool nfc_cnt_pwd_prot : 1;
            bool nfc_cnt_en : 1;
            bool nfc_dis_sec1 : 1; // NTAG I2C Plus only
            bool cfglck : 1;
            bool prot : 1;
        };
    } access;
    uint8_t vctid;
    uint8_t rfui2[2];
    MfUltralightAuthPassword password;
    MfUltralightAuthPack pack;
    uint8_t rfui3[2];
} MfUltralightConfigPages;

typedef struct {
    NfcaData nfca_data;
    MfUltralightType type;
    MfUltralightVersion version;
    MfUltralightSignature signature;
    MfUltralightCounter counter[MF_ULTRALIGHT_COUNTER_NUM];
    MfUltralightTearingFlag tearing_flag[MF_ULTRALIGHT_TEARING_FLAG_NUM];
    MfUltralightPage page[MF_ULTRALIGHT_MAX_PAGE_NUM];
    uint16_t pages_read;
    uint16_t pages_total;
} MfUltralightData;

MfUltralightType mf_ultralight_get_type_by_version(MfUltralightVersion* version);

uint16_t mf_ultralight_get_pages_total(MfUltralightType type);

uint32_t mf_ultralight_get_feature_support_set(MfUltralightType type);

const char* mf_ultralight_get_name(MfUltralightType type, bool full_name);

bool mf_ultralight_is_all_data_read(MfUltralightData* data);

bool mf_ultralight_detect_protocol(NfcaData* nfca_data);

bool mf_ultralight_is_counter_configured(MfUltralightData* data);

#ifdef __cplusplus
}
#endif