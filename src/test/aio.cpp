#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>

static const int BUF_SIZE = 64 * 1024;

class Async : public ::testing::Test
{
protected:
	ASYNCIO *m_pAio;
	CTestInfo m_Info;
	bool Delete;

	void SetUp()
	{
		IOHANDLE File = io_open(m_Info.m_aFilename, IOFLAG_WRITE);
		ASSERT_TRUE(File);
		m_pAio = aio_new(File);
		Delete = false;
	}

	~Async()
	{
		if(Delete)
		{
			fs_remove(m_Info.m_aFilename);
		}
	}

	void Write(const char *pText)
	{
		aio_write(m_pAio, pText, str_length(pText));
	}

	void Expect(const char *pOutput)
	{
		aio_close(m_pAio);
		aio_wait(m_pAio);
		aio_free(m_pAio);

		char aBuf[BUF_SIZE];
		IOHANDLE File = io_open(m_Info.m_aFilename, IOFLAG_READ);
		ASSERT_TRUE(File);
		int Read = io_read(File, aBuf, sizeof(aBuf));
		io_close(File);

		ASSERT_EQ(str_length(pOutput), Read);
		ASSERT_TRUE(mem_comp(aBuf, pOutput, Read) == 0);
		Delete = true;
	}
};

TEST_F(Async, Empty)
{
	Expect("");
}

TEST_F(Async, Simple)
{
	static const char TEXT[] = "a\n";
	Write(TEXT);
	Expect(TEXT);
}

TEST_F(Async, Long)
{
	char aText[BUF_SIZE + 1];
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		aText[i] = 'a';
	}
	aText[sizeof(aText) - 1] = 0;
	Write(aText);
	Expect(aText);
}

TEST_F(Async, Pieces)
{
	char aText[BUF_SIZE + 1];
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		aText[i] = 'a';
	}
	aText[sizeof(aText) - 1] = 0;
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		Write("a");
	}
	Expect(aText);
}

TEST_F(Async, Mixed)
{
	char aText[BUF_SIZE + 1];
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		aText[i] = 'a' + i % 26;
	}
	aText[sizeof(aText) - 1] = 0;
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		char w = 'a' + i % 26;
		aio_write(m_pAio, &w, 1);
	}
	Expect(aText);
}

TEST_F(Async, NonDivisor)
{
	static const int NUM_LETTERS = 13;
	static const int SIZE = BUF_SIZE / NUM_LETTERS * NUM_LETTERS;
	char aText[SIZE + 1];
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		aText[i] = 'a' + i % NUM_LETTERS;
	}
	aText[sizeof(aText) - 1] = 0;
	for(unsigned i = 0; i < (sizeof(aText) - 1) / NUM_LETTERS; i++)
	{
		Write("abcdefghijklm");
	}
	Expect(aText);
}

TEST_F(Async, Transaction)
{
	static const int NUM_LETTERS = 13;
	static const int SIZE = BUF_SIZE / NUM_LETTERS * NUM_LETTERS;
	char aText[SIZE + 1];
	for(unsigned i = 0; i < sizeof(aText) - 1; i++)
	{
		aText[i] = 'a' + i % NUM_LETTERS;
	}
	aText[sizeof(aText) - 1] = 0;
	for(unsigned i = 0; i < (sizeof(aText) - 1) / NUM_LETTERS; i++)
	{
		aio_lock(m_pAio);
		for(char c = 'a'; c < 'a' + NUM_LETTERS; c++)
		{
			aio_write_unlocked(m_pAio, &c, 1);
		}
		aio_unlock(m_pAio);
	}
	Expect(aText);
}
