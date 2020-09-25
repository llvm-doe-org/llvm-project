This document describes the current status of Clacc, which extends
Clang and LLVM with support for OpenACC.

Supported Features
==================

This section catalogs features that Clacc currently implements to
support OpenACC, and it identifies known limitations in those
features.  Clacc does not yet support OpenACC features that are not
mentioned here.

* Build platforms
    * Ubuntu 18.04 is regularly tested.
    * CentOS 7.8 is regularly tested.
    * Windows and macOS are not yet supported.
* Command-line options
    * See the section "Using" in `../README.md` for an introduction to
      Clacc's command-line options.
    * For brief descriptions of all OpenACC-related and OpenMP-related
      command-line options, run Clacc's `clang -help` and search for
      `openacc` or `openmp`.
    * `-f[no-]openacc` enables OpenACC support.  Traditional
      compilation mode is the default.
    * `-fopenacc[-ast]-print=acc|omp|acc-omp|omp-acc`
        * Enables OpenACC support and source-to-source mode.
        * See the section "Source-to-Source Translation" in
          `README-OpenACC-design.md` for design details.
    * `-fopenmp-targets=<triples>`
        * In traditional compilation mode, specifies offloading device
          instead of targeting host.
        * Not supported for source-to-source mode
        * See the section "Using" in `../README.md` for a list of
          tested offloading target triples.
        * See the section "Interaction with OpenMP Support" in
          `README-OpenACC-design.md` for design details.
    * Other OpenMP options
        * `-fopenmp` produces an error when OpenACC support is enabled
          as Clacc does not currently support combining OpenMP and
          OpenACC in the same application.
        * `-fopenmp-*` options not mentioned above are not supported
          when OpenACC support is enabled.
        * As usual when `-fopenmp` is not specified, OpenMP directives
          are discarded but `-Wsource-uses-openmp` is available to
          produce warnings for them.
    * `-fopenacc-update-present-omp=KIND` where `KIND` is either
      `present` or `no-present`
        * See the discussion of the `update` directive below.
    * `-fopenacc-present-omp=KIND` where `KIND` is either `present` or
      `alloc`
        * See the discussion of the `present` clause below.
    * `-fopenacc-no-create-omp=KIND` where `KIND` is either `no_alloc`
      or `alloc`
        * See the discussion of the `no_create` clause below.
    * `-Wsource-uses-openacc`
        * Similar to `-Wsource-uses-openmp`, this enables warnings
          about OpenACC directives when OpenACC support is not
          enabled.
    * `-Wopenacc-ignored-clause`
        * See the discussion of the `vector_length` clause below.
    * `-Wopenacc-omp-update-present`
        * See the discussion of the `update` directive below.
    * `-Wopenacc-omp-map-present`
        * See the discussion of the `present` clause below.
    * `-Wopenacc-omp-map-no-alloc`
        * See the discussion of the `no_create` clause below.
* Run-time environment variables
    * `OMP_TARGET_OFFLOAD=disabled` for specifying at run time to
      target the host.
    * `ACC_PROFLIB` for the OpenACC Profiling Interface.
