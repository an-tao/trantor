#include <trantor/utils/Utilities.h>

#include <openssl/evp.h>

namespace trantor
{
namespace utils
{

Hash128 md5(const void* data, size_t len)
{
    Hash128 hash;
    auto md5 = EVP_MD_fetch(nullptr, "MD5", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md5, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(md5);
    return hash;
}

Hash160 sha1(const void* data, size_t len)
{
    Hash160 hash;
    auto sha1 = EVP_MD_fetch(nullptr, "SHA1", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha1, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(sha1);
    return hash;
}

Hash256 sha256(const void* data, size_t len)
{
    Hash256 hash;
    auto sha256 = EVP_MD_fetch(nullptr, "SHA256", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha256, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(sha256);
    return hash;
}

Hash256 sha3(const void* data, size_t len)
{
    Hash256 hash;
    auto sha3 = EVP_MD_fetch(nullptr, "SHA3-256", nullptr);
    auto ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, sha3, nullptr);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, (unsigned char*)&hash, nullptr);
    EVP_MD_CTX_free(ctx);
    EVP_MD_free(sha3);
    return hash;
}

}  // namespace utils
}  // namespace trantor