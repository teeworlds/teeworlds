#include <gtest/gtest.h>

#include <base/system.h>

TEST(Str, Startswith)
{
	EXPECT_TRUE(str_startswith("abcdef", "abc"));
	EXPECT_FALSE(str_startswith("abc", "abcdef"));

	EXPECT_TRUE(str_startswith("xyz", ""));
	EXPECT_FALSE(str_startswith("", "xyz"));

	EXPECT_FALSE(str_startswith("house", "home"));
	EXPECT_FALSE(str_startswith("blackboard", "board"));

	EXPECT_TRUE(str_startswith("поплавать", "по"));
	EXPECT_FALSE(str_startswith("плавать", "по"));

	static const char ABCDEFG[] = "abcdefg";
	static const char ABC[] = "abc";
	EXPECT_EQ(str_startswith(ABCDEFG, ABC) - ABCDEFG, str_length(ABC));
}

TEST(Str, StartswithNocase)
{
	EXPECT_TRUE(str_startswith_nocase("Abcdef", "abc"));
	EXPECT_FALSE(str_startswith_nocase("aBc", "abcdef"));

	EXPECT_TRUE(str_startswith_nocase("xYz", ""));
	EXPECT_FALSE(str_startswith_nocase("", "xYz"));

	EXPECT_FALSE(str_startswith_nocase("house", "home"));
	EXPECT_FALSE(str_startswith_nocase("Blackboard", "board"));

	EXPECT_TRUE(str_startswith_nocase("поплавать", "по"));
	EXPECT_FALSE(str_startswith_nocase("плавать", "по"));

	static const char ABCDEFG[] = "aBcdefg";
	static const char ABC[] = "abc";
	EXPECT_EQ(str_startswith_nocase(ABCDEFG, ABC) - ABCDEFG, str_length(ABC));
}

TEST(Str, Endswith)
{
	EXPECT_TRUE(str_endswith("abcdef", "def"));
	EXPECT_FALSE(str_endswith("def", "abcdef"));

	EXPECT_TRUE(str_endswith("xyz", ""));
	EXPECT_FALSE(str_endswith("", "xyz"));

	EXPECT_FALSE(str_endswith("rhyme", "mine"));
	EXPECT_FALSE(str_endswith("blackboard", "black"));

	EXPECT_TRUE(str_endswith("люди", "юди"));
	EXPECT_FALSE(str_endswith("люди", "любовь"));

	static const char ABCDEFG[] = "abcdefg";
	static const char DEFG[] = "defg";
	EXPECT_EQ(str_endswith(ABCDEFG, DEFG) - ABCDEFG,
		str_length(ABCDEFG) - str_length(DEFG));
}

TEST(Str, EndswithNocase)
{
	EXPECT_TRUE(str_endswith_nocase("abcdef", "deF"));
	EXPECT_FALSE(str_endswith_nocase("def", "abcdef"));

	EXPECT_TRUE(str_endswith_nocase("xyz", ""));
	EXPECT_FALSE(str_endswith_nocase("", "xyz"));

	EXPECT_FALSE(str_endswith_nocase("rhyme", "minE"));
	EXPECT_FALSE(str_endswith_nocase("blackboard", "black"));

	EXPECT_TRUE(str_endswith_nocase("люди", "юди"));
	EXPECT_FALSE(str_endswith_nocase("люди", "любовь"));

	static const char ABCDEFG[] = "abcdefG";
	static const char DEFG[] = "defg";
	EXPECT_EQ(str_endswith_nocase(ABCDEFG, DEFG) - ABCDEFG,
		str_length(ABCDEFG) - str_length(DEFG));
}

TEST(StrFormat, Positional)
{
	char aBuf[256];

	// normal
	str_format(aBuf, sizeof(aBuf), "%s %s", "first", "second");
	EXPECT_STREQ(aBuf, "first second");

	// normal with positional arguments
	str_format(aBuf, sizeof(aBuf), "%1$s %2$s", "first", "second");
	EXPECT_STREQ(aBuf, "first second");

	// reverse
	str_format(aBuf, sizeof(aBuf), "%2$s %1$s", "first", "second");
	EXPECT_STREQ(aBuf, "second first");

	// duplicate
	str_format(aBuf, sizeof(aBuf), "%1$s %1$s %2$d %1$s %2$d", "str", 1);
	EXPECT_STREQ(aBuf, "str str 1 str 1");
}
