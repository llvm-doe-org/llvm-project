This document describes the current status of Clacc, which extends
Clang and LLVM with support for OpenACC.

Supported Features
==================

We have implemented and tested support for the following features:

* build platforms:
    * linux
* command-line options:
    * `-f[no-]openacc`
    * `-fopenacc[-ast]-print=acc|omp|acc-omp|omp-acc`
    * `-Wsource-uses-openacc`
    * `-Wopenacc-ignored-clause`
    * `-fopenmp-targets=<triples>` for traditional compilation mode
* offloading targets:
    * host, x86_64, or nvptx64
* `parallel` directive:
    * use without clauses
    * data attributes:
        * implicit `copy` for non-scalars
        * implicit `firstprivate` for scalars
        * `copy` clause and aliases `pcopy` and `present_or_copy`
        * `copyin` clause and aliases `pcopyin` and
          `present_or_copyin`
        * `copyout` clause and aliases `pcopyout` and
          `present_or_copyout`
        * in `copy`, `copyin`, and `copyout` clauses and their
          aliases, subarrays specifying contiguous blocks
        * `firstprivate` clause
        * `private` clause
        * `reduction` clause:
            * supports only scalars for now
            * implies `copy` clause (overriding the implicit
              `firstprivate` clause)
            * `+`, `*`, `&&`, and `||` support exactly C11's
              arithmetic types (integer and floating types).
            * `max` and `min` support exactly C11's real types
              (integer types and floating types except complex types)
              and pointer types.
            * `&`, `|`, and `^` support exactly C11's integer types.
            * deviations from OpenACC 2.6:
                * OpenACC 2.6 sec. 2.5.12p774 mistypes `^` as `%`.
                  The latter would be nonsense as a reduction
                  operator.
                * OpenACC 2.6 specifies that reduction operators
                  support "the numerical data types in C", which is
                  not terminology used by the C11 standard, and then
                  it lists specific types.  We use this description
                  only as a guideline as some operators more
                  reasonably support different types than the types
                  listed.  For example, all operators can support
                  `bool` and enumerated types, which are not listed,
                  and `max` and `min` cannot support complex types,
                  which are listed.
                * Specifically, we have designed the operand type
                  constraints for reduction operators to match the
                  operand type constraints for C11's corresponding
                  operators when combined with the general reduction
                  constraint that the result type and both operand
                  types must all be the same type.
                * All such support depends on Clang's corresponding
                  support for OpenMP reductions.  Some operand types
                  might not be supported when compiling the generated
                  OpenMP using a different compiler.
    * `num_gangs`, `num_workers`, `vector_length` clause:
        * The argument in all cases must be a positive integer
          expression.
        * The `vector_length` argument must also be a constant or
          Clacc does not use the clause and reports a warning
          diagnostic, which can be suppressed or converted to an error
          using the `-W{no-,error=}openacc-ignored-clause`
          command-line options.
        * Notes:
            * OpenACC 2.6 specifies only that the arguments must be
              integer expressions.  However, OpenMP specifies the
              stricter requirements above for `num_teams`,
              `num_threads`, and `simdlen`, to which Clacc translates
              the above clauses.
            * A non-positive value here probably doesn't make sense
              anyway.  Moreover, if the argument is an integer
              constant (so that it can be statically analyzed), gcc
              7.3.0 warns if it's non-positive.  However, pgcc 18.4-0
              doesn't warn even if it's negative.
            * Neither gcc 7.3.0 nor pgcc 18.4-0 warn if
              `vector_length` receives a non-constant expression.
              However, we're stuck with this restriction because we
              translate to OpenMP `simdlen`.  OpenACC does permit the
              compiler to ignore `vector_length` as a hint, so we
              choose to ignore it and warn in the case of a
              non-constant expression.
