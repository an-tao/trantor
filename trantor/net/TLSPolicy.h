#pragma once
#include <trantor/exports.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>


namespace trantor
{
struct TRANTOR_EXPORT SSLPolicy final
{
    SSLPolicy &setConfCmds(
        const std::vector<std::pair<std::string, std::string>> &sslConfCmds)
    {
        sslConfCmds_ = sslConfCmds;
        return *this;
    }
    SSLPolicy &setHostname(const std::string &hostname)
    {
        hostname_ = hostname;
        return *this;
    }
    SSLPolicy &setCertPath(const std::string &certPath)
    {
        certPath_ = certPath;
        return *this;
    }
    SSLPolicy &setKeyPath(const std::string &keyPath)
    {
        keyPath_ = keyPath;
        return *this;
    }
    SSLPolicy &setCaPath(const std::string &caPath)
    {
        caPath_ = caPath;
        return *this;
    }
    SSLPolicy &setUseOldTLS(bool useOldTLS)
    {
        useOldTLS_ = useOldTLS;
        return *this;
    }
    SSLPolicy &setValidateDomain(bool validateDomain)
    {
        validateDomain_ = validateDomain;
        return *this;
    }
    SSLPolicy &setValidateChain(bool validateChain)
    {
        validateChain_ = validateChain;
        return *this;
    }
    SSLPolicy &setValidateDate(bool validateDate)
    {
        validateDate_ = validateDate;
        return *this;
    }
    SSLPolicy &setAlpnProtocols(const std::vector<std::string> &alpnProtocols)
    {
        alpnProtocols_ = alpnProtocols;
        return *this;
    }
    SSLPolicy &setAlpnProtocols(std::vector<std::string> &&alpnProtocols)
    {
        alpnProtocols_ = std::move(alpnProtocols);
        return *this;
    }
    SSLPolicy &setUseSystemCertStore(bool useSystemCertStore)
    {
        useSystemCertStore_ = useSystemCertStore;
        return *this;
    }

    // Composite pattern
    SSLPolicy &setValidate(bool enable)
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

    static std::shared_ptr<SSLPolicy> defaultServerPolicy(
        const std::string &certPath,
        const std::string &keyPath)
    {
        auto policy = std::make_shared<SSLPolicy>();
        policy->setValidate(false)
            .setUseOldTLS(false)
            .setUseSystemCertStore(false)
            .setCertPath(certPath)
            .setKeyPath(keyPath);
        return policy;
    }

    static std::shared_ptr<SSLPolicy> defaultClientPolicy(
        const std::string &hostname = "")
    {
        auto policy = std::make_shared<SSLPolicy>();
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
using SSLPolicyPtr = std::shared_ptr<SSLPolicy>;
}