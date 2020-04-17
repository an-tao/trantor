#include <trantor/utils/Date.h>
#include <gtest/gtest.h>
#include <trantor/utils/Funcs.h>
#include <iostream>
using namespace trantor;
TEST(splitString, split_string)
{
    std::string originString = "1,2,3";
    auto out = splitString(originString, ",");
    EXPECT_EQ(out.size(), 3);
    originString = ",1,2,3";
    out = splitString(originString, ",");
    EXPECT_EQ(out.size(), 4);
    EXPECT_TRUE(out[0].empty());
    originString = ",1,2,3,";
    out = splitString(originString, ",");
    EXPECT_EQ(out.size(), 5);
    EXPECT_TRUE(out[0].empty());
    originString = ",1,2,3,";
    out = splitString(originString, ":");
    EXPECT_EQ(out.size(), 1);
    originString = "trantor::splitString";
    out = splitString(originString, "::");
    EXPECT_EQ(out.size(), 2);
    EXPECT_STREQ(out[0].data(), "trantor");
    EXPECT_STREQ(out[1].data(), "splitString");
    originString = "trantor::::splitString";
    out = splitString(originString, "::");
    EXPECT_EQ(out.size(), 3);
    EXPECT_STREQ(out[0].data(), "trantor");
    EXPECT_STREQ(out[1].data(), "");
    EXPECT_STREQ(out[2].data(), "splitString");
    originString = "trantor:::splitString";
    out = splitString(originString, "::");
    EXPECT_EQ(out.size(), 2);
    EXPECT_STREQ(out[0].data(), "trantor");
    EXPECT_STREQ(out[1].data(), ":splitString");
    originString = "trantor:::splitString";
    out = splitString(originString, "trantor:::splitString");
    EXPECT_EQ(out.size(), 2);
    EXPECT_STREQ(out[0].data(), "");
    EXPECT_STREQ(out[1].data(), "");
}
int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}