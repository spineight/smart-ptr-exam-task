#pragma once

#include <cstddef>
#include <memory>

template <typename T, typename Deleter = std::default_delete<T>>
class shared_ptr {
public:
  shared_ptr() noexcept;

  ~shared_ptr();

  shared_ptr(std::nullptr_t) noexcept;

  explicit shared_ptr(T* ptr);

  shared_ptr(T* ptr, Deleter deleter);

  shared_ptr(const shared_ptr& other) noexcept;

  shared_ptr& operator=(const shared_ptr& other) noexcept;

  T* get() const noexcept;

  explicit operator bool() const noexcept;

  T& operator*() const noexcept;

  T* operator->() const noexcept;

  std::size_t use_count() const noexcept;

  void reset() noexcept;

  void reset(T* new_ptr);

  void reset(T* new_ptr, Deleter deleter);

  friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) noexcept;

  friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) noexcept;

private:
};