* `update` directive
    * Lexical context
        * Appearing within a `data` construct is supported.
    * Supported clauses
        * `if_present`
        * `self` and alias `host`
        * `device`
    * Subarrays specifying contiguous blocks are supported.
    * Multiple subarrays of the same array on the same directive are
      not yet supported.
    * Members of structs or classes are not yet supported.
    * Source-to-source mode caveats for presence restriction when
      `if_present` is not specified
        * OpenACC 3.0 requires variables appearing in `self`, `host`,
          or `device` to be already present on the device when
          `if_present` is not specified.  However, while it suggests a
          runtime error when this restriction is violated by an
          application, it does not strictly require any specific
          behavior.  Changes have been proposed to the OpenACC
          specification to strictly require a runtime error instead.
        * Traditional compilation mode
            * The behavior described above is fully supported.
            * Although the traditional compilation mode user typically
              does not need to be aware, the OpenMP translation of the
              `self`, `host`, and `device` clauses use an OpenMP TR8
              feature when `if_present` is not specified: the
              `present` motion modifier.
            * If desired, it is possible to adjust the translation or
              related diagnostics by using the command-line options
              discussed below for source-to-source mode.  The
              difference is that, by default in traditional
              compilation mode, the related diagnostics are disabled.
        * Source-to-source mode
            * Occurrences of the `self`, `host`, or `device` clause
              without the `if_present` clause produce compile-time
              error diagnostics by default.  The purpose of the
              diagnostics is to ensure the user is aware that the
              OpenMP translation includes the `present` motion
              modifier because it is unlikely to be supported yet by
              foreign OpenMP compilers.
            * The diagnostics are actually warnings enabled by
              `-Wopenacc-omp-update-present`, which is enabled and
              treated as an error by default in source-to-source mode.
            * To work around this issue, any of the following
              command-line options can be specified:
                * `-Wno-error=openacc-omp-update-present` converts the
                  diagnostic to a warning to make it easier to find
                  all occurrences.
                * `-Wno-openacc-omp-update-present` disables the
                  diagnostic entirely.  This is useful if the
                  generated OpenMP will not be compiled or if the
                  OpenMP compiler actually already supports the
                  `present` motion modifier.
                * `-fopenacc-update-present-omp=no-present` suppresses
                  the diagnostic by changing the translation not to
                  use the `present` motion modifier.  Contrary to
                  OpenACC semantics, this translation does not produce
                  a runtime error when the specified variable is not
                  present on the device.  However, this translation
                  should be sufficient for OpenACC applications that
                  are robust enough not to actually encounter this
                  runtime error.
                * There are various other imperfect translations of
                  these clauses to standard OpenMP that might be
                  useful under other conditions.  We will consider
                  adding support if there is user demand.  See the
                  section "Update Directives" in
                  `README-OpenACC-design.md` for a discussion of
                  alternative translations that we considered.
        * `-fopenacc[-ast]-print=acc`, `-ast-print`, `-ast-dump`,
          etc. mode
            * Debugging modes like these do not actually print OpenMP
              source code, so they leave the aforementioned diagnostic
              disabled as in traditional compilation mode.
* `data` directive
    * Lexical context
        * Any number of levels of nesting within other `data`
          constructs is supported.
    * Supported clauses
        * `present`
        * `copy` and aliases `pcopy` and `present_or_copy`
        * `copyin` and aliases `pcopyin` and `present_or_copyin`
            * `readonly` modifier is not yet supported.
        * `copyout` and aliases `pcopyout` and `present_or_copyout`
            * `zero` modifier is not yet supported.
        * `create` and aliases `pcreate` and `present_or_create`
            * `zero` modifier is not yet supported.
        * `no_create`
    * Subarrays specifying contiguous blocks are supported.
    * Source-to-source mode caveats for `present` and `no_create`
        * Traditional compilation mode
            * The `present` and `no_create` clauses are fully
              supported.
            * Although the traditional compilation mode user typically
              does not need to be aware, the OpenMP translation of
              these clauses uses the following features:
                * The `present` translation uses an OpenMP
                  TR8 feature: the `present` map type modifier.
                * The `no_create` translation uses a Clacc-specific
                  OpenMP extension: the `no_alloc` map type modifier.
            * In each case, the map type modifier is combined with the
              OpenMP `alloc` map type as opposed to, for example,
              `tofrom` in order to suppress device-to-host transfers
              in the case that there is a reference count decrement
              within the associated region.
            * If desired, it is possible to adjust the translation or
              related diagnostics by using the command-line options
              discussed below for source-to-source mode.  The
              difference is that, by default in traditional
              compilation mode, the related diagnostics are disabled.
        * Source-to-source mode
            * Occurrences of the `present` or `no_create` clause
              produce compile-time error diagnostics by default.  The
              purpose of the diagnostics is to ensure the user is
              aware that the OpenMP translation includes the `present`
              or `no_alloc` map type modifier because it is unlikely
              to be supported yet by foreign OpenMP compilers.
            * The diagnostics are actually warnings enabled by
              `-Wopenacc-omp-map-present` or
              `-Wopenacc-omp-map-no-alloc`, which is enabled and
              treated as an error by default in source-to-source mode.
            * To work around this issue, any of the following
              command-line options can be specified:
                * `-Wno-error=openacc-omp-map-present` or
                  `-Wno-error=openacc-omp-map-no-alloc` converts the
                  diagnostic to a warning to make it easier to find
                  all occurrences.
                * `-Wno-openacc-omp-map-present` or
                  `-Wno-openacc-omp-map-no-alloc` disables the
                  diagnostic entirely.  This is useful if the
                  generated OpenMP will not be compiled or if the
                  OpenMP compiler actually already supports the
                  `present` or `no_alloc` map type modifier.
                * `-fopenacc-present-omp=alloc` suppresses the
                  diagnostic for the OpenACC `present` clause by
                  changing its translation to use the standard OpenMP
                  `alloc` map type without the `present` modifier.
                  Contrary to OpenACC `present` clause semantics, this
                  translation does not produce a runtime error when
                  the specified variable is not present on the device.
                  However, this translation should be sufficient for
                  OpenACC applications that are robust enough not to
                  actually encounter this runtime error.
                * `-fopenacc-no-create-omp=alloc` suppresses the
                  diagnostic for the OpenACC `no_create` clause by
                  changing its translation to use the standard OpenMP
                  map type `alloc` without the `no_alloc` modifier.
                  Contrary to OpenACC `no_create` clause semantics,
                  this translation attempts to allocate the specified
                  data when it is not already present on the device.
                  Thus, this translation is probably only useful when
                  all of the following conditions are met:
                    * The application is being tested with smaller
                      data sets, perhaps in initial attempts to port
                      to OpenMP.  Otherwise, the unexpected
                      allocations could produce performance
                      degradation or memory exhaustion.
                    * Any subarray specified in a `no_create` clause
                      never conflicts with any subarray already
                      present on the device or any subarray
                      specification encountered during the execution
                      of the associated region.  The unexpected
                      allocations can cause such subarray conflicts to
                      produce compile-time or runtime errors.
                    * For any variable specified in a `no_create`
                      clause, no data transfers for that variable are
                      encountered during the execution of the
                      associated region.  The unexpected allocations
                      can suppress those data transfers.
                * There are various other imperfect translations of
                  these clauses to standard OpenMP that might be
                  useful under other conditions.  We will consider
                  adding support if there is user demand.  See the
                  section "Data Directives" in
                  `README-OpenACC-design.md` for a discussion of
                  alternative translations that we considered for the
                  OpenACC `present` clause.
        * `-fopenacc[-ast]-print=acc`, `-ast-print`, `-ast-dump`,
          etc. mode
            * Debugging modes like these do not actually print OpenMP
              source code, so they leave the aforementioned
              diagnostics disabled as in traditional compilation mode.
        * Currently, Clacc's implementation of its `no_alloc` map type
          modifier extension for OpenMP is not well tested outside of
          Clacc's translations from OpenACC to OpenMP.  Thus, it is
          not yet recommended for use in hand-written OpenMP code as
          it might not integrate well with some OpenMP features.
