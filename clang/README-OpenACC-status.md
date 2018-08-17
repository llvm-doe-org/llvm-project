The lists below give the current status of Clang's OpenACC support
(clacc).

Supported Features
==================

We have implemented the following features:

* command-line options:
    * `-f[no-]openacc`
    * `-Wsource-uses-openacc`
    * `-fopenacc-print=acc|omp|acc-omp|omp-acc`
* targets:
    * host
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
                * All such support depends on clang's corresponding
                  support for OpenMP reductions.  Some operand types
                  might not be supported when compiling the generated
                  OpenMP using a different compiler.
    * `num_gangs`, `num_workers`, `vector_length` clause:
        * The argument in all cases must be a positive integer
          expression.
        * The `vector_length` argument must also be a constant.
        * Notes:
            * OpenACC 2.6 specifies only that the arguments must be
              integer expressions.  However, OpenMP specifies the
              stricter requirements above for `num_teams`,
              `num_threads`, and `simdlen`, to which clacc translates
              the above clauses.
            * A non-positive value here probably doesn't make sense
              anyway.  Moreover, if the argument is an integer
              constant (so that it can be statically analyzed), gcc
              7.3.0 warns if it's non-positive.  However, pgcc 18.4-0
              doesn't warn even if it's negative.
            * It's not clear to me yet why OpenMP requires `simdlen`
              to have a constant expression.  Neither gcc 7.3.0 nor
              pgcc 18.4-0 warn if `vector_length` receives a
              non-constant expression.  However, we're stuck with this
              restriction because we translate to OpenMP.  OpenACC
              does permit the compiler to ignore `vector_length` as a
              hint, so we could choose to ignore it in the case of a
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
        * For now, if none of these three is specified, then a
          sequential loop is produced.
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
        * This is important because clang's implementation does not
          permit `break` statements for OpenMP loops.
        * In the future when `auto` doesn't always produce a
          sequential loop, a `break` statement will force it to be
          sequential, probably with a warning.
    * any number of levels of nesting within other `loop` directives
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
    * offloading to accelerators or multicore
* all directives:
    * clauses not listed in the previous section
    * nesting (other than `loop` directives within `loop` directives
      or within `parallel` directives)
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
