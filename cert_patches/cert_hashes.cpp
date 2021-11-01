#include "cert_hashes.hpp"

#include <map>
#include <string>
#include <array>
#include <cstdio>
#include <charconv>

#include <picosha2.h>

typedef std::array<uint8_t, picosha2::k_digest_size> sha256;

constexpr static int char2int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw std::invalid_argument("Invalid input string");
}
constexpr static sha256 operator ""_sha(const char* s, size_t l) {
    sha256 out;
    for (size_t i = 0; i < l; i += 2) {
        out[i/2] = char2int(s[i])*0x10 + char2int(s[i+1]);
    }
    return out;
}

struct Hash_Info {
    sha256 sha;
    Cert_State state;
};
const static std::array<Hash_Info, 4> cert_hashes = {
    Hash_Info{"ab7036365c7154aa29c2c29f5d4191163b162a2225011357d56d07ffa7bc1f72"_sha,{
        .id = CERT_ID_THWATE_PREMIUM_SERVER,
        .patch = CERT_PATCH_STATE_STOCK,
    }},
    Hash_Info{"96bcec06264976f37460779acf28c5a7cfe8a3c0aae11a8ffcee05c0bddf08c6"_sha, {
        .id = CERT_ID_ISRG_ROOT_X1,
        .patch = CERT_PATCH_STATE_PRETENDO,
    }},
    Hash_Info{"0687260331a72403d909f105e69bcf0d32e1bd2493ffc6d9206d11bcd6770739"_sha, {
        .id = CERT_ID_DST_ROOT_X3,
        .patch = CERT_PATCH_STATE_PRETENDO,
    }},
    Hash_Info{"4c30bf140963ec0ca48f76e9f87ec8943739d1ea55350a2970713b632a984936"_sha, {
        .id = CERT_ID_PKCA_Z_ROOT,
        .patch = CERT_PATCH_STATE_PRETENDO,
    }}
};

Cert_State cert_hash(std::ifstream& is) {
    sha256 hash;
    picosha2::hash256(is, hash.begin(), hash.end());
    printf("cert: ");
    for (auto c : hash) {
        printf("%02x", c);
    }
    printf("\n");

    auto hash_info = std::find_if(cbegin(cert_hashes), cend(cert_hashes), [&](const auto& h) {
        return h.sha == hash;
    });
    if (hash_info != cend(cert_hashes)) {
        return hash_info->state;
    }

    return (Cert_State) {
        .id = CERT_ID_UNKNOWN,
        .patch = CERT_PATCH_STATE_UNKNOWN,
    };
}

#define ENUM_NAME(e) case e: return #e
const char* cert_id_name(Cert_ID id) {
    switch (id) {
        ENUM_NAME(CERT_ID_UNKNOWN);
        ENUM_NAME(CERT_ID_THWATE_PREMIUM_SERVER);
        ENUM_NAME(CERT_ID_ISRG_ROOT_X1);
        ENUM_NAME(CERT_ID_DST_ROOT_X3);
        ENUM_NAME(CERT_ID_PKCA_Z_ROOT);
    }
    return "bug: unknown";
}
