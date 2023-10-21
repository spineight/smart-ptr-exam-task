#include "linked-ptr.h"
#include "shared-ptr.h"
#include "test-classes.h"

#include <gtest/gtest.h>

namespace {
template <typename IsShared>
class common_tests : public ::testing::Test {
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

class unifiedDestructorForBase {
public:
  unifiedDestructorForBase() = default;

  void operator()(destruction_tracker_base* val) {
    delete val;
  }
};

TYPED_TEST_SUITE(common_tests, tested_extents, extent_name_generator);
} // namespace

TYPED_TEST(common_tests, default_ctor) {
  typename TestFixture::template smart_ptr<test_object> p;
  EXPECT_EQ(nullptr, p.get());
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_tests, ptr_ctor) {
  test_object* p = new test_object(42);
  typename TestFixture::template smart_ptr<test_object> q(p);
  EXPECT_TRUE(static_cast<bool>(q));
  EXPECT_EQ(p, q.get());
  EXPECT_EQ(42, *q);
}

TYPED_TEST(common_tests, ptr_ctor_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p(nullptr);
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_EQ(0, p.use_count());
}

TYPED_TEST(common_tests, ptr_ctor_non_empty_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p(static_cast<test_object*>(nullptr));
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_EQ(1, p.use_count());
}

TYPED_TEST(common_tests, const_dereferencing) {
  const typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  EXPECT_EQ(42, *p);
  EXPECT_EQ(42, p->operator int());
}

TYPED_TEST(common_tests, reset) {
  typename TestFixture::template smart_ptr<test_object> q(new test_object(42));
  EXPECT_TRUE(static_cast<bool>(q));
  q.reset();
  EXPECT_FALSE(static_cast<bool>(q));
}

TYPED_TEST(common_tests, reset_nullptr) {
  typename TestFixture::template smart_ptr<test_object> q;
  EXPECT_FALSE(static_cast<bool>(q));
  q.reset();
  EXPECT_FALSE(static_cast<bool>(q));
}

TYPED_TEST(common_tests, reset_ptr) {
  typename TestFixture::template smart_ptr<test_object> q(new test_object(42));
  EXPECT_TRUE(static_cast<bool>(q));
  q.reset(new test_object(43));
  EXPECT_EQ(43, *q);
}

TYPED_TEST(common_tests, copy_ctor) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  EXPECT_EQ(1, p.use_count());
  typename TestFixture::template smart_ptr<test_object> q = p;
  EXPECT_TRUE(static_cast<bool>(p));
  EXPECT_TRUE(static_cast<bool>(q));
  EXPECT_TRUE(p == q);
  EXPECT_EQ(42, *p);
  EXPECT_EQ(42, *q);
  EXPECT_EQ(2, q.use_count());
}

TYPED_TEST(common_tests, copy_ctor_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  typename TestFixture::template smart_ptr<test_object> q = p;
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_FALSE(static_cast<bool>(q));
}

TYPED_TEST(common_tests, copy_assignment_operator) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  typename TestFixture::template smart_ptr<test_object> q(new test_object(43));
  p = q;
  EXPECT_EQ(43, *p);
  EXPECT_TRUE(p == q);
}

TYPED_TEST(common_tests, copy_assignment_operator_from_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  typename TestFixture::template smart_ptr<test_object> q;
  p = q;
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_tests, copy_assignment_operator_to_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  typename TestFixture::template smart_ptr<test_object> q(new test_object(43));
  p = q;
  EXPECT_EQ(43, *p);
  EXPECT_TRUE(p == q);
}

TYPED_TEST(common_tests, copy_assignment_operator_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  typename TestFixture::template smart_ptr<test_object> q;
  p = q;
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_tests, copy_assignment_operator_self) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  p = p;
  EXPECT_EQ(42, *p);
}

TYPED_TEST(common_tests, copy_assignment_operator_self_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  p = p;
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_tests, non_copyable_deleter) {
  typename TestFixture::template smart_ptr<test_object, non_copyable_tacker> p(new test_object(42));
}

TYPED_TEST(common_tests, ptr_ctor_inheritance) {
  bool deleted = false;
  { typename TestFixture::template smart_ptr<destruction_tracker_base> p(new destruction_tracker(&deleted)); }
  EXPECT_TRUE(deleted); // cause no polymorphism so we use dstr from base
}

TYPED_TEST(common_tests, reset_ptr_inheritance) {
  bool deleted = false;
  {
    typename TestFixture::template smart_ptr<destruction_tracker_base> p;
    p.reset(new destruction_tracker(&deleted));
  }
  EXPECT_TRUE(deleted);
}

TYPED_TEST(common_tests, custom_deleter) {
  bool deleted = false;
  {
    typename TestFixture::template smart_ptr<test_object, tracking_deleter<test_object>> p(
        new test_object(42), tracking_deleter<test_object>(&deleted));
  }
  EXPECT_TRUE(deleted);
}

