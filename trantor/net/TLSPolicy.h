#pragma once
#include <trantor/exports.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace trantor
{
struct TRANTOR_EXPORT TLSPolicy final
{
    TLSPolicy &setConfCmds(
        const std::vector<std::pair<std::string, std::string>> &sslConfCmds)
    {
        sslConfCmds_ = sslConfCmds;
        return *this;
    }
    TLSPolicy &setHostname(const std::string &hostname)
    {
        hostname_ = hostname;
        return *this;
    }
    TLSPolicy &setCertPath(const std::string &certPath)
    {
        certPath_ = certPath;
        return *this;
    }
    TLSPolicy &setKeyPath(const std::string &keyPath)
    {
        keyPath_ = keyPath;
        return *this;
    }
    TLSPolicy &setCaPath(const std::string &caPath)
    {
        caPath_ = caPath;
        return *this;
    }
    TLSPolicy &setUseOldTLS(bool useOldTLS)
    {
        useOldTLS_ = useOldTLS;
        return *this;
    }
    TLSPolicy &setValidateDomain(bool validateDomain)
    {
        validateDomain_ = validateDomain;
        return *this;
    }
    TLSPolicy &setValidateChain(bool validateChain)
    {
        validateChain_ = validateChain;
        return *this;
    }
    TLSPolicy &setValidateDate(bool validateDate)
    {
        validateDate_ = validateDate;
        return *this;
    }
    TLSPolicy &setAlpnProtocols(const std::vector<std::string> &alpnProtocols)
    {
        alpnProtocols_ = alpnProtocols;
        return *this;
    }
    TLSPolicy &setAlpnProtocols(std::vector<std::string> &&alpnProtocols)
    {
        alpnProtocols_ = std::move(alpnProtocols);
        return *this;
    }
    TLSPolicy &setUseSystemCertStore(bool useSystemCertStore)
    {
        useSystemCertStore_ = useSystemCertStore;
        return *this;
    }

    // Composite pattern
    TLSPolicy &setValidate(bool enable)
    {
        validateDomain_ = enable;
        validateChain_ = enable;
        validateDate_ = enable;
        return *this;
    }

    const std::vector<std::pair<std::string, std::string>> &getConfCmds() const
    {
        return sslConfCmds_;
    }
    const std::string &getHostname() const
    {
        return hostname_;
    }
    const std::string &getCertPath() const
    {
        return certPath_;
    }
    const std::string &getKeyPath() const
    {
        return keyPath_;
    }
    const std::string &getCaPath() const
    {
        return caPath_;
    }
    bool getUseOldTLS() const
    {
        return useOldTLS_;
    }
    bool getValidateDomain() const
    {
        return validateDomain_;
    }
    bool getValidateChain() const
    {
        return validateChain_;
    }
    bool getValidateDate() const
    {
        return validateDate_;
    }
    const std::vector<std::string> &getAlpnProtocols() const
    {
        return alpnProtocols_;
    }
    const std::vector<std::string> &getAlpnProtocols()
    {
        return alpnProtocols_;
    }

    bool getUseSystemCertStore() const
    {
        return useSystemCertStore_;
    }

    static std::shared_ptr<TLSPolicy> defaultServerPolicy(
        const std::string &certPath,
        const std::string &keyPath)
    {
        auto policy = std::make_shared<TLSPolicy>();
        policy->setValidate(false)
            .setUseOldTLS(false)
            .setUseSystemCertStore(false)
            .setCertPath(certPath)
            .setKeyPath(keyPath);
        return policy;
    }

    static std::shared_ptr<TLSPolicy> defaultClientPolicy(
        const std::string &hostname = "")
    {
        auto policy = std::make_shared<TLSPolicy>();
        policy->setValidate(true)
            .setUseOldTLS(false)
            .setUseSystemCertStore(true)
            .setHostname(hostname);
        return policy;
    }

  protected:
    std::vector<std::pair<std::string, std::string>> sslConfCmds_ = {};
    std::string hostname_ = "";
    std::string certPath_ = "";
    std::string keyPath_ = "";
    std::string caPath_ = "";
    std::vector<std::string> alpnProtocols_ = {};
    bool useOldTLS_ = false;  // turn into specific version
    bool validateDomain_ = true;
    bool validateChain_ = true;
    bool validateDate_ = true;
    bool useSystemCertStore_ = true;
};
using TLSPolicyPtr = std::shared_ptr<TLSPolicy>;
}  // namespace trantor