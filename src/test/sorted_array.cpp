#include <gtest/gtest.h>

#include <base/tl/sorted_array.h>

TEST(SortedArray, SortEmptyRange)
{
	sorted_array<int> x;
	x.sort_range();
}
