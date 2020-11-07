This document describes the current status of Clacc, which extends
Clang and LLVM with support for OpenACC.

Supported Features
==================

This section catalogs features that Clacc currently implements to
support OpenACC, and it identifies known limitations in those
features.  Clacc does not yet support OpenACC features that are not
mentioned here.

Build Platforms
---------------

* Ubuntu 18.04 is regularly tested.
* CentOS 7.8 is regularly tested.
* Windows and macOS are not yet supported.

Command-Line Options
--------------------

See the section "Using" in `../README.md` for an introduction to
Clacc's command-line options.  For brief descriptions of all
OpenACC-related and OpenMP-related command-line options, run Clacc's
`clang -help` and search for `openacc` or `openmp`.

* `-f[no-]openacc`
    * Enables OpenACC support.
    * Traditional compilation mode is the default.
* `-fopenacc[-ast]-print=acc|omp|acc-omp|omp-acc`
    * Enables OpenACC support and source-to-source mode.
    * See the section "Source-to-Source Translation" in
      `README-OpenACC-design.md` for design details.
* `-fopenmp-targets=<triples>`
    * In traditional compilation mode, specifies offloading device
      instead of targeting host.
    * Not supported for source-to-source mode
    * See the section "Using" in `../README.md` for a list of tested
      offloading target triples.
    * See the section "Interaction with OpenMP Support" in
      `README-OpenACC-design.md` for design details.
* Other OpenMP options
    * `-fopenmp` produces an error when OpenACC support is enabled as
      Clacc does not currently support combining OpenMP and OpenACC in
      the same application.
    * `-fopenmp-*` options not mentioned above are not supported when
      OpenACC support is enabled.
    * As usual when `-fopenmp` is not specified, OpenMP directives are
      discarded but `-Wsource-uses-openmp` is available to produce
      warnings for them.
* Options controlling the translation to OpenMP and related
  diagnostics
    * `-fopenacc-update-present-omp=KIND` where `KIND` is either
      `present` or `no-present`
    * `-fopenacc-structured-ref-count-omp=KIND` where `KIND` is either
      `hold` or `no-hold`
    * `-fopenacc-present-omp=KIND` where `KIND` is either `present` or
      `no-present`
    * `-fopenacc-no-create-omp=KIND` where `KIND` is either `no_alloc`
      or `no-no_alloc`
    * `-Wopenacc-omp-update-present`
    * `-Wopenacc-omp-map-hold`
    * `-Wopenacc-omp-map-present`
    * `-Wopenacc-omp-map-no-alloc`
    * See the section "OpenMP Extensions" below for details.
* Other diagnostic options
    * `-Wsource-uses-openacc`
        * Similar to `-Wsource-uses-openmp`, this enables warnings
          about OpenACC directives when OpenACC support is not
          enabled.
    * `-Wopenacc-ignored-clause`
        * See the discussion of the `vector_length` clause below.

Run-Time Environment Variables
------------------------------

* `OMP_TARGET_OFFLOAD=disabled` for specifying at run time to target
  the host.
* `ACC_PROFLIB` for the OpenACC Profiling Interface.

`update` Directive
------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Supported clauses
    * `if`
    * `if_present`
    * `self` and alias `host`
    * `device`
* Subarrays specifying contiguous blocks are supported.
* Multiple subarrays of the same array on the same directive are not
  yet supported.
* Members of structs or classes are not yet supported.
* See the section "OpenMP Extensions" below for caveats related to
  source-to-source mode.

`enter data` Directive
----------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Supported clauses
    * `copyin` and aliases `pcopyin` and `present_or_copyin`
    * `create`
        * `zero` modifier is not yet supported.
* Subarrays specifying contiguous blocks are supported.

`exit data` Directive
----------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Supported clauses
    * `copyout` and aliases `pcopyout` and `present_or_copyout`
    * `delete`
* Subarrays specifying contiguous blocks are supported.
* When offloading, the `exit data` directive must not be the first
  OpenACC directive encountered at run time.  Otherwise, LLVM's OpenMP
  runtime will complain that the device is uninitialized.

`data` Directive
----------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Any number of levels of nesting within other `data` constructs
      is supported.
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
* See the section "OpenMP Extensions" below for caveats related to
  source-to-source mode.

