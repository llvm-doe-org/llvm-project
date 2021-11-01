# Clang's OpenACC Test Suite

Clang's OpenACC test suite is organized into subdirectories.  A
subdirectory not only identifies the high-level subject of its tests,
but it also identifies the kinds of checks they run, as described
below.

## Kinds of checks

Major categories of checks in this test suite include:

* Dump
    * FileCheck prefixes start with `DMP`.
    * These checks examine Clang's AST representation for OpenACC
      source as well as for its OpenMP translation.  Thus, they use
      Clang's `-ast-dump` option.
    * Besides checking that `-ast-dump` itself is reliable, these
      checks usually catch some of the earliest issues in Clang's
      handling of OpenACC directives, independently of whether
      traditional compilation or source-to-source mode is selected.
* Print
    * FileCheck prefixes start with `PRT`.
    * These checks examine Clang's printing of source code after
      translating OpenACC to OpenMP within the AST.  Thus, they use
      Clang options like `-fopenacc-print=omp`.
    * While these checks can be helpful in catching traditional
      compilation issues, they are especially critical for testing
      source-to-source mode.
* Execution
    * FileCheck prefixes start with `EXE`.
    * These checks fully compile, execute, and examine the behavior of
      toy OpenACC applications on all supported offload device types
      for which hardware is available on the system.
    * These checks separately attempt both traditional compilation
      mode and also source-to-source mode followed by OpenMP
      compilation.
    * These checks are critical for verifying correct end-to-end
      behavior of Clang's OpenACC support when combined with the rest
      of LLVM's OpenACC and OpenMP support.

## Subdirectories

The subdirectories of this test suite are as follows, where dump,
print, and execution checks are included only where stated below:

* `diagnostics`
    * These tests check various OpenACC-related diagnostics from
      Clang.
* `directives`
    * These tests check Clang's handling of OpenACC directives and
      clauses.
    * Most tests include dump, print, and execution checks, reusing
      the same OpenACC application source for all three.
* `driver`
    * These tests check OpenACC support in Clang's driver.
* `preprocessor`
    * These tests check OpenACC support in Clang's preprocessor.
    * For example, `_OPENACC` handling.
* `print`
    * These tests check various print issues that are general to many
      OpenACC directives and that seem to deserve special scrutiny
      beyond the directive-specific print checks in `directives`.

## Running the test suite

Clang's entire OpenACC test suite can be run as follows:

```
$ make check-clang-openacc
```

To run only a particular subdirectory, add its name after a hypen.
For example:

```
$ make check-clang-openacc-diagnostics
$ make check-clang-openacc-directives
```

## Relationship with libacc2omp's test suite

libacc2omp's test suite contains execution checks for OpenACC runtime
routines, profiling support, etc.  However, libacc2omp's test suite
also has a `directives` subdirectory.  Tests belong there instead of
in Clang's OpenACC `directives` subdirectory when they focus on
directives' runtime behavior and do not include dump, print, or other
Clang-focused checks.

Outside of OpenACC testing, it is unusual for Clang's test suite to
include execution checks.  For example, for OpenMP, such execution
tests are normally only part of the OpenMP runtime test suite.  In the
future, Clang's OpenACC test suite's execution checks might migrate to
execute as part of libacc2omp's test suite.  However, the tests
themselves will likely continue to reside within Clang's OpenACC test
suite so that only one copy of the tests' OpenACC source has to be
maintained across dump, print, and execution checks.
