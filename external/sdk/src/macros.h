#ifndef macros_h
#define macros_h

#define NO_INIT(classname) \
  classname() = delete; \
  classname(const classname&) = delete; \
  classname(classname&&) = delete; \
  classname& operator=(const classname&) = delete;

#endif