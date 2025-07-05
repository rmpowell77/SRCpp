#include "SRCppTestUtils.hpp"
#include <gtest/gtest.h>

#if defined(FROM_TYPE)
using From = FROM_TYPE;
#else
#error need type
using From = float;
#endif

#if defined(TO_TYPE)
using To = TO_TYPE;
#else
#error need type
using To = float;
#endif

TEST(SRCppPush, Resample) { RunPullConvertTest<To, From>(); }
