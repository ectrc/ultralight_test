#ifndef uobject_h
#define uobject_h

#include <cstdint>
#include <variant>

#include "macros.h"

namespace sdk {

class uobject {
public:
  NO_INIT(uobject);

public:
  uintptr_t** vtable_;
  uint32_t flags_;
  uint32_t index_;
  class uclass* class_ptr_;
  class fname* name_;
  uobject* outer_;
};

class ufield : public uobject {
public:
  ufield* next_;
};

class uproperty : public ufield {

};

class uboolproperty : public uproperty {

};

class ustruct : public ufield {

};

class uclass : public ustruct {

};

class ufunction : public ustruct {

};

struct fuobject_item {
  uobject* object;
  uint32_t flags;
  uint32_t root_index;
  uint32_t serial_number;
};

class fdefault_uobject_array {
public:
  NO_INIT(fdefault_uobject_array);

public:
  fuobject_item* object_array_;
  int32_t max_elements_;
  int32_t num_elements_;

public:
  auto find(const int32_t index) -> fuobject_item {
    if (index >= this->num_elements_ || index < 0) {
      return {nullptr, 0, 0, 0};
    }

    return object_array_[index];
  }

  auto find_object(const int32_t index) -> uobject* {
    auto item = this->find(index);
    if (!item.object) {
      return nullptr;
    }

    return item.object;
  }

  auto size() const -> int32_t {
    return this->num_elements_;
  }

  auto max_size() const -> int32_t {
    return this->max_elements_;
  }
};

class fchunked_object_array {
public:
  NO_INIT(fchunked_object_array);

public:
  static constexpr int32_t elements_per_chunk = 64 * 1024;

  fuobject_item** items_;
  fuobject_item* allocated_items_;
  int32_t max_elements_;
  int32_t num_elements_;
  int32_t max_chunks_;
  int32_t num_chunks_;

public:
  auto find(const int32_t index) -> fuobject_item {
    if (index > this->num_elements_ || index < 0) {
      return {nullptr, 0, 0, 0};
    }

    const int32_t chunk_index = index / elements_per_chunk;
    const int32_t item_index = index % elements_per_chunk;

    return items_[chunk_index][item_index];
  }

  auto find_object(const int32_t index) -> uobject* {
    auto item = this->find(index);
    if (!item.object) {
      return nullptr;
    }

    return item.object;
  }

  auto size() const -> int32_t {
    return this->num_elements_;
  }

  auto max_size() const -> int32_t {
    return this->max_elements_;
  }
};

class uobject_array {
public:
  uobject_array(uintptr_t array, bool is_default_array = true)
    : array_(reinterpret_cast<fobject_array*>(array)), is_default_array_(is_default_array) {}
public:
  typedef std::variant<fdefault_uobject_array, fchunked_object_array> fobject_array;
  
  fobject_array* array_;
  bool is_default_array_;

  class iterator {
  public:
    iterator(uobject_array* parent, int32_t index)
      : parent_(parent), index_(index) {}

    auto operator*() const -> fuobject_item {
      return parent_->find(index_);
    }

    auto operator++() -> iterator& {
      ++index_;
      return *this;
    }

    auto operator!=(const iterator& other) const -> bool {
      return index_ != other.index_;
    }

  private:
    uobject_array* parent_;
    int32_t index_;
  };

  auto begin() -> iterator {
    return iterator(this, 0);
  }

  auto end() -> iterator {
    return iterator(this, this->size());
  }
public:
  auto find(const int32_t index) -> fuobject_item {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->find(index);
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->find(index);
    }
  }

  auto find_object(const int32_t index) -> uobject* {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->find_object(index);
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->find_object(index);
    }
  }

  auto size() const -> int32_t {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->size();
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->size();
    }
  }

  auto max_size() const -> int32_t {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->max_size();
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->max_size();
    }
  }

  auto chunks() const -> uint32_t {
    if (is_default_array_) {
      return 1;
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->num_chunks_;
    }
  }

  auto max_chunks() const -> uint32_t {
    if (is_default_array_) {
      return 1;
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->max_chunks_;
    }
  }

};

}

#endif