#ifndef misc_h
#define misc_h

#include <cstdint>
#include <string>
#include <string_view>
#include <codecvt>
#include <locale>

#include "../macros.h"

namespace sdk {

template<typename type_>
class tarray {
  friend class fstring;
public:
  tarray() : data_(nullptr), count_(0), max_(0) {}
public:
  type_* data_;
  uint32_t count_;
  uint32_t max_;

public:
  auto at(const uint32_t index) -> type_* {
    if (index >= count_ || index < 0) {
      return nullptr;
    }

    return &data_[index];
  }

  auto operator[](const uint32_t index) const -> type_* {
    return &data_[index];
  }

  auto begin() const -> type_* {
    return data_;
  }

  auto end() const -> type_* {
    return data_ + count_;
  }
};

class fstring : public tarray<wchar_t> {
public:
  fstring() : tarray<wchar_t>() {}
  fstring(const wchar_t* str) {
    if (!str) {
      return;
    }

    count_ = static_cast<uint32_t>(wcslen(str));
    max_ = count_;
    data_ = new wchar_t[max_ + 1];
    wcscpy_s(data_, max_ + 1, str);
  }
  fstring(const std::wstring& str) {
    count_ = static_cast<uint32_t>(str.length());
    max_ = count_;
    data_ = new wchar_t[max_ + 1];
    wcscpy_s(data_, max_ + 1, str.c_str());
  }
  auto operator=(const wchar_t* str) -> fstring {
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

struct fname_entry_id
{
  uint32_t id_;
};

class fname {
public:
  NO_INIT(fname);

public:
  fname_entry_id index_;
  uint32_t number_;
};

}

#endif