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

#ifndef DILITHIUM_EXDSA_COMPOSITE_H_
#define DILITHIUM_EXDSA_COMPOSITE_H_

#include "config.h"
#include <rnp/rnp_def.h>
#include <vector>
#include <repgp/repgp_def.h>
#include "crypto/rng.h"
#include "crypto/dilithium.h"
#include "crypto/dilithium_common.h"
#include "crypto/ecdsa.h"
#include "crypto/eddsa.h"
#include <memory>


struct pgp_dilithium_exdsa_key_t; /* forward declaration */

class pgp_dilithium_exdsa_composite_key_t {

public:
  virtual ~pgp_dilithium_exdsa_composite_key_t() = 0;

  static rnp_result_t gen_keypair(rnp::RNG *rng, pgp_dilithium_exdsa_key_t *key, pgp_pubkey_alg_t alg);

  static size_t exdsa_curve_privkey_size(pgp_curve_t curve);
  static size_t exdsa_curve_pubkey_size(pgp_curve_t curve);
  static size_t exdsa_curve_signature_size(pgp_curve_t curve);
  static pgp_curve_t pk_alg_to_curve_id(pgp_pubkey_alg_t pk_alg);
  static dilithium_parameter_e pk_alg_to_dilithium_id(pgp_pubkey_alg_t pk_alg);

  bool is_initialized() const {
    return exdsa_initialized_ && dilithium_initialized_;
  }
  
protected: 
  bool exdsa_initialized_ = false;
  bool dilithium_initialized_ = false;
  void initialized_or_throw() const;
};

typedef struct pgp_dilithium_exdsa_signature_t {
    std::vector<uint8_t> composite_ciphertext;
    std::vector<uint8_t> wrapped_sesskey;

    static size_t composite_signature_size(pgp_pubkey_alg_t pk_alg) {
      return dilithium_signature_size(pgp_dilithium_exdsa_composite_key_t::pk_alg_to_dilithium_id(pk_alg)) 
          + pgp_dilithium_exdsa_composite_key_t::exdsa_curve_signature_size(pgp_dilithium_exdsa_composite_key_t::pk_alg_to_curve_id(pk_alg));
    }
} pgp_dilithium_exdsa_signature_t;

class pgp_dilithium_exdsa_composite_private_key_t : public pgp_dilithium_exdsa_composite_key_t {
  public:
    pgp_dilithium_exdsa_composite_private_key_t(const uint8_t *key_encoded, size_t key_encoded_len, pgp_pubkey_alg_t pk_alg);
    pgp_dilithium_exdsa_composite_private_key_t(std::vector<uint8_t> const &exdsa_key_encoded, std::vector<uint8_t> const &dilithium_key_encoded, pgp_pubkey_alg_t pk_alg);
    pgp_dilithium_exdsa_composite_private_key_t(std::vector<uint8_t> const &key_encoded, pgp_pubkey_alg_t pk_alg);
    //pgp_dilithium_exdsa_composite_private_key_t(pgp_dilithium_exdsa_composite_private_key_t &other);
    pgp_dilithium_exdsa_composite_private_key_t& operator=(const pgp_dilithium_exdsa_composite_private_key_t &other);
    pgp_dilithium_exdsa_composite_private_key_t() = default;


    rnp_result_t sign(pgp_dilithium_exdsa_signature_t *sig, const uint8_t *msg, size_t msg_len);

    std::vector<uint8_t> get_encoded() const;

    pgp_pubkey_alg_t pk_alg(pgp_pubkey_alg_t) const 
    {
      return pk_alg_;
    }

    void secure_clear();

    static size_t encoded_size(pgp_pubkey_alg_t pk_alg);

  private:
    void dilithium_key_from_encoded(std::vector<uint8_t> key_encoded);
    void exdsa_key_from_encoded(std::vector<uint8_t> key_encoded);

    pgp_pubkey_alg_t pk_alg_;

    /* dilithium part */
    pgp_dilithium_private_key_t dilithium_key_;

    /* ecc part*/
    exdsa_private_key_t exdsa_key_;
};


class pgp_dilithium_exdsa_composite_public_key_t : public pgp_dilithium_exdsa_composite_key_t {
  public:
    pgp_dilithium_exdsa_composite_public_key_t(const uint8_t *key_encoded, size_t key_encoded_len, pgp_pubkey_alg_t pk_alg);
    pgp_dilithium_exdsa_composite_public_key_t(std::vector<uint8_t> const &exdsa_key_encoded, std::vector<uint8_t> const &dilithium_key_encoded, pgp_pubkey_alg_t pk_alg);
    pgp_dilithium_exdsa_composite_public_key_t(std::vector<uint8_t> const &key_encoded, pgp_pubkey_alg_t pk_alg);
    //pgp_dilithium_exdsa_composite_public_key_t(pgp_dilithium_exdsa_composite_public_key_t &other);
    pgp_dilithium_exdsa_composite_public_key_t& operator=(const pgp_dilithium_exdsa_composite_public_key_t &other);
    pgp_dilithium_exdsa_composite_public_key_t() = default;

    rnp_result_t verify(rnp::RNG *rng, pgp_dilithium_exdsa_signature_t sig);

    std::vector<uint8_t> get_encoded() const;

    pgp_pubkey_alg_t pk_alg(pgp_pubkey_alg_t) const 
    {
      return pk_alg_;
    }

    static size_t encoded_size(pgp_pubkey_alg_t pk_alg);

  private:
    void dilithium_key_from_encoded(std::vector<uint8_t> key_encoded);
    void exdsa_key_from_encoded(std::vector<uint8_t> key_encoded);

    pgp_pubkey_alg_t pk_alg_;

    /* dilithium part */
    pgp_dilithium_public_key_t dilithium_key_;

    /* ecc part*/
    exdsa_public_key_t exdsa_key_;
};

typedef struct pgp_dilithium_exdsa_key_t {
    pgp_dilithium_exdsa_composite_private_key_t priv;
    pgp_dilithium_exdsa_composite_public_key_t pub;
} pgp_dilithium_exdsa_key_t;

rnp_result_t dilithium_exdsa_validate_key(rnp::RNG *rng, const pgp_dilithium_exdsa_key_t *key, bool secret);


#endif
