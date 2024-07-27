/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <gtest/gtest.h>

#include <base/tl/sorted_array.h>

TEST(SortedArray, SortEmptyRange)
{
	sorted_array<int> x;
	x.sort_range();
}
