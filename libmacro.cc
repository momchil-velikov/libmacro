// -*- mode: c++; indent-tabs-mode: nil;
#include "libmacro.hh"
#include <algorithm>
#include <cassert>
#include <locale>
#include <list>

namespace libmacro {

namespace {

std::locale C_locale("C");

// Parse a macro define string, with syntax as specified by Sec
// 6.3.1.1 of the DWARF4 standard.
// Note: an empty parameter list (|#define foo() bar|) is
// representedby a vector containing one empty string. Macro with
// parameters (|#define foo bar|) is represented with an empty vector.
void
parse_macro_def(const std::string &def, std::string &name,
                std::vector<std::string> &params, std::string &repl) {
  // First space (if any) separates macro name and parameters from the
  // expansion text.
  auto p = def.find(' ');
  if (p == std::string::npos) {
    // No space, hence a simple macro name definition, without
    // parameters and empty replacement.
    name = def;
    return;
  }

  // The replacement list follows the first space.
  repl = def.substr(p + 1);

  if (def[p - 1] == ')') {
    // Parameter list is present.
    p = def.find('(');

    // Split parameter names.
    size_t start = p, len;
    do {
      ++start;
      len = 0;
      while (def[start + len] != ',' && def[start + len] != ')')
        ++len;
      auto begin = def.cbegin() + start, end = begin + len;
      params.emplace_back(begin, end);
      start += len;
    } while (def[start] != ')');
  }

  // Separate the macro name.
  assert (def[p] == ' ' || def[p] == '(');
  name = def.substr(0, p);
}

inline bool
isoctal(char ch) {
  return ch >= '0' && ch <= '7';
}

// Scan a sequence of between one and three octal digits.
std::string::const_iterator
scan_oct_seq(std::string::const_iterator str, std::string::const_iterator end) {
  assert(str != end && isoctal(*str));
  ++str;
  if (str != end && isoctal(*str)) {
    ++str;
    if (str != end && isoctal(*str))
      ++str;
  }
  return str;
}

// Scan a sequence of one or two hexadecimal digits.
std::string::const_iterator
scan_hex_seq(std::string::const_iterator str, std::string::const_iterator end) {
  assert(str != end && std::isxdigit(*str, C_locale));
  ++str;
  if (str != end && std::isxdigit(*str, C_locale))
    ++str;
  return str;
}

// Scan a character escape sequence
std::string::const_iterator
scan_escape_seq(std::string::const_iterator str,
                std::string::const_iterator end) {

  auto err = str;

  assert(str != end && *str == '\\');
  ++str;
  if (str == end)
    return err;

  switch (*str) {
  case '\'':
  case '"':
  case '\\':
  case '\?':
  case 'a':
  case 'b':
  case 'f':
  case 'n':
  case 'r':
  case 't':
  case 'v':
    return ++str;
  case 'x':
    ++str;
    if (str == end)
      return err;
    else
      return scan_hex_seq(str, end);
  }

  if (isoctal(*str))
    return scan_oct_seq(str, end);

  return err;
}

// Scan a character constant.
std::string::const_iterator
scan_character_constant(std::string::const_iterator str,
                        std::string::const_iterator end) {
  std::string::const_iterator err = str;
  std::string::const_iterator next;
  assert(str != end && *str == '\'');
  ++str;
  if (str == end)
    return err;

  if (*str == '\\') {
    next = scan_escape_seq(str, end);
    if (next == str)
      return err;
    else
      str = next;
  } else {
    ++str;
  }

  if (str == end || *str != '\'')
    return err;
  else
    return ++str;
}

// Scan a string literal.
std::string::const_iterator
scan_string_literal(std::string::const_iterator str,
                    std::string::const_iterator end) {
  auto err = str;

  assert(str != end && *str == '"');
  ++str;

  while(str != end && *str != '"') {
    if (*str == '\\') {
      auto next = scan_escape_seq(str, end);
      if (next == str)
        return err;
      str = next;
    } else {
      ++str;
    }
  }

  assert(str == end || *str == '"');
  if (str == end)
    return err;
  else
    return ++str;
}

// Scan a preprocessing number
// pp-number:
//     digit
//     . digit
//     pp-number digit
//     pp-number identifier-nondigit
//     pp-number e sign
//     pp-number E sign
//     pp-number p sign
//     pp-number P sign
//     pp-number .
std::string::const_iterator
scan_pp_number(std::string::const_iterator str,
               std::string::const_iterator end) {
  assert(str != end && std::isdigit(*str, C_locale));
  while (str != end) {
    if (*str == 'e' || *str == 'E' || *str == 'p' || *str == 'P') {
      auto next = str;
      ++next;
      if (next != end && (*next == '+' || *next == '-'))
        str = next;
      ++str;
    } else if (std::isdigit(*str, C_locale)
               || std::isalpha(*str, C_locale)
               || *str == '.') {
      ++str;
    } else {
      break;
    }
  }
  return str;
}

struct token {
  enum kind { ID, STRINGIFY, PASTE, PLACEMARKER, END, OTHER } kind;
  bool ws;
  bool noexpand;
  size_t pop;
  std::string text;
};

// Scan a preprocessing token
// preprocessing-token:
//     header-name
//     identifier
//     pp-number
//     character-constant
//     string-literal
//     punctuator
//     each non-white-space character that cannot be one of the above
std::string::const_iterator
scan_pp_token(std::string::const_iterator str,
              std::string::const_iterator end, enum token::kind &kind,
              size_t &ws) {
  // Advance past the leading whitespace.
  ws = 0;
  while (str != end && std::isspace(*str, C_locale)) {
    ++ws;
    ++str;
  }

  if (str == end) {
    kind = token::END;
    return end;
  }

  kind = token::OTHER;
  if (std::isdigit(*str, C_locale)) {
    // Scan pp-number
    return scan_pp_number(str, end);
  } else if (*str == '\'') {
    // Scan character constant.
    return scan_character_constant(str, end);
  } else if (*str == '"') {
    // Scan string literal.
    return scan_string_literal(str, end);
  } else if (*str == '_' || std::isalpha(*str, C_locale)) {
    // Scan identifier.
    kind = token::ID;
    ++str;
    while (str != end && (*str == '_' || std::isalnum(*str, C_locale)))
      ++str;
    return str;
  }

  // Punctuator or other non-whitespace character.
  switch (*str) {
  default:
  case '[': case ']': case '(': case ')': case '{': case '}':
  case '?': case ',': case ';':
    assert(!std::isspace (*str, C_locale));
    ++str;
    break;
  case '-':
    ++str;
    if (str != end && (*str == '-' || *str == '=' || *str == '>'))
      ++str;
    break;
  case '+':
    ++str;
    if (str != end && (*str == '+' || *str == '='))
      ++str;
    break;
  case '&':
    ++str;
    if (str != end && (*str == '&' || *str == '='))
      ++str;
    break;
  case '*': case '~': case '!': case '/': case '=': case '^':
    ++str;
    if (str != end && *str == '=')
      ++str;
    break;
  case '%':
    ++str;
    if (str != end) {
      if (*str == '=' || *str == '>')
        ++str;
      else if (*str == ':') {
        ++str;
        if (end - str > 1 && *str == '%' && *(str + 1) == ':')
          str += 2;
      }
    }
    break;
  case '<':
    ++str;
    if (str != end) {
      if (*str == ':' || *str == '%' || *str == '=')
        ++str;
      else if (*str == '<') {
        ++str;
        if (str != end && *str == '=')
          ++str;
      }
    }
    break;
  case '>':
    ++str;
    if (str != end) {
      if (*str == '=')
        ++str;
      else if (*str == '>') {
        ++str;
        if (str != end && *str == '=')
          ++str;
      }
    }
    break;
  case '|':
    ++str;
    if (str != end && (*str == '|' || *str == '='))
      ++str;
    break;
  case ':':
    ++str;
    if (str != end && *str == '>')
      ++str;
    break;
  case '.':
    ++str;
    if (str != end) {
      if (std::isdigit(*str, C_locale))
        str = scan_pp_number(str, end);
      else if (end - str > 1 && *str == '.' && *(str + 1) == '.')
        str += 2;
    }
    break;
  case '#':
    kind = token::STRINGIFY;
    ++str;
    if (str != end && *str == '#') {
      kind = token::PASTE;
      ++str;
    }
    break;
  }
  return str;
}

// Type for a list of tokens.
typedef std::list<token> token_list;

// Tokenize a string.
token_list
tokenize(const std::string &str, bool func_like, bool replacement = true) {
  token_list tokens;
  auto curr = str.cbegin();
  while (curr != str.cend()) {
    token t;
    size_t ws;
    auto next = scan_pp_token(curr, str.cend(), t.kind, ws);
    t.noexpand = false;
    t.pop = 0;
    t.ws = ws != 0;
    if (curr == next)
      throw "Invalid preprocessing token";
    assert(t.kind != token::PLACEMARKER);
    switch (t.kind) {
    case token::ID:
    case token::OTHER:
      t.text = std::string(curr + ws, next);
      tokens.push_back(t);
      break;
    case token::STRINGIFY:
      if (!replacement || !func_like) {
        t.kind = token::OTHER;
        t.text = std::string(curr + ws, next);
      }
      tokens.push_back(t);
      break;
    case token::PASTE:
      if (!replacement) {
        t.kind = token::OTHER;
        t.text = std::string(curr + ws, next);
      }
      tokens.push_back(t);
      break;
    default:
      break;
    }
    curr = next;
  }
  return tokens;
}

// Verify compliance of a replacement token list with the C11
// requirements.
void
verify_replacement_tokens(const macro_table::define *def,
                          const token_list &tokens) {
  if (tokens.empty())
    return;
  // A ## preprocessing token shall not occur at the beginning or at the
  // end of a replacement (C11 6.10.3.3 #1).
  if (tokens.front().kind == token::PASTE || tokens.back().kind == token::PASTE)
    throw "## cannot appear at either end of a macro";
  for(auto i = tokens.cbegin(); i != tokens.cend(); ++i) {
    // The identifier __VA_ARGS__ shall occur only in the
    // replacement-list of a function-like macro that uses the ellipsis
    // notation in the parameters (C11 6/10.3.1 #2).
    if (i->kind == token::ID && i->text == "__VA_ARGS__") {
      if (def->params.size() == 0 || def->params.back() != "...")
        throw "__VA_ARGS__ can only appear in a variadic macro";
    } else if (i->kind == token::STRINGIFY) {
      // Each # preprocessing token in the replacement list for a
      // function-like macro shall be followed by a parameter as the
      // next preprocessing token in the replacement list
      // (C11 6.10.3.2).
      auto next = i;
      ++next;
      if (next == tokens.end()
          || next->kind != token::ID
          || (next->text != "__VA_ARGS__"
              && (std::find(def->params.cbegin(), def->params.cend(), next->text)
                  == def->params.cend()))) {
        throw "# is not followed by a macro parameter";
      }
    }
  }
}

// Tokenize a macro definition.
token_list
tokenize(const macro_table::define *def) {
  auto r = tokenize(def->repl, def->params.size() != 0);
  if (!def->checked) {
    verify_replacement_tokens(def, r);
    def->checked = true;
  }
  return r;
}

token_list::iterator
gather_arguments(std::vector<std::string> &blacklist,
                 token_list &tokens,
                 token_list::iterator begin,
                 bool variadic,
                 size_t n,
                 std::vector<token_list> &args) {
  assert(begin != tokens.end() && begin->text == "(");
  auto level = 0U;
  auto next = begin;
  while (next != tokens.end()) {
    assert(blacklist.size() >= next->pop);
    if (next->pop) {
      blacklist.resize(blacklist.size() - next->pop);
      next->pop = 0;
    }
    if (next->text == "(") {
      // Increment nesting level.
      ++level;
      if (level == 1) {
        begin = next;
        ++begin;
      }
    } else if (next->text == ")") {
      // Decrement nesting level. If it becomes zero, then we have
      // collected the last argument. Return the token following the
      // closing parenthesis.
      --level;
      if (level == 0) {
        args.resize(args.size() + 1);
        args.back().splice(args.back().begin(), tokens, begin, next);
        return ++next;
      } 
    } else if (next->text == ",") {
      // Comma at nesting level one is argument separator.
      if (level == 1 && (!variadic || args.size() + 1 < n)) {
        args.resize(args.size() + 1);
        args.back().splice(args.back().begin(), tokens, begin, next);
        begin = next;
        ++begin;
      }
    }
    ++next;
  }
  // We failed to find the closing parenthesis.
  throw "Missing closing parenthesis";
}

// Convert a token to a string, as for stringification.
void
stringify(const token &t, std::string &out) {
  switch (t.kind) {
  case token::OTHER:
    if (t.text[0] == '"' || t.text[0] == '\'') {
      for (auto ch : t.text) {
        if (ch == '"')
          out += "\\\"";
        else if (ch == '\\')
          out += "\\\\";
        else
          out += ch;
      }
    } else {
      out += t.text;
    }
    break;
  case token::ID:
    out += t.text;
    break;
  case token::STRINGIFY:
    out += "#";
    break;
  case token::PASTE:
    out += "##";
    break;
  default:
    break;
  }
}

// Convert a list of tokens to a string constant.
std::string
stringify(const token_list &tokens) {
  std::string res;
  res += '"';
  auto t = tokens.cbegin();
  // Output the first token without any leading whitespace.
  if (t != tokens.cend()) {
    stringify(*t, res);
    ++t;
  }
  while (t != tokens.cend()) {
    if (t->ws) {
      // Whitespace between tokens is output as a single space
      // character.
      res += ' ';
    }
    stringify(*t, res);
    ++t;
  }
  res += '"';
  return res;
}

std::vector<std::string>::const_iterator
find(const std::vector<std::string> &params, const std::string &name) {
  if (name == "__VA_ARGS__")
    // Correctness is ensured by the verification at tokenize time.
    return params.cbegin() + (params.size() - 1);
  else
    return std::find(params.cbegin(), params.cend(), name);
}

// Perform parameter substitution (including inserting placemarkers)
// and stringification.
void macro_expand(std::vector<std::string> &, const macro_table *, unsigned int,
                  token_list &);
void
substitute_parameters(std::vector<std::string> &blacklist,
                      const std::vector<token_list> &args,
                      const std::vector<std::string> &params,
                      const macro_table *macros,
                      unsigned int lineno,
                      token_list &repl) {
  std::vector<std::string>::const_iterator p;
  token_list::iterator prev, curr, next;
  curr = repl.begin();
  while(curr != repl.end()) {
    if (curr->kind == token::ID) {
      // Check if the identifier is a macro parameter.
      if ((p = find(params, curr->text)) != params.end()) {
        // Make a copy of the argument tokens list.
        token_list arg = args[p - params.begin()];
        // If a parameter is preceded by a # or ## or followed by ##,
        // then macro replacement does not take place before its
        // substitution (C11, 6.10.3.1).
        prev = next = curr;
        ++next;
        if (curr != repl.begin())
          --prev;
        if (prev->kind == token::PASTE
            || (next != repl.end() && next->kind == token::PASTE)) {
          if (arg.empty()) {
            // If the argument token list consists of no preprocessing
            // tokens and the parameter name is preceded or followed by
            // a token paste operator, replace the name with a
            // placemarker token (C11, 6.10.3.3 #2).
            curr->kind = token::PLACEMARKER;
            curr->text.clear();
            ++curr;
          } else {
            // Replace the argument as is.
            arg.front().ws = curr->ws;
            repl.splice(curr, arg);
            curr = repl.erase(curr);
          }
        } else {
          // Completely macro-expand the agrument.
          size_t depth = blacklist.size();
          macro_expand(blacklist, macros, lineno, arg);
          assert(blacklist.size() >= depth);
          blacklist.resize(depth);
          if (arg.empty()) {
            next = curr;
            ++next;
            if (next != repl.end())
              next->ws = curr->ws;
          } else {
            arg.front().ws = curr->ws;
          }
          repl.splice(curr, arg);
          curr = repl.erase(curr);
        }
      } else {
        // Not a parameter.
        ++curr;
      }
    } else if (curr->kind == token::STRINGIFY) {
      // The stringify operator is followed by a parameter name token.
      next = curr;
      ++next;
      assert(next != repl.end() && next->kind == token::ID);
      p = find(params, next->text);
      assert(p != params.end());
      const token_list &arg = args[p - params.begin()];
      curr->kind = token::OTHER;
      curr->text = stringify(arg);
      curr = repl.erase(next);
    } else {
      ++curr;
    }
  }
}

// Perform token pasting.
void
paste_tokens(token_list &repl) {
  token_list::iterator prev, curr, next;
  prev = repl.end();
  curr = repl.begin();
  while (curr != repl.end()) {
    if (curr->kind == token::PASTE) {
      // The ## operator is not the first or the last token.
      next = curr;
      ++next;
      while(next->kind == token::PASTE)
        next = repl.erase(next);
      assert(prev != repl.end() && next != repl.end());
      if (prev->kind == token::PLACEMARKER
          && next->kind == token::PLACEMARKER) {
        // Two placemarker tokens are replaced with a single
        // placemarker token.
        ++next;
        curr = repl.erase(curr, next);
      }
      else if (prev->kind == token::PLACEMARKER) {
        prev = repl.erase(prev, next);
        curr = prev;
        ++curr;
      } else if (next->kind == token::PLACEMARKER) {
        ++next;
        curr = repl.erase(curr, next);
      } else {
        // When concatenating tokens, ignore inner whitespace.
        prev->text.append(next->text);
        // Check we have obtained a valid preprocessing token.
        size_t ws;
        std::string::const_iterator end = 
          scan_pp_token(prev->text.begin(), prev->text.end(), prev->kind, ws);
        if (end != prev->text.end())
          throw "Token paste results in invalid preprocessing token";
        // The resulting token is available for a further macro
        // replacement (C11, 6.10.3.3 #3), but is never # or ##
        // operator.
        if (prev->kind != token::ID)
          prev->kind = token::OTHER;
        prev->noexpand = false;
        prev->pop += next->pop;
        ++next;
        curr = repl.erase(curr, next);
      }
    } else {
      prev = curr;
      ++curr;
    }
  }
}

void
macro_expand(std::vector<std::string> &blacklist, const macro_table *macros,
             unsigned int lineno, token_list &tokens) {
  token_list repl;
  const macro_table::define *def;
  token_list::iterator prev, curr, next;
  curr = tokens.begin();
  while (curr != tokens.end()) {
    // Pop names from the blacklist if the current token ends the range
    // where their replacement is forbiden.
    assert(blacklist.size() >= curr->pop);
    if (curr->pop) {
      blacklist.resize(blacklist.size() - curr->pop);
      curr->pop = 0;
    }

    // Not an identifier, no replacement.
    if (curr->kind != token::ID
        // Expansion forbidden, because at one point found in blacklist.
        || curr->noexpand) {
      ++curr;
      continue;
    }

    // If found an identifier, check the blacklist.
    if (std::find(blacklist.begin(), blacklist.end(), curr->text)
        !=  blacklist.end()) {
      // Do not replace this token anymore, even if it is re-examined in
      // a context where it is not blacklisted (C11, 16.3.4 #2).
      curr->noexpand = true;
      ++curr;
      continue;
    }
    
    // If not blacklisted, check if there is such a macro definition.
    if ((def = macros->find_define(lineno, curr->text)) == 0) {
      ++curr;
      continue;
    }

    // Found a macro to expand.
    if (def->params.size() == 0) {
      // Object-like macro.
      repl = tokenize(def);
      if (repl.empty()) {
        next = curr;
        ++next;
        if (next != tokens.end())
          next->ws = curr->ws;
        curr = tokens.erase(curr);
      } else {
        blacklist.push_back(def->name);
        repl.front().ws = curr->ws;
        next = curr;
        ++next;
        if (next != tokens.end())
          ++next->pop;
        tokens.splice(next, repl);
        curr = tokens.erase(curr);
      }
    } else {
      // Function-like macro. If the next token is an opening
      // parenthesis, expand the macro, otherwise skip the name.
      next = curr;
      ++next;
      if (next != tokens.end()
          && next->kind == token::OTHER && next->text == "(") {
        // Gather arguments.
        bool variadic = def->params.size() && def->params.back() == "...";
        std::vector<token_list> args;
        next = gather_arguments(blacklist, tokens, next, variadic,
                                def->params.size(), args);
        // Check the number of actual arguments matches the number of
        // macro parameters.
        if (variadic)  {
          // A variadic macro should have an argument for every named
          // parameter.
          if (args.size() < def->params.size() - 1)
            throw "Insufficient number of arguments";
          // "Pad" the arguments list with an empty one.
          args.resize(def->params.size());
        } else {
          if (args.size() == def->params.size()) {
            // A function-like macro with empty parameter list must be
            // given a single empty argument.
            if (def->params.size() == 1 && def->params[0].empty()
                && !args[0].empty()) {
              throw "Too many macro arguments";
            }
          } else if (args.size() > def->params.size()) {
            throw "Too many macro arguments";
          } else {
            throw "Insufficient number of arguments";
          }
        }
        // Perform parameter substitution and stringification.
        repl = tokenize(def);
        substitute_parameters(blacklist, args, def->params, macros, lineno,
                              repl);
        // Paste tokens.
        paste_tokens(repl);
        // Remove placemarkers.
        token_list::iterator p = repl.begin();
        while (p != repl.end()) {
          if (p->kind == token::PLACEMARKER)
            p = repl.erase(p);
          else
            ++p;
        }
        // Rescan/repeat expand.
        if (repl.empty()) {
          if (next != tokens.end())
            next->ws = curr->ws;
          curr = tokens.erase(curr, next);
        } else {
          blacklist.push_back(def->name);
          if (next != tokens.end())
            ++next->pop;
          repl.front().ws = curr->ws;
          if (curr == tokens.begin()) {
            tokens.splice(curr, repl);
            tokens.erase(curr, next);
            curr = tokens.begin();
          } else {
            prev = curr;
            --prev;
            tokens.splice(curr, repl);
            tokens.erase(curr, next);
            curr = prev;
            ++curr;
          }
        }
      } else {
        ++curr;
      }
    }
  }
}

// Helper template for exception safe save/restore of a value.
template<typename T>
class safe_save_restore {
public:
  safe_save_restore(T &loc, T val) : location_(loc) {
    saved_value_ = location_;
    location_ = val;
  }

