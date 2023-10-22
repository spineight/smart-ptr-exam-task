#include "linked-ptr.h"
#include "shared-ptr.h"
#include "test-classes.h"

#include <gtest/gtest.h>

namespace {

using destruction_tracker_base_deleter = std::default_delete<destruction_tracker_base>;

template <typename IsShared>
class common_test : public ::testing::Test {
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

TYPED_TEST_SUITE(common_test, tested_extents, extent_name_generator);

} // namespace

TYPED_TEST(common_test, default_ctor) {
  typename TestFixture::template smart_ptr<test_object> p;
  EXPECT_EQ(nullptr, p.get());
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_test, ptr_ctor) {
  test_object* p = new test_object(42);
  typename TestFixture::template smart_ptr<test_object> q(p);
  EXPECT_TRUE(static_cast<bool>(q));
  EXPECT_EQ(p, q.get());
  EXPECT_EQ(42, *q);
}

TYPED_TEST(common_test, ptr_ctor_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p(nullptr);
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_EQ(0, p.use_count());
}

TYPED_TEST(common_test, ptr_ctor_non_empty_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p(static_cast<test_object*>(nullptr));
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_EQ(1, p.use_count());
}

TYPED_TEST(common_test, const_dereferencing) {
  const typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  EXPECT_EQ(42, *p);
  EXPECT_EQ(42, p->operator int());
}

TYPED_TEST(common_test, reset) {
  typename TestFixture::template smart_ptr<test_object> q(new test_object(42));
  EXPECT_TRUE(static_cast<bool>(q));
  q.reset();
  EXPECT_FALSE(static_cast<bool>(q));
}

TYPED_TEST(common_test, reset_nullptr) {
  typename TestFixture::template smart_ptr<test_object> q;
  EXPECT_FALSE(static_cast<bool>(q));
  q.reset();
  EXPECT_FALSE(static_cast<bool>(q));
}

TYPED_TEST(common_test, reset_ptr) {
  typename TestFixture::template smart_ptr<test_object> q(new test_object(42));
  EXPECT_TRUE(static_cast<bool>(q));
  q.reset(new test_object(43));
  EXPECT_EQ(43, *q);
}

TYPED_TEST(common_test, copy_ctor) {
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

TYPED_TEST(common_test, copy_ctor_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  typename TestFixture::template smart_ptr<test_object> q = p;
  EXPECT_FALSE(static_cast<bool>(p));
  EXPECT_FALSE(static_cast<bool>(q));
}

TYPED_TEST(common_test, copy_assignment_operator) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  typename TestFixture::template smart_ptr<test_object> q(new test_object(43));
  p = q;
  EXPECT_EQ(43, *p);
  EXPECT_TRUE(p == q);
}

TYPED_TEST(common_test, copy_assignment_operator_from_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  typename TestFixture::template smart_ptr<test_object> q;
  p = q;
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_test, copy_assignment_operator_to_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  typename TestFixture::template smart_ptr<test_object> q(new test_object(43));
  p = q;
  EXPECT_EQ(43, *p);
  EXPECT_TRUE(p == q);
}

TYPED_TEST(common_test, copy_assignment_operator_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  typename TestFixture::template smart_ptr<test_object> q;
  p = q;
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_test, copy_assignment_operator_self) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));
  p = p;
  EXPECT_EQ(42, *p);
}

TYPED_TEST(common_test, copy_assignment_operator_self_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;
  p = p;
  EXPECT_FALSE(static_cast<bool>(p));
}

TYPED_TEST(common_test, non_copyable_deleter) {
  typename TestFixture::template smart_ptr<test_object, non_copyable_tacker> p(new test_object(42));
}

TYPED_TEST(common_test, ptr_ctor_inheritance) {
  bool deleted = false;
  { typename TestFixture::template smart_ptr<destruction_tracker_base> p(new destruction_tracker(&deleted)); }
  EXPECT_TRUE(deleted);
}

TYPED_TEST(common_test, reset_ptr_inheritance) {
  bool deleted = false;
  {
    typename TestFixture::template smart_ptr<destruction_tracker_base> p;
    p.reset(new destruction_tracker(&deleted));
  }
  EXPECT_TRUE(deleted);
}

