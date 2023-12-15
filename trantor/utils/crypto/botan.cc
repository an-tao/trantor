#include <botan/hash.h>
#include <trantor/utils/Utilities.h>

#include <cassert>

#include "md5.h"
#include "sha1.h"
#include "sha3.h"
#include "blake2.h"

namespace trantor
{
namespace utils
{
Hash128 md5(const void* data, size_t len)
{
    Hash128 hash;
    auto md5 = Botan::HashFunction::create("MD5");
    if (md5 == nullptr)
    {
        MD5_CTX ctx;
        trantor_md5_init(&ctx);
        trantor_md5_update(&ctx, (const unsigned char*)data, len);
        trantor_md5_final(&ctx, (unsigned char*)&hash);
        return hash;
    }

    md5->update((const unsigned char*)data, len);
    md5->final((unsigned char*)&hash);
    return hash;
}

Hash160 sha1(const void* data, size_t len)
{
    Hash160 hash;
    auto sha1 = Botan::HashFunction::create("SHA-1");
    if (sha1 == nullptr)
    {
        SHA1_CTX ctx;
        TrantorSHA1Init(&ctx);
        TrantorSHA1Update(&ctx, (const unsigned char*)data, len);
        TrantorSHA1Final((unsigned char*)&hash, &ctx);
        return hash;
    }
    sha1->update((const unsigned char*)data, len);
    sha1->final((unsigned char*)&hash);
    return hash;
}

Hash256 sha256(const void* data, size_t len)
{
    Hash256 hash;
    auto sha256 = Botan::HashFunction::create("SHA-256");
    assert(sha256 != nullptr);  // Botan guarantees that SHA-256 is available
    sha256->update((const unsigned char*)data, len);
    sha256->final((unsigned char*)&hash);
    return hash;
}

Hash256 sha3(const void* data, size_t len)
{
    Hash256 hash;
    auto sha3 = Botan::HashFunction::create("SHA-3(256)");
    if (sha3 == nullptr)
    {
        trantor_sha3((const unsigned char*)data, len, &hash, sizeof(hash));
        return hash;
    }
    assert(sha3 != nullptr);
    sha3->update((const unsigned char*)data, len);
    sha3->final((unsigned char*)&hash);
    return hash;
}

Hash256 blake2b(const void* data, size_t len)
{
    Hash256 hash;
    auto blake2b = Botan::HashFunction::create("BLAKE2b(256)");
    if (blake2b == nullptr)
    {
        trantor_blake2b(&hash, sizeof(hash), data, len, NULL, 0);
        return hash;
    }
    blake2b->update((const unsigned char*)data, len);
    blake2b->final((unsigned char*)&hash);
    return hash;
}

}  // namespace utils
}  // namespace trantor