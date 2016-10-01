#include "libmacro.hh"
#include "gtest/gtest.h"

#include <cstdio>

namespace {

class simple_func_macros : public ::testing::Test {
protected:
  simple_func_macros() {
    macros.add_define(1, "A() a");
    macros.add_define(2, "B b");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_func_macros, func_vs_obj) {
  std::string out;
  out = libmacro::macro_expand("A A() B B()", &macros, 0);
  ASSERT_EQ("A a b b()", out);
}

class simple_func_macros_with_param : public ::testing::Test {
protected:
  simple_func_macros_with_param() {
    macros.add_define(1, "A(x) {x}");
    macros.add_define(2, "B(x,y) {x}{y}");
    macros.add_define(2, "C(x,y,z) {x}{y}{z}");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_func_macros_with_param, param_subst) {
  std::string out;
  out = libmacro::macro_expand("A A() A(b) A(a   b)", &macros, 0);
  ASSERT_EQ("A {} {b} {a b}", out);
  out = libmacro::macro_expand("B B(b, ) B( , b) B(u,v) B(u, v   w)", &macros, 0);
  ASSERT_EQ("B {b}{} {}{b} {u}{v} {u}{v w}", out);
  out = libmacro::macro_expand(
      "C C(u, ,) C(, v, ) C( , ,w) C(u,v,) C(u, ,w) C( ,v ,w) C(u,v,w)", &macros, 0);
  ASSERT_EQ("C {u}{}{} {}{v}{} {}{}{w} {u}{v}{} {u}{}{w} {}{v}{w} {u}{v}{w}", out);
}

class simple_func_macros_with_param1 : public ::testing::Test {
protected:
  simple_func_macros_with_param1() {
    macros.add_define(1, "A(x) { x }");
    macros.add_define(2, "B(x,y) A(x)A({ y }) A(x)");
    macros.add_define(3, "C(x,y,z) B(x, B(y, z))");
    macros.add_define(4, "D(x,y) x, y, z");
  }

  libmacro::macro_table macros;
};

TEST_F(simple_func_macros_with_param1, whitespace_handling) {
  std::string out;
  out = libmacro::macro_expand("A( a )", &macros, 0);
  ASSERT_EQ("{ a }", out);
  out = libmacro::macro_expand("B( a , b )", &macros, 0);
  ASSERT_EQ("{ a }{ { b } } { a }", out);
  out = libmacro::macro_expand("C( a , b , c )", &macros, 0);
  ASSERT_EQ("{ a }{ { { b }{ { c } } { b } } } { a }", out);
  out = libmacro::macro_expand("D(x, )", &macros, 0);
  ASSERT_EQ("x, , z", out);
  out = libmacro::macro_expand(" D(, x)", &macros, 0);
  ASSERT_EQ(" , x, z", out);
  out = libmacro::macro_expand("D(x y, u v)", &macros, 0);
  ASSERT_EQ("x y, u v, z", out);
}

class rec_func_macros : public ::testing::Test {
protected:
  rec_func_macros() {
    macros.add_define(1, "A() D(u,v)E(u,v)");
    macros.add_define(2, "B(x) E(x,v)F(x,v,w)");
    macros.add_define(3, "C(x) F(x,v,w)E(x,v)");
    macros.add_define(4, "D(x,y) E(x,y)F(x,y,w)");
    macros.add_define(5, "E(x,y) F(x,y,w).");
    macros.add_define(6, "F(x,y,z) D(x,y)E(z,x)");
  }

