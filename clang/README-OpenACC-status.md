This document describes the current status of Clacc, which extends
Clang and LLVM with support for OpenACC.

Supported Features
==================

We have implemented the following features:

* command-line options:
    * `-f[no-]openacc`
    * `-fopenacc[-ast]-print=acc|omp|acc-omp|omp-acc`
    * `-Wsource-uses-openacc`
    * `-Wopenacc-discarded-clause`
* targets:
    * host or multicore
* `parallel` directive:
    * use without clauses
    * data sharing:
        * implicit `shared`
        * implicit `firstprivate`
        * `firstprivate` clause
        * `private` clause
        * `reduction` clause:
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
                * The interaction between `reduction` and other data
                  sharing attributes is not exactly as specified in
                  OpenACC 2.6.  For example, in Clacc, any gang-like
                  reduction specified on an `acc loop` or `acc
                  parallel loop` directive implies a reduction clause
                  on the enclosing `acc parallel` directive, and that
                  makes the reduction variable gang-private within the
                  `acc parallel`, but OpenACC leaves the possibility
                  that it could be gang-shared.  Moreover, Clacc
                  considers `firstprivate`, `private`, and `reduction`
                  to be contradictory specifications for a variable,
                  but OpenACC treats `reduction` as orthogonal to the
                  others.  OpenACC 2.6 is actually unclear in many
                  cases about the handling of `reduction`, and many
                  improvements have been proposed for OpenACC 2.7.  We
                  will update Clacc as the OpenACC specification is
                  improved.
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
          Clacc discards the clause and reports a warning diagnostic,
          which can be suppressed or converted to an error using the
          `-W{no-,error=}openacc-discarded-clause` command-line
          options.
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
              translate to OpenMP.  OpenACC does permit the compiler
              to ignore `vector_length` as a hint, so we choose to
              discard it and warn in the case of a non-constant
              expression.
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
        * For now, if none of these three is specified, then a
          sequential loop is produced.
    * `collapse` clause
    * data sharing:
        * for loop control variable:
            * implicit `shared` if sequential loop and loop control
              variable is assigned but not declared in init of
              attached `for` loop
            * implicit `private` otherwise
        * implicit `shared` for other referenced variables
        * `private` clause
        * `reduction` clause:
            * Discarded in the case of sequential loops.  This
              decision should not impact behavior as long as the
              operator that the loop body effectively performs on the
              variable is the same as the `reduction` operator.
            * See `reduction` clause for `parallel` directive for
              general details about operand types and limitations.
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
* `acc parallel loop`:
    * All features currently supported by `acc parallel` and `acc
      loop` are also supported here.
* language support:
    * C11 with the following extensions:
        * `__uint128_t`, `__int128_t`, `__SIZEOF_INT128__`

Skipped Features
================

While implementing the above features, we have intentionally skipped
the following features for now:

* combining OpenMP and OpenACC in the same application
* command-line options:
    * option to specify target
* targets:
    * offloading to accelerators
* all directives:
    * clauses not listed in the previous section
    * nesting (other than `loop` directives within `loop` directives
      or within `parallel` directives)
* `loop` directive:
    * outside a `parallel` directive
    * `gang`, `worker`, and `vector` clause arguments
* Implicit `copy` clause for variables specified in `reduction`
  clauses on `acc parallel` and `acc parallel loop` directives:
    * This behavior is proposed for OpenACC 2.7 and is more intuitive
      than the behavior specified by OpenACC 2.6.
    * For example, for `acc parallel loop worker reduction(*:x)`,
      OpenACC 2.6 specifies an implicit `firstprivate(x)` on the
      effective `acc parallel`, making the reduction useless.
      However, the proposed OpenACC 2.7 behavior is for `copy(x)` to
      be implied on the effective `acc parallel`, making the reduction
      useful.
    * Imagine if we change `worker` to `gang` in the above example.
      The correct OpenACC 2.6 behavior is not clear because now we
      have a gang reduction on a gang-private variable.  If we split
      the combined directive with the reduction explicitly specified
      only on the `acc loop`, the proposed OpenACC 2.7 behavior is
      also not clear because `firstprivate(x)` is then implied, so we
      again have a gang reduction on a gang-private variable.
      `copy(x)` probably should be implied in this case.
    * Currently, Clacc handles all gang-like reductions that would be
      specified by the OpenACC 2.7 behavior as a reduction on the `acc
      parallel`.  However, that means all references to the reduction
      variable within the `acc parallel` refer to gang-private copies
      even though the proposed OpenACC 2.7 behavior in some cases is
      for those references to refer to the gang-shared variable.  See
      "Unmappable Features" in README-OpenACC-design.md for details on
      how we might or might not fix this issue in the OpenMP
      translation.
* language support:
    * C extensions:
        * nested function definitions
    * C++
    * Objective-C/C++

Other Features
==============

We have not yet considered features that are not mentioned in either
list above.
