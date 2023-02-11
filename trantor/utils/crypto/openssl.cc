#include <trantor/utils/Utilities.h>

#include <openssl/evp.h>

#if OPENSSL_VERSION_MAJOR < 3
#include <openssl/md5.h>
#include <openssl/sha.h>
#endif

// Hack: LibreSSL does not support SHA3. We use our own implementation.
#if defined(LIBRESSL_VERSION_NUMBER)
#include "sha3.h"
#include "sha3.cc"
#endif

namespace trantor
{
namespace utils
{
Hash128 md5(const void* data, size_t len)
{
#if OPENSSL_VERSION_MAJOR >= 3
    Hash128 hash;
    auto md5 = EVP_MD_fetch(nullptr, "MD5", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md5, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(md5);
    return hash;
#else
    Hash128 hash;
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data, len);
    MD5_Final((unsigned char*)&hash, &ctx);
    return hash;
#endif
}

Hash160 sha1(const void* data, size_t len)
{
#if OPENSSL_VERSION_MAJOR >= 3
    Hash160 hash;
    auto sha1 = EVP_MD_fetch(nullptr, "SHA1", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha1, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(sha1);
    return hash;
#else
    Hash160 hash;
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    SHA1_Final((unsigned char*)&hash, &ctx);
    return hash;
#endif
}

Hash256 sha256(const void* data, size_t len)
{
#if OPENSSL_VERSION_MAJOR >= 3
    Hash256 hash;
    auto sha256 = EVP_MD_fetch(nullptr, "SHA256", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha256, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(sha256);
    return hash;
#else
    Hash256 hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final((unsigned char*)&hash, &ctx);
    return hash;
#endif
}

Hash256 sha3(const void* data, size_t len)
{
#if defined(LIBRESSL_VERSION_NUMBER)
    Hash256 hash;
    trantor_sha3((const unsigned char*)data, len, &hash, sizeof(hash));
    return hash;
#elif OPENSSL_VERSION_MAJOR >= 3
    Hash256 hash;
    auto sha3 = EVP_MD_fetch(nullptr, "SHA3-256", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha3, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(sha3);
    return hash;
#else
    Hash256 hash;
    auto sha3 = EVP_sha3_256();
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha3, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    return hash;
#endif
}

}  // namespace utils
}  // namespace trantor