  libmacro::macro_table macros;
};

TEST_F(rec_func_macros, param_subst) {
  std::string out;
  out = libmacro::macro_expand("A()", &macros, 0);
  ASSERT_EQ("D(u,v)E(w,u).D(u,v)F(w,u,w).E(u,v)F(u,v,w)E(w,u).", out);
  out = libmacro::macro_expand("B(abc)", &macros, 0);
  ASSERT_EQ("E(abc,v)F(abc,v,w)E(w,abc).F(abc,v,w).F(abc,v,w)F(w,abc,w).", out);
  out = libmacro::macro_expand("C(abc)", &macros, 0);
  ASSERT_EQ("F(abc,v,w).F(abc,v,w)F(w,abc,w).E(abc,v)F(abc,v,w)E(w,abc).", out);
  out = libmacro::macro_expand("D(abc,mno)", &macros, 0);
  ASSERT_EQ("D(abc,mno)E(w,abc).D(abc,mno)F(w,abc,w).", out);
  out = libmacro::macro_expand("E(abc,mno)", &macros, 0);
  ASSERT_EQ("E(abc,mno)F(abc,mno,w)E(w,abc).", out);
  out = libmacro::macro_expand("F(abc,mno,str)", &macros, 0);
  ASSERT_EQ("F(abc,mno,w).F(abc,mno,w)F(str,abc,w).", out);
}

class rec_func_macros1 : public ::testing::Test {
protected:
  rec_func_macros1() {
    macros.add_define(1, "A(x) B(x, B(z, x))");
    macros.add_define(2, "B(x,y) A(B(A(x),y))");

    macros.add_define(4, "D(x,y) F(x,E(x,y))");
    macros.add_define(5, "E(x,y) F(x,y)");
    macros.add_define(6, "F(x,y) D(F(x,y),E(y,x))");
  }

  libmacro::macro_table macros;
};

TEST_F(rec_func_macros1, blacklisting) {
  std::string out;
  out = libmacro::macro_expand("A(b)", &macros, 0);
  ASSERT_EQ("A(B(A(b),A(B(A(z),b))))", out);
  out = libmacro::macro_expand("B(c,d)", &macros, 0);
  ASSERT_EQ("B(B(B(c, B(z, c)),d), B(z, B(B(c, B(z, c)),d)))", out);
  out = libmacro::macro_expand("D(a,b)", &macros, 0);
  ASSERT_EQ("D(F(a,D(F(a,b),E(b,a))),F(D(F(a,b),E(b,a)),a))", out);
}

class rec_func_macros2 : public ::testing::Test {
protected:
  rec_func_macros2() {
    macros.add_define(1, "A() D(u,E(u,v))");
    macros.add_define(2, "B(x) E(x,F(x,v,w))");
    macros.add_define(3, "C(x) F(x,E(x,v),w)");
    macros.add_define(4, "D(x,y) F(x,E(x,y),w)");
    macros.add_define(5, "E(x,y) F(x,y,w).");
    macros.add_define(6, "F(x,y,z) D(F(x,y,z),E(z,x))");
  }

  libmacro::macro_table macros;
};

TEST_F(rec_func_macros2, param_subst) {
  std::string out;
  out = libmacro::macro_expand("A()", &macros, 0);
  ASSERT_EQ("D(F(u,D(F(u,F(F(u,v,w),E(F(u,v,w),E(w,u)),w).,w),E(w,u)).,w),F(w,u,w).)",
            out);
  out = libmacro::macro_expand("B(a)", &macros, 0);
  ASSERT_EQ(
      "F(F(a,F(F(a,v,w),F(F(a,v,w),F(w,a,w).,w).,w),w),E(F(a,F(F(a,v,w),F(F(a,v,w),F(w,a,w).,w).,w),w),E(w,a)),w).",
      out);
  out = libmacro::macro_expand("C(a)", &macros, 0);
  ASSERT_EQ(
      "F(F(a,F(F(a,v,w),E(F(a,v,w),E(w,a)),w).,w),F(F(a,F(F(a,v,w),E(F(a,v,w),E(w,a)),w).,w),F(w,a,w).,w).,w)",
      out);
  out = libmacro::macro_expand("D(a,b)", &macros, 0);
  ASSERT_EQ("D(F(a,D(F(a,b,w),E(w,a)).,w),F(w,a,w).)", out);
  out = libmacro::macro_expand("E(a,b)", &macros, 0);
  ASSERT_EQ("F(F(a,b,w),E(F(a,b,w),E(w,a)),w).", out);
  out = libmacro::macro_expand("F(a,b,c)", &macros, 0);
  ASSERT_EQ("F(F(a,b,c),F(F(a,b,c),F(c,a,w).,w).,w)", out);
}

class stringify_macros : public ::testing::Test {
protected:
  stringify_macros() {
    macros.add_define(1, "A(x) #x");
    macros.add_define(2, "B(x,y,z) x, y, z");
    macros.add_define(3, "C(x,y,z) A(B(x, y, z))");
    macros.add_define(1, "D A(# ##)");
  }