TYPED_TEST(common_test, custom_deleter) {
  bool deleted = false;
  {
    typename TestFixture::template smart_ptr<test_object, tracking_deleter<test_object>> p(
        new test_object(42), tracking_deleter<test_object>(&deleted));
  }
  EXPECT_TRUE(deleted);
}

TYPED_TEST(common_test, custom_deleter_reset) {
  bool deleted = false;
  {
    typename TestFixture::template smart_ptr<test_object, tracking_deleter<test_object>> p;
    p.reset(new test_object(42), tracking_deleter<test_object>(&deleted));
  }
  EXPECT_TRUE(deleted);
}

TEST(linked_ptr_test, copy_ctor_const) {
  linked_ptr<test_object, std::default_delete<const test_object>> p(new test_object(42));
  linked_ptr<const test_object> q = p;
  EXPECT_EQ(42, *q);
}

TEST(linked_ptr_test, copy_assignment_operator_const) {
  linked_ptr<test_object, std::default_delete<const test_object>> p(new test_object(42));
  linked_ptr<const test_object> q(new test_object(43));
  q = p;
  EXPECT_EQ(42, *q);
  EXPECT_EQ(42, *p);
}

TEST(linked_ptr_test, copy_assignment_operator_const_to_nullptr) {
  linked_ptr<test_object, std::default_delete<const test_object>> p(new test_object(42));
  linked_ptr<const test_object> q;
  q = p;
  EXPECT_EQ(42, *q);
  EXPECT_EQ(42, *p);
}

TEST(linked_ptr_test, copy_assignment_operator_const_from_nullptr) {
  linked_ptr<test_object, std::default_delete<const test_object>> p;
  linked_ptr<const test_object> q(new test_object(43));
  q = p;
  EXPECT_FALSE(static_cast<bool>(q));
  EXPECT_FALSE(static_cast<bool>(p));
}

TEST(linked_ptr_test, copy_ctor_inheritance) {
  bool deleted = false;
  {
    destruction_tracker* ptr = new destruction_tracker(&deleted);
    linked_ptr<destruction_tracker, destruction_tracker_base_deleter> d(ptr);
    {
      linked_ptr<destruction_tracker_base, destruction_tracker_base_deleter> b = d;
      EXPECT_EQ(ptr, b.get());
      EXPECT_EQ(ptr, d.get());
    }
    EXPECT_FALSE(deleted);
  }
  EXPECT_TRUE(deleted);
}

TEST(linked_ptr_test, copy_assignment_operator_inheritance) {
  bool derived_deleted = false;
  {
    destruction_tracker* ptr = new destruction_tracker(&derived_deleted);
    linked_ptr<destruction_tracker, destruction_tracker_base_deleter> d(ptr);
    bool base_deleted = false;
    {
      linked_ptr<destruction_tracker_base, destruction_tracker_base_deleter> b(
          new destruction_tracker_base(&base_deleted));
      b = d;
      EXPECT_EQ(ptr, b.get());
      EXPECT_EQ(ptr, d.get());
      EXPECT_FALSE(derived_deleted);
      EXPECT_TRUE(base_deleted);
    }
    EXPECT_FALSE(derived_deleted);
    EXPECT_TRUE(base_deleted);
  }
  EXPECT_TRUE(derived_deleted);
}

TYPED_TEST(common_test, equivalence) {
  typename TestFixture::template smart_ptr<test_object> p1(new test_object(42));
  typename TestFixture::template smart_ptr<test_object> p2(new test_object(43));
  auto p3 = p2;

  EXPECT_FALSE(p1 == p2);
  EXPECT_TRUE(p1 != p2);

  EXPECT_FALSE(p3 == p1);
  EXPECT_TRUE(p3 == p2);
}

TYPED_TEST(common_test, equivalence_self) {
  typename TestFixture::template smart_ptr<test_object> p(new test_object(42));

  EXPECT_TRUE(p == p);
  EXPECT_FALSE(p != p);
}

TYPED_TEST(common_test, equivalence_nullptr) {
  typename TestFixture::template smart_ptr<test_object> p;

  EXPECT_TRUE(p == nullptr);
  EXPECT_FALSE(p != nullptr);
  EXPECT_TRUE(nullptr == p);
  EXPECT_FALSE(nullptr != p);
}

