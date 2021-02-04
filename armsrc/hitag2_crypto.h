#ifndef __HITAG2_CRYPTO_H
#define __HITAG2_CRYPTO_H

#include "common.h"

struct hitag2_tag {
    uint32_t uid;
    enum {
        TAG_STATE_RESET      = 0x01,       // Just powered up, awaiting GetSnr
        TAG_STATE_ACTIVATING = 0x02,       // In activation phase (password mode), sent UID, awaiting reader password
        TAG_STATE_ACTIVATED  = 0x03,       // Activation complete, awaiting read/write commands
        TAG_STATE_WRITING    = 0x04,       // In write command, awaiting sector contents to be written
    } state;
    uint16_t active_sector;
    uint8_t crypto_active;
    uint64_t cs;
    uint8_t sectors[12][4];
};

uint32_t _f20(const uint64_t x);
uint64_t _hitag2_init(const uint64_t key, const uint32_t serial, const uint32_t IV);
uint64_t _hitag2_round(uint64_t *state);
uint32_t _hitag2_byte(uint64_t *x);
void hitag2_cipher_reset(struct hitag2_tag *tag, const uint8_t *iv);
int hitag2_cipher_authenticate(uint64_t *cs, const uint8_t *authenticator_is);
int hitag2_cipher_transcrypt(uint64_t *cs, uint8_t *data, uint16_t bytes, uint16_t bits) ;

#endif
