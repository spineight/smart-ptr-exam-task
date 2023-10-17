#include "solution.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(dummy_test, abi) {
  EXPECT_THROW(throwing_func(), std::logic_error);
}