`parallel` Directive
--------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Use without clauses is supported.
* Supported data attributes and clauses
    * Implicit `copy` for non-scalars
    * Implicit `firstprivate` for scalars
    * For `present`, `copy`, `copyin`, `copyout`, `create`, and
      `no_create` clauses and their aliases, support is the same as
      for the `data` directive, as described above, including
      source-to-source mode caveats.
    * `firstprivate` clause
    * `private` clause
    * `reduction` clause
        * Supports only scalars for now.
        * Implies `copy` clause (overriding the implicit
          `firstprivate` clause).
        * `+`, `*`, `&&`, and `||` support exactly C11's arithmetic
          types (integer and floating types).
        * `max` and `min` support exactly C11's real types (integer
          types and floating types except complex types) and pointer
          types.
        * `&`, `|`, and `^` support exactly C11's integer types.
        * Deviations from OpenACC 2.6
            * OpenACC 2.6 sec. 2.5.12p774 mistypes `^` as `%`.  The
              latter would be nonsense as a reduction operator.
            * OpenACC 2.6 specifies that reduction operators support
              "the numerical data types in C", which is not
              terminology used by the C11 standard, and then it lists
              specific types.  We use this description only as a
              guideline as some operators more reasonably support
              different types than the types listed.  For example, all
              operators can support `bool` and enumerated types, which
              are not listed, and `max` and `min` cannot support
              complex types, which are listed.
            * Specifically, we have designed the operand type
              constraints for reduction operators to match the operand
              type constraints for C11's corresponding operators when
              combined with the general reduction constraint that the
              result type and both operand types must all be the same
              type.
            * All such support depends on Clang's corresponding
              support for OpenMP reductions.  Some operand types might
              not be supported when compiling the generated OpenMP
              using a different compiler.
* `num_gangs`, `num_workers`, `vector_length` clauses
    * The argument in all cases must be a positive integer expression.
    * The `vector_length` argument must also be a constant or Clacc
      does not use the clause and reports a warning diagnostic, which
      can be suppressed or converted to an error using the
      `-W{no-,error=}openacc-ignored-clause` command-line options.
    * Notes:
        * OpenACC 2.6 specifies only that the arguments must be
          integer expressions.  However, OpenMP specifies the stricter
          requirements above for `num_teams`, `num_threads`, and
          `simdlen`, to which Clacc translates the above clauses.
        * A non-positive value here probably doesn't make sense
          anyway.  Moreover, if the argument is an integer constant
          (so that it can be statically analyzed), gcc 7.3.0 warns if
          it's non-positive.  However, pgcc 18.4-0 doesn't warn even
          if it's negative.
        * Neither gcc 7.3.0 nor pgcc 18.4-0 warn if `vector_length`
          receives a non-constant expression.  However, we're stuck
          with this restriction because we translate to OpenMP
          `simdlen`.  OpenACC does permit the compiler to ignore
          `vector_length` as a hint, so we choose to ignore it and
          warn in the case of a non-constant expression.

`loop` directive
----------------

* Lexical context
    * Appearing within a `parallel` construct and any number of levels
      of nesting within other `loop` directives are supported.
    * Appearing outside a `parallel` construct (that is, an orphaned
      loop) is not yet supported.
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
    * For now, all three are ignored when combined with `auto` clause
      because, for now, `auto` produces a sequential loop.
    * Implicit `gang` clause
        * This feature is not specified in OpenACC 2.7, but existing
          OpenACC compilers implement it in some form.  Clacc attempts
          to mimic their behavior, but some details might be
          different.  See the "Semantic Clarifications" section in
          `README-OpenACC-design.md` for details.
    * For now, if none of these clauses appear (explicitly or
      implicitly), then a sequential loop is produced.