* `parallel` directive
    * Lexical context
        * Appearing within a `data` construct is supported.
    * Use without clauses is supported.
    * Supported data attributes and clauses
        * Implicit `copy` for non-scalars
        * Implicit `firstprivate` for scalars
        * For `present`, `copy`, `copyin`, `copyout`, `create`, and
          `no_create` clauses and their aliases, support is the same
          as for the `data` directive, as described above, including
          source-to-source mode caveats.
        * `firstprivate` clause
        * `private` clause
        * `reduction` clause
            * Supports only scalars for now.
            * Implies `copy` clause (overriding the implicit
              `firstprivate` clause).
            * `+`, `*`, `&&`, and `||` support exactly C11's
              arithmetic types (integer and floating types).
            * `max` and `min` support exactly C11's real types
              (integer types and floating types except complex types)
              and pointer types.
            * `&`, `|`, and `^` support exactly C11's integer types.
            * Deviations from OpenACC 2.6
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
    * `num_gangs`, `num_workers`, `vector_length` clauses
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
* `loop` directive
    * Lexical context
        * Appearing within a `parallel` construct and any number of
          levels of nesting within other `loop` directives are
          supported.
        * Appearing outside a `parallel` construct (that is, an
          orphaned loop) is not yet supported.
    * Use without clauses is supported.
    * Supported partitionability clauses
        * Implicit `independent`
        * `seq`
        * `independent`
        * `auto`
            * For now, always produces a sequential loop.
    * Supported partitioning clauses
        * `gang`
        * `worker`
        * `vector`
        * Arguments to those clauses are not yet supported.
        * For now, all three are ignored when combined with `auto`
          clause because, for now, `auto` produces a sequential loop.
        * Implicit `gang` clause
            * This feature is not specified in OpenACC 2.7, but
              existing OpenACC compilers implement it in some form.
              Clacc attempts to mimic their behavior, but some details
              might be different.  See the "Semantic Clarifications"
              section in `README-OpenACC-design.md` for details.
        * For now, if none of these clauses appear (explicitly or
          implicitly), then a sequential loop is produced.
    * The `collapse` clause is supported.
    * Supported data attributes and clauses
        * A loop control variable is:
            * Implicit `shared` if `seq` is explicitly specified and
              loop control variable is assigned but not declared in
              init of attached `for` loop.
            * Predetermined `private` otherwise.
        * Implicit `shared` for other referenced variables
        * `private` clause
        * `reduction` clause
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
    * Detection of `break` statement for the associated loop
        * Compile error if implicit/explicit `independent`.
        * No error if `seq` or `auto`.
        * This is important because Clang's implementation does not
          permit `break` statements for OpenMP loops.
        * In the future when `auto` doesn't always produce a
          sequential loop, a `break` statement will force it to be
          sequential, probably with a warning.
        * Both the OpenACC and OpenMP implementations currently permit
          `break` statements for nested loops that are associated via
          a `collapse` clause, but that's probably a bug.
