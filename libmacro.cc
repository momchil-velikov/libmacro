// -*- mode: c++; indent-tabs-mode: nil;
#include "libmacro.hh"
#include <algorithm>
#include <cassert>

namespace libmacro {

namespace {
// Parse a macro define string, with syntax as specified by Sec
// 6.3.1.1 of the DWARF4 standard.
// Note: an empty arguments list (|#define foo() bar|) is
// representedby a vector containing one empty string. Macro with no
// arguments (|#define foo bar|) is represented with an empty vector.
void
parse_macro_def(const std::string &def, std::string &name,
                std::vector<std::string> &args, std::string &repl) {
  // First space (if any) separates macro name and args from the
  // expansion text.
  size_t p = def.find(' ');
  if (p == std::string::npos) {
    // No space, hence a simple macro name definition, without args
    // and empty replacement.
    name = def;
    return;
  }

  // The replacement list follows the first space.
  repl = def.substr(p + 1);

  if (def[p - 1] == ')') {
    // Argument list is present.
    p = def.find('(');

    // Split argument names.
    size_t start = p, len;
    do {
      ++start;
      len = 0;
      while (def[start + len] != ',' && def[start + len] != ')')
        ++len;
      args.push_back(def.substr(start, len));
      start += len;
    } while (def[start] != ')');
  }

  // Separate the macro name.
  assert (def[p] == ' ' || def[p] == '(');
  name = def.substr(0, p);
}
} // end namespace

macro_table::~macro_table() {
  for (std::vector<entry>::iterator i = table_.begin(); i != table_.end(); ++i)
    i->destroy();
}

macro_table::entry *
macro_table::make_entry(unsigned int lineno) {
  // Shortcut for the common case of entries made in increasing line
  // number order.
  if (table_.size() == 0 || table_.back().lineno <= lineno) {
    table_.resize(table_.size() + 1);
    return &table_.back();
  }

  // Slow path
  table_.resize(table_.size() + 1);
  size_t i = table_.size() - 1;
  while (i > 0 && table_[i - 1].lineno > lineno) {
    std::swap(table_[i], table_[i - 1]);
    --i;
  }

  return &table_[i];
}

void
macro_table::add_define(unsigned int lineno, const std::string &def) {
  entry *e = make_entry(lineno);
  e->kind = entry::DEFINE;
  e->lineno = lineno;
  e->def = new define;
  parse_macro_def(def, e->def->name, e->def->args, e->def->repl);
}

void
macro_table::add_undefine(unsigned int lineno, const std::string &name) {
  entry *e = make_entry(lineno);
  e->kind = entry::UNDEFINE;
  e->lineno = lineno;
  e->undef = new undefine;
  e->undef->name = name;
}

void
macro_table::add_include(unsigned int lineno, const included_macros *nested) {
  entry *e = make_entry(lineno);
  e->kind = entry::INCLUDE;
  e->lineno = lineno;
  e->include = nested;
}

macro_table::entry::entry() : kind(INVALID), lineno(0), def(0) {}

void
macro_table::entry::destroy() {
  switch(kind) {
  case DEFINE:
    delete def;
    break;
  case UNDEFINE:
    delete undef;
    break;
  default:
  case INVALID:
    break;
  }
}

const macro_table::define *
macro_table::find_define(unsigned int lineno, const std::string &name) const {
  if (table_.size() == 0)
    return 0;

  size_t idx;
  if (lineno > 0) {
    // Binary search in the macros table for the smallest line number,
    // greater than or equal to the given one.
    size_t lower = 0, upper = table_.size() - 1;
    while (lower < upper) {
      size_t m = (lower + upper) / 2;
      if (table_[m].lineno >= lineno)
        upper = m;
      else
        lower = m + 1;
    }

    if (lower > upper)
      return 0;

    assert(table_[lower].lineno >= lineno);
    idx = lower;
  } else {
    idx = table_.size();
  }

  // Examine the macro entries from the next smaller index downwards.
  while(idx-- > 0) {
    switch(table_[idx].kind) {
    case entry::DEFINE:
      // Found a macro definition for the name
      if (table_[idx].def->name == name)
        return table_[idx].def;
      break;
    case entry::UNDEFINE:
      // Found an undefine directive; terminate search.
      if (table_[idx].undef->name == name)
        return 0;
      break;
    case entry::INCLUDE:
      // Search among the directives in the included file
      if (const define *d =
          table_[idx].include->get_macros()->find_define(0, name))
        return d;
      break;
    }
  }

  // Macro definition now found.
  return 0;
}

} // end namespace
