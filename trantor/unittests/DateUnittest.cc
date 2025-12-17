#include <trantor/utils/Date.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <iostream>
using namespace trantor;
TEST(Date, constructorTest)
{
    EXPECT_STREQ("1985-01-01 00:00:00",
                 trantor::Date(1985, 1, 1)
                     .toCustomFormattedStringLocal("%Y-%m-%d %H:%M:%S")
                     .c_str());
    EXPECT_STREQ("2004-02-29 00:00:00.000000",
                 trantor::Date(2004, 2, 29)
                     .toCustomFormattedStringLocal("%Y-%m-%d %H:%M:%S", true)
                     .c_str());
    EXPECT_STRNE("2001-02-29 00:00:00.000000",
                 trantor::Date(2001, 2, 29)
                     .toCustomFormattedStringLocal("%Y-%m-%d %H:%M:%S", true)
                     .c_str());
    EXPECT_STREQ("2018-01-01 00:00:00.000000",
                 trantor::Date(2018, 1, 1, 12, 12, 12, 2321)
                     .roundDay()
                     .toCustomFormattedStringLocal("%Y-%m-%d %H:%M:%S", true)
                     .c_str());
}
TEST(Date, DatabaseStringTest)
{
    auto now = trantor::Date::now();
    EXPECT_EQ(now, trantor::Date::fromDbStringLocal(now.toDbStringLocal()));
    EXPECT_EQ(now, trantor::Date::fromDbString(now.toDbString()));
    std::string dbString = "2018-01-01 00:00:00.123";
    auto dbDate = trantor::Date::fromDbStringLocal(dbString);
    auto ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 123);
    EXPECT_EQ(dbDate,
              trantor::Date::fromDbStringLocal(dbDate.toDbStringLocal()));
    EXPECT_EQ(dbDate, trantor::Date::fromDbString(dbDate.toDbString()));
    dbString = "2018-01-01 00:00:00.023";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 23);
    EXPECT_EQ(dbDate,
              trantor::Date::fromDbStringLocal(dbDate.toDbStringLocal()));
    EXPECT_EQ(dbDate, trantor::Date::fromDbString(dbDate.toDbString()));
    dbString = "2018-01-01 00:00:00.003";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 3);
    EXPECT_EQ(dbDate,
              trantor::Date::fromDbStringLocal(dbDate.toDbStringLocal()));
    EXPECT_EQ(dbDate, trantor::Date::fromDbString(dbDate.toDbString()));
    dbString = "2018-01-01 00:00:00.000123";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    auto us = (dbDate.microSecondsSinceEpoch() % 1000000);
    EXPECT_EQ(us, 123);
    EXPECT_EQ(dbDate,
              trantor::Date::fromDbStringLocal(dbDate.toDbStringLocal()));
    EXPECT_EQ(dbDate, trantor::Date::fromDbString(dbDate.toDbString()));
    dbString = "2018-01-01 00:00:00.000023";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    us = (dbDate.microSecondsSinceEpoch() % 1000000);
    EXPECT_EQ(us, 23);
    EXPECT_EQ(dbDate,
              trantor::Date::fromDbStringLocal(dbDate.toDbStringLocal()));
    EXPECT_EQ(dbDate, trantor::Date::fromDbString(dbDate.toDbString()));
    dbString = "2018-01-01 00:00:00.000003";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    us = (dbDate.microSecondsSinceEpoch() % 1000000);
    EXPECT_EQ(us, 3);

    dbString = "2018-01-01 00:00:00";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 0);

    dbString = "2018-01-01 00:00:00";
    dbDate = trantor::Date::fromDbStringLocal(dbString);
    auto dbDateGMT = trantor::Date::fromDbString(dbString);
    auto secLocal = (dbDate.microSecondsSinceEpoch() / 1000000);
    auto secGMT = (dbDateGMT.microSecondsSinceEpoch() / 1000000);
    // timeZone at least 1 minute (can be >=1 hour, 30 min, 15 min. Error if
    // difference less then minute)
    auto timeZoneOffsetMinutePart = (secLocal - secGMT) % 60;
    EXPECT_EQ(timeZoneOffsetMinutePart, 0);
    dbString = "2018-01-01 00:00:00.123";
    dbDate = trantor::Date::fromDbString(dbString);
    ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 123);
    dbString = "2018-01-01 00:00:00.023";
    dbDate = trantor::Date::fromDbString(dbString);
    ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 23);
    dbString = "2018-01-01 00:00:00.003";
    dbDate = trantor::Date::fromDbString(dbString);
    ms = (dbDate.microSecondsSinceEpoch() % 1000000) / 1000;
    EXPECT_EQ(ms, 3);
    dbString = "2018-01-01 00:00:00.000123";
    dbDate = trantor::Date::fromDbString(dbString);
    us = (dbDate.microSecondsSinceEpoch() % 1000000);
    EXPECT_EQ(us, 123);
    dbString = "2018-01-01 00:00:00.000023";
    dbDate = trantor::Date::fromDbString(dbString);
    us = (dbDate.microSecondsSinceEpoch() % 1000000);
    EXPECT_EQ(us, 23);
    dbString = "2018-01-01 00:00:00.000003";
    dbDate = trantor::Date::fromDbString(dbString);
    us = (dbDate.microSecondsSinceEpoch() % 1000000);
    EXPECT_EQ(us, 3);

    dbString = "1970-01-01";
    dbDateGMT = trantor::Date::fromDbString(dbString);
    auto epoch = dbDateGMT.microSecondsSinceEpoch();
    EXPECT_EQ(epoch, 0);
}
TEST(Date, TimezoneTest)
{
    std::string str0 = "2024-01-01 04:00:00.123";
    std::vector<std::string> strs{"2024-01-01 12:00:00.123 +08:00",
                                  "2024-01-01 11:00:00.123+0700",
                                  "2024-01-01 10:00:00.123 0600",
                                  "2024-01-01 03:00:00.123 -01:00",
                                  "2024-01-01 02:00:00.123-02:00",
                                  "2024-01-01 01:00:00.123 -0300",
                                  "2024-01-01 12:00:00.123 08",
                                  "2024-01-01 02:00:00.123-02",
                                  "2024-01-01 14:00:00.123+10"};

    auto date = trantor::Date::fromDbString(str0);
    for (auto &s : strs)
    {
        auto dateTz = trantor::Date::parseDatetimeTz(s);
        EXPECT_EQ(date.microSecondsSinceEpoch(),
                  dateTz.microSecondsSinceEpoch());
    }

    // time string without tz, should be parsed as local time
    EXPECT_EQ(trantor::Date::fromDbString(str0).microSecondsSinceEpoch(),
              trantor::Date::parseDatetimeTz(str0).microSecondsSinceEpoch());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
