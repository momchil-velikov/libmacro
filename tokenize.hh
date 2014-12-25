// mode: c++; indent-tabs-mode: nil; -*-
#ifndef libmacro_tokenize_hh__
#define libmacro_tokenize_hh__ 1

#include <string>
#include <vector>
#include <locale>
#include <cassert>

namespace libmacro {
namespace detail {

extern std::locale C_locale;

inline bool
isoctal(char ch) {
  return ch >= '0' && ch <= '7';
}

// Scan a sequence of between one and three octal digits.
template<typename InputIterator>
InputIterator
scan_oct_seq(InputIterator str, InputIterator end) {
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
template<typename InputIterator>
InputIterator
scan_hex_seq(InputIterator str, InputIterator end) {
  assert(str != end && std::isxdigit(*str, C_locale));
  ++str;
  if (str != end && std::isxdigit(*str, C_locale))
    ++str;
  return str;
}

// Scan a character escape sequence
template<typename InputIterator>
InputIterator
scan_escape_seq(InputIterator str, InputIterator end) {

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
template<typename InputIterator>
InputIterator
scan_character_constant(InputIterator str, InputIterator end) {
  auto err = str;
  assert(str != end && *str == '\'');
  ++str;
  if (str == end)
    return err;

  if (*str == '\\') {
    auto next = scan_escape_seq(str, end);
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
template<typename InputIterator>
InputIterator
scan_string_literal(InputIterator str, InputIterator end) {
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
template<typename InputIterator>
InputIterator
scan_pp_number(InputIterator str, InputIterator end) {
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
  enum kind { ID, STRINGIFY, PASTE, PLACEMARKER, END, OTHER };

  token(enum kind k, bool ws) : kind(k), ws(ws), noexpand(false), pop(0) {}

  template<typename It>
  token(enum kind k, bool ws, It begin, It end)
      : kind(k), ws(ws), noexpand(false), pop(), text(begin, end) {}

  token(const token &other) : kind(other.kind), ws(other.ws), noexpand(other.noexpand),
                              pop (other.pop), text(other.text) {}

  token(token &&other) : kind(other.kind), ws(other.ws), noexpand(other.noexpand),
                         pop (other.pop), text(std::move(other.text)) {}

  token &operator=(const token &other) {
    token tmp(other);
    std::swap(*this, tmp);
    return *this;
  }

  token &operator=(token &&other) {
    kind = other.kind;
    ws = other.ws;
    noexpand = other.noexpand;
    pop = other.pop;
    text = std::move(other.text);
    return *this;
  }

  enum kind kind;
  bool ws;
  bool noexpand;
  size_t pop;
  std::string text;
};

// Type for a list of tokens.
typedef std::vector<token> token_list;


// Scan a preprocessing token
// preprocessing-token:
//     header-name
//     identifier
//     pp-number
//     character-constant
//     string-literal
//     punctuator
//     each non-white-space character that cannot be one of the above
template<typename InputIterator>
InputIterator
scan_pp_token(InputIterator str, InputIterator end, enum token::kind &kind, size_t &ws) {
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

template<typename It>
class tokenizer {
 public:
  tokenizer(It begin, It end, bool func_like, bool repl = true)
      : token_(token::END, false), next_(begin), end_(end), func_like_(func_like),
        replacement_(repl) {}

  class iterator {
   public:
    explicit iterator() : owner_(nullptr) {}
    explicit iterator(tokenizer<It> *owner) : owner_(owner) {
      const auto &t = owner_->fetch();
      if (t.kind == token::END)
        owner_ = nullptr;
    }

    bool operator==(const iterator &other) const {
      if (owner_ == nullptr && other.owner_ == nullptr)
        return true;
      return this == &other;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

    token &operator*() { return owner_->token_; }

    const token &operator*() const { return owner_->token_; }

    token *operator->() { return &owner_->token_; }

    const token *operator->() const { return &owner_->token_;  }

    iterator &operator++() {
      const auto &t = owner_->fetch();
      if (t.kind == token::END)
        owner_ = nullptr;
      return *this;
    }

   private:
    tokenizer<It> *owner_;
  };

  iterator begin() {
    return iterator(this);
  }

  iterator end() const {
    return iterator();
  }

 private:
  const token &fetch();
  friend class iterator;

  token token_;
  It next_;
  It end_;
  bool func_like_;
  bool replacement_;
};

template<typename It>
const token &
tokenizer<It>::fetch() {
  enum token::kind kind;
  size_t ws;
  if (next_ == end_)
    return token_ = {token::END, false};
  auto next = scan_pp_token(next_, end_, kind, ws);
  if (next == next_)
    throw "Invalid preprocessing token";
  auto start = next_;
  next_ = next;
  assert(kind != token::PLACEMARKER);
  switch (kind) {
    case token::ID:
    case token::OTHER:
      return token_ = {kind, ws != 0, start + ws, next};
    case token::STRINGIFY:
      if (!replacement_ || !func_like_)
        return token_ = {token::OTHER, ws != 0, start + ws, next};
      else
        return token_ = {kind, ws != 0};
    case token::PASTE:
      if (!replacement_)
        return token_ = {token::OTHER, ws != 0, start + ws, next};
      else
        return token_ = {kind, ws != 0};
    default:
    case token::END:
      return token_ = {token::END, false};
  }
}

// Tokenize a character sequence
template<typename InputIterator>
token_list
tokenize(InputIterator begin, InputIterator end, bool func_like, bool replacement = true) {
  token_list tokens;
  tokenizer<InputIterator> t(begin, end, func_like, replacement);
  const auto stop = t.end();
  auto curr = t.begin();
  while (curr != stop) {
    tokens.push_back(*curr);
    ++curr;
  }
  return tokens;
}

} // end namespace detail
} // end namespace libmacro
#endif // libmacro_tokenize_hh__
