This document describes the current status of Clacc, which extends
Clang and LLVM with support for OpenACC.

Supported Features
==================

This section catalogs features that Clacc currently implements to
support OpenACC, and it identifies known limitations in those
features.  Clacc does not yet support OpenACC features that are not
mentioned here.

Architectures
-------------

* x86_64 and ppc64le (Power 9) are supported as hosts or accelerator
  devices.
* nvptx64 (NVIDIA GPUs) and amdgcn (AMD GPUs) are supported as
  accelerator devices.
* Offload support inherits any limitations from upstream LLVM's OpenMP offload
  support.  For example, stdio (e.g., `printf`), some standard math libraries
  (e.g., `complex.h`), and recursive functions are not yet properly supported in
  kernels offloaded to AMD GPUs.

Build Platforms
---------------

* Ubuntu 18.04 and 20.04 are regularly tested.
* CentOS 7.9 and 8.3 are regularly tested.
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
* `-fopenmp-targets=<triples>` or `--offload-arch=<arch>`
    * In traditional compilation mode, specifies offloading device
      instead of targeting host.
    * Not supported for source-to-source mode
    * See the section "Using" in `../README.md` for more usage details.
    * See the section "Interaction with OpenMP Support" in
      `README-OpenACC-design.md` for design details.
* Options controlling the implicit determination of `worker` and `vector`
  clauses on `loop` constructs
    * The goal is to try to increase parallelism and thus performance.
    * Currently, implicit `worker` and `vector` clauses are disabled by default.
    * `-fopenacc-implicit-worker=none|vector|outer|vector-outer` specifies the
      `loop` constructs on which `worker` clauses are implicitly determined.
    * `-fopenacc-implicit-vector=none|outer` specifies the `loop` constructs on
       which `vector` clauses are implicitly determined.
    * `-f[no-]openacc-implicit-worker` are currently aliases for
      `-fopenacc-implicit-worker=none|outer`.
    * `-f[no-]openacc-implicit-vector` are currently aliases for
      `-fopenacc-implicit-vector=none|outer`.
    * See the section "`loop` Directive" below for details.
* Other OpenMP options
    * `-fopenmp` produces an error when OpenACC support is enabled as
      Clacc does not currently support combining OpenMP and OpenACC in
      the same application.
    * `-fopenmp-*` options not mentioned above are not supported when
      OpenACC support is enabled.
    * As usual when `-fopenmp` is not specified, OpenMP directives are
      discarded but `-Wsource-uses-openmp` is available to produce
      warnings for them.
* Options controlling the translation to OpenMP and their associated
  diagnostics
    * `-fopenacc-update-present-omp=present|no-present`
    * `-fopenacc-structured-ref-count-omp=ompx-hold|no-ompx-hold`
    * `-fopenacc-present-omp=present|no-present`
    * `-fopenacc-no-create-omp=ompx-no-alloc|no-ompx-no-alloc`
    * `-Wopenacc-omp-update-present`
    * `-Wopenacc-omp-map-ompx-hold`
    * `-Wopenacc-omp-map-present`
    * `-Wopenacc-omp-map-ompx-no-alloc`
    * `-Wopenacc-omp-atomic-in-teams`
    * `-Wopenacc-omp-tile-in-teams`
    * `-Wopenacc-omp-ext`
    * See the section "OpenMP Extensions" below for details.
* Other diagnostic options
    * `-Wopenacc-omp-macro-inserted`
        * See "Source-to-Source Mode Limitations" below for details.
    * `-Wsource-uses-openacc`
        * Similar to `-Wsource-uses-openmp`, this enables warnings
          about OpenACC directives when OpenACC support is not
          enabled.
    * `-Wopenacc-ignored-clause`
        * See the discussion of the `vector_length` clause below.
    * `-Wopenacc-and-cxx`
        * Warns that OpenACC support for C++ is highly experimental and might
          not function correctly.
        * This warning is an error by default.  To try out C++ support, you
          might wish to disable the warning entirely with
          `-Wno-openacc-and-cxx`.
    * `-Wopenacc-routine-cxx-lambda`
        * This option is deprecated and is a no-op now that implicit `routine`
          directives are computed for C++ lambdas based on their bodies.
* Options that enable incomplete or "fake" support for specific OpenACC features
  that Clacc does not yet fully support
    * Commonalities
        * Each option and its associated features are not well tested and are
          not recommended for production use.
        * When an option is not specified, compile-time diagnostics (warnings or
          errors) are normally produced upon uses of the associated features.
        * When an option is specified, uses of the associated features are
          partially or fully validated, and compile-time diagnostics are
          produced if uses are found to be incorrect.  Otherwise, the features
          have no effect or a partial effect on behavior, as described below.
        * Each option will be removed when Clacc develops full support for the
          associated features.
    * `-fopenacc-fake-async-wait`
        * Clacc accepts but discards some OpenACC directives and clauses
          associated with async/wait support.  That is, they have no OpenMP
          translation in source-to-source mode.
        * Clacc inserts preprocessor definitions to handle OpenACC Runtime
          Library API routines and other symbols associated with async/wait
          support.
        * The effect in both cases is that behavior remains synchronous.
        * Some async/wait features might not be covered yet and thus will
          still produce compile-time diagnostics.  We are adding them as the
          need arises in the applications we are investigating.
        * Some async/wait features are starting to be supported when
          `-fopenacc-fake-async-wait` is not specified.  They are documented
          along with other supported features below.  However,
          `-fopenacc-fake-async-wait` fakes even those async features that
          Clacc otherwise supports because they are not safe while it fakes at
          least some wait features.  Because that produces synchronous behavior,
          wait features have no effect on correctness, so
          `-fopenacc-fake-async-wait` does not bother to fake wait features that
          are already supported.
        * Summary: If an application uses only fully supported async/wait
          features, it doesn't need `-fopenacc-fake-async-wait`.  Otherwise,
          `-fopenacc-fake-async-wait` might enable the application to compile
          and run successfully, but it will eliminate the asynchronous behavior.
    * `-fopenacc-fake-tile-clause`
        * Clacc now fully supports the `tile` clause, so this option is
          deprecated and has no effect.

Run-Time Environment Variables
------------------------------