  libmacro::macro_table macros;
};

TEST_F(stringify_macros, stringification) {
  std::string out;
  out = libmacro::macro_expand("#A", &macros, 0);
  ASSERT_EQ("#A", out);
  out = libmacro::macro_expand("A()", &macros, 0);
  ASSERT_EQ("\"\"", out);
  out = libmacro::macro_expand("A(x)", &macros, 0);
  ASSERT_EQ("\"x\"", out);
  out = libmacro::macro_expand("A(  x    y   z)", &macros, 0);
  ASSERT_EQ("\"x y z\"", out);
  out = libmacro::macro_expand("A(\"x\")", &macros, 0);
  ASSERT_EQ("\"\\\"x\\\"\"", out);
  out = libmacro::macro_expand("A(\"x y   z\")", &macros, 0);
  ASSERT_EQ("\"\\\"x y   z\\\"\"", out);
  out = libmacro::macro_expand("C(x,  , z)", &macros, 0);
  ASSERT_EQ("\"B(x, , z)\"", out);
  out = libmacro::macro_expand("A(a \\b c)", &macros, 0);
  ASSERT_EQ("\"a \\b c\"", out);
  out = libmacro::macro_expand("A(a '\\b' c)", &macros, 0);
  ASSERT_EQ("\"a \'\\\\b\' c\"", out);
  out = libmacro::macro_expand("A(\"a \\b c\")", &macros, 0);
  ASSERT_EQ("\"\\\"a \\\\b c\\\"\"", out);
  out = libmacro::macro_expand("A(\"a '\\b' c\")", &macros, 0);
  ASSERT_EQ("\"\\\"a '\\\\b' c\\\"\"", out);
  out = libmacro::macro_expand("D", &macros, 0);
  ASSERT_EQ("\"# ##\"", out);
}

class token_paste_macros : public ::testing::Test {
protected:
  token_paste_macros() {
    macros.add_define(1, "A(y) x ## y");
    macros.add_define(2, "B(x) {x}");
    macros.add_define(3, "C(x,y,z) x ## y ## z");
  }

