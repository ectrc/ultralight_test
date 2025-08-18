#ifndef finder_h
#define finder_h

#include <cstdint>
#include <expected>
#include <ranges>
#include <cctype>
#include <algorithm>
#include <span>
#include <vector>

#include <libhat.hpp>

struct address {
  uintptr_t address_;

  template <typename T>
  [[nodiscard]] auto as() -> T {
    return (T)(address_);
  }

  [[nodiscard]] auto get() const -> uintptr_t {
    return address_;
  }

  template<std::integral Int>
  [[nodiscard]] constexpr Int read(size_t offset) const {
    return *reinterpret_cast<const Int*>(this->address_ + offset);
  }

  [[nodiscard]] auto rel(size_t offset) const -> address {
    return address_ == 0 ? address{0} : address{address_ + this->read<int32_t>(offset) + offset + sizeof(int32_t)};
  }
};

namespace sdk {
  enum class find_error {
    none,
    pattern_not_found,
    offset_not_found
  };

  auto find_pattern(const hat::signature_view& signature, size_t offset = 0) -> std::expected<uintptr_t, sdk::find_error>;
  auto find_pattern_all(const hat::signature_view& signature, size_t offset = 0) -> std::vector<uintptr_t>;
  
  enum class find_direction {
    forward,
    backward,
    both
  };
  auto find_pattern_near(uintptr_t start, const hat::signature_view& signature, size_t offset = 0, find_direction direction = find_direction::forward, size_t search_offset = 0x10, uint32_t recursive_calls = 3) -> std::expected<uintptr_t, sdk::find_error>;

  auto find_offset(const char* name) -> std::expected<uintptr_t, sdk::find_error>;
  
  template<typename T>
  auto find_string(const T* str) -> uintptr_t {
    const auto result = find_pattern(hat::string_to_signature(std::basic_string_view<T>(str)));
    return result.value_or(0);
  }

  template<typename T>
  auto find_string_ref(const T* str) -> uintptr_t {
    address string_addr = {find_string(str)};

    auto process_pattern = [&string_addr](const std::vector<uintptr_t>& search) -> address {
      std::vector<address> addresses;
      addresses.reserve(search.size());
      for (const auto& addr : search) {
        addresses.emplace_back(address{addr});
      }

      auto it = std::ranges::find_if(addresses, [&string_addr](const auto& found) {
        return found.rel(3).get() == string_addr.get();
      });

      return it != addresses.end() ? *it : address{0};
    };
    
    const auto sigs = {
      hat::compile_signature<"48 8D 05 ?? ?? ?? ??">(),
      hat::compile_signature<"4C 8D 05 ?? ?? ?? ??">(),
      hat::compile_signature<"4C 8D 15 ?? ?? ?? ??">()
    };

    for (const auto& search : sigs) {
      std::vector<uintptr_t> results = find_pattern_all(search);
      if (results.empty()) {
        continue;
      }

      address result = process_pattern(results);
      if (result.get() != 0) {
        return result.get();
      }
    }

    return 0;
  }
}

#endif