* `ACC_DEVICE_TYPE` and `ACC_DEVICE_NUM`
    * Accepted device types are case-insensitive versions of:
        * `host` with the same semantics as `acc_device_host`.
        * `not_host` with the same semantics as `acc_device_not_host`.
        * All supported `acc_device_t` architecture-specific
          enumerators without the `acc_device_` prefix.
    * See `acc_device_t` notes below for a more complete description
      of the above device types.
    * `ACC_DEVICE_NUM` must be a non-negative integer less than the
      number of devices of the type specified by `ACC_DEVICE_TYPE`.
    * If `OMP_DEFAULT_DEVICE` is set, `ACC_DEVICE_TYPE` and
      `ACC_DEVICE_NUM` are ignored.
    * If only `ACC_DEVICE_TYPE` is set, `ACC_DEVICE_NUM` is treated as
      `0`.
    * If only `ACC_DEVICE_NUM` is set, `ACC_DEVICE_TYPE` is treated as
      the type of the device that the OpenMP implementation would
      otherwise select by default.
* `ACC_PROFLIB` for the OpenACC Profiling Interface.
* `OMP_TARGET_OFFLOAD=disabled` for specifying at run time to target
  the host.

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
    * See "Data Expressions in Clauses" below for details of their support in
      these clauses.
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
    * See "Data Expressions in Clauses" below for details of their support in
      these clauses.

`exit data` Directive
----------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Supported clauses
    * `copyout` and aliases `pcopyout` and `present_or_copyout`
    * `delete`
    * See "Data Expressions in Clauses" below for details of their support in
      these clauses.
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
    * See "Data Expressions in Clauses" below for details of their support in
      these clauses.
* See the section "OpenMP Extensions" below for caveats related to
  source-to-source mode.

`parallel` Directive
--------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Use without clauses is supported.
* Supported data attributes and clauses
    * For `present`, `copy`, `copyin`, `copyout`, `create`, and
      `no_create` clauses and their aliases, support is the same as
      for the `data` directive, as described above, including
      source-to-source mode caveats.  Notes:
        * OpenACC 3.3 specifies that `no_create` causes device code to use a
          variable's host memory address if it's not already present on the
          device.  That seems to mean that accessing the variable's memory is
          illegal but you can take its address.  It's unclear if real code
          actually depends on that behavior, but Clacc does not consistently
          support it.  Instead, all device uses of a `no_create` variable must
          be unreachable if the variable is not already present on the device.
          See the discussion of `nomap` in the "Parallel Directives" section in
          `README-OpenACC-design.md` for related implementation details.
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
    * Implicit `copy` for uses of non-scalar variables in the associated
      statement
    * Implicit `firstprivate` for uses of scalar variables in the associated
      statement
    * Implicit `copy` on C++ `this[0:1]` for implicit or explicit uses of C++
      `this` in the associated statement.  Notes:
        * This behavior appears to mimic NVHPC 22.7.
        * This behavior is especially useful for a call to a `this` member
          function (e.g., `this->foo()` possibly without the explicit `this->`),
          whose implementation might access any member of `this`.
        * The implicit data attribute for `this` is not described by OpenACC
          3.3.  While `this` has scalar type like a pointer variable, an
          implicit `firstprivate(this)` is not specified because `this` is not a
          variable, as discussed below under "Data Expressions in Clauses".
    * Implicit data attributes are computed for a member expression used in the
      directive's associated statement as follows:
        * If the full member expression (e.g., `s.x`) does not appear in a
          visible data clause, then its base expression (e.g., `s`) is
          considered for an implicit data attribute.  If the base expression is
          also a member expression, then this process recurses.
        * This behavior might be unexpected when the base expression is a
          pointer variable (e.g., `p` in  `p->x`) and thus the implicit data
          attribute for the base expression is `firstprivate`.  However, this
          behavior is consistent with how an implicit data attribute is computed
          for a pointer variable outside a member expression.
    * If the `parallel` construct appears within a C++ lambda definition within
      a `data` construct, implicit data attributes are not suppressed by
      explicit data clauses on the `data` construct :
        * OpenACC 3.3, sec. 2.6.2 "Variables with Implicitly Determined Data
          Attributes", L1283-1284 states: "Visible data clause: Any data clause
          on the compute construct, a lexically containing data construct, or a
          visible declare directive."
        * Based on that definition, it seems the data clauses on the `data`
          construct should be considered visible at the `parallel` construct.
          However, are the variables considered to be the same variables?  If
          they are captured by value for the lambda, they certainly are not the
          same.  If they are captured by reference, it's less clear, but Clacc
          still assumes they are not the same.  Thus, implicit data attributes
          are not suppressed.
    * See "Data Expressions in Clauses" below for details of their support in
      explicit data clauses.
* `num_gangs`, `num_workers`, `vector_length` clauses
    * The argument in all cases must be a positive integer expression.
    * The `vector_length` argument must also be a constant or Clacc
      does not use the clause and reports a warning diagnostic, which
      can be suppressed or converted to an error using the
      `-W{no-,error=}openacc-ignored-clause` command-line options.
    * `vector_length` currently does not affect orphaned `loop` constructs.
    * Notes:
        * In the cases of `num_workers` and `vector_length`, OpenACC 3.3
          specifies only that the arguments must be integer expressions.
          However, OpenMP 5.2 specifies the stricter requirements above for
          `thread_limit` and `simdlen`, to which Clacc translates them.
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
* `async` clause
    * The argument can be omitted, or it must be an integer expression.  It must
      evaluate to a non-negative integer or the value of `acc_async_sync`,
      `acc_async_noval`, or `acc_async_default` as defined in Clacc's
      `openacc.h`.  An argument that is not of integer type or that is an
      integer constant expression that does not meet the above restrictions
      produces a compile-time error diagnostic.  Otherwise, an expression that
      does not meet the above restrictions produces a runtime error.
    * The function prototype `char *acc2omp_async2dep(int)` must be in scope
      and must link to Clacc's implementation of it in `libacc2omp.so`.  The
      easiest way to accomplish this is usually to just add
      `#include <openacc.h>`.  In the future, Clacc might insert the prototype
      automatically where it is not in scope.  Also see "Source-to-Source Mode
      Limitations" below.
    * Activity queues are currently common to all offload devices instead of per
      device, limiting some concurrency.  In the future, this limitation might
      be removed.
    * Due to a nondeterminism apparently inherited from upstream Clang's OpenMP
      implementation, concurrent execution of activity queues is not guaranteed.
      Moreover, there are occasional runtime assertion failures when targeting
      the host.  We need to investigate these issues further.