TYPED_TEST(common_tests, custom_deleter_reset) {
  bool deleted = false;
  {
    typename TestFixture::template smart_ptr<test_object, tracking_deleter<test_object>> p;
    p.reset(new test_object(42), tracking_deleter<test_object>(&deleted));
  }
  EXPECT_TRUE(deleted);
}

TEST(linked_tests, copy_ctor_const) {
  linked_ptr<test_object, std::default_delete<const test_object>> p(new test_object(42));
  linked_ptr<const test_object> q = p;
  EXPECT_EQ(42, *q);
}

TEST(linked_tests, copy_assignment_operator_const) {
  linked_ptr<test_object, std::default_delete<const test_object>> p(new test_object(42));
  linked_ptr<const test_object> q(new test_object(43));
  q = p;
  EXPECT_EQ(42, *q);
  EXPECT_EQ(42, *p);
}

TEST(linked_tests, copy_assignment_operator_const_to_nullptr) {
  linked_ptr<test_object, std::default_delete<const test_object>> p(new test_object(42));
  linked_ptr<const test_object> q;
  q = p;
  EXPECT_EQ(42, *q);
  EXPECT_EQ(42, *p);
}

TEST(linked_tests, copy_assignment_operator_const_from_nullptr) {
  linked_ptr<test_object, std::default_delete<const test_object>> p;
  linked_ptr<const test_object> q(new test_object(43));
  q = p;
  EXPECT_FALSE(static_cast<bool>(q));
  EXPECT_FALSE(static_cast<bool>(p));
}

TEST(linked_tests, copy_ctor_inheritance) {
  bool deleted = false;
  {
    destruction_tracker* ptr = new destruction_tracker(&deleted);
    linked_ptr<destruction_tracker, unifiedDestructorForBase> d(ptr);
    {
      linked_ptr<destruction_tracker_base, unifiedDestructorForBase> b = d;
      EXPECT_EQ(ptr, b.get());
      EXPECT_EQ(ptr, d.get());
    }
    EXPECT_FALSE(deleted);
  }
  EXPECT_TRUE(deleted); // same reason
}

TEST(linked_tests, copy_assignment_operator_inheritance) {
  bool deleted = false;
  {
    destruction_tracker* ptr = new destruction_tracker(&deleted);
    linked_ptr<destruction_tracker, unifiedDestructorForBase> d(ptr);
    bool tmpDeleted = false;
    {
      linked_ptr<destruction_tracker_base, unifiedDestructorForBase> b(new destruction_tracker_base(&tmpDeleted));
      b = d;
      EXPECT_EQ(ptr, b.get());
      EXPECT_EQ(ptr, d.get());
      EXPECT_FALSE(deleted);
      EXPECT_TRUE(tmpDeleted);
    }
    EXPECT_FALSE(deleted);
    EXPECT_TRUE(tmpDeleted);
  }
  EXPECT_TRUE(deleted); // same reason
}

TYPED_TEST(common_tests, equivalence) {
  typename TestFixture::template smart_ptr<test_object> p1(new test_object(42));
  typename TestFixture::template smart_ptr<test_object> p2(new test_object(43));
  auto p3 = p2;

  EXPECT_FALSE(p1 == p2);
  EXPECT_TRUE(p1 != p2);

  EXPECT_FALSE(p3 == p1);
  EXPECT_TRUE(p3 == p2);
}

TYPED_TEST(common_tests, equivalence_self) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));

  EXPECT_TRUE(p == p);
  EXPECT_FALSE(p != p);
}

TYPED_TEST(common_tests, equivalence_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;

  EXPECT_TRUE(p == nullptr);
  EXPECT_FALSE(p != nullptr);
  EXPECT_TRUE(nullptr == p);
  EXPECT_FALSE(nullptr != p);
}

TYPED_TEST(common_tests, check_lifetime) {
  bool deleted = false;
  {
    destruction_tracker* ptr = new destruction_tracker(&deleted);
    typename TestFixture::template smart_ptr<destruction_tracker> d(ptr);
    {
      typename TestFixture::template smart_ptr<destruction_tracker> b1 = d;
      auto b2 = d;
      EXPECT_EQ(ptr, b1.get());
      EXPECT_EQ(ptr, b2.get());
      EXPECT_EQ(ptr, d.get());
      EXPECT_FALSE(deleted);
    }
    EXPECT_FALSE(deleted);
  }
  EXPECT_TRUE(deleted);
}

TEST(type_test, shared_prt_cstr) {
  static_assert(std::is_constructible_v<shared_ptr<int>, int*>);
  static_assert(std::is_constructible_v<shared_ptr<int>, int*, std::default_delete<int>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, int*, std::default_delete<char>>);
  static_assert(std::is_constructible_v<shared_ptr<destruction_tracker_base>, destruction_tracker*>);
  static_assert(
      std::is_constructible_v<shared_ptr<destruction_tracker_base, std::default_delete<destruction_tracker_base>>,
                              destruction_tracker*, std::default_delete<destruction_tracker>>);
}

