/*
 * Copyright (c) 2023 MTG AG
 * All rights reserved.
 *
 * This code is originally derived from software contributed to
 * The NetBSD Foundation by Alistair Crooks (agc@netbsd.org), and
 * carried further by Ribose Inc (https://www.ribose.com).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "kyber.h"

#include <botan/kyber.h>
#include <botan/pubkey.h>
#include <botan/system_rng.h>
#include <utility>
#include <vector>

using namespace Botan;
using namespace std;

namespace {
KyberMode
rnp_kyber_param_to_botan_kyber_mode(kyber_parameter_e mode)
{
    KyberMode result = KyberMode::Kyber1024;
    if (mode == kyber_768) {
        result = KyberMode::Kyber768;
    }
    return result;
}

uint32_t
key_share_size_from_kyber_param(kyber_parameter_e param)
{
    if (param == kyber_768) {
        return 24;
    }
    return 32; // kyber_1024
}
} // namespace

std::pair<pgp_kyber_public_key_t, pgp_kyber_private_key_t>
kyber_generate_keypair(/*rnp::RNG *rng,*/ kyber_parameter_e kyber_param)
{
    System_RNG       rng;
    Kyber_PrivateKey kyber_priv(rng, rnp_kyber_param_to_botan_kyber_mode(kyber_param));
    kyber_priv.set_binary_encoding(KyberKeyEncoding::Raw);
    secure_vector<uint8_t>      encoded_private_key = kyber_priv.private_key_bits();
    std::unique_ptr<Public_Key> kyber_pub = kyber_priv.public_key();

    std::vector<uint8_t> encoded_public_key = kyber_priv.public_key_bits();
    return std::make_pair(pgp_kyber_public_key_t(encoded_public_key, kyber_param),
                          pgp_kyber_private_key_t(encoded_private_key.data(),
                                                  encoded_private_key.size(),
                                                  kyber_param));
}

kem_encap_result_t
pgp_kyber_public_key_t::encapsulate()
{
    System_RNG      rng;
    Kyber_PublicKey decoded_kyber_pub(
      key_encoded_, rnp_kyber_param_to_botan_kyber_mode(kyber_mode_), KyberKeyEncoding::Raw);

    PK_KEM_Encryptor       kem_enc(decoded_kyber_pub, rng, "Raw", "base");
    secure_vector<uint8_t> encap_key;           // this has to go over the wire
    secure_vector<uint8_t> data_encryption_key; // this is the key used for
    // encryption of the payload data
    kem_enc.encrypt(encap_key, data_encryption_key, key_share_size_from_kyber_param(kyber_mode_), rng);
    kem_encap_result_t result;
    result.ciphertext.insert(
      result.ciphertext.end(), encap_key.data(), encap_key.data() + encap_key.size());
    result.symmetric_key.insert(result.symmetric_key.end(),
                                data_encryption_key.data(),
                                data_encryption_key.data() + data_encryption_key.size());
    return result;
}

std::vector<uint8_t>
pgp_kyber_private_key_t::decapsulate(const uint8_t *ciphertext, size_t ciphertext_len)
{
    System_RNG             rng;
    secure_vector<uint8_t> key_sv(key_encoded_.data(),
                                  key_encoded_.data() + key_encoded_.size());
    Kyber_PrivateKey       decoded_kyber_priv(
      key_sv, rnp_kyber_param_to_botan_kyber_mode(kyber_mode_), KyberKeyEncoding::Raw);
    PK_KEM_Decryptor       kem_dec(decoded_kyber_priv, rng, "Raw", "base");
    secure_vector<uint8_t> dec_shared_key = kem_dec.decrypt(ciphertext, ciphertext_len, key_share_size_from_kyber_param(kyber_mode_));
    return std::vector<uint8_t>(dec_shared_key.data(),
                                dec_shared_key.data() + dec_shared_key.size());
}