#include <trantor/utils/MsgBuffer.h>
#include <gtest/gtest.h>
#include <string>
#include <iostream>
using namespace trantor;
TEST(MsgBufferTest, readableTest)
{
    MsgBuffer buffer;

    EXPECT_EQ(0,buffer.readableBytes());
    buffer.append(std::string(128,'a'));
    EXPECT_EQ(128,buffer.readableBytes());
    buffer.retrieve(100);
    EXPECT_EQ(28,buffer.readableBytes());
    EXPECT_EQ('a',buffer.peekInt8());
    buffer.retrieveAll();
    EXPECT_EQ(0,buffer.readableBytes());
}
TEST(MsgBufferTest, writableTest)
{
    MsgBuffer buffer(100);

    EXPECT_EQ(100,buffer.writableBytes());
    buffer.append("abcde");
    EXPECT_EQ(95,buffer.writableBytes());
    buffer.append(std::string(100,'x'));
    EXPECT_EQ(111,buffer.writableBytes());
    buffer.retrieve(100);
    EXPECT_EQ(111,buffer.writableBytes());
    buffer.append(std::string(112,'c'));
    EXPECT_EQ(99,buffer.writableBytes());
    buffer.retrieveAll();
    EXPECT_EQ(216,buffer.writableBytes());
}

TEST(MsgBufferTest, addInFrontTest)
{
    MsgBuffer buffer(100);

    EXPECT_EQ(100,buffer.writableBytes());
    buffer.addInFrontInt8('a');
    EXPECT_EQ(100,buffer.writableBytes());
    buffer.addInFrontInt64(123);
    EXPECT_EQ(92,buffer.writableBytes());
    buffer.addInFrontInt64(100);
    EXPECT_EQ(84,buffer.writableBytes());
    buffer.addInFrontInt8(1);
    EXPECT_EQ(84,buffer.writableBytes());
}

int main( int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}