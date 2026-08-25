#include <trantor/net/TcpClient.h>
#include <trantor/net/TcpServer.h>

#include <atomic>
#include <fstream>
#include <iostream>
#include <string>

using namespace trantor;

static std::string readFile(const std::string &path)
{
    std::ifstream file(path);
    return {std::istreambuf_iterator<char>(file), {}};
}

int main()
{
    constexpr char kHostname[] = "vhost.example.test";
    const auto certificatePem = readFile("server.crt");
    const auto privateKeyPem = readFile("server.key");
    if (certificatePem.empty() || privateKeyPem.empty())
    {
        std::cerr << "Failed to read the test certificate or key\n";
        return 1;
    }

    EventLoop loop;
    std::string selectedHostname;
    TcpServer server(&loop, InetAddress(0), "sync-certificate-provider");
    auto serverPolicy = std::make_shared<TLSPolicy>();
    serverPolicy->setServerCertificateProvider(
        [&selectedHostname, certificatePem, privateKeyPem, &kHostname](
            std::string hostname) {
            selectedHostname = std::move(hostname);
            if (selectedHostname != kHostname)
                return ServerCertificate{};
            return ServerCertificate{certificatePem, privateKeyPem};
        });
    server.enableSSL(serverPolicy);
    server.start();

    std::atomic<bool> timedOut{false};
    std::atomic<bool> handshakeComplete{false};
    auto client =
        std::make_shared<TcpClient>(&loop,
                                    InetAddress("127.0.0.1",
                                                server.address().toPort()),
                                    "sync-certificate-provider-client");
    auto clientPolicy = TLSPolicy::defaultClientPolicy(kHostname);
    clientPolicy->setValidate(false);
    client->enableSSL(clientPolicy);
    client->setConnectionCallback(
        [&loop, &handshakeComplete](const TcpConnectionPtr &connection) {
            if (connection->connected())
            {
                handshakeComplete = true;
                loop.quit();
            }
            else
                loop.quit();
        });
    loop.runAfter(3.0, [&loop, &timedOut]() {
        timedOut = true;
        loop.quit();
    });
    client->connect();
    loop.loop();
    client.reset();
    server.stop();

    if (timedOut || !handshakeComplete || selectedHostname != kHostname)
    {
        std::cerr << "Synchronous SNI certificate selection failed: timeout="
                  << timedOut << ", handshake=" << handshakeComplete
                  << ", hostname=" << selectedHostname << '\n';
        return 1;
    }
    return 0;
}
