#ifndef uobject_h
#define uobject_h

#include <cstdint>

namespace sdk {

class uobject {
public:
  uobject() = delete;
  uobject(const uobject&) = delete;
  uobject(uobject&&) = delete;
  uobject& operator=(const uobject&) = delete;

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

}

#endif