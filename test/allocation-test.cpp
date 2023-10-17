#include "linked-ptr.h"
#include "shared-ptr.h"
#include "test-classes.h"

#include <gtest/gtest.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define DISABLE_ALLOCATION_TESTS 1
#endif
#endif

#ifndef DISABLE_ALLOCATION_TESTS

namespace {

struct injected_fault : std::runtime_error {
  using runtime_error::runtime_error;
};

bool should_inject_fault();

void* injected_allocate(size_t count);
void injected_deallocate(void* ptr);

struct fault_injection_disable {
  fault_injection_disable();

  fault_injection_disable(const fault_injection_disable&) = delete;
  fault_injection_disable& operator=(const fault_injection_disable&) = delete;

  ~fault_injection_disable();

private:
  bool was_disabled;
};

template <typename T>
struct fault_injection_allocator {
  using value_type = T;

  fault_injection_allocator() = default;

  template <typename U>
  constexpr fault_injection_allocator(const fault_injection_allocator<U>&) noexcept {}

  T* allocate(size_t count) {
    return static_cast<T*>(injected_allocate(count * sizeof(T)));
  }

  void deallocate(void* ptr, size_t) {
    injected_deallocate(ptr);
  }
};

struct fault_injection_context {
  std::vector<size_t, fault_injection_allocator<size_t>> skip_ranges;
  size_t error_index = 0;
  size_t skip_index = 0;
  bool fault_registered = false;
};

thread_local bool disabled = false;
thread_local fault_injection_context* context = nullptr;

thread_local std::size_t new_calls = 0;
thread_local std::size_t delete_calls = 0;

void* injected_allocate(size_t count) {
  if (!disabled) {
    ++new_calls;
  }

  if (should_inject_fault()) {
    throw std::bad_alloc();
  }

  void* ptr = std::malloc(count);
  assert(ptr);
  return ptr;
}

void injected_deallocate(void* ptr) {
  if (!disabled) {
    ++delete_calls;
  }
  std::free(ptr);
}

bool should_inject_fault() {
  if (!context) {
    return false;
  }

  if (disabled) {
    return false;
  }

  assert(context->error_index <= context->skip_ranges.size());
  if (context->error_index == context->skip_ranges.size()) {
    fault_injection_disable dg;
    ++context->error_index;
    context->skip_ranges.push_back(0);
    context->fault_registered = true;
    return true;
  }

  assert(context->skip_index <= context->skip_ranges[context->error_index]);

  if (context->skip_index == context->skip_ranges[context->error_index]) {
    ++context->error_index;
    context->skip_index = 0;
    context->fault_registered = true;
    return true;
  }

  ++context->skip_index;
  return false;
}

void faulty_run(const std::function<void()>& f) {
  assert(!context);
  fault_injection_context ctx;
  context = &ctx;
  for (;;) {
    try {
      f();
    } catch (...) {
      fault_injection_disable dg;
      ctx.skip_ranges.resize(ctx.error_index);
      ++ctx.skip_ranges.back();
      ctx.error_index = 0;
      ctx.skip_index = 0;
      assert(ctx.fault_registered);
      ctx.fault_registered = false;
      continue;
    }
    assert(!ctx.fault_registered);
    break;
  }
  context = nullptr;
}

fault_injection_disable::fault_injection_disable() : was_disabled(disabled) {
  disabled = true;
}

fault_injection_disable::~fault_injection_disable() {
  disabled = was_disabled;
}

} // namespace

void* operator new(std::size_t count) {
  return injected_allocate(count);
}

void* operator new[](std::size_t count) {
  return injected_allocate(count);
}

void* operator new(std::size_t count, const std::nothrow_t&) noexcept {
  fault_injection_disable dg;
  return injected_allocate(count);
}

void* operator new[](std::size_t count, const std::nothrow_t&) noexcept {
  fault_injection_disable dg;
  return injected_allocate(count);
}

