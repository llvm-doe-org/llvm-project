# Overview

For C++'s special functions (e.g., constructors, destructors, operators),
Clang's OpenACC `routine` directive diagnostics used to misbehave in multiple
ways because they were originally implemented only for C:

* They overlooked many calls to those functions because, for example, they only
  looked for the creation of CallExpr AST nodes.
* When reporting those diagnostics (or collecting those diagnostics in case they
  might later need to be reported), they failed assertions because they used a
  getter that assumed the kinds of simple function names found in C.

The tests in this directory attempt to check that no known cases are overlooked,
that C++ names are handled, and that diagnostics are not duplicated by
unintentional duplicate checks in Clang.  It's too much to check every possible
kind of constructor and overloaded operator, but we try to check major
categories (e.g., default constructor, copy constructor, unary operator, binary
operator, conversion operator), which are typically handled separately within
Clang.

# Organization

This directory contains the following files:

* `*.cpp` files (e.g., `early-uses.cpp`, `call-par-level.cpp`) are the main
  test files, each of which checks a particular kind of diagnostic.  Many of
  those diagnostics concern user/usee relationship (typically one function
  calling another).  In that case, the corresponding test files include the
  `*.h` and `*.inc` files below, which provide a common set of usee/user cases
  to check.
* `usee-decls.h` contains declarations of the functions that act as usees in
  the tests.
* `usee-defs.inc` contains definitions and `routine` directives for those
  functions.
* `users.inc` contains various uses of those functions.

# Common Notes

This section contains some notes that are relevant to many of the tests in this directory.

## FIXME: Inheriting Constructors

FIXME: An inheriting construct is a constructor created by a declaration like
`using MyBaseClass::MyBaseClass;`.  For some reason, Clang doesn't build an
inheriting constructor and mark the functions it uses until it is actually
called.  However, some uses within it (e.g., its call to the base class
destructor) are marked with the location of the `using` declaration, but other
uses within it (in particular, its call to the base class's inherited
constructor) are marked with the location of the inheriting constructor's first
call.  This is the way Clang internally handles locations of uses within an
inheriting constructor, so we haven't tried to change it for OpenACC support.
However, it makes some OpenACC diagnostics confusing.

Occurrences of this issue in these tests are marked by FIXME comments.  When
reporting that a `routine` directive is too late (in `early-uses.cpp` ), the
location should be fixed to point to wherever Clang requires the `routine`
directive to appear before, which currently seems to be the first call to the
inheriting constructor.  In that case, also reporting the `using` declaration's
location would be helpful.

This issue is not relevant to the default constructor, which is built regardless
of the `using` declaration.)

## `-fexceptions`

The Clang driver normally adds `-fexceptions`, but we use `-cc1` in these tests,
so we add it manually.  With exceptions thus enabled, some diagnostics below
recognize that a `new` expression might call a `delete` operator to clean up
memory allocated by the `new` expression's `new` operator call in the case that
one of the `new` expression's subsequent constructor calls throws an exception.
