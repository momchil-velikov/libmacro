#include "libmacro.hh"
#include "gtest/gtest.h"

namespace {

class empty_macros : public ::testing::Test {
 protected:
  empty_macros() {
    macros.add_define(1, "A");
    macros.add_define(2, "B");
    macros.add_define(3, "C");
    macros.add_define(4, "D");
  }

  libmacro::macro_table macros;
};

TEST_F(empty_macros, table_search) {
  std::string out;
  out = libmacro::macro_expand("A", &macros, 0);
  ASSERT_EQ("", out);
  out = libmacro::macro_expand("A", &macros, 1);
  ASSERT_EQ("A", out);
  out = libmacro::macro_expand("A", &macros, 2);
  ASSERT_EQ("", out);
  out = libmacro::macro_expand("A", &macros, 3);
  ASSERT_EQ("", out);

  out = libmacro::macro_expand("B", &macros, 0);
  ASSERT_EQ("",out);
  out = libmacro::macro_expand("B", &macros, 1);
  ASSERT_EQ("B", out);
  out = libmacro::macro_expand("B", &macros, 2);
  ASSERT_EQ("B", out);
  out = libmacro::macro_expand("B", &macros, 3);
  ASSERT_EQ("", out);

  out = libmacro::macro_expand("C", &macros, 0);
  ASSERT_EQ("", out);
  out = libmacro::macro_expand("C", &macros, 1);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 2);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 3);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 4);
  ASSERT_EQ("", out);

  out = libmacro::macro_expand("D", &macros, 0);
  ASSERT_EQ("", out);
  out = libmacro::macro_expand("D", &macros, 1);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 2);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 3);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 4);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 5);
  ASSERT_EQ("", out);

  out = libmacro::macro_expand("E", &macros, 0);
  ASSERT_EQ("E", out);
  out = libmacro::macro_expand("E", &macros, 1);
  ASSERT_EQ("E", out);
  out = libmacro::macro_expand("E", &macros, 2);
  ASSERT_EQ("E", out);
  out = libmacro::macro_expand("E", &macros, 3);
  ASSERT_EQ("E", out);
  out = libmacro::macro_expand("E", &macros, 4);
  ASSERT_EQ("E", out);
  out = libmacro::macro_expand("E", &macros, 5);
  ASSERT_EQ("E", out);
}

class simple_macros : public ::testing::Test {
 protected:
  simple_macros() {
    macros.add_define(1, "A a");
    macros.add_define(2, "B b");
    macros.add_define(3, "C c");
    macros.add_define(4, "D d");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_macros, simple_macro_expand) {
  std::string out;
  out = libmacro::macro_expand("A", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("A", &macros, 1);
  ASSERT_EQ("A", out);
  out = libmacro::macro_expand("A", &macros, 2);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("A", &macros, 3);
  ASSERT_EQ("a", out);

  out = libmacro::macro_expand("B", &macros, 0);
  ASSERT_EQ("b",out);
  out = libmacro::macro_expand("B", &macros, 1);
  ASSERT_EQ("B", out);
  out = libmacro::macro_expand("B", &macros, 2);
  ASSERT_EQ("B", out);
  out = libmacro::macro_expand("B", &macros, 3);
  ASSERT_EQ("b", out);

  out = libmacro::macro_expand("C", &macros, 0);
  ASSERT_EQ("c", out);
  out = libmacro::macro_expand("C", &macros, 1);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 2);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 3);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 4);
  ASSERT_EQ("c", out);

  out = libmacro::macro_expand("D", &macros, 0);
  ASSERT_EQ("d", out);
  out = libmacro::macro_expand("D", &macros, 1);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 2);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 3);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 4);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 5);
  ASSERT_EQ("d", out);
}

TEST_F(simple_macros, whitespace_preserve) {
  std::string out;
  out = libmacro::macro_expand(" A", &macros, 0);
  ASSERT_EQ(" a", out);
  out = libmacro::macro_expand("( B", &macros, 0);
  ASSERT_EQ("( b", out);
  out = libmacro::macro_expand(" (C", &macros, 0);
  ASSERT_EQ(" (c", out);
  out = libmacro::macro_expand("D ", &macros, 0);
  ASSERT_EQ("d", out);
  out = libmacro::macro_expand("A B (C) D", &macros, 0);
  ASSERT_EQ("a b (c) d", out);
}