void operator delete(void* ptr) noexcept {
  injected_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
  injected_deallocate(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
  injected_deallocate(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
  injected_deallocate(ptr);
}

namespace {
template <typename IsShared>
class allocation_calls_test : public ::testing::Test {
protected:
  template <typename T, typename Deleter = std::default_delete<T>>
  using smart_ptr = std::conditional_t<IsShared::value, shared_ptr<T, Deleter>, linked_ptr<T, Deleter>>;
  using amountOfAllocations = std::integral_constant<int, (IsShared::value ? 1 : 0) + 1>;
  test_object::no_new_instances_guard instances_guard;
};

template <typename IsShared>
class fault_injection_test : public ::testing::Test {
protected:
  template <typename T, typename Deleter = std::default_delete<T>>
  using smart_ptr = std::conditional_t<IsShared::value, shared_ptr<T, Deleter>, linked_ptr<T, Deleter>>;
  test_object::no_new_instances_guard instances_guard;
};

using tested_extents = ::testing::Types<std::false_type, std::true_type>;

class extent_name_generator {
public:
  template <typename IsShared>
  static std::string GetName(int) {
    if (IsShared::value) {
      return "shared-ptr";
    } else {
      return "linked-ptr";
    }
  }
};

TYPED_TEST_SUITE(allocation_calls_test, tested_extents, extent_name_generator);
TYPED_TEST_SUITE(fault_injection_test, tested_extents, extent_name_generator);

} // namespace

TYPED_TEST(allocation_calls_test, allocations) {
  size_t new_calls_before = new_calls;
  size_t delete_calls_before = delete_calls;
  int* i_p = new int(1337);
  {
    typename TestFixture::template smart_ptr<int> p(i_p);
    EXPECT_EQ(*i_p, *p);
  }
  const auto new_calls_after = new_calls;
  const auto delete_calls_after = delete_calls;
  EXPECT_EQ(new_calls_after - new_calls_before, TestFixture::amountOfAllocations::value);
  EXPECT_EQ(delete_calls_after - delete_calls_before, TestFixture::amountOfAllocations::value);
}

TYPED_TEST(fault_injection_test, pointer_ctor) {
  faulty_run([] {
    bool deleted = false;
    destruction_tracker* ptr = new destruction_tracker(&deleted);
    try {
      typename TestFixture::template smart_ptr<destruction_tracker> sp(ptr);
    } catch (...) {
      fault_injection_disable dg;
      EXPECT_TRUE(deleted);
      throw;
    }
  });
}

TYPED_TEST(fault_injection_test, pointer_ctor_with_custom_deleter) {
  faulty_run([] {
    bool deleted = false;
    int* ptr = new int(42);
    try {
      typename TestFixture::template smart_ptr<int, tracking_deleter<int>> sp(ptr, tracking_deleter<int>(&deleted));
    } catch (...) {
      fault_injection_disable dg;
      EXPECT_TRUE(deleted);
      throw;
    }
  });
}

TYPED_TEST(fault_injection_test, reset_ptr) {
  faulty_run([] {
    bool deleted1 = false;
    bool deleted2 = false;
    ::disabled = true;
    destruction_tracker* ptr1 = new destruction_tracker(&deleted1);
    destruction_tracker* ptr2 = new destruction_tracker(&deleted2);
    typename TestFixture::template smart_ptr<destruction_tracker> sp(ptr1);
    ::disabled = false;
    try {
      sp.reset(ptr2);
    } catch (...) {
      fault_injection_disable dg;
      EXPECT_TRUE(deleted2);
      EXPECT_FALSE(deleted1);
      EXPECT_TRUE(sp.get() == ptr1);
      throw;
    }
  });
}

TYPED_TEST(fault_injection_test, reset_ptr_with_custom_deleter) {
  faulty_run([] {
    bool deleted1 = false;
    bool deleted2 = false;
    ::disabled = true;
    int* ptr1 = new int(42);
    int* ptr2 = new int(43);
    typename TestFixture::template smart_ptr<int, tracking_deleter<int>> sp(ptr1, tracking_deleter<int>(&deleted1));
    ::disabled = false;
    try {
      sp.reset(ptr2, tracking_deleter<int>(&deleted2));
    } catch (...) {
      fault_injection_disable dg;
      EXPECT_TRUE(deleted2);
      EXPECT_FALSE(deleted1);
      EXPECT_TRUE(sp.get() == ptr1);
      throw;
    }
  });
}

#endif