* The `collapse` clause is supported.
* Supported data attributes and clauses
    * A loop control variable is:
        * Implicit `shared` if `seq` is explicitly specified and loop
          control variable is assigned but not declared in init of
          attached `for` loop.
        * Predetermined `private` otherwise.
    * Implicit `shared` for other referenced variables
    * `private` clause
    * `reduction` clause
        * See `reduction` clause for `parallel` directive for general
          details about operand types and limitations.
        * Various subtleties in the semantics of `reduction` clauses
          on `loop` directives are discussed in the "Semantic
          Clarifications" section in `README-OpenACC-design.md`.  For
          example, a reduction on a sequential loop specifies a
          reduction across gangs if the reduction variable is
          gang-shared.  Many of these subtleties are under discussion
          among the OpenACC technical committee for clarification in
          the OpenACC spec after 2.7.
* Detection of `break` statement for the associated loop
    * Compile error if implicit/explicit `independent`.
    * No error if `seq` or `auto`.
    * This is important because Clang's implementation does not permit
      `break` statements for OpenMP loops.
    * In the future when `auto` doesn't always produce a sequential
      loop, a `break` statement will force it to be sequential,
      probably with a warning.
    * Both the OpenACC and OpenMP implementations currently permit
      `break` statements for nested loops that are associated via a
      `collapse` clause, but that's probably a bug.

`parallel loop` Directive
-------------------------

* All features currently supported by `parallel` and `loop` directives
  are also supported here.
* A `reduction` clause implies a `copy` clause (overriding the
  implicit `firstprivate` clause).

Subarrays
---------

* Subarrays specifying contiguous blocks are supported.
* Subarrays specifying non-contiguous blocks in dynamic
  multidimensional arrays are not yet supported.
* Subarrays in `firstprivate`, `private`, and `reduction` clauses are
  not yet supported.
* Subarrays with no `:` and one integer (syntactically an array
  subscript, such as `arr[5]`) are not yet supported.

Device-Side Directives
----------------------

Nesting of an `update`, `data`, `parallel`, or `parallel loop`
directive inside a `parallel`, `loop`, or `parallel loop` construct is
not yet supported.  We're not aware of any OpenACC implementation that
supports this yet.

OpenACC Profiling Interface
---------------------------

Clacc's support for the OpenACC Profiling Interface has been tested
with TAU.

The main limitations are currently as follows:

* Multiple callbacks per event type and callback toggling are
  not yet supported.
* Callback registration is permitted only within the
  `acc_register_library` function.
* `acc_ev_wait_start` and `acc_ev_wait_end` event types are not yet
  supported.
* The `kernel_name`, `num_gangs`, `num_workers`, `vector_length`,
  `thread_id`, and `tool_info` fields are not yet supported.
* The `acc_api_info` structure is not yet supported.

See the section "OpenACC Profiling Interface" in
`README-OpenACC-design.md` for a more detailed description of Clacc's
support.

Language Support
----------------

* C11 is supported with the following extensions:
    * `__uint128_t`, `__int128_t`, `__SIZEOF_INT128__`
* The nested function definition extension for C is not yet supported.
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
  not OpenACC, and OpenACC runtime diagnostics are prefixed with
  "OMP:".
* Some compiler warnings are reported twice, once while parsing
  OpenACC, and once while transforming to OpenMP.

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
specification.

This section describes the OpenMP extensions that Clacc employs.  It
also describes associated diagnostics for discovering occurrences of
these extensions as well as command-line options for choosing
alternative OpenMP translations.

OpenMP Directives and Clauses
-----------------------------

By default, Clacc's translation of some OpenACC directives and clauses
employs extensions to OpenMP's directives and clauses.  Clacc's
behavior is affected as follows:

* Traditional compilation mode
    * Standard OpenACC behavior is intended to be fully supported in
      all cases.
    * Although the traditional compilation mode user typically does
      not need to be aware, the OpenMP translation does still use
      OpenMP extensions for OpenACC features that require them.
    * If desired, it is possible to adjust the translation or
      associated diagnostics by using the command-line options
      discussed below for source-to-source mode.  The difference is
      that, by default in traditional compilation mode, the related
      diagnostics are disabled.
