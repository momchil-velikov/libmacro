// mode: c++; indent-tabs-mode: nil; -*-
#ifndef libmacro_hh__
#define libmacro_hh__ 1

#include <string>
#include <vector>

#ifdef _WIN32
#if defined(_LIBMACRO_STATIC)
#define _LIBMACRO_EXPORT
#else
#if defined(_LIBMACRO_DLL)
#define _LIBMACRO_EXPORT __declspec(dllexport)
#else
#define _LIBMACRO_EXPORT __declspec(dllimport)
#endif
#endif
#else
#define _LIBMACRO_EXPORT
#endif

namespace libmacro {
class macro_table;
class included_macros {
public:
  virtual const macro_table *get_macros() const = 0;
};

class macro_table {
public:
  macro_table() : in_use_(false) {}

  ~macro_table();

  struct define {
    std::string name;
    std::vector<std::string> params;
    std::string repl;
    mutable bool checked;
  };

  _LIBMACRO_EXPORT void add_define(unsigned int, const std::string &);
  _LIBMACRO_EXPORT void add_undefine(unsigned int, const std::string &);
  _LIBMACRO_EXPORT void add_include(unsigned int, const included_macros *);

  _LIBMACRO_EXPORT const define *find_define(unsigned int, const std::string &) const;

protected:
  struct undefine {
    std::string name;
  };

  class entry {
  public:
    entry();
    void destroy();

    enum { INVALID, DEFINE, UNDEFINE, INCLUDE } kind;
    unsigned int lineno;
    union {
      define *def;
      undefine *undef;
      const included_macros *include;
    };
  };

  entry *make_entry(unsigned int);
  std::vector<entry> table_;
  mutable bool in_use_;
};

std::string macro_expand(const std::string &input,
                         const macro_table *macros,
                         unsigned int lineno);
}  // end namespace
#endif  // libmacro_hh__
