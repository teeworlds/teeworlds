#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>

CTestInfo::CTestInfo()
{
	const ::testing::TestInfo *pTestInfo =
		::testing::UnitTest::GetInstance()->current_test_info();
	str_format(m_aFilenamePrefix, sizeof(m_aFilenamePrefix), "%s.%s-%d",
		pTestInfo->test_case_name(), pTestInfo->name(), pid());
	Filename(m_aFilename, sizeof(m_aFilename), ".tmp");
}

void CTestInfo::Filename(char *pBuffer, int BufferLength, const char *pSuffix)
{
	str_format(pBuffer, BufferLength, "%s%s", m_aFilenamePrefix, pSuffix);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	net_init();
	return RUN_ALL_TESTS();
}