  ~safe_save_restore() {
    location_ = saved_value_;
  }

protected:
  T saved_value_;
  T &location_;
};

} // end namespace

macro_table::~macro_table() {
  for (auto &e : table_)
    e.destroy();
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
  e->def->checked = false;
  parse_macro_def(def, e->def->name, e->def->params, e->def->repl);
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
  if (in_use_ || table_.size() == 0)
    return 0;
  
  // Protect from cycles in the incuded files.
  safe_save_restore<bool> in_use(in_use_, true);

  // Find the source location to start the search from.
  size_t idx;
  if (lineno > 0) {
    // Binary search in the macros table for the first line number,
    // greater than or equal to the given one.
    size_t lower = 0, upper = table_.size();
    while (lower < upper) {
      size_t m = (lower + upper) / 2;
      if (table_[m].lineno < lineno)
        lower = m + 1;
      else
        upper = m;
    }

    idx = lower;
  } else {
    // 0 means start from the end of the table.
    idx = table_.size();
  }

  // Examine the macro entries from the next smaller index downwards.
  assert(idx == table_.size() || table_[idx].lineno >= lineno);
  while(idx-- > 0) {
    switch(table_[idx].kind) {
    case entry::DEFINE:
      // Found a macro definition for the name
      if (table_[idx].def->name == name)
        return table_[idx].def;
      break;
    case entry::UNDEFINE:
      // Found an undefine directive; terminate search.
      if (table_[idx].def->name == name)
        return 0;
      break;
    case entry::INCLUDE:
      // Search among the directives in the included file
      if (const define *d =
          table_[idx].include->get_macros()->find_define(0, name))
        return d;
      break;
    default:
      break;
    }
  }

  // Macro definition not found.
  return 0;
}

std::string
macro_expand(const std::string &in, const macro_table *macros,
             unsigned int lineno) {
  // Tokenize the input string.
  std::list<token> tokens = tokenize(in, false, false);

  // Perform the expansion.
  std::vector<std::string> blacklist;
  macro_expand(blacklist, macros, lineno, tokens);

  // Construct and return the output string.
  std::string out;
  for (const auto &t : tokens) {
    assert(t.kind == token::ID || t.kind == token::OTHER);
    if (t.ws)
      out += ' ';
    out += t.text;
  }
  return out;
}

} // end namespace
