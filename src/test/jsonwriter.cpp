#include "test.h"
#include <gtest/gtest.h>

#include <engine/shared/jsonwriter.h>
#include <limits.h>

class JsonWriter : public ::testing::Test
{
protected:
	CTestInfo m_Info;
	CJsonWriter *m_pJson;
	char m_aOutputFilename[64];

	JsonWriter()
		: m_pJson(0)
	{
		m_Info.Filename(m_aOutputFilename, sizeof(m_aOutputFilename),
			"-got.json");
		IOHANDLE File = io_open(m_aOutputFilename, IOFLAG_WRITE);
		EXPECT_TRUE(File);
		m_pJson = new CJsonWriter(File);
	}

	void Expect(const char *pExpected)
	{
		ASSERT_TRUE(m_pJson);
		delete m_pJson;
		m_pJson = 0;

		char *pOutput = fs_read_str(m_aOutputFilename);
		ASSERT_TRUE(pOutput);
		EXPECT_STREQ(pOutput, pExpected);
		bool Correct = str_comp(pOutput, pExpected) == 0;
		mem_free(pOutput);

		if(!Correct)
		{
			char aFilename[64];
			m_Info.Filename(aFilename, sizeof(aFilename),
				"-expected.json");
			IOHANDLE File = io_open(aFilename, IOFLAG_WRITE);
			ASSERT_TRUE(File);
			io_write(File, pExpected, str_length(pExpected));
			io_close(File);
		}
		else
		{
			fs_remove(m_aOutputFilename);
		}
	}
};

TEST_F(JsonWriter, EmptyObject)
{
	m_pJson->BeginObject();
	m_pJson->EndObject();
	Expect("{\n}\n");
}

TEST_F(JsonWriter, EmptyArray)
{
	m_pJson->BeginArray();
	m_pJson->EndArray();
	Expect("[\n]\n");
}

TEST_F(JsonWriter, SpecialCharacters)
{
	m_pJson->BeginObject();
	m_pJson->WriteAttribute("\x01\"'\r\n\t");
	m_pJson->BeginArray();
	m_pJson->WriteStrValue(" \"'abc\x01\n");
	m_pJson->EndArray();
	m_pJson->EndObject();
	Expect(
		"{\n"
		"\t\"\\u0001\\\"'\\r\\n\\t\": [\n"
		"\t\t\" \\\"'abc\\u0001\\n\"\n"
		"\t]\n"
		"}\n"
	);
}

TEST_F(JsonWriter, HelloWorld)
{
	m_pJson->WriteStrValue("hello world");
	Expect("\"hello world\"\n");
}

TEST_F(JsonWriter, True) { m_pJson->WriteBoolValue(true); Expect("true\n"); }
TEST_F(JsonWriter, False) { m_pJson->WriteBoolValue(false); Expect("false\n"); }
TEST_F(JsonWriter, Null) { m_pJson->WriteNullValue(); Expect("null\n"); }
TEST_F(JsonWriter, EmptyString) { m_pJson->WriteStrValue(""); Expect("\"\"\n"); }
TEST_F(JsonWriter, Zero) { m_pJson->WriteIntValue(0); Expect("0\n"); }
TEST_F(JsonWriter, One) { m_pJson->WriteIntValue(1); Expect("1\n"); }
TEST_F(JsonWriter, MinusOne) { m_pJson->WriteIntValue(-1); Expect("-1\n"); }
TEST_F(JsonWriter, Large) { m_pJson->WriteIntValue(INT_MAX); Expect("2147483647\n"); }
TEST_F(JsonWriter, Small) { m_pJson->WriteIntValue(INT_MIN); Expect("-2147483648\n"); }