* `loop` directive within a `parallel` directive:
    * use without clauses
    * partitionability:
        * implicit `independent`
        * `seq` clause
        * `independent` clause
        * `auto` clause: for now, always produces a sequential loop
    * partitioning:
        * `gang` clause without arguments
        * `worker` clause without arguments
        * `vector` clause without arguments
        * For now, all three are ignored when combined with `auto`
          clause because, for now, `auto` produces a sequential loop.
        * implicit `gang` clause
            * This feature is not specified in OpenACC 2.7, but
              existing OpenACC compilers implement it in some form.
              Clacc attempts to mimic their behavior, but some details
              might be different.  See the "Semantic Clarifications"
              section in `README-OpenACC-design.md` for details.
        * For now, if none of these clauses appear (explicitly or
          implicitly), then a sequential loop is produced.
    * `collapse` clause
    * data attributes:
        * for loop control variable:
            * implicit `shared` if `seq` is explicitly specified and
              loop control variable is assigned but not declared in
              init of attached `for` loop
            * predetermined `private` otherwise
        * implicit `shared` for other referenced variables
        * `private` clause
        * `reduction` clause:
            * See `reduction` clause for `parallel` directive for
              general details about operand types and limitations.
            * Various subtleties in the semantics of `reduction`
              clauses on `loop` directives are discussed in the
              "Semantic Clarifications" section in
              `README-OpenACC-design.md`.  For example, a reduction on
              a sequential loop specifies a reduction across gangs if
              the reduction variable is gang-shared.  Many of these
              subtleties are under discussion among the OpenACC
              technical committee for clarification in the OpenACC
              spec after 2.7.
    * detection of `break` statement for the associated loop:
        * compile error if implicit/explicit `independent`
        * no error if `seq` or `auto`
        * This is important because Clang's implementation does not
          permit `break` statements for OpenMP loops.
        * In the future when `auto` doesn't always produce a
          sequential loop, a `break` statement will force it to be
          sequential, probably with a warning.
        * Both the OpenACC and OpenMP implementations currently permit
          `break` statements for nested loops that are associated via
          a `collapse` clause, but that's probably a bug.
    * any number of levels of nesting within other `loop` directives
* `parallel loop` directive:
    * All features currently supported by `parallel` and `loop`
      directives are also supported here.
    * `copy` clause implied by `reduction` clause (overriding the
      implicit `firstprivate` clause)
* language support:
    * C11 with the following extensions:
        * `__uint128_t`, `__int128_t`, `__SIZEOF_INT128__`

Skipped Features
================

While implementing support for the above features, we have
intentionally skipped or have not fully tested support for the
following features for now:

* build platforms:
    * windows
* combining OpenMP and OpenACC in the same application:
    * `-fopenmp` is an error when OpenACC support is enabled.
    * As usual when `-fopenmp` is not specified, OpenMP directives are
      discarded, but `-Wsource-uses-openmp` is available as usual to
      produce warnings for them.
* command-line options:
    * `-fopenmp-targets=<triples>` for source-to-source mode
    * other `-fopenmp-*` options
* all directives:
    * clauses not listed in the previous section
    * nesting (other than `loop` directives within `loop` directives
      or within `parallel` directives)
    * non-scalars in `reduction` clauses
    * subarrays specifying non-contiguous blocks in dynamic
      multidimensional arrays
    * subarrays in `firstprivate`, `private`, and `reduction` clauses
    * subarrays with no `:` and one integer (syntactically an array
      subscript)
* `loop` directive:
    * outside a `parallel` directive
    * `gang`, `worker`, and `vector` clause arguments
* language support:
    * C extensions:
        * nested function definitions
    * C++
    * Objective-C/C++

Other Features
==============

We have not yet considered features that are not mentioned in either
list above.

OpenMP Exposure
===============

The Clacc user should not have to be aware that OpenMP is utilized
internally when running Clacc in traditional OpenACC compilation mode.
However, as listed below, there are several cases where Clacc's
OpenACC implementation currently relies on Clang's OpenMP
implementation in ways that expose the use of OpenMP to the Clacc
user.  Each case is intended as a temporary strategy to accelerate
development of Clacc, and the intention is to eliminate these cases as
Clacc matures.  Please report any cases not listed below.

* OpenMP command-line options for offloading.  Notes:
    * For usage details, see the section "Using" in `../README.md`.
    * For design details, see the section "Interaction with OpenMP
      Support" in `README-OpenACC-design.md`.
* OpenMP canonical loop form diagnostics for an OpenACC `loop`
  directive's associated loop.  Notes:
    * OpenACC 2.7 does not specify a canonical loop form, but Clacc is
      bound by the forms Clang's OpenMP implementation will accept.
    * This case will be easier to eliminate if the OpenACC
      specification does specify a canonical loop form, [which has
      been discussed by the OpenACC technical
      committee](https://github.com/OpenACC/openacc-spec/issues/39).
* Some OpenMP array section diagnostics for OpenACC subarrays.  Notes:
    * For example, for non-contiguous subarrays, the diagnostics use
      array section terminology instead.
* `OMPArraySectionExpr` AST node for OpenACC subarrays.  Notes:
    * This might impact Clang tool developers but should not impact
      OpenACC application developers.
    * It seems the most straight-forward fix would be to rename this
      class to something more general like `ArrayRangeExpr` or
      `ArraySubscriptExtendedExpr`.  See the todo on this class in the
      implementation.