class simple_chain_macros : public ::testing::Test {
 protected:
  simple_chain_macros() {
    macros.add_define(1, "A a");
    macros.add_define(2, "B A");
    macros.add_define(3, "C c");
    macros.add_define(4, "D C");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_chain_macros, simple_macro_expand) {
  std::string out;
  out = libmacro::macro_expand("A", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("A  ", &macros, 1);
  ASSERT_EQ("A", out);
  out = libmacro::macro_expand("A", &macros, 2);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("A", &macros, 3);
  ASSERT_EQ("a", out);

  out = libmacro::macro_expand("B", &macros, 0);
  ASSERT_EQ("a",out);
  out = libmacro::macro_expand("B", &macros, 1);
  ASSERT_EQ("B", out);
  out = libmacro::macro_expand("B", &macros, 2);
  ASSERT_EQ("B", out);
  out = libmacro::macro_expand("B", &macros, 3);
  ASSERT_EQ("a", out);

  out = libmacro::macro_expand("C", &macros, 0);
  ASSERT_EQ("c", out);
  out = libmacro::macro_expand("C", &macros, 1);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 2);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 3);
  ASSERT_EQ("C", out);
  out = libmacro::macro_expand("C", &macros, 4);
  ASSERT_EQ("c", out);

  out = libmacro::macro_expand("D", &macros, 0);
  ASSERT_EQ("c", out);
  out = libmacro::macro_expand("D", &macros, 1);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 2);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 3);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 4);
  ASSERT_EQ("D", out);
  out = libmacro::macro_expand("D", &macros, 5);
  ASSERT_EQ("c", out);
}

TEST_F(simple_chain_macros, whitespace_preserve) {
  std::string out;
  out = libmacro::macro_expand(" A", &macros, 0);
  ASSERT_EQ(" a", out);
  out = libmacro::macro_expand("( B", &macros, 0);
  ASSERT_EQ("( a", out);
  out = libmacro::macro_expand(" (C", &macros, 0);
  ASSERT_EQ(" (c", out);
  out = libmacro::macro_expand("D ", &macros, 0);
  ASSERT_EQ("c", out);
  out = libmacro::macro_expand("A B (C) D", &macros, 0);
  ASSERT_EQ("a a (c) c", out);
}

class simple_rec_macros  : public ::testing::Test {
 protected:
  simple_rec_macros() {
    macros.add_define(1, "A D");
    macros.add_define(2, "B E");
    macros.add_define(3, "C F");
    macros.add_define(4, "D E");
    macros.add_define(5, "E F");
    macros.add_define(6, "F D");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_rec_macros, simple_macro_expand) {
  std::string out;
  out = libmacro::macro_expand("A B C D E F", &macros, 0);
  ASSERT_EQ("D E F D E F", out);
}

class simple_rec_macros1  : public ::testing::Test {
 protected:
  simple_rec_macros1() {
    macros.add_define(1, "A {D}{E}");
    macros.add_define(2, "B {E}{F}");
    macros.add_define(3, "C {F}{E}");
    macros.add_define(4, "D {E}{F}");
    macros.add_define(5, "E {F}.");
    macros.add_define(6, "F {D}{E}");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_rec_macros1, simple_macro_expand) {
  std::string out;
  out = libmacro::macro_expand("A B C D E F", &macros, 0);
  ASSERT_EQ("{{{{D}{E}}.}{{D}{{F}.}}}{{{{E}{F}}{E}}.} {{{{E}{F}}{E}}.}{{{{F}.}{F}}{{F}.}} {{{{F}.}{F}}{{F}.}}{{{{E}{F}}{E}}.} {{{D}{E}}.}{{D}{{F}.}} {{{E}{F}}{E}}. {{{F}.}{F}}{{F}.}", out);
}

} // end namespace
