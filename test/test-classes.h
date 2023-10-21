#pragma once

#include "test-object.h"

struct non_copyable_tacker {
public:
  non_copyable_tacker() = default;
  non_copyable_tacker(const non_copyable_tacker&) = delete;

  void operator()(test_object* obj) {
    delete obj;
  }
};

template <typename T>
struct tracking_deleter {
  tracking_deleter() = default;

  explicit tracking_deleter(bool* deleted) : deleted(deleted) {}

  tracking_deleter(const tracking_deleter&) = default;
  tracking_deleter(tracking_deleter&&) = default;

  tracking_deleter operator=(const tracking_deleter&) = delete;
  tracking_deleter operator=(tracking_deleter&&) = delete;

  friend void swap(tracking_deleter& left, tracking_deleter& right) noexcept {
    std::swap(left.deleted, right.deleted);
  }

  void operator()(T* object) {
    if (!deleted) {
      return;
    }
    *deleted = true;
    delete object;
  }

private:
  bool* deleted{nullptr};
};

struct destruction_tracker_base {
  destruction_tracker_base() = default;

  explicit destruction_tracker_base(bool* deleted) : deleted(deleted) {}

  ~destruction_tracker_base() {
    *deleted = true;
  }

  destruction_tracker_base(const destruction_tracker_base&) = delete;
  destruction_tracker_base& operator=(const destruction_tracker_base&) = delete;

  destruction_tracker_base(destruction_tracker_base&&) = delete;
  destruction_tracker_base& operator=(destruction_tracker_base&&) = delete;

private:
  bool* deleted;
};

struct destruction_tracker : destruction_tracker_base {
  explicit destruction_tracker(bool* deleted) : destruction_tracker_base(deleted) {}

  ~destruction_tracker() = default;
};
