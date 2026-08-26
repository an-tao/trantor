#include <trantor/net/TcpConnection.h>

#include <exception>
#include <iostream>

using namespace trantor;

int main()
{
    try
    {
        auto filePolicy = TLSPolicy::defaultClientPolicy();
        filePolicy->setCaPath("server.crt");
        (void)newSSLContext(*filePolicy, false);

        auto directoryPolicy = TLSPolicy::defaultClientPolicy();
        directoryPolicy->setCaPath("openssl-policy-ca");
        (void)newSSLContext(*directoryPolicy, false);

        auto serverDirectoryPolicy = TLSPolicy::defaultServerPolicy("", "");
        serverDirectoryPolicy->setCaPath("openssl-policy-ca");
        (void)newSSLContext(*serverDirectoryPolicy, true);

        auto validConfiguration = TLSPolicy::defaultClientPolicy();
        validConfiguration->setConfCmds({{"MinProtocol", "TLSv1.2"}});
        (void)newSSLContext(*validConfiguration, false);
    }
    catch (const std::exception &error)
    {
        std::cerr << "Valid OpenSSL policy failed: " << error.what() << '\n';
        return 1;
    }

    try
    {
        auto invalidConfiguration = TLSPolicy::defaultClientPolicy();
        invalidConfiguration->setConfCmds(
            {{"ThisCommandDoesNotExist", "enabled"}});
        (void)newSSLContext(*invalidConfiguration, false);
    }
    catch (const std::exception &)
    {
        return 0;
    }

    std::cerr << "Invalid OpenSSL configuration command was accepted\n";
    return 1;
}
