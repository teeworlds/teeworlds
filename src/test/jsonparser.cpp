/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "test.h"
#include <gtest/gtest.h>

#include <engine/shared/jsonparser.h>
#include <limits.h>

#if defined(CONF_FAMILY_WINDOWS)
	#define LINE_ENDING "\r\n"
#else
	#define LINE_ENDING "\n"
#endif

TEST(JsonParser, EmptyObject)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("{" LINE_ENDING "}" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_object);
	EXPECT_EQ(pParsed->u.object.length, 0);
}

TEST(JsonParser, EmptyArray)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("[" LINE_ENDING "]" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_array);
	EXPECT_EQ(pParsed->u.array.length, 0);
}

TEST(JsonParser, SpecialCharacters)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString(
		"{" LINE_ENDING
		"\t\"\\u0001\\\"'\\r\\n\\t\": [" LINE_ENDING
		"\t\t\" \\\"'abc\\u0001\\n\"" LINE_ENDING
		"\t]" LINE_ENDING
		"}" LINE_ENDING
	);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_object);
	ASSERT_EQ(pParsed->u.object.length, 1);
	const json_value &rAttr = (*pParsed)["\x01\"'\r\n\t"];
	ASSERT_EQ(rAttr.type, json_array);
	ASSERT_EQ(rAttr.u.array.length, 1);
	EXPECT_EQ(rAttr[0].type, json_string);
	EXPECT_STREQ(rAttr[0].u.string.ptr, " \"'abc\x01\n");
}

TEST(JsonParser, HelloWorld)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("\"hello world\"" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_string);
	EXPECT_STREQ(pParsed->u.string.ptr, "hello world");
}

TEST(JsonParser, Unicode)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("\"Heizölrückstoßabdämpfung\"" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_string);
	EXPECT_STREQ(pParsed->u.string.ptr, "Heizölrückstoßabdämpfung");
}

TEST(JsonParser, True)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("true" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_boolean);
	EXPECT_EQ(pParsed->u.boolean, true);
}

TEST(JsonParser, False)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("false" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	EXPECT_EQ(pParsed->type, json_boolean);
	EXPECT_EQ(pParsed->u.boolean, false);
}

TEST(JsonParser, Null)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("null" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_null);
}

TEST(JsonParser, EmptyString)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("\"\"" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_string);
	EXPECT_STREQ(pParsed->u.string.ptr, "");
}

TEST(JsonParser, Zero)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("0" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_integer);
	EXPECT_EQ(pParsed->u.integer, 0);
}

TEST(JsonParser, One)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("1" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_integer);
	EXPECT_EQ(pParsed->u.integer, 1);
}

TEST(JsonParser, MinusOne)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("-1" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_integer);
	EXPECT_EQ(pParsed->u.integer, -1);
}

TEST(JsonParser, Large)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("2147483647" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_integer);
	EXPECT_EQ(pParsed->u.integer, INT_MAX);
}

TEST(JsonParser, Small)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("-2147483648" LINE_ENDING);
	ASSERT_TRUE(pParsed);
	ASSERT_EQ(pParsed->type, json_integer);
	EXPECT_EQ(pParsed->u.integer, INT_MIN);
}

TEST(JsonParser, Error)
{
	CJsonParser Parser;
	json_value *pParsed = Parser.ParseString("{[}]" LINE_ENDING);
	EXPECT_FALSE(pParsed);
	EXPECT_FALSE(Parser.ParsedJson());
	EXPECT_TRUE(Parser.Error()[0] != '\0');
}
