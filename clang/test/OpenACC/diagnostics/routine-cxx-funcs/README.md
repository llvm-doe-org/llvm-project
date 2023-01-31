# Overview

For C++'s special functions (e.g., constructors, destructors, operators),
Clang's OpenACC `routine` directive diagnostics used to misbehave in multiple
ways because they were originally implemented only for C:

* They overlooked many calls to those functions because, for example, they only
  looked for the creation of CallExpr AST nodes.
* When reporting those diagnostics (or collecting those diagnostics in case they
  might later need to be reported), they failed assertions because they used a
  getter that assumed the kinds of simple function names found in C.
* They overlooked enclosing lambdas because `Sema::getCurFunctionDecl` by
  default skips lambdas.
* When iterating upward through lexically enclosing OpenACC constructs, they
  continued beyond any enclosing lambda and thus saw OpenACC constructs in the
  function containing the lambda.
* They incorrectly assumed that any lexically enclosing `routine` directive
  applies to the nearest lexically enclosing function, but that can be a lambda
  enclosed in the function to which the `routine` directive applies.

The tests in this directory attempt to check that no known cases are overlooked,
that C++ names are handled, that diagnostics are not duplicated by unintentional
duplicate checks in Clang, and that diagnostics affected by the above issues are
correct.  It's too much to check every possible kind of constructor and
overloaded operator, but we try to check major categories (e.g., default
constructor, copy constructor, unary operator, binary operator, conversion
operator), which are typically handled separately within Clang.

Additional diagnostic checking for C++ appears in `../routine.cpp`.

# Organization

This directory contains the following files:

* `*.cpp` files (e.g., `early-uses.cpp`, `call-par-level-*.cpp`) are the main
  test files, each of which checks a particular kind of diagnostic.  Many of
  those diagnostics concern user/usee relationship (typically one function
  calling another).  In that case, the corresponding test files include the
  `*.h` and `*.inc` files below, which provide a common set of usee/user cases
  to check.
* `usee-decls.h` contains declarations of the functions that act as usees in
  the tests.  If `USEE_ADD_DEF_TO_DECL` is `1`, it also produces definitions
  and `routine` directives for for those functions (under the assumption
  `usee-defs.inc` will not be included later), and both appear within the
  enclosing classes and namespace in the case of member functions.
* `usee-defs.inc` contains out-of-class/namespace definitions and `routine`
  directives for those functions (assuming `USEE_ADD_DEF_TO_DECL` is `0`).
* `users.inc` contains various uses of those functions.

# Common Notes

This section contains some notes that are relevant to many of the tests in this directory.

## FIXME: Inheriting Constructors

An inheriting construct is a constructor created by a declaration like
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
reporting that a `routine` directive is too late (in `early-uses.cpp`), the
location should be fixed to point to wherever Clang requires the `routine`
directive to appear before, which currently seems to be the first call to the
inheriting constructor.  In that case, also reporting the `using` declaration's
location would be helpful.

This issue is not relevant to the default constructor, which is built regardless
of the `using` declaration.)

## FIXME: clang::trivial_abi

When a class object is passed by value to a function foo, Clang enforces
restrictions on calling its destructor (e.g., due to a deleted or private
destructor) at foo's callers.  That is true even if the destructor is trivial
(perhaps via `~MyClass() = default;`) and thus isn't actually called.

That is also currently true if the destructor is non-trivial but is considered
trivial due to a `[[clang::trivial_abi]]` attribute on the class.  However, in
this case, the destructor is actually called by foo not by foo's callers (as can
be proven by examining the generated LLVM IR), so it would seem Clang would
enforce the destructor call's restrictions at foo's definition instead.

Just like the rest of Clang, Clang's OpenACC analysis always enforces the
destructor call's restrictions at foo's callers.  However, in the case of a
destructor considered trivial due to `[[clang::trivial_abi]]`, it *additionally*
enforces them at foo's definition.  It needs to because that's where the
destructor is actually called.  Should we then eliminate enforcement at foo's
callers?  So far, we've discovered the following consequences from not
eliminating enforcement there:

* In the case of level-of-parallelism restrictions (`call-par-level-*.cpp`) for
  the destructor's call, enforcement at foo's definition means the
  level-of-parallelism relationship must be foo's caller `>=` foo `>=`
  destructor, which already requires foo's caller `>=` destructor.  Thus,
  enforcement also at foo's callers adds redundant diagnostics but does not
  actually add any new restrictions.
* In the case of late `routine` directive restrictions (`early-uses.cpp`) for
  the destructor's call, enforcement also at foo's callers further restricts
  where the `routine` directive for the destructor can appear.

We might later decide to change where Clang's OpenACC analysis enforces
restrictions for destructors considered trivial due to `[[clang::trivial_abi]]`.
First, we need to better understand the rationale behind Clang's handling of
this case elsewhere.  We've chosen maximal restrictiveness in the meantime.

Occurrences of this issue in these tests are marked by FIXME comments.

Reference for `[[clang::trivial_abi]]`:
<https://quuxplusone.github.io/blog/2018/05/02/trivial-abi-101/>

## `-fexceptions`

The Clang driver normally adds `-fexceptions`, but we use `-cc1` in these tests,
so we add it manually.  With exceptions thus enabled, some diagnostics below
recognize that a `new` expression might call a `delete` operator to clean up
memory allocated by the `new` expression's `new` operator call in the case that
one of the `new` expression's subsequent constructor calls throws an exception.