TEST(type_test, linked_prt_cstr) {
  static_assert(std::is_constructible_v<linked_ptr<int>, int*>);
  static_assert(std::is_constructible_v<linked_ptr<int>, int*, std::default_delete<int>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, int*, std::default_delete<char>>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base>, destruction_tracker*>);
  static_assert(
      std::is_constructible_v<linked_ptr<destruction_tracker_base, std::default_delete<destruction_tracker_base>>,
                              destruction_tracker*, std::default_delete<destruction_tracker>>);
}

TEST(type_test, shared_ptr_copy) {
  static_assert(std::is_constructible_v<shared_ptr<int>, shared_ptr<int>>);
  static_assert(std::is_constructible_v<shared_ptr<int>, shared_ptr<int, std::default_delete<int>>>);
  static_assert(
      std::is_constructible_v<shared_ptr<int, std::default_delete<int>>, shared_ptr<int, std::default_delete<int>>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, shared_ptr<char>>);
  static_assert(!std::is_constructible_v<shared_ptr<char>, shared_ptr<int>>);
  // don't look at incorrect dstr for int. Just need type checking
  static_assert(!std::is_constructible_v<shared_ptr<char, std::default_delete<char>>,
                                         shared_ptr<int, std::default_delete<char>>>);
  static_assert(!std::is_constructible_v<shared_ptr<destruction_tracker_base>, shared_ptr<destruction_tracker>>);
  static_assert(!std::is_constructible_v<shared_ptr<destruction_tracker_base, unifiedDestructorForBase>,
                                         shared_ptr<destruction_tracker, unifiedDestructorForBase>>);
}

TEST(type_test, linked_ptr_copy) {
  static_assert(std::is_constructible_v<linked_ptr<int>, linked_ptr<int>>);
  static_assert(std::is_constructible_v<linked_ptr<int>, linked_ptr<int, std::default_delete<int>>>);
  static_assert(
      std::is_constructible_v<linked_ptr<int, std::default_delete<int>>, linked_ptr<int, std::default_delete<int>>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, linked_ptr<char>>);
  static_assert(!std::is_constructible_v<linked_ptr<char>, linked_ptr<int>>);
  // don't look at incorrect dstr for int. Just need type checking
  static_assert(
      std::is_constructible_v<linked_ptr<char, std::default_delete<char>>, linked_ptr<int, std::default_delete<char>>>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base>, linked_ptr<destruction_tracker>>);
  static_assert(!std::is_constructible_v<linked_ptr<destruction_tracker>, linked_ptr<destruction_tracker_base>>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base, unifiedDestructorForBase>,
                                        linked_ptr<destruction_tracker, unifiedDestructorForBase>>);
}

TEST(type_test, shared_ptr_assign) {
  static_assert(std::is_assignable_v<shared_ptr<int>, shared_ptr<int>>);
  static_assert(std::is_assignable_v<shared_ptr<int>, shared_ptr<int, std::default_delete<int>>>);
  static_assert(
      std::is_assignable_v<shared_ptr<int, std::default_delete<int>>, shared_ptr<int, std::default_delete<int>>>);
  static_assert(!std::is_assignable_v<shared_ptr<int>, shared_ptr<char>>);
  static_assert(!std::is_assignable_v<shared_ptr<char>, shared_ptr<int>>);
  // don't look at incorrect dstr for int. Just need type checking
  static_assert(
      !std::is_assignable_v<shared_ptr<char, std::default_delete<char>>, shared_ptr<int, std::default_delete<char>>>);
  static_assert(!std::is_assignable_v<shared_ptr<destruction_tracker_base>, shared_ptr<destruction_tracker>>);
  static_assert(!std::is_assignable_v<shared_ptr<destruction_tracker_base, unifiedDestructorForBase>,
                                      shared_ptr<destruction_tracker, unifiedDestructorForBase>>);
}

TEST(type_test, linked_ptr_assign) {
  static_assert(std::is_assignable_v<linked_ptr<int>, linked_ptr<int>>);
  static_assert(std::is_assignable_v<linked_ptr<int>, linked_ptr<int, std::default_delete<int>>>);
  static_assert(
      std::is_assignable_v<linked_ptr<int, std::default_delete<int>>, linked_ptr<int, std::default_delete<int>>>);
  static_assert(!std::is_assignable_v<linked_ptr<int>, linked_ptr<char>>);
  static_assert(!std::is_assignable_v<linked_ptr<char>, linked_ptr<int>>);
  // don't look at incorrect dstr for int. Just need type checking
  static_assert(
      std::is_assignable_v<linked_ptr<char, std::default_delete<char>>, linked_ptr<int, std::default_delete<char>>>);
  static_assert(std::is_assignable_v<linked_ptr<destruction_tracker_base>, linked_ptr<destruction_tracker>>);
  static_assert(!std::is_assignable_v<linked_ptr<destruction_tracker>, linked_ptr<destruction_tracker_base>>);
  static_assert(std::is_assignable_v<linked_ptr<destruction_tracker_base, unifiedDestructorForBase>,
                                     linked_ptr<destruction_tracker, unifiedDestructorForBase>>);
}