* `parallel loop` directive
    * All features currently supported by `parallel` and `loop`
      directives are also supported here.
    * A `reduction` clause implies a `copy` clause (overriding the
      implicit `firstprivate` clause).
* Subarrays
    * Subarrays specifying contiguous blocks are supported.
    * Subarrays specifying non-contiguous blocks in dynamic
      multidimensional arrays are not yet supported.
    * Subarrays in `firstprivate`, `private`, and `reduction` clauses
      are not yet supported.
    * Subarrays with no `:` and one integer (syntactically an array
      subscript, such as `arr[5]`) are not yet supported.
* Device-side directives
    * Nesting of an `update`, `data`, `parallel`, or `parallel loop`
      directive inside a `parallel`, `loop`, or `parallel loop`
      construct is not yet supported.
    * We're not aware of any OpenACC implementation that supports this
      yet.
* OpenACC Profiling Interface
    * Clacc's support has been tested with TAU.
    * The main limitations are currently as follows:
        * Multiple callbacks per event type and callback toggling are
          not yet supported.
        * Callback registration is permitted only within the
          `acc_register_library` function.
        * `acc_ev_wait_start` and `acc_ev_wait_end` event types are
          not yet supported.
        * The `kernel_name`, `num_gangs`, `num_workers`,
          `vector_length`, and `tool_info` fields are not yet
          supported.
        * The `acc_api_info` structure is not yet supported.
    * See the section "OpenACC Profiling Interface" in
      `README-OpenACC-design.md` for a more detailed description.
* Language support
    * C11 is supported with the following extensions:
        * `__uint128_t`, `__int128_t`, `__SIZEOF_INT128__`
    * The nested function definition extension for C is not yet
      supported.
    * C++ is not yet supported.
    * Objective-C/C++ are not supported.

Source-to-Source Mode Limitations
=================================

* Preprocessor macros appearing within OpenACC clauses or associated
  statements are expanded in the OpenMP translation.  Notes:
    * See the "Source-to-Source Translation" section in
      `README-OpenACC-design.md` for an explanation of why this
      happens and how Clacc might evolve to prevent it in the future.
* Preprocessor macro usage and the `_Pragma` operator form can
  sometimes prevent OpenACC directives from being translated:
    * Clacc cannot translate an OpenACC directive if it meets any of
      the following conditions:
        * The directive is expanded from a preprocessor macro and thus
          uses `_Pragma` form.
        * The directive uses `_Pragma` form and has no associated
          statement.
        * The associated statement must be rewritten but its last
          token is expanded from a preprocessor macro:
            * In the case of `#pragma` form, whether the associated
              statement must be rewritten depends on Clacc's mapping
              for the construct to OpenMP.
            * In the case of `_Pragma` form, currently the associated
              statement must always be rewritten.
    * Clacc reports an error diagnostic for every such OpenACC
      construct in the source file.  Clacc then prints a version of
      the source in which all OpenACC constructs are transformed
      except the reported ones.
    * For a full transformation of the source file in this case, try
      `-fopenacc-ast-print` instead.  However, its output looks like
      the preprocessor output, which is not appropriate for some use
      cases.
* Also see the section "OpenMP Extensions" below.

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

* The `update` directive translation depends on the OpenMP TR8 motion
  modifier `present` by default:
    * See the discussion of the `update` directive above for details.
* The `present` clause translation depends on the OpenMP TR8 map type
  modifier `present` by default:
    * See the discussion of the `present` clause above for details.
* The `no_create` clause translation depends on the map type modifier
  `no_alloc` by default:
    * See the discussion of the `no_create` clause above for details.
* Some features of the OpenACC Profiling Interface depend on OMPT
  extensions:
    * See the section "OpenACC Profiling Interface" in
      `README-OpenACC-design.md` for details.
