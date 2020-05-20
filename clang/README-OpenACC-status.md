This document describes the current status of Clacc, which extends
Clang and LLVM with support for OpenACC.

Supported Features
==================

We have implemented and tested support for the following features:

* build platforms:
    * Ubuntu 18.04
    * CentOS 7.7
* command-line options:
    * `-f[no-]openacc`
    * `-fopenacc[-ast]-print=acc|omp|acc-omp|omp-acc`
        * See the section "Source-to-Source Translation" in
          `README-OpenACC-design.md` for design details.
    * `-fopenmp-targets=<triples>` for offloading in traditional
      compilation mode, and omitted for targeting host
        * See the section "Using" in `../README.md` for a list of
          tested offloading target triples.
        * See the section "Interaction with OpenMP Support" in
          `README-OpenACC-design.md` for design details.
    * `-fopenacc-present-omp=KIND` where `KIND` is either `present` or
      `alloc`
        * See the discussion of the `present` clause below.
    * `-Wsource-uses-openacc`
    * `-Wopenacc-ignored-clause`
        * See the discussion of the `vector_length` clause below.
    * `-Wopenacc-omp-map-present`
        * See the discussion of the `present` clause below.
    * Notes:
        * See the section "Using" in `../README.md` for an
          introduction to Clacc's command-line options.
        * For brief descriptions of all OpenACC-related and
          OpenMP-related command-line options, run Clacc's `clang
          -help` and search for `openacc` or `openmp`.
* run-time environment variables:
    * `OMP_TARGET_OFFLOAD=disabled` for targeting host
* `data` directive:
    * `present` clause
        * Traditional compilation mode:
            * The `present` clause is fully supported.
            * Although the traditional compilation mode user typically
              does not need to be aware, the OpenMP translation of the
              `present` clause uses an OpenMP TR8 feature: the
              `present` map type modifier.  This modifier is combined
              with the OpenMP `alloc` map type as opposed to, for
              example, `tofrom` in order to suppress device-to-host
              transfers in the case that there is a reference count
              decrement within the associated region.
            * If desired, it is possible to adjust the translation or
              related diagnostics by using the command-line options
              discussed below for source-to-source mode.  The
              difference is that, by default in traditional
              compilation mode, the related diagnostics are disabled.
        * Source-to-source mode:
            * Occurrences of the `present` clause produce a
              compile-time error diagnostic by default.  The purpose
              of the diagnostic is to ensure the user is aware that
              the translation includes the `present` map type modifier
              because it is unlikely to be supported yet by foreign
              OpenMP compilers.
            * The diagnostic is actually the warning enabled by
              `-Wopenacc-omp-map-present`, which is enabled and
              treated as an error by default in source-to-source mode.
            * To work around this issue, the following command-line
              options can be specified:
                * `-Wno-error=openacc-omp-map-present` converts the
                  diagnostic to a warning to make it easier to find
                  all occurrences.
                * `-Wno-openacc-omp-map-present` disables the
                  diagnostic entirely.  This is useful if the
                  generated OpenMP will not be compiled or if the
                  OpenMP compiler actually already supports the
                  `present` map type modifier.
                * `-fopenacc-present-omp=alloc` suppresses the
                  diagnostic by changing the translation to use the
                  standard OpenMP `alloc` map type without the
                  `present` modifier.  In this case, there is no
                  runtime error when the specified variable is not
                  present on the device.  However, this translation
                  should be sufficient for OpenACC applications that
                  are robust enough not to actually encounter this
                  runtime error.
        * `-fopenacc[-ast]-print=acc`, `-ast-print`, `-ast-dump`,
          etc. mode:
            * Debugging modes like these do not actually print OpenMP
              source code, so they leave the aforementioned diagnostic
              disabled as in traditional compilation mode.
        * Currently, Clacc's implementation of the OpenMP TR8
          `present` map type modifier is not well tested outside of
          Clacc translations from OpenACC to OpenMP.  Thus, it is not
          yet recommended for use in hand-written OpenMP code as it
          might not integrate well with some OpenMP features.
        * See the section "Data Directives" in
          `README-OpenACC-design.md` for a discussion of alternative
          translations that we considered for the OpenACC `present`
          clause.
    * `copy` clause and aliases `pcopy` and `present_or_copy`
    * `copyin` clause and aliases `pcopyin` and
      `present_or_copyin`
    * `copyout` clause and aliases `pcopyout` and
      `present_or_copyout`
    * in `present`, `copy`, `copyin`, and `copyout` clauses and their
      aliases, subarrays specifying contiguous blocks
    * any number of levels of nesting within other `data` directives
* `parallel` directive:
    * use without clauses
    * data attributes:
        * implicit `copy` for non-scalars
        * implicit `firstprivate` for scalars
        * For `present`, `copy`, `copyin`, and `copyout` clauses and
          their aliases, support is the same as for the `data`
          directive, as described above.
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
    * nesting within a `data` directive
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
* OpenACC Profiling Interface
    * See the section "OpenACC Profiling Interface" in
      `README-OpenACC-design.md` for features and limitations.
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
    * non-scalars in `reduction` clauses
    * subarrays specifying non-contiguous blocks in dynamic
      multidimensional arrays
    * subarrays in `firstprivate`, `private`, and `reduction` clauses
    * subarrays with no `:` and one integer (syntactically an array
      subscript)
* `loop` directive:
    * outside a `parallel` directive (that is, an orphaned loop)
    * `gang`, `worker`, and `vector` clause arguments
* `data`, `parallel`, and `parallel loop` directive:
    * inside a `parallel`, `loop`, or `parallel loop` directive (we're
      not aware of any implementation that supports this)
* language support:
    * C extensions:
        * nested function definitions
    * C++
    * Objective-C/C++
* source-to-source mode using `-fopenacc-print`:
    * Preprocessor macros appearing within OpenACC clauses or
      associated statements are expanded in the OpenMP translation.
      Notes:
        * See the "Source-to-Source Translation" section in
          `README-OpenACC-design.md` for an explanation of why this
          happens and how Clacc might evolve to prevent it in the
          future.
    * Preprocessor macro usage can sometimes prevent OpenACC
      constructs from being translated:
        * Clacc cannot translate an OpenACC construct if it meets
          either of the following conditions:
            * The construct's first token is expanded from a
              preprocessor macro.  Notes:
                * C/C++ syntax limits this case to the `_Pragma` form.
                  That is, it's not possible for `#pragma` form.
            * The associated statement must be rewritten but its last
              token is expanded from a preprocessor macro.  Notes:
                * In the case of `#pragma` form, whether the
                  associated statement must be rewritten depends on
                  Clacc's mapping for the construct to OpenMP.
                * In the case of `_Pragma` form, currently the
                  associated statement must always be rewritten.  Due
                  to the first condition above, this case is only
                  relevant when `_Pragma` form is used outside a
                  preprocessor macro.
        * Clacc reports an error diagnostic for every such OpenACC
          construct in the source file.  Clacc then prints a version
          of the source in which all OpenACC constructs are
          transformed except the reported ones.
        * For a full transformation of the source file in this case,
          try `-fopenacc-ast-print` instead.  However, its output
          looks like the preprocessor output, which is not appropriate
          for some use cases.

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
* OpenMP runtime error diagnostics are expressed in terms of OpenMP
  not OpenACC.

OpenMP Extensions
=================

There are some OpenACC features for which Clacc depends on OpenMP
extensions because we are not aware of standard OpenMP features that
are sufficient.  While this dependence should not affect traditional
compilation using Clacc's compiler and OpenACC/OpenMP runtime, it can
affect compilation using Clacc's source-to-source mode followed by a
foreign OpenMP compiler or runtime.  A goal of Clacc is to rely on
standard OpenMP as much as possible, and to that end it might be
worthwhile to propose these extensions for inclusion in the OpenMP
specification.  Currently, Clacc uses OpenMP extensions as follows:

* The `present` clause translation depends on the OpenMP TR8 map type
  modifier `present` by default:
    * See the discussion of the `present` clause above for details.
* Some features of the OpenACC Profiling Interface depend on OMPT
  extensions:
    * See the section "OpenACC Profiling Interface" in
      `README-OpenACC-design.md` for details.