  libmacro::macro_table macros;
};

TEST_F(token_paste_macros, paste) {
  std::string out;
  out = libmacro::macro_expand("A(a)", &macros, 0);
  ASSERT_EQ("xa", out);
  out = libmacro::macro_expand("A(B(b))", &macros, 0);
  ASSERT_EQ("xB(b)", out);
  out = libmacro::macro_expand("C(a,b,c)", &macros, 0);
  ASSERT_EQ("abc", out);
  out = libmacro::macro_expand("C(,b,c)", &macros, 0);
  ASSERT_EQ("bc", out);
  out = libmacro::macro_expand("C(a,,c)", &macros, 0);
  ASSERT_EQ("ac", out);
  out = libmacro::macro_expand("C(a,b,)", &macros, 0);
  ASSERT_EQ("ab", out);
  out = libmacro::macro_expand("C(a,,)", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("C(,b,)", &macros, 0);
  ASSERT_EQ("b", out);
  out = libmacro::macro_expand("C(,,c)", &macros, 0);
  ASSERT_EQ("c", out);
}

class variadic_macros : public ::testing::Test {
protected:
  variadic_macros() {
    macros.add_define(1, "A(...) foo(__VA_ARGS__)");
    macros.add_define(2, "B(x,...) foo(x,__VA_ARGS__)");
    macros.add_define(3, "C(x,y,...) foo(x,__VA_ARGS__,y)");
    macros.add_define(4, "D(...) #__VA_ARGS__");
    macros.add_define(5, "E(x,...) x #__VA_ARGS__");
    macros.add_define(6, "F(x,y,...) x y #__VA_ARGS__");
    macros.add_define(7, "G(x,y,...) x #__VA_ARGS__ y");
    macros.add_define(8, "P(x,...) x ## __VA_ARGS__");
    macros.add_define(9, "Q(x,...) __VA_ARGS__ ## x");
  }

  libmacro::macro_table macros;
};

TEST_F(variadic_macros, subsitution) {
  std::string out;
  out = libmacro::macro_expand("A B C", &macros, 0);
  ASSERT_EQ("A B C", out);
  out = libmacro::macro_expand("A()", &macros, 0);
  ASSERT_EQ("foo()", out);
  out = libmacro::macro_expand("A(a)", &macros, 0);
  ASSERT_EQ("foo(a)", out);
  out = libmacro::macro_expand("A(a,b)", &macros, 0);
  ASSERT_EQ("foo(a,b)", out);
  out = libmacro::macro_expand("B(a)", &macros, 0);
  ASSERT_EQ("foo(a,)", out);
  out = libmacro::macro_expand("B(a,b)", &macros, 0);
  ASSERT_EQ("foo(a,b)", out);
  out = libmacro::macro_expand("B(a,b,c)", &macros, 0);
  ASSERT_EQ("foo(a,b,c)", out);
  out = libmacro::macro_expand("B(a,b,c,d)", &macros, 0);
  ASSERT_EQ("foo(a,b,c,d)", out);
  out = libmacro::macro_expand("C(a,b)", &macros, 0);
  ASSERT_EQ("foo(a,,b)", out);
  out = libmacro::macro_expand("C(a,b,c)", &macros, 0);
  ASSERT_EQ("foo(a,c,b)", out);
  out = libmacro::macro_expand("C(a,b,c,d)", &macros, 0);
  ASSERT_EQ("foo(a,c,d,b)", out);
  out = libmacro::macro_expand("C(a,b,c,d,e)", &macros, 0);
  ASSERT_EQ("foo(a,c,d,e,b)", out);
}

TEST_F(variadic_macros, stringify) {
  std::string out;
  out = libmacro::macro_expand("D()", &macros, 0);
  ASSERT_EQ("\"\"", out);
  out = libmacro::macro_expand("D(,)", &macros, 0);
  ASSERT_EQ("\",\"", out);
  out = libmacro::macro_expand("D(,,)", &macros, 0);
  ASSERT_EQ("\",,\"", out);
  out = libmacro::macro_expand("D(,  ,)", &macros, 0);
  ASSERT_EQ("\", ,\"", out);
  out = libmacro::macro_expand("D(,, ,  ,)", &macros, 0);
  ASSERT_EQ("\",, , ,\"", out);

  out = libmacro::macro_expand("E(a)", &macros, 0);
  ASSERT_EQ("a \"\"", out);
  out = libmacro::macro_expand("E(a,)", &macros, 0);
  ASSERT_EQ("a \"\"", out);
  out = libmacro::macro_expand("E(a,,)", &macros, 0);
  ASSERT_EQ("a \",\"", out);
  out = libmacro::macro_expand("E(a,  ,)", &macros, 0);
  ASSERT_EQ("a \",\"", out);
  out = libmacro::macro_expand("E(a,, ,  ,)", &macros, 0);
  ASSERT_EQ("a \", , ,\"", out);

  out = libmacro::macro_expand("F(a,b)", &macros, 0);
  ASSERT_EQ("a b \"\"", out);
  out = libmacro::macro_expand("F(a,b,,)", &macros, 0);
  ASSERT_EQ("a b \",\"", out);
  out = libmacro::macro_expand("F(a,b,  ,)", &macros, 0);
  ASSERT_EQ("a b \",\"", out);
  out = libmacro::macro_expand("F(a,b,, ,  ,)", &macros, 0);
  ASSERT_EQ("a b \", , ,\"", out);

  out = libmacro::macro_expand("G(a,b)", &macros, 0);
  ASSERT_EQ("a \"\" b", out);
  out = libmacro::macro_expand("G(a,b,c)", &macros, 0);
  ASSERT_EQ("a \"c\" b", out);
  out = libmacro::macro_expand("G(a,b, c,d)", &macros, 0);
  ASSERT_EQ("a \"c,d\" b", out);
  out = libmacro::macro_expand("G(a,b, c, d,  e)", &macros, 0);
  ASSERT_EQ("a \"c, d, e\" b", out);
}

TEST_F(variadic_macros, paste) {
  std::string out;
  out = libmacro::macro_expand("P(a)", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("P(a,)", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("P(a,b)", &macros, 0);
  ASSERT_EQ("ab", out);
  out = libmacro::macro_expand("P(a,b,)", &macros, 0);
  ASSERT_EQ("ab,", out);
  out = libmacro::macro_expand("P(a,b,c)", &macros, 0);
  ASSERT_EQ("ab,c", out);
  out = libmacro::macro_expand("P(a,b,c,)", &macros, 0);
  ASSERT_EQ("ab,c,", out);

  out = libmacro::macro_expand("Q(a)", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("Q(a,)", &macros, 0);
  ASSERT_EQ("a", out);
  out = libmacro::macro_expand("Q(a,b)", &macros, 0);
  ASSERT_EQ("ba", out);
  out = libmacro::macro_expand("Q(a,b,c)", &macros, 0);
  ASSERT_EQ("b,ca", out);
  out = libmacro::macro_expand("Q(a,b,c,d)", &macros, 0);
  ASSERT_EQ("b,c,da", out);
}

class c11_std_example_macros : public ::testing::Test {
protected:
  c11_std_example_macros() {
    macros.add_define(1, "f(a) a*g");
    macros.add_define(2, "g(a) f(a)");
    macros.add_define(3, "u v");
    macros.add_define(4, "v(x) {x}");

    macros.add_define(5, "hash_hash # ## #");
    macros.add_define(6, "mkstr(a) # a");
    macros.add_define(7, "in_between(a) mkstr(a)");
    macros.add_define(8, "join(c,d) in_between(c hash_hash d)");
  }

  libmacro::macro_table macros;
};

TEST_F(c11_std_example_macros, unspec_rescan) {
  std::string out;
  out = libmacro::macro_expand("f(2)(9)", &macros, 0);
  ASSERT_EQ("2*9*g", out);
  out = libmacro::macro_expand("u(a)", &macros, 0);
  ASSERT_EQ("{a}", out);
}

TEST_F(c11_std_example_macros, paste) {
  std::string out;
  out = libmacro::macro_expand("join(x,y)", &macros, 0);
  ASSERT_EQ("\"x ## y\"", out);
}

class c11_std_example_macros1 : public ::testing::Test {
protected:
  c11_std_example_macros1() {
    macros.add_define(1, "x 3");
    macros.add_define(2, "f(a) f(x * (a))");
    macros.add_undefine(3, "x");
    macros.add_define(4, "x 2");
    macros.add_define(6, "g f");
    macros.add_define(7, "z z[0]");
    macros.add_define(8, "h g(~");
    macros.add_define(9, "m(a) a(w)");
    macros.add_define(10, "w 0, 1");
    macros.add_define(11, "t(a) a");
    macros.add_define(12, "p() int");
    macros.add_define(13, "q(x) x");
    macros.add_define(14, "r(x,y) x ## y");
    macros.add_define(15, "str(x) #x");
  }
  libmacro::macro_table macros;
};

TEST_F(c11_std_example_macros1, reexamine) {
  std::string out;
  out = libmacro::macro_expand("f(y+1) + f(f(z)) % t(t(g)(0) + t)(1);", &macros, 0);
  ASSERT_EQ("f(2 * (y+1)) + f(2 * (f(2 * (z[0])))) % f(2 * (0)) + t(1);", out);
  out = libmacro::macro_expand("g(x+(3,4)-w) | h 5) & m(f)^m(m);", &macros, 0);
  ASSERT_EQ("f(2 * (2+(3,4)-0, 1)) | f(2 * (~ 5)) & f(2 * (0, 1))^m(0, 1);", out);
}

class c11_std_example_macros2 : public ::testing::Test {
protected:
  c11_std_example_macros2() {
    macros.add_define(15, "str(x) #x");
    macros.add_define(16, "xstr(s) str(s)");
    macros.add_define(
        17, "debug(s,t) printf(\"x\" # s \"= %d, x\" # t \"= %s\", x ## s, x ## t)");
    macros.add_define(18, "INCFILE(n) vers ## n");
    macros.add_define(19, "glue(a,b) a ## b");
    macros.add_define(20, "xglue(a,b) glue(a, b)");
    macros.add_define(21, "HIGHLOW \"hello\"");
    macros.add_define(22, "LOW LOW \", world\"");
    macros.add_define(23, "t(x,y,z) x ## y ## z");
  }

  libmacro::macro_table macros;
};

TEST_F(c11_std_example_macros2, string_and_paste) {
  std::string out;
  out = libmacro::macro_expand("debug(1, 2);", &macros, 0);
  ASSERT_EQ("printf(\"x\" \"1\" \"= %d, x\" \"2\" \"= %s\", x1, x2);", out);
  out = libmacro::macro_expand(
      "fputs(str(strncmp(\"abc\\0d\", \"abc\", '\\4') == 0) str(: @\\n), s);",
      &macros,
      0);
  ASSERT_EQ(
      "fputs(\"strncmp(\\\"abc\\\\0d\\\", \\\"abc\\\", '\\\\4') == 0\" \": @\\n\", s);",
      out);
  out = libmacro::macro_expand("#include xstr(INCFILE(2).h)", &macros, 0);
  ASSERT_EQ("#include \"vers2.h\"", out);
  out = libmacro::macro_expand("glue(HIGH, LOW)", &macros, 0);
  ASSERT_EQ("\"hello\"", out);
  out = libmacro::macro_expand("xglue(HIGH, LOW)", &macros, 0);
  ASSERT_EQ("\"hello\" \", world\"", out);
}

TEST_F(c11_std_example_macros2, placemarkers) {
  std::string out;
  out = libmacro::macro_expand(
      "int j[] = { t(1,2,3), t(,4,5), t(6,,7), t(8,9,), t(10,,), t(,11,), t(,,12), t(,,) };",
      &macros,
      0);
  ASSERT_EQ("int j[] = { 123, 45, 67, 89, 10, 11, 12, };", out);
}

class c11_std_example_macros3 : public ::testing::Test {
protected:
  c11_std_example_macros3() {
    macros.add_define(1, "debug(...) fprintf(stderr, __VA_ARGS__)");
    macros.add_define(2, "showlist(...) puts(#__VA_ARGS__)");
    macros.add_define(3, "report(test,...) ((test)?puts(#test):printf(__VA_ARGS__))");
  }

  libmacro::macro_table macros;
};

TEST_F(c11_std_example_macros3, variadic) {
  std::string out;
  out = libmacro::macro_expand("debug(\"Flag\");", &macros, 0);
  ASSERT_EQ("fprintf(stderr, \"Flag\");", out);
  out = libmacro::macro_expand("debug(\"X = %d\\n\", x);", &macros, 0);
  ASSERT_EQ("fprintf(stderr, \"X = %d\\n\", x);", out);
  out = libmacro::macro_expand(
      "showlist(The first, second, and third items.);", &macros, 0);
  ASSERT_EQ("puts(\"The first, second, and third items.\");", out);
  out = libmacro::macro_expand("report(x>y, \"x is %d but y is %d\", x, y);", &macros, 0);
  ASSERT_EQ("((x>y)?puts(\"x>y\"):printf(\"x is %d but y is %d\", x, y));", out);
}

class variadic_headache : public ::testing::Test {
protected:
  variadic_headache() {
    macros.add_define(1, "ADD_END(...) ADD_END_(__VA_ARGS__)");
    macros.add_define(2, "ADD_END_(...) __VA_ARGS__##_END");
    macros.add_define(3, "TEST(args) ADD_END(TEST1 args)");
    macros.add_define(3, "TEST1(arg) #arg TEST2");
    macros.add_define(5, "TEST2(arg) #arg TEST1");
    macros.add_define(6, "TEST1_END");
    macros.add_define(7, "TEST2_END");
  }

  libmacro::macro_table macros;
};

TEST_F(variadic_headache, variadic) {
  std::string out;
  out = libmacro::macro_expand("TEST( (x) (y) (z))", &macros, 0);
  ASSERT_EQ("\"x\" \"y\" \"z\"", out);
}

}  // end namespace
