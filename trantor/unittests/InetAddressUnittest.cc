#include <trantor/net/InetAddress.h>
#include <gtest/gtest.h>
#include <string>
#include <iostream>
using namespace trantor;
TEST(InetAddress, innerIpTest)
{
    EXPECT_EQ(true, InetAddress("192.168.0.1", 0).isIntranetIp());
    EXPECT_EQ(true, InetAddress("192.168.12.1", 0).isIntranetIp());
    EXPECT_EQ(true, InetAddress("10.168.0.1", 0).isIntranetIp());
    EXPECT_EQ(true, InetAddress("10.0.0.1", 0).isIntranetIp());
    EXPECT_EQ(true, InetAddress("172.31.10.1", 0).isIntranetIp());
    EXPECT_EQ(true, InetAddress("127.0.0.1", 0).isIntranetIp());
}
int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}