TYPED_TEST(common_test, check_lifetime) {
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

TEST(traits_test, shared_prt_ctors) {
  static_assert(std::is_constructible_v<shared_ptr<int>, int*>);
  static_assert(std::is_constructible_v<shared_ptr<int>, int*, std::default_delete<int>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, double*, std::default_delete<double>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, int*, std::default_delete<double>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, double*, std::default_delete<int>>);

  static_assert(!std::is_constructible_v<shared_ptr<destruction_tracker>, destruction_tracker_base*>);
  static_assert(std::is_constructible_v<shared_ptr<destruction_tracker_base>, destruction_tracker*>);
  static_assert(std::is_constructible_v<shared_ptr<destruction_tracker_base>, destruction_tracker*,
                                        std::default_delete<destruction_tracker>>);
}

TEST(traits_test, linked_prt_ctors) {
  static_assert(std::is_constructible_v<linked_ptr<int>, int*>);
  static_assert(std::is_constructible_v<linked_ptr<int>, int*, std::default_delete<int>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, double*, std::default_delete<double>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, int*, std::default_delete<double>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, double*, std::default_delete<int>>);

  static_assert(!std::is_constructible_v<linked_ptr<destruction_tracker>, destruction_tracker_base*>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base>, destruction_tracker*>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base>, destruction_tracker*,
                                        std::default_delete<destruction_tracker>>);
}

TEST(traits_test, shared_ptr_copy_ctor) {
  static_assert(!std::is_constructible_v<shared_ptr<const int>, shared_ptr<int>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, shared_ptr<const int>>);
  static_assert(!std::is_constructible_v<shared_ptr<int>, shared_ptr<double>>);

  static_assert(!std::is_constructible_v<shared_ptr<destruction_tracker>, shared_ptr<destruction_tracker_base>>);
  static_assert(!std::is_constructible_v<shared_ptr<destruction_tracker_base>, shared_ptr<destruction_tracker>>);
  static_assert(!std::is_constructible_v<shared_ptr<destruction_tracker_base>,
                                         shared_ptr<destruction_tracker, destruction_tracker_base_deleter>>);
}

TEST(traits_test, linked_ptr_copy_ctor) {
  static_assert(std::is_constructible_v<linked_ptr<const int>, linked_ptr<int>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, linked_ptr<const int>>);
  static_assert(!std::is_constructible_v<linked_ptr<int>, linked_ptr<double>>);

  static_assert(!std::is_constructible_v<linked_ptr<destruction_tracker>, linked_ptr<destruction_tracker_base>>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base>, linked_ptr<destruction_tracker>>);
  static_assert(std::is_constructible_v<linked_ptr<destruction_tracker_base>,
                                        linked_ptr<destruction_tracker, destruction_tracker_base_deleter>>);
}

TEST(traits_test, shared_ptr_assignment) {
  static_assert(!std::is_assignable_v<shared_ptr<const int>, shared_ptr<int>>);
  static_assert(!std::is_assignable_v<shared_ptr<int>, shared_ptr<const int>>);
  static_assert(!std::is_assignable_v<shared_ptr<int>, shared_ptr<double>>);

  static_assert(!std::is_assignable_v<shared_ptr<destruction_tracker>, shared_ptr<destruction_tracker_base>>);
  static_assert(!std::is_assignable_v<shared_ptr<destruction_tracker_base>, shared_ptr<destruction_tracker>>);
  static_assert(!std::is_assignable_v<shared_ptr<destruction_tracker_base>,
                                      shared_ptr<destruction_tracker, destruction_tracker_base_deleter>>);
}

TEST(traits_test, linked_ptr_assignment) {
  static_assert(std::is_assignable_v<linked_ptr<const int>, linked_ptr<int>>);
  static_assert(!std::is_assignable_v<linked_ptr<int>, linked_ptr<const int>>);
  static_assert(!std::is_assignable_v<linked_ptr<int>, linked_ptr<double>>);

  static_assert(!std::is_assignable_v<linked_ptr<destruction_tracker>, linked_ptr<destruction_tracker_base>>);
  static_assert(std::is_assignable_v<linked_ptr<destruction_tracker_base>, linked_ptr<destruction_tracker>>);
  static_assert(std::is_assignable_v<linked_ptr<destruction_tracker_base>,
                                     linked_ptr<destruction_tracker, destruction_tracker_base_deleter>>);
}