* Source-to-source mode
    * An occurrence of an OpenACC directive or clause whose
      translation employs an OpenMP extension produces a compile-time
      error diagnostic by default.  The purpose of the diagnostic is
      to ensure the user is aware that the translation is unlikely to
      be supported yet by foreign OpenMP compilers.
    * The diagnostic is actually a warning enabled by a command-line
      option of the form `-Wopenacc-omp-<OMP-EXT>`, where `<OMP-EXT>`
      identifies the OpenMP extension.  The warning is enabled and
      treated as an error by default in source-to-source mode.
    * To work around this issue, any of the following
      command-line options can be specified:
        * `-Wno-error=openacc-omp-<OMP-EXT>`
            * This converts the diagnostic to a warning to make it
              easier to find all occurrences.
        * `-Wno-openacc-omp-<OMP-EXT>`
            * This disables the diagnostic entirely.
            * This is useful if the generated OpenMP will not be
              compiled or if the OpenMP compiler actually already
              supports the OpenMP extension.
        * `-fopenacc-<ACC-FEATURE>-omp=<KIND>`
            * This suppresses the diagnostic by choosing the
              alternative translation identified by `<KIND>` for the
              OpenACC feature identified by `<ACC-FEATURE>`.
            * For some OpenACC applications, the alternative
              translation does not strictly conform to OpenACC
              semantics as the OpenMP extension does, but it might be
              sufficient for other OpenACC applications.
            * There are various such imperfect translations that Clacc
              currently does not support but that might be useful for
              some OpenACC applications.  We will consider adding
              support if there is user demand.  For each affected
              OpenACC feature, the associated section of "OpenACC to
              OpenMP Mapping" in `README-OpenACC-design.md` sometimes
              includes a discussion of alternative translations that
              we considered.
* `-fopenacc[-ast]-print=acc`, `-ast-print`, `-ast-dump`,
  etc. mode
    * Debugging modes like these do not actually print OpenMP source
      code, so they leave the aforementioned diagnostics disabled as
      in traditional compilation mode.

The remainder of this section describes OpenACC features that use
OpenMP extensions by default, and it describes the associated
command-line options mentioned above.

### `update` Directive's Presence Restriction ###

* OpenACC Features Affected
    * `update` directive's `self`, `host`, or `device` clause when
      `if_present` is not specified.
* OpenMP Extension Employed
    * `present` motion modifier (OpenMP TR8).
* OpenACC Semantics Required
    * OpenACC 3.0 requires variables appearing in `self`, `host`, or
      `device` to be already present on the device when `if_present`
      is not specified.  However, while it suggests a runtime error
      when this restriction is violated by an application, it does not
      strictly require any specific behavior.
    * Changes have been proposed to the OpenACC specification to
      strictly require a runtime error instead.
    * Clacc uses the `present` motion modifier to implement the
      proposed runtime error.
* Diagnostic Options
    * `-Wopenacc-omp-update-present`
    * `-Wno-error=openacc-omp-update-present`
    * `-Wno-openacc-omp-update-present`
* Translation Options
    * `-fopenacc-update-present-omp=present`
        * This is the default translation.
    * `-fopenacc-update-present-omp=no-present`
        * This translation omits the `present` motion modifier.  The
          `update` directive then always behaves as if it has an
          `if_present` clause.  Thus, contrary to OpenACC semantics,
          it never produces a runtime error when the specified
          variable is not present on the device.
        * However, this translation should be sufficient for OpenACC
          applications that are robust enough not to actually
          encounter this runtime error.

### Structured Reference Counter ###

* OpenACC Features Affected
    * `data` and `parallel` directives' `present`, `copy`, `copyin`,
      `copyout`, `create`, and `no_create` data attributes and
      clauses.
* OpenMP Extension Employed
    * `hold` map type modifier.
* OpenACC Semantics Required
    * OpenACC specifies two reference counters for tracking device
      allocations: a structured reference counter for `data` and
      `parallel` directives, and a dynamic reference counter for
      `enter data` and `exit data` directives and corresponding
      OpenACC Runtime Library routines.
    * OpenMP 5.0 specifies only one, which can thus be considered a
      dynamic reference counter.
    * Clacc uses the `hold` map type modifier extension to implement a
      structured reference counter.
* Diagnostic Options
    * `-Wopenacc-omp-map-hold`
    * `-Wno-error=openacc-omp-map-hold`
    * `-Wno-openacc-omp-map-hold`
