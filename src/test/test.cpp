#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>

CTestInfo::CTestInfo()
{
	const ::testing::TestInfo *pTestInfo =
		::testing::UnitTest::GetInstance()->current_test_info();
	str_format(m_aFilename, sizeof(m_aFilename), "%s.%s-%d.tmp",
		pTestInfo->test_case_name(), pTestInfo->name(), pid());
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	net_init();
	return RUN_ALL_TESTS();
}
