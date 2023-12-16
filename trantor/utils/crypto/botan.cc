#include <botan/hash.h>
#include <trantor/utils/Utilities.h>

#include <cassert>
#include <string_view>

#include "md5.h"
#include "sha1.h"
#include "sha3.h"
#include "blake2.h"

template <typename Hash>
inline bool attemptHash(const std::string_view& name,
                        Hash& hash,
                        const void* data,
                        size_t len)
{
    auto hashFunction = Botan::HashFunction::create(name);
    if (hashFunction == nullptr)
        return false;

    hashFunction->update((const unsigned char*)data, len);
    hashFunction->final((unsigned char*)&hash);
    return true;
}

namespace trantor
{
namespace utils
{
Hash128 md5(const void* data, size_t len)
{
    Hash128 hash;
    if (attemptHash("MD5", hash, data, len))
        return hash;

    MD5_CTX ctx;
    trantor_md5_init(&ctx);
    trantor_md5_update(&ctx, (const unsigned char*)data, len);
    trantor_md5_final(&ctx, (unsigned char*)&hash);
    return hash;
}

Hash160 sha1(const void* data, size_t len)
{
    Hash160 hash;
    if (attemptHash("SHA-1", hash, data, len))
        return hash;

    SHA1_CTX ctx;
    TrantorSHA1Init(&ctx);
    TrantorSHA1Update(&ctx, (const unsigned char*)data, len);
    TrantorSHA1Final((unsigned char*)&hash, &ctx);
    return hash;
}

Hash256 sha256(const void* data, size_t len)
{
    Hash256 hash;
    // Botan should guarantees that SHA-256 is available
    bool ok = attemptHash("SHA-256", hash, data, len);
    if (!ok)
        assert(false);
    return hash;
}

Hash256 sha3(const void* data, size_t len)
{
    Hash256 hash;
    if (attemptHash("SHA-3(256)", hash, data, len))
        return hash;

    trantor_sha3((const unsigned char*)data, len, &hash, sizeof(hash));
    return hash;
}

Hash256 blake2b(const void* data, size_t len)
{
    Hash256 hash;
    if (attemptHash("BLAKE2b(256)", hash, data, len))
        return hash;

    trantor_blake2b(&hash, sizeof(hash), data, len, NULL, 0);
    return hash;
}

}  // namespace utils
}  // namespace trantor