* Translation Options
    * `-fopenacc-structured-ref-count-omp=hold`
        * This is the default translation.
    * `-fopenacc-structured-ref-count-omp=no-hold`
        * This translation omits the `hold` map type modifier.  Thus,
          contrary to OpenACC semantics, it always uses the dynamic
          reference counter, and so device memory might be deallocated
          prematurely for some OpenACC applications.
        * However, this translation should be sufficient if, for
          example, an OpenACC application always pairs `enter data`
          and `exit data` directives and corresponding OpenACC Runtime
          Library routine calls in a structured manner.

Currently, Clacc's implementation of its `hold` map type modifier
extension for OpenMP is not well tested outside of Clacc's
translations from OpenACC to OpenMP.  Thus, it is not yet recommended
for use in hand-written OpenMP code as it might not integrate well
with some OpenMP features.

### `present` Clause's Presence Restriction ###

* OpenACC Features Affected
    * `present` clause.
* OpenMP Extension Employed
    * `present` map type modifier (OpenMP TR8)
* OpenACC Semantics Required
    * Clacc uses the `present` map type modifier to implement the
      runtime error required by the OpenACC `present` clause in the
      case that data is not already present on the device.
    * This modifier is combined with the OpenMP `alloc` map type as
      opposed to, for example, `tofrom` in order to suppress
      device-to-host transfers in the case that there is a reference
      count decrement within the associated region.
* Diagnostic Options
    * `-Wopenacc-omp-map-present`
    * `-Wno-error=openacc-omp-map-present`
    * `-Wno-openacc-omp-map-present`
* Translation Options
    * `-fopenacc-present-omp=present`
        * This is the default translation.
    * `-fopenacc-present-omp=no-present`
        * This translation omits the `present` map type modifier.
          Thus, contrary to OpenACC semantics, it never produces a
          runtime error when the specified variable is not present
          on the device.
        * However, this translation should be sufficient for
          OpenACC applications that are robust enough not to
          actually encounter this runtime error.

### `no_create` Clause's Presence Check ###

* OpenACC Features Affected
    * `no_create` clause
* OpenMP Extensions Employed
    * `no_alloc` map type modifier
* OpenACC Semantics Required
    * Clacc uses the `no_alloc` map type modifier to suppress all
      runtime actions as required by the OpenACC `no_create` clause in
      the case that data is not already present on the device.
    * This modifier is combined with the OpenMP `alloc` map type as
      opposed to, for example, `tofrom` in order to suppress
      device-to-host transfers in the case that there is a reference
      count decrement within the associated region.
* Diagnostic Options
    * `-Wopenacc-omp-map-no-alloc`
    * `-Wno-error=openacc-omp-map-no-alloc`
    * `-Wno-openacc-omp-map-no-alloc`
* Translation Options
    * `-fopenacc-no-create-omp=no_alloc`
        * This is the default translation.
    * `-fopenacc-no-create-omp=no-no_alloc`
        * This translation omits the `no_alloc` map type modifier.
          Thus, contrary to OpenACC semantics, it attempts to allocate
          the specified variable when it is not already present on the
          device.
        * This translation is probably only useful when all of the
          following conditions are met:
            * The application is being tested with smaller data sets,
              perhaps in initial attempts to port to OpenMP.
              Otherwise, the unexpected allocations could produce
              performance degradation or memory exhaustion.
            * Any subarray specified in a `no_create` clause never
              conflicts with any subarray already present on the
              device or any subarray specification encountered during
              the execution of the associated region.  The unexpected
              allocations can cause such subarray conflicts to produce
              compile-time or runtime errors.
            * For any variable specified in a `no_create` clause, no
              data transfers for that variable as specified by data
              clauses are encountered during the execution of the
              associated region.  The unexpected allocations can
              suppress those data transfers.

Currently, Clacc's implementation of its `no_alloc` map type modifier
extension for OpenMP is not well tested outside of Clacc's
translations from OpenACC to OpenMP.  Thus, it is not yet recommended
for use in hand-written OpenMP code as it might not integrate well
with some OpenMP features.

OMPT
----

Some features of the OpenACC Profiling Interface depend on OMPT
extensions. See the section "OpenACC Profiling Interface" in
`README-OpenACC-design.md` for details.
