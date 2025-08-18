#ifndef misc_h
#define misc_h

#include <cstdint>
#include <string>
#include <string_view>
#include <codecvt>
#include <locale>

#include "../macros.h"
#include "../memory.h"

#include "../../../include/logger.h"

namespace sdk {

template<typename type_>
class tarray {
  friend class fstring;
public:
  tarray() : data_(nullptr), count_(0), max_(0) {}
public:
  type_* data_;
  int32_t count_;
  int32_t max_;

public:
  auto at(const int32_t index) -> type_* {
    if (index >= count_ || index < 0) {
      return nullptr;
    }

    return &data_[index];
  }

  auto reserve(const int32_t count, size_t size = sizeof(type_)) -> void {
    data_ = static_cast<type_*>(fmemory::get().realloc(data_, (max_ = count + count_) * size));
  }

  auto add(const type_& item, size_t size = sizeof(type_)) -> type_& {
    reserve(count_ + 1, size);
    std::memcpy(&data_[count_], &item, size);
    count_++;
    return data_[count_ - 1];
  }

  auto remove(const int32_t index) -> void {
    if (index >= count_ || index < 0) {
      return;
    }

    if (index < count_ - 1) {
      std::memmove(&data_[index], &data_[index + 1], (count_ - index - 1) * sizeof(type_));
    }

    count_--;
  }

  auto size() const -> int32_t {
    return count_;
  }

  auto max_size() const -> int32_t {
    return max_;
  }

  auto operator[](const int32_t index) const -> type_* {
    return &data_[index];
  }

  auto begin() const -> type_* {
    return data_;
  }

  auto end() const -> type_* {
    return data_ + count_;
  }

  auto free() -> void {
    if (!data_) return;
    fmemory::get().free(data_);
    data_ = nullptr;
    count_ = 0;
    max_ = 0;
  }
};

class fstring : public tarray<wchar_t> {
public:
  fstring() : tarray<wchar_t>() {
    LOG("fstring default constructor called");
  }
  
  fstring(const wchar_t* str) {
    LOG("fstring constructor called with str");

    max_ = count_ = *str ? static_cast<uint32_t>(wcslen(str)) + 1 : 0;
    if (count_ && str) {
      data_ = static_cast<wchar_t*>(fmemory::get().malloc(count_ * 2));
      memcpy_s(data_, count_ * 2, str, count_ * 2);
    }
  }
  fstring(const std::wstring& str) {
    LOG("fstring constructor called with wstring");
    max_ = count_ = static_cast<uint32_t>(str.size()) + 1;
    if (count_) {
      data_ = static_cast<wchar_t*>(fmemory::get().malloc(count_ * 2));
      memcpy_s(data_, count_ * 2, str.data(), count_ * 2);
    }
  }
  auto operator=(const wchar_t* str) -> fstring {
    LOG("fstring assignment operator called with str");
    return fstring(str);
  }

public:
  auto to_wstring() const -> std::wstring {
    return std::wstring(data_, count_);
  }

  auto to_string() const -> std::string {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(to_wstring());
  }

  auto to_string_view() const -> std::string_view {
    return std::string_view(reinterpret_cast<const char*>(data_), count_ * sizeof(wchar_t));
  }

  auto to_wstring_view() const -> std::wstring_view {
    return std::wstring_view(data_, count_);
  }
};

class fname {
public:
  auto operator==(const fname& other) const -> bool {
    return index_ == other.index_;
  }

  auto operator!=(const fname& other) const -> bool {
    return !(*this == other);
  }

  auto to_string() const -> std::string;
public:
  uint32_t index_;
  uint32_t number_;
};

}

#endif