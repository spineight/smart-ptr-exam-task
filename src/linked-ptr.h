#pragma once

#include <cstddef>
#include <memory>

template <typename T, typename Deleter = std::default_delete<T>>
class linked_ptr {
public:
  linked_ptr() noexcept;

  ~linked_ptr();

  linked_ptr(std::nullptr_t) noexcept;

  explicit linked_ptr(T* ptr);

  linked_ptr(T* ptr, Deleter deleter);

  linked_ptr(const linked_ptr& other) noexcept;

  template <typename Y, typename Del>
  linked_ptr(const linked_ptr<Y, Del>& other) noexcept;

  linked_ptr& operator=(const linked_ptr& other) noexcept;

  template <typename Y, typename Del>
  linked_ptr& operator=(const linked_ptr<Y, Del>& other) noexcept;

  T* get() const noexcept;

  explicit operator bool() const noexcept;

  T& operator*() const noexcept;

  T* operator->() const noexcept;

  std::size_t use_count() const noexcept;

  void reset() noexcept;

  void reset(T* new_ptr);

  void reset(T* new_ptr, Deleter deleter);

  friend bool operator==(const linked_ptr& lhs, const linked_ptr& rhs) noexcept;

  friend bool operator!=(const linked_ptr& lhs, const linked_ptr& rhs) noexcept;

private:
};
