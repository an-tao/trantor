#include <trantor/utils/Date.h>
#include <gtest/gtest.h>
#include <stdlib.h>
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
    std::string dat0 = "2024-01-01";
    std::string str0 = "2024-01-01 04:00:00.123";
    std::vector<std::string> strs{
        // in case we miss any comma, put brackets around
        {"2024-01-01 04:00:00.123Z"},
        {"2024-01-01 12:00:00.123 +08:00"},
        {"2024-01-01 11:00:00.123+0700"},
        {"2024-01-01 10:00:00.123 0600"},
        {"2024-01-01 09:00:00.123 +0500"},
        {"2024-01-01 08:00:00.123 04"},
        {"2024-01-01 07:00:00.123+03"},
        {"2024-01-01 06:30:00.123+02:30"},
        {"2024-01-01 03:00:00.123 -01:00"},
        {"2024-01-01 02:00:00.123-02:00"},
        {"2024-01-01 01:00:00.123 -0300"},
        {"2024-01-01 00:00:00.123-04"},
        {"2023-12-31 23:00:00.123 -05"},
        // with T
        {"2024-01-01T04:00:00.123000Z"},
        {"2024-01-01T12:00:00.123 +08:00"},
        // bad ones, but should pass
        {"2024-01-01T04:00:00.123+0"},
        {"2024-01-01T04:00:00.123-"},
    };

    auto date = trantor::Date::fromDbString(str0);
    for (auto &s : strs)
    {
        auto dateTz = trantor::Date::fromISOString(s);
        EXPECT_EQ(date.microSecondsSinceEpoch(),
                  dateTz.microSecondsSinceEpoch());
    }

    // time string without tz, should be parsed as local time
    auto dateLocal = trantor::Date::fromDbStringLocal(str0);
    EXPECT_EQ(dateLocal.microSecondsSinceEpoch(),
              trantor::Date::fromISOString(str0).microSecondsSinceEpoch());

    // only date part
    EXPECT_EQ(dateLocal.secondsSinceEpoch() - 4 * 3600,
              trantor::Date::fromISOString(dat0).secondsSinceEpoch());
}

TEST(Date, TimezoneOffsetTest)
{
#ifndef _WIN32

    struct TestCase
    {
        std::string timezone;
        std::string local_db_date;
        std::string utc;
        int offset;
    };

    // clang-format off
    std::vector<TestCase> cases{
        {"/usr/share/zoneinfo/Europe/Vienna", "2026-03-28 12:00:00", "2026-03-28 11:00:00", 1*60*60},
        {"/usr/share/zoneinfo/Europe/Vienna", "2026-04-01 12:00:00", "2026-04-01 10:00:00", 2*60*60},
        {"/usr/share/zoneinfo/Europe/Vienna", "2026-10-25 02:00:00", "2026-10-25 00:00:00", 2*60*60},
        {"/usr/share/zoneinfo/Europe/Vienna", "2026-10-25 03:00:00", "2026-10-25 02:00:00", 1*60*60}
    };
    // clang-format on

    for (auto &t : cases)
    {
        EXPECT_TRUE(setenv("TZ", t.timezone.c_str(), 1) != -1);
        tzset();

        auto local = trantor::Date::fromDbStringLocal(t.local_db_date);
        auto utc = trantor::Date::fromDbString(t.utc);
        EXPECT_EQ(local, utc);
        auto offset = local.timezoneOffsetCurrentDate();
        EXPECT_EQ(t.offset, offset);
    }  // for
#endif
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