`loop` Directive
----------------

* Lexical context
    * Appearing within a `parallel` construct is supported.
    * Appearing outside any `parallel` construct and within a function with an
      explicit `routine` directive is supported.  In this case, the `loop`
      construct is said to be an orphaned `loop` construct.
    * Appearing outside any `parallel` construct and within a function without
      an explicit `routine` directive produces a compile-time error diagnostic.
      OpenACC 3.2 does not specify this restriction, but we are preparing a
      discussion for the OpenACC technical committee.
    * Appearing within any number of levels of nesting within other `loop`
      constructs is supported.
* Use without clauses is supported.
* Supported partitionability clauses
    * Implicit `independent`
    * `seq`
    * `independent`
    * `auto`
        * For now, always produces a sequential loop.
* Supported partitioning clauses
    * `gang`
        * `static:` argument, which must be `*` or a positive integer
          expression.  A non-positive constant integer expression produces a
          compile-time error diagnostic, and the behavior of a non-positive
          non-constant integer expression is undefined.  OpenACC 3.3 does not
          specify that the expression must be positive but also does not clarify
          the semantics if it's not.  GCC 12.2.0 warns about a non-positive
          constant integer expression, and NVHPC 22.11 quietly accepts it.
    * `worker`
    * `vector`
    * Otherwise, arguments to those clauses are not yet supported.
    * For now, all three are ignored when combined with `auto` clause
      because, for now, `auto` produces a sequential loop.
    * Implicit `gang` clause
        * This is always enabled because it is specified by OpenACC (introduced
          in 3.1) and can be important for correct behavior of the application.
    * Implicit `worker` and `vector` clauses
        * Currently, they are disabled by default, but that might change in the
          future.
        * Their goal is to try to increase parallelism and thus performance.
        * For a conforming OpenACC application (e.g., `loop` constructs are
          never misidentified as `independent`), they should never affect
          behavioral correctness.  Thus, the OpenACC specification does not
          specify whether or how they are determined but also does not prohibit
          them, and OpenACC compilers typically implement them.
        * The OpenACC spec does specify the implicit determination of `gang`
          clauses.  Like implicit `gang` clauses, implicit `worker` and `vector`
          clauses are determined after any conversion of `auto` clauses to `seq`
          and after implicit `routine` directives are determined (see "Implicit
          `routine` directive" below).
        * `-fopenacc-implicit-worker=none|vector|outer|vector-outer` specifies
          the `loop` constructs on which `worker` clauses are implicitly
          determined:
          - `none` suppresses all implicit `worker` clauses.
            `-fno-openacc-implicit-worker` is an alias.
          - `vector` specifies `loop` constructs with explicit `vector` clauses.
            This choice can be useful when compiling OpenACC applications
            primarily employing explicit `gang` and `vector` clauses while
            targeting an OpenMP implementation (like Clacc's) for which
            `omp simd` (to which Clacc translates `vector`) does not increase
            parallelism for the given offload devices.
          - `outer` specifies each loop nest's outermost `loop` constructs on
             which `worker` clauses are permitted.  This is similar to how the
             OpenACC spec places implicit `gang` clauses.
            `-fopenacc-implicit-worker` is currently an alias.
          - `vector-outer` applies `vector` followed by `outer`.
        * `-fopenacc-implicit-vector=none|outer` specifies the `loop` constructs
          on which `vector` clauses are implicitly determined:
          - `none` suppresses all implicit `vector` clauses.
            `-fno-openacc-implicit-vector` is an alias.
          - `outer` specifies each loop nest's outermost `loop` constructs on
             which `vector` clauses are permitted.  This is similar to how the
             OpenACC spec places implicit `gang` clauses.
            `-fopenacc-implicit-vector` is currently an alias.
        * The algorithms selected by `-fopenacc-implicit-worker` and
          `-fopenacc-implicit-vector` might change in the future as we determine
          better defaults.
    * For now, if none of these clauses appear (explicitly or
      implicitly), then a sequential loop is produced.
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
        * OpenACC's restriction that a scalar loop reduction variable sometimes
          requires an explicit data clause on the parent `parallel` construct
          (see OpenACC 3.2, sec. 2.6.2, L1235-1239) is enforced with a
          compile-time error diagnostic.  The diagnostic includes a suggestion
          for a `copy` or `firstprivate` clause based on whether the `parallel`
          construct contains a gang reduction for that variable.  The diagnostic
          should aid the development of OpenACC applications that are portable
          among existing OpenACC compilers, which do not always implicitly
          determine the same data clause for this case.  The restriction is not
          enforced if the `loop` and `parallel` construct are combined as our
          expectation is that all compilers implicitly determine `copy` in this
          case.
        * OpenACC's restriction that an orphaned `loop` construct reduction's
          variable must be (gang-)private is enforced with a compile-time error
          diagnostic.
        * Various subtleties in the semantics of `reduction` clauses
          on `loop` directives are discussed in the "Semantic
          Clarifications" section in `README-OpenACC-design.md`.
    * See "Data Expressions in Clauses" below for details of their support in
      these clauses.
* Supported multiloop clauses
    * `collapse`
    * `tile`
    * If a `collapse` clause and a `tile` clause appear on the same `loop`
      construct, a compile-time error diagnostic is produced.
        * While OpenACC 3.3 does not specify this restriction, both NVHPC 22.11
          and GCC 12.2.0 enforce it.
    * If a `collapse` clause's argument is *N*, or if a `tile` clause contains
      *N* size expressions, there must be *N* tightly nested loops following the
      `loop` construct, or a compile-time error diagnostic is produced.
    * Predetermined `private` is computed for the loop control variables in
      those *N* loops in the same manner as it would be for a single loop
      without a `collapse` or `tile` clause.
        * This appears to mimic the behavior of GCC 12.2.0.
        * OpenACC 3.3 uses the term "associated loop" in the specification of
          predetermined `private` clauses and `collapse` clauses but not in the
          specification of `tile` clauses, so this behavior is not clear.
        * NVHPC 22.11 instead performs a liveness analysis to determine data
          attributes.
    * A `collapse` clause's argument must be a positive constant integer
      expression, and each size expression within a `tile` clause must be either
      `*` or a positive constant integer expression.  Otherwise, a compile-time
      error diagnostic is produced.
        * There is one exception for now: a size expression in a `tile` clause
          can also be a non-constant integer expression.
            * Clacc currently implements each such size expression as `1` but
              might produce a compile-time error diagnostic in the future.
            * Clacc accepts them for now just because the version of Kokkos's
              OpenACC backend that we target uses them (even though they are
              apparently discarded by NVHPC).
            * OpenACC 3.3 does not require support for non-constant integer
              expressions.
            * GCC 12.2.0 rejects them with compile-time error diagnostics.
            * NVHPC 22.11 ignores the entire `tile` clause if it contains one,
              as reported by `-Minfo`.
        * `*` is currently implemented as `1` because there currently is no
          corresponding OpenMP 5.2 feature.
    * Where OpenACC 3.3 specifies that `worker` and `vector` are applied to the
      generated element loop, Clacc currently discards them instead because
      there appears to be no way to express this behavior in OpenMP 5.2.  Thus,
      the generated element loops are always sequential.
    * The above rules apply and tiling is performed regardless of whether the
      associated loop nest is partitioned or executed sequentially (e.g., due to
      a `seq` clause).
    * See the section "OpenMP Extensions" below for caveats related to
      source-to-source mode and the `tile` clause.
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

`atomic` Directive
------------------

* Lexical context
    * Appearing within any OpenACC construct besides `atomic` is supported.
    * Appearing outside any OpenACC construct is supported.
    * Currently, the `atomic` directive is supported even in contexts that are
      compiled only for the host, including the `data` construct.  The semantics
      and usefulness of this usage are not clear.  In the future, this usage
      might produce a compile-time error diagnostic.
* Supported clauses
    * `read`
    * `write`
    * `update`
    * Implicit `update`
    * `capture`
    * `compare`
        * This is an OpenACC extension based on the OpenMP `atomic` construct's
          `compare` clause except that Clacc currently does not support
          combining `compare` with other clauses.
        * `compare` permits the associated statement to take any of the
          following forms:
            * `x = expr ordop x ? expr : x;`
            * `x = x ordop expr ? expr : x;`
            * `x = x == e ? d : x;`
            * `if (expr ordop x) { x = expr; }`
            * `if (x ordop expr) { x = expr; }`
            * `if (x == e) { x = d; }`
            * `if (expr ordop x) x = expr;`
            * `if (x ordop expr) x = expr;`
            * `if (x == e) x = d;`
        * `x` and `expr` are as specified by OpenACC 3.2 for other forms of the
          associated statement.
        * `ordop` is either `<` or `>`.
        * `e` and `d` are expressions with scalar type.
        * Because this extension is translated directly to OpenMP, refer to the
          OpenMP specification for a detailed description of the semantics.
* Gang-private atomically accessed variable
    * A run-time error diagnostic is produced for some offload targets in this
      case.
    * Specifically, we have observed CUDA errors when the target is nvptx64.
    * We have observed similar behavior when compiling using nvc 21.11.
    * In the future, this usage might produce a compile-time error diagnostic.
* See the section "OpenMP Extensions" below for caveats related to
  source-to-source mode.

`routine` Directive
-------------------

* Lexical context
    * Appearing at file scope or within the member list of a C++ class or
      namespace is supported.
    * Appearing within another function's definition is supported.  There are
      currently some obscure inconsistencies in the visibility of the `routine`
      directive outside that function definition, as described under "Scope of
      the directive" below, but these should not affect most users.
    * Appearing within any OpenACC construct besides `atomic` is supported.
    * Appearing outside any OpenACC construct is supported.
* Supported level-of-parallelism clauses
    * `gang`
    * `worker`
    * `vector`
    * `seq`
    * Exactly one must appear, and it must be the same as on any other
      `routine` directive applying to the same function within the
      same compilation unit.  Otherwise, a compile-time error
      diagnostic is produced.
    * A compile-time error diagnostic is produced if a function's level of
      parallelism is incompatible with the calling context (e.g., `loop`
      construct or enclosing function).
        * An incompatible level of parallelism is not diagnosed if a function's
          address is stored and called later.
        * A special case of this diagnostic is produced if a function's level of
          parallelism is `gang`, `worker`, or `vector` and if a call to it
          appears in host-only code.
            * In C, host-only code is any code outside of any compute or `loop`
              construct and within a function that has no `routine` directive.
            * In C++, host-only code also includes code executed at file scope.
            * In these cases, the call site can execute only outside compute
              regions, but the called function requires execution modes
              (gang-redundant, etc.) that are impossible outside compute
              regions.
* Other clauses
    * No other clauses are supported yet.
    * Specifying a name is not yet supported, so an immediately
      following prototype or definition is required.
* Immediately following declaration
    * A lone function prototype or definition is supported, and the
      `routine` directive applies to that function.
    * A declaration containing multiple declarators is not supported.
      For example, `void foo(), bar();`.
    * A C++ lambda definition is not currently supported.  However, implicit
      `routine` directives are computed based on a lambda's body, as described
      below.
* Scope of the directive
    * A `routine` directive's scope starts at the `routine` directive and ends
      at the end of the compilation unit.  Of course, its visibility is only
      useful where the function to which it applies is also visible.  Caveat:
        * Unlike many other kinds of declarations (e.g., local variables), the
          visibility of a `routine` directive should extend to the end of the
          compilation unit even if the `routine` directive appears within
          another function's definition.
        * However, the visibility of `routine` directives is limited in a subtle
          manner by some obscure issues in Clang's general handling of multiple
          declarations of a function and by limitations in Clang's OpenMP
          support.
        * As a general rule thumb, if replacing a `routine` directive with
          another kind of declaration would not make the latter declaration
          visible in some other scope, then it is best to assume the `routine`
          directive's visibility in that other scope is currently unreliable.
          Such usage is confusing anyway and appears to be uncommon.
    * A definition or use of any function must appear within the scope
      of at least one explicit and applying `routine` directive if
      there is any within the compilation unit.  Otherwise, a
      compile-time error diagnostic is produced.  Uses include host
      uses and accelerator uses but only if they're evaluated (e.g., a
      reference in `sizeof` is not a use).
* Body of the associated function's definition
    * Appearance of any OpenACC directive other than an orphaned `loop`
      construct, an `atomic` construct, or a `routine` directive produces a
      compile-time error diagnostic.
    * Appearance of an orphaned `loop` construct with an incompatible level of
      parallelism produces a compile-time error diagnostic.
    * Declaration of a static local variable produces a compile-time
      error diagnostic.
* Implicit `routine` directive
    * Unless a function has an explicit `routine` directive in scope or has an
      implicit `routine` directive based on its body as described below, a
      `routine seq` directive is implied for it by any use of the function
      within a compute construct or within another function with a `routine`
      directive (whether implicit or explicit).  Caveats:
        * Currently, if a function *x* is defined in a compilation unit, if the
          only use of *x* within that compilation unit appears in an inline
          function *y* (e.g., *y* might be a member function defined within a
          C++ class), and if *y* is not used in that compilation unit, then the
          definition of *x* will not be compiled for accelerators as expected.
          This behavior is inherited from Clang's current `omp declare target`
          behavior, and Clacc currently does not diagnose it.
    * An implicit `routine` directive is implied for a C++ lambda if the
      lambda's body encloses, outside any enclosed compute construct or other
      lambda's body, either (1) a call to another function with a `routine`
      directive (whether implicit or explicit) with a `gang`, `worker`, or
      `vector` clause, or (2) a `loop` construct.  The level of parallelism
      clause on the implicit `routine` directive is the highest level of
      parallelism from such enclosed calls or `loop` constructs.  Caveats:
        * OpenACC 3.3 does not specify implicit `routine` directives based on
          the bodies of C++ lambdas.  A proposal is under discussion among the
          OpenACC technical committee for inclusion in the OpenACC spec after
          3.3.
        * Currently, for one of the aforementioned `loop` constructs, any
          conversion of `auto` to `seq` is performed before the implicit
          `routine` directive is computed for the lambda, and thus the implied
          level of parallelism from that `loop` construct is `seq`.  This
          behavior could change based on committee discussions.
        * Currently, implicit `gang`, `worker`, and `vector` clauses on the
          aforementioned `loop` constructs are computed after the implicit
          `routine` directive on the lambda and thus only if the lambda body
          contains other `loop` constructs or calls with sufficient parallelism.
          This behavior could change based on committee discussions.
    * An implicit `routine` directive has scope throughout the compilation unit
      and thus triggers the above diagnostics for the function definition's body
      as long it's in the same compilation unit.  Caveat:
        * If the function to which an implicit `routine` directive applies is
          prototyped within the same function in which the implying use appears,
          the visibility of the implicit `routine` directive might be limited in
          the manner discussed above for an explicit `routine` directive within
          a function.
        * As for the explicit `routine` directive, such usage is confusing
          anyway and appears to be uncommon.
* C++ function uses and calls
    * For all behavior described above for function uses and calls, both
      explicit and implicit function calls in C++ are included.
    * Examples of implicit function calls include:
        * A local variable definition's implicit calls to a default constructor
          and destructor.
        * An implicit default constructor definition's call to a base class's
          constructor.
        * An explicit constructor definition's implicit call to a base class's
          constructor.
        * A `new` expression's calls to a `new` operator, constructor, and
          `delete` operator.

`wait` Directive
------------------

* Lexical context
    * Appearing outside any OpenACC construct is supported.
    * Appearing within a `data` construct is supported.
* Only usage without clauses is supported so far.

Data Expressions in Clauses
---------------------------

* Subarray and member expression support varies among clauses and directives:
    * Subarrays and member expressions are supported in the `self` and `device`
      clauses of the `update` directive.
    * Subarrays and member expressions are supported in the `present`, `copy`,
      `copyin`, `copyout`, `create`, `no_create`, and `delete` clauses of the
      `enter data`, `exit data`, `data`, and `parallel` directives.
    * Member expressions either implicitly or explicitly on C++ `this` (e.g.,
      `this->x` possibly without the explicit `this->`) are supported in the
      `firstprivate`, `private`, and `reduction` clauses of the `parallel`
      directive and in the `reduction` clause of a non-orphaned `loop`
      directive.  They are not currently supported in the `reduction` clause of
      an orphaned `loop` directive or in the `private` clause of any `loop`
      directive.
    * Otherwise, subarrays and member expressions are not supported in the
      `firstprivate`, `private`, and `reduction` clauses of the `parallel` and
      `loop` directives.
    * Support is the same among any clause and its aliases.
* In clauses where subarrays are supported, the following constraints always
  hold:
    * Subarrays specifying contiguous blocks are supported.
    * Subarrays specifying non-contiguous blocks in dynamic multidimensional
      arrays are not supported.
    * Subarrays with no `:` and one integer (syntactically an array subscript,
      such as `arr[5]`) are not supported.
    * Subarrays on member expressions (e.g., `s.arr[0:3]`) are supported.
    * Multiple subarrays of the same array appearing multiple times on the same
      directive are not supported.
* In clauses where member expressions are supported, the following constraints
  always hold:
    * Member expressions on member expressions are not supported (e.g.,
      `x.y.z`).
    * Member expressions on subarrays are not supported (e.g., `arr[0:3].x`).
* The appearance of C++ `this` in the above clauses is constrained as follows:
    * If a subarray is specified on `this`, the starting index must evaluate to
      0, and the length must evaluate to 1 (e.g., `copy(this[0:1])`).
    * If `this` appears as a variable (e.g., `copy(this)` or `self(this)`), a
      compile-time error diagnostic is produced because `this` is not a variable
      (lvalue).  That is, like a pointer variable, `this` can be used as an
      expression that evaluates to an address.  However, unlike a pointer
      variable, `this` does not represent storage that contains that address,
      that can be overwritten, that should thus be synced between host and
      device, and that has its own address.

Device-Side Directives
----------------------

Nesting of an `update`, `enter data`, `exit data`, `data`, `parallel`,
or `parallel loop` directive inside a `parallel`, `loop`, or `parallel
loop` construct or inside a function attributed with a `routine`
directive is not yet supported.  We're not aware of any OpenACC
implementation that supports such cases yet.

OpenACC Runtime Library API and Preprocessor
--------------------------------------------

* The `_OPENACC` preprocessor macro is supported.
* `acc_device_t`
    * All enumerators required by OpenACC 3.1 are supported with the
      following semantics:
        * `acc_device_none`: No devices.
        * `acc_device_host`: The host device.  This includes the case
          where the host device executes a kernel without offloading
          data or code.  In this way, Clacc follows the recommendation
          of OpenACC 3.1, sec. A.1.3 "Multicore Host CPU Target".
        * `acc_device_not_host`: All devices other than host that are
          available for offloading code or data.  This includes any
          device that is physically the same device as the host device
          but that is logically treated as a separate offloading
          device with discrete memory.  In particular, this case
          arises when specifying the host architecture to
          `-fopenmp-targets`.
        * `acc_device_default`: All devices of the current device
          type.  This corresponds to the OpenACC 3.1 ICV
          `acc-current-device-type-var`, which is retrieved by
          `acc_get_device_type`.  See `acc_get_device_type` notes
          below for further details.
        * `acc_device_current`: The current device.  This corresponds
          to the combination of the OpenACC 3.1 ICVs
          `acc-current-device-type-var` and
          `acc-current-device-num-var`.
    * Architecture-specific enumerators are also supported and only
      include devices that `acc_device_not_host` also includes:
        * `acc_device_nvidia` is supported as recommended in OpenACC
          3.1, sec. A.1.1 "NVIDIA GPU Targets".
        * `acc_device_radeon` is supported as recommended in OpenACC
          3.1, sec. A.1.2 "AMD GPU Targets".
        * `acc_device_x86_64` and `acc_device_ppc64le` are supported
          but are not mentioned by OpenACC 3.1
    * `acc_device_not_host` might include devices that no
      architecture-specific enumerator includes.  This situation is
      not ideal but can arise when either the OpenMP implementation's
      `omp_device_t` (a Clacc extension) or Clacc's `acc_device_t` is
      missing an architecture supported by the OpenMP implementation.
      Because Clacc is designed to ultimately be compatible with
      foreign OpenMP implementations, this situation is unavoidable.
    * The semantics of the above enumerators are not always clear in
      OpenACC 3.1, and many details need to be discussed with the
      OpenACC committee.
* Device management routines supported on the host are:
    * `acc_get_num_devices(acc_device_t dev_type)`
        * Returns 1 if `dev_type=acc_device_current` because there's
          always one current device.  nvc 20.11-0 returns 0 instead.
          OpenACC 3.1 is unclear about this behavior.
        * Otherwise, counts the devices included by `dev_type`.
    * `acc_set_device_type(acc_device_t dev_type)`
        * Always just calls `acc_set_device_num(0, dev_type)`.
        * OpenACC 3.1 does not specify how `acc_set_device_type`
          should affect the device number.  nvc 20.11-0 appears to
          always set it 0 as Clacc does.
        * Also see issues discussed below for `acc_set_device_num`.
    * `acc_get_device_type()`
        * Retrieves the current device type, which corresponds to the
          OpenACC 3.1 ICV `acc-current-device-type-var`.
        * The result is either `acc_device_host`,
          `acc_device_not_host`, or an architecture-specific
          enumerator.
        * The result is `acc_device_not_host` only when the current
          device is not the host device and no architecture-specific
          `acc_device_t` is appropriate for it, as described in the
          `acc_device_not_host` discussion above.
    * `acc_set_device_num(int dev_num, acc_device_t dev_type)`
        * Does nothing if `dev_type=acc_device_current`.  This appears
          to match nvc 20.11-0's behavior, but OpenACC 3.1 does not
          make this behavior clear.
        * Produces a runtime error if `dev_num` is invalid for
          `dev_type` (always the case if `dev_type=acc_device_none`).
          nvc 20.11-0 appears to do nothing in this case instead.
          OpenACC 3.1 says the behavior is implementation-defined.
        * If `dev_type=acc_device_not_host`, `dev_num` is an index
          into the entire list of available devices other than host.
        * Clacc does not attempt to implement OpenACC 3.1, sec. 3.2.4
          "acc\_set\_device\_num", L3054-3056: "If the value of
          dev\_num is negative, the runtime will revert to its default
          behavior, which is implementation-defined.  If the value of
          the dev\_type is zero, the selected device number will be
          used for all device types."  We don't know what this text
          means and cannot seem to reproduce it using nvc 20.11-0.
    * `acc_get_device_num(acc_device_t dev_type)`
        * Returns the `dev_num` that would have to be specified in
          `acc_set_device_num(dev_num, dev_type)` in order to specify
          the current device.  nvc 20.11-0 appears to have the same
          behavior.
        * Returns -1 if the current device is not included by
          `dev_type`.  nvc 20.11-0 returns 0 instead, but that's
          misleading as it seems to indicate the current device is the
          first device included by `dev_type`.
        * OpenACC 3.1 does not mention how `dev_type` affects the
          result.
* Async/wait management routines supported on the host are:
    * `int acc_get_default_async(void)`
    * `void acc_set_default_async(async_arg)`
        * Produces a runtime error if `async_arg` is neither a non-negative
          integer nor the value of `acc_async_sync`, `acc_async_noval`, or
          `acc_async_default`.
* `acc_on_device` is supported on the host and on offloading devices:
    * The result is always false if the argument is `acc_device_none`.
      This behavior appears to follow nvc 20.9-0's behavior.  OpenACC
      3.1 is unclear, but this behavior seems obvious.
    * The result is always false if the argument is
      `acc_device_default` or `acc_device_current`.  While this
      behavior doesn't seem to match the semantics of these
      enumerators, our rationale is that the current device and
      current device type cannot be determined at compile time and
      otherwise are only defined on the host, but `acc_on_device` must
      be able to expand as a constant when given a constant and must
      be defined on offloading devices.  Moreover, this behavior
      appears to follow nvc 20.9-0's behavior.  OpenACC 3.1 is unclear
      about the case of `acc_device_current`, but sec. 3.2.23
      "acc_on_device", L3425 states "The result with argument
      acc_device_default is undefined."  It might be better if these
      arguments were handled as errors, either at compile time or run
      time depending on whether the argument is constant.
    * One potential point of confusion is that, when executing on the
      host, `acc_on_device` does not return true for any
      architecture-specific enumerator, such as `acc_device_x86_64`.
      These semantics seem consistent with OpenACC 3.1's multicore
      recommendation for `acc_device_host`, as cited above, and they
      avoid confusion for the behavior of routines like
      `acc_get_device_type`.  If detection of the host architecture is
      useful, a compile-time macro that expands to, say, an
      `acc_host_t` enum could be provided in the future for use after
      `acc_on_device(acc_device_host)` determines that execution is
      currently on the host.
    * When OpenACC and OpenMP are disabled, the result is true only if
      the argument is `acc_device_host`.  This behavior appears to
      follow nvc 21.7-0's behavior.
    * When OpenMP is enabled, the behavior is the same for OpenMP
      target regions as it would be for OpenACC compute constructs if
      OpenACC were enabled.  This behavior supports OpenMP compilation
      after OpenACC source-to-source translation.  It does not appear
      to follow nvc 21.7-0's behavior.
    * `acc_on_device` is defined as a preprocessor function-like macro
      in Clacc's `openacc.h`.  Thus, it has no function address and
      cannot be linked.  This implementation facilitates Clacc's
      source-to-source mode together with OpenACC's requirement that
      `acc_on_device` evaluate to a constant when its argument is a
      constant.  See the section "Source-to-Source Mode Limitations"
      below for caveats for source-to-source mode.
* Data and memory management routines supported on the host are:
    * `acc_malloc`, `acc_free`
        * OpenACC 3.1 is unclear about handling of a `bytes` argument
          equal to zero.  In that case, Clacc implements `acc_malloc`
          as a no-op and returns a null pointer.
        * OpenACC 3.1 does not specify special behavior for shared
          memory.  As in the case of discrete memory, Clacc allocates
          and frees on the current device.
    * `acc_copyin`, `acc_create`, `acc_copyout`,
      `acc_copyout_finalize`, `acc_delete`, `acc_delete_finalize`,
      `acc_update_device`, `acc_update_self`; and aliases
      `acc_present_or_copyin`, `acc_pcopyin`, `acc_present_or_create`,
      `acc_pcreate`
        * OpenACC 3.1 is unclear about handling of null pointers or a
          `bytes` argument equal to zero.  In that case, Clacc
          implements these as no-ops and returns a null pointer where
          the return type is not void.
    * `acc_map_data`, `acc_unmap_data`
        * OpenACC 3.1 is unclear about handling a `bytes` argument
          equal to zero.  In that case, Clacc implements
          `acc_map_data` as a no-op.
        * OpenACC 3.1 is unclear about handling of null pointers.
          Clacc produces a runtime error in that case.
        * OpenACC 3.1 is unclear about the behavior for shared memory.
          Clacc produces a runtime error in that case.
        * OpenACC 3.1 is unclear about whether it is an error if only
          part of the host memory specified to `acc_map_data` has
          already been mapped.  Clacc produces a runtime error in that
          case.
    * `acc_deviceptr`
        * OpenACC 3.1 is unclear about handling of a null pointer.
          Clacc returns a null pointer in that case.
        * OpenACC 3.1 is unclear about the behavior for shared memory.
          Clacc returns the specified host pointer in that case.
    * `acc_hostptr`
        * As in other OpenACC implementations, Clacc's implementation
          of `acc_hostptr` has poor performance.  Calls to
          `acc_hostptr` are not recommended in production code but might
          be useful for debugging.
        * OpenACC 3.1 is unclear about the behavior for shared memory.
          Clacc returns the specified device pointer in that case.
    * `acc_is_present`
        * OpenACC 3.1 is unclear about handling of a null pointer.
          Clacc returns true in that case.
    * `acc_memcpy_to_device`, `acc_memcpy_from_device`,
      `acc_memcpy_device`, `acc_memcpy_d2d`
        * OpenACC 3.1 is unclear about handling a `bytes` argument
          equal to zero.  In that case, Clacc implements these as
          no-ops.
        * OpenACC 3.1 is unclear about handling of null pointers.
          Clacc produces a runtime error in that case.

See the section "OpenACC Runtime" in `README-OpenACC-design.md` for
further design details.

See the section "Source-to-Source Mode Limitations" below for caveats
related to source-to-source mode.

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
* C++ support is highly experimental and incomplete.  See `-Wopenacc-and-cxx`
  above for how to enable it.
* Objective-C/C++ are not supported.

Source-to-Source Mode Limitations
=================================

These limitations affect source-to-source mode but have no effect on traditional
compilation mode.

* Calls to the OpenACC Runtime Library API are not translated to
  OpenMP at compile time:
    * `acc_on_device` is implemented in terms of OpenMP fully within
      Clacc's `openacc.h`.  However, support for an OpenMP 5.1
      extension, enum variants, is required for full support.
      Moreover, it can malfunction when enabling source-to-source mode
      using `-fopenacc-ast-print`.  See the comments on
      `acc_on_device` in Clacc's `openacc.h` for details.
    * Other routines are implemented as wrappers around the OpenMP
      Runtime Library Routines and require Clacc's OpenACC runtime
      library to be linked.  See the section "Linking" in
      `../README.md` for details.
* Calls to `acc2omp_async2dep` are used in the translation of `async` clauses
  but are not specified by OpenMP.  It also requires Clacc's OpenACC runtime
  library to be linked.
* Occurrences of the `_OPENACC` preprocessor macro are not translated
  to OpenMP:
    * Instead, the compiler inserts its `_OPENACC` definition at the
      beginning of the OpenMP translation of any compilation unit that
      uses it.
    * The reason for the insertion is that a compiler defines
      `_OPENACC` only when OpenACC compilation is enabled, so the
      OpenMP compiler for the translation normally does not.  The
      insertion is then necessary so that any uses of `_OPENACC`
      remaining after translation do not behave differently than
      before.
    * The compilation warning diagnostic controlled by
      `-Wopenacc-omp-macro-inserted` reports the inserted definition
      because it might prove problematic if the user adds new code to
      the OpenMP translation that expects `_OPENACC` to reflect the
      actual compilation mode.
    * When source-to-source mode is enabled by `-fopenacc-ast-print`
      instead of `-fopenacc-print`, macros are fully expanded, so this
      issue is irrelevant and the warning diagnostic controlled by
      `-Wopenacc-omp-macro-inserted` is never reported.
* Preprocessor macros appearing within OpenACC clauses or associated
  statements are expanded in the OpenMP translation:
    * See the "Source-to-Source Translation" section in
      `README-OpenACC-design.md` for an explanation of why this
      happens and how Clacc might evolve to prevent it in the future.
* Preprocessor macro usage and the `_Pragma` operator form can
  sometimes prevent OpenACC directives from being translated:
    * Clacc cannot translate an OpenACC directive if it meets any of
      the following conditions:
        * The directive is expanded entirely from a preprocessor macro
          (and thus uses `_Pragma` form).
        * The directive uses `_Pragma` form and is either an
          executable directive or the `routine` directive.
        * The associated code must be rewritten but its last token is
          expanded from a preprocessor macro:
            * In the case of a `routine` directive, the associated
              function declaration must always be rewritten.
            * In the case of `_Pragma` form, the associated code must
              always be rewritten.
            * In the case of `#pragma` form, whether the associated
              code must be rewritten depends on Clacc's mapping for
              the directive to OpenMP.
    * Clacc reports an error diagnostic for every such OpenACC
      directive in the source file.  Clacc then prints a version of
      the source in which all OpenACC directives are transformed
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
    * `<OMP-EXT>` can be `ext` to control all warnings about OpenMP extensions.
    * Other possible values for `<OMP-EXT>`, `<ACC-FEATURE>`, and `<KIND>` are
      described in the remainder of this section.
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
    * `ompx_hold` map type modifier.
* OpenACC Semantics Required
    * OpenACC specifies two reference counters for tracking device
      allocations: a structured reference counter for `data` and
      `parallel` directives, and a dynamic reference counter for
      `enter data` and `exit data` directives and corresponding
      OpenACC Runtime Library routines.
    * OpenMP 5.0 specifies only one, which can thus be considered a
      dynamic reference counter.
    * Clacc uses the `ompx_hold` map type modifier extension to
      implement a structured reference counter.
* Diagnostic Options
    * `-Wopenacc-omp-map-ompx-hold`
    * `-Wno-error=openacc-omp-map-ompx-hold`
    * `-Wno-openacc-omp-map-ompx-hold`
* Translation Options
    * `-fopenacc-structured-ref-count-omp=ompx-hold`
        * This is the default translation.
    * `-fopenacc-structured-ref-count-omp=no-ompx-hold`
        * This translation omits the `ompx_hold` map type modifier.
          Thus, contrary to OpenACC semantics, it always uses the
          dynamic reference counter, and so device memory might be
          deallocated prematurely for some OpenACC applications.
        * However, this translation should be sufficient if, for
          example, an OpenACC application always pairs `enter data`
          and `exit data` directives and corresponding OpenACC Runtime
          Library routine calls in a structured manner.

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
    * `ompx_no_alloc` map type modifier
* OpenACC Semantics Required
    * Clacc uses the `ompx_no_alloc` map type modifier to suppress all
      runtime actions as required by the OpenACC `no_create` clause in
      the case that data is not already present on the device.
    * This modifier is combined with the OpenMP `alloc` map type as
      opposed to, for example, `tofrom` in order to suppress
      device-to-host transfers in the case that there is a reference
      count decrement within the associated region.
* Diagnostic Options
    * `-Wopenacc-omp-map-ompx-no-alloc`
    * `-Wno-error=openacc-omp-map-ompx-no-alloc`
    * `-Wno-openacc-omp-map-ompx-no-alloc`
* Translation Options
    * `-fopenacc-no-create-omp=ompx-no-alloc`
        * This is the default translation.
    * `-fopenacc-no-create-omp=no-ompx-no-alloc`
        * This translation omits the `ompx-no-alloc` map type
          modifier.  Thus, contrary to OpenACC semantics, it attempts
          to allocate the specified variable when it is not already
          present on the device.
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

Currently, Clacc's implementation of its `ompx_no_alloc` map type
modifier extension for OpenMP is not well tested outside of Clacc's
translations from OpenACC to OpenMP.  Thus, it is not yet recommended
for use in hand-written OpenMP code as it might not integrate well
with some OpenMP features.

### `atomic` Construct in Gang-Redundant Mode ###

* OpenACC Features Affected
    * `atomic` construct
* OpenMP Extension Employed
    * `atomic` construct strictly nested in `target teams` region (not permitted
      in OpenMP 5.2)
* OpenACC Semantics Required
    * OpenACC 3.2 permits the `atomic` construct in gang-redundant mode.  This
      case occurs when an `atomic` construct is encountered within a `parallel`
      region and there is no partitioned `loop` region nested in between.
    * Clacc strictly nests OpenMP's `atomic` construct within a `target teams`
      region to implement the case that an OpenACC `atomic` construct appears
      in gang-redundant mode.
* Diagnostic Options
    * `-Wopenacc-omp-atomic-in-teams`
    * `-Wno-error=openacc-omp-atomic-in-teams`
    * `-Wno-openacc-omp-atomic-in-teams`
    * These warnings diagnose use of the above OpenMP extension only when the
      nesting is lexical.  Dynamic cases are not diagnosed by Clacc's compiler.
* Translation Options
    * None.

### `tile` Clause in Gang-Redundant Mode ###

* OpenACC Features Affected
    * `tile` clause on sequential `loop` construct
* OpenMP Extension Employed
    * `tile` construct strictly nested in `target teams` region (not permitted
      in OpenMP 5.2)
* OpenACC Semantics Required
    * OpenACC 3.3 permits a sequential `loop` construct with a `tile` clause in
      gang-redundant mode.  This case occurs when a sequential `loop` construct
      with a `tile` clause is encountered within a `parallel` region and there
      is no partitioned `loop` region nested in between.
    * Clacc strictly nests OpenMP's `tile` construct within a `target teams`
      region to implement the case that an OpenACC sequential `loop` construct
      with a `tile` clause appears in gang-redundant mode.
* Diagnostic Options
    * `-Wopenacc-omp-tile-in-teams`
    * `-Wno-error=openacc-omp-tile-in-teams`
    * `-Wno-openacc-omp-tile-in-teams`
    * These warnings diagnose use of the above OpenMP extension only when the
      nesting is lexical.  Dynamic cases are not diagnosed by Clacc's compiler.
* Translation Options
    * None.

OpenMP Runtime Library API
--------------------------

Some OpenACC Runtime Library API routines and some features of the
OpenACC Profiling Interface are implemented in terms of extensions to
OpenMP's Runtime Library Routines.  See the section "OpenACC Runtime"
in `README-OpenACC-design.md` for details.

OMPT
----

Some features of the OpenACC Profiling Interface depend on OMPT
extensions.  See the section "OpenACC Profiling Interface" in
`README-OpenACC-design.md` for details.
