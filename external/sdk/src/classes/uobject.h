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
  uint32_t max_elements_;
  uint32_t num_elements_;

public:
  auto find(const uint32_t index) -> fuobject_item* {
    if (index >= this->num_elements_ || index < 0) {
      return nullptr;
    }

    return &object_array_[index];
  }

  auto find_object(const uint32_t index) -> uobject* {
    auto item = this->find(index);
    if (!item) {
      return nullptr;
    }

    return item->object;
  }

  auto size() const -> uint32_t {
    return this->num_elements_;
  }

  auto max_size() const -> uint32_t {
    return this->max_elements_;
  }
};

class fchunked_object_array {
public:
  NO_INIT(fchunked_object_array);

public:
  enum
  {
    elements_per_chunk = 64 * 1024,
  };

  fuobject_item** items_;
  fuobject_item* allocated_items_;
  uint32_t max_elements_;
  uint32_t num_elements_;
  uint32_t max_chunks_;
  uint32_t num_chunks_;

public:
  auto find(const uint32_t index) -> fuobject_item* {
    if (index > this->num_elements_ || index < 0) {
      return nullptr;
    }

    return &items_[index / elements_per_chunk][index % elements_per_chunk];
  }

  auto find_object(const uint32_t index) -> uobject* {
    auto item = this->find(index);
    if (!item) {
      return nullptr;
    }

    return item->object;
  }

  auto size() const -> uint32_t {
    return this->num_elements_;
  }

  auto max_size() const -> uint32_t {
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
public:
  auto find(const uint32_t index) -> fuobject_item* {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->find(index);
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->find(index);
    }
  }

  auto find_object(const uint32_t index) -> uobject* {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->find_object(index);
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->find_object(index);
    }
  }

  auto size() const -> uint32_t {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->size();
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->size();
    }
  }

  auto max_size() const -> uint32_t {
    if (is_default_array_) {
      auto default_array = reinterpret_cast<fdefault_uobject_array*>(array_);
      return default_array->max_size();
    } else {
      auto chunked_array = reinterpret_cast<fchunked_object_array*>(array_);
      return chunked_array->max_size();
    }
  }

  auto begin() -> fuobject_item* {
    return this->find(0);
  }

  auto end() -> fuobject_item* {
    return this->find(this->size());
  }
};

}

#endif