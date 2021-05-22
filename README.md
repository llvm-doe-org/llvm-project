# Clacc

Clacc is a project to add [OpenACC](https://www.openacc.org/) support
to [Clang](http://clang.llvm.org/) and [LLVM](http://llvm.org/).
Clacc is funded by the [U.S. Exascale Computing Project
(ECP)](https://www.exascaleproject.org) and is maintained by [Oak
Ridge National Laboratory](https://www.ornl.gov/).  Please contact
[Joel E. Denny](mailto:dennyje@ornl.gov) with any questions.

## Git Repo

Clacc's main branch can be found at:

> <https://github.com/llvm-doe-org/llvm-project/tree/clacc/main>

That is the `clacc/main` branch within the LLVM DOE Fork maintained by
[Oak Ridge National Laboratory](https://www.ornl.gov/) as a fork of
[LLVM's github repo](https://github.com/llvm/llvm-project).  As for
any project hosted by the LLVM DOE Fork, links to all Clacc branches,
issues, etc. can be found in the wiki:

> <https://github.com/llvm-doe-org/llvm-project/wiki>

## Building and Testing

For a brief guide to getting started with LLVM, see the section [The
LLVM Compiler Infrastructure](#the-llvm-compiler-infrastructure)
below, which contains the contents of the upstream LLVM `README.md`.

Clacc should be built in the same manner as upstream LLVM when Clang
and OpenMP support are desired.  At minimum, you must build the
`clang` and `openmp` subprojects of LLVM.  For example, depending on
your system configuration, the following might prove sufficient:

```
$ cd $LLVM_GIT_DIR
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=../install \
        -DLLVM_ENABLE_PROJECTS='clang' \
        -DLLVM_ENABLE_RUNTIMES='openmp' \
        ../llvm
$ make
$ make install
```

For further details on building OpenMP support, see the following FAQ:

> <https://openmp.llvm.org/docs/SupportAndFAQ.html>

Clacc's OpenACC test suites currently build and run OpenACC test
programs.  Normally it builds them without offloading to avoid
requiring specific hardware.  To specify hardware architectures
available on your system so that it tests offloading to them as well,
specify one or more of the following when running `cmake`:

```
-DCLANG_ACC_TEST_EXE_X86_64=4  # x86_64-unknown-linux-gnu
-DCLANG_ACC_TEST_EXE_PPC64LE=4 # ppc64le-unknown-linux-gnu
-DCLANG_ACC_TEST_EXE_NVPTX64=6 # nvptx64-nvidia-cuda
```

In each case, the value should be `0`, the empty string, or
unspecified to turn off testing of that device type.  Otherwise, it
should be the number of devices of that type that LLVM's OpenMP
runtime is expected to report as available on the system.  In the
latter case, the value is used to test device management
functionality, such as `acc_get_num_devices`.  Specifying a smaller
non-zero value will not significantly affect device resources used
during testing but will cause some tests to fail.

Test suites checking Clacc's OpenACC support can be run by themselves
or as part of larger test suites, as follows:

```
$ make check-clang-openacc # OpenACC directives
$ make check-clang         # all of Clang including OpenACC directives
$ make check-libacc2omp    # OpenACC runtime
$ make check-openmp        # OpenMP and OpenACC runtime
$ make check-all           # all LLVM subprojects
```

## Basic Usage

Clacc is still under development and requires significant manual
intervention for some applications (see the Documentation section
below).  Currently, Clacc only supports OpenACC programs with C as the
base language.

Clacc's compiler is the `clang` executable in the `bin` subdirectory
of the install directory.  It's also possible to work from the build
directory, but additional effort is usually then required to help
`clang` find its own libraries and header files.

Here's a simple example of using Clacc's `clang`, where
`$CLACC_INSTALL_DIR` is `$LLVM_GIT_DIR/install` when following the
build procedure above:

```
$ export PATH=$CLACC_INSTALL_DIR/bin:$PATH
$ export LD_LIBRARY_PATH=$CLACC_INSTALL_DIR/lib:$LD_LIBRARY_PATH
$ cat test.c
#include <stdio.h>
int main() {
  #pragma acc parallel num_gangs(2)
  printf("Hello World\n");
  return 0;
}
```

To compile and run only for host:

```
$ clang -fopenacc test.c && ./a.out
Hello World
Hello World
```

To compile and run for an NVIDIA GPU:

```
$ clang -fopenacc -fopenmp-targets=nvptx64-nvidia-cuda test.c && ./a.out
Hello World
Hello World
```

To use source-to-source mode:

```
$ clang -fopenacc-print=omp test.c
#include <stdio.h>
int main() {
  #pragma omp target teams num_teams(2)
  printf("Hello World\n");
  return 0;
}
```
## Compiler Options

The most relevant `clang` command-line options are as follows:

* OpenACC traditional compilation mode:
    * `-fopenacc` enables OpenACC support.  Unless source-to-source
      mode is enabled as discussed below, Clacc translates OpenACC to
      OpenMP and then compiles the OpenMP.
    * Without `-fopenmp-targets`, the host is targeted.
    * `-fopenmp-targets=<triples>` specifies desired offloading
      targets.  So far, the following triples have been tested:
        * `x86_64-unknown-linux-gnu` for x86_64.
        * `ppc64le-unknown-linux-gnu` for Power9.
        * `nvptx64-nvidia-cuda` for NVIDIA GPUs.
    * In general, options starting with `-fopenmp-` adjust various
      OpenMP features when compiling the OpenMP translation.  So far,
      only `-fopenmp-targets=<triples>` has been tested.
* OpenACC source-to-source mode:
    * `-fopenacc-print=omp` enables OpenACC support and prints the
      OpenMP translation instead of performing traditional
      compilation.
    * `-fopenacc-print=omp-acc` is the same except it also prints the
      original OpenACC in comments next to the OpenMP.
    * `-fopenacc-print=acc-omp` is the same except it prints the
      OpenMP in comments and leaves the original OpenACC uncommented.
    * `-fopenacc` is redundant with any of those options.
    * Options starting with `-fopenmp-` have not been tested in
      OpenACC source-to-source mode.
* `-fopenmp` produces an error diagnostic when OpenACC support is
  enabled in either mode as Clacc currently does not support OpenACC
  and OpenMP in the same source.

For brief descriptions of all OpenACC-related and OpenMP-related
command-line options, run Clacc's `clang -help` and search for
`openacc` or `openmp`.  The section "Supported Features" in
`clang/README-OpenACC-status.md` also provides a full list of
OpenACC-related command-line options and cross-references to related
status and design documentation.

## Linking

When compiling an OpenACC application with the Clacc compiler, you
must link an OpenMP runtime library.  Currently, we have only tested
with Clacc's version of LLVM's OpenMP runtime library.  Moreover, some
OpenACC features as compiled by Clacc depend on OpenMP extensions
implemented there.

If an OpenACC application uses the OpenACC Runtime Library API, or if
you wish to profile an OpenACC application using an OpenACC profiling
library depending on the OpenACC Profiling Interface, you must also
link Clacc's `libacc2omp.so` with your application.  This library
serves as a wrapper around Clacc's version of LLVM's OpenMP runtime
(`libomp.so`, `libomptarget.so`, etc.) and is installed in the same
directories.

Normally, the `-fopenacc` option to `clang` triggers linking of all
such required libraries automatically.  However, if you are linking
with a separate command, or if you are using Clacc's source-to-source
mode followed by OpenMP compilation, you must specify linking of the
required libraries.

## Documentation

The following documentation is maintained in the Clacc git repo:

* `README.md` is this file.
* `clang/README-OpenACC-status.md` describes the current status of
  Clacc's support for OpenACC.
* `clang/README-OpenACC-design.md` describes the current design of
  Clacc.

The [next section](#the-llvm-compiler-infrastructure) contains the
contents of the upstream LLVM `README.md`.

# The LLVM Compiler Infrastructure

This directory and its sub-directories contain source code for LLVM,
a toolkit for the construction of highly optimized compilers,
optimizers, and run-time environments.

The README briefly describes how to get started with building LLVM.
For more information on how to contribute to the LLVM project, please
take a look at the
[Contributing to LLVM](https://llvm.org/docs/Contributing.html) guide.

## Getting Started with the LLVM System

Taken from https://llvm.org/docs/GettingStarted.html.

### Overview

Welcome to the LLVM project!

The LLVM project has multiple components. The core of the project is
itself called "LLVM". This contains all of the tools, libraries, and header
files needed to process intermediate representations and converts it into
object files.  Tools include an assembler, disassembler, bitcode analyzer, and
bitcode optimizer.  It also contains basic regression tests.

C-like languages use the [Clang](http://clang.llvm.org/) front end.  This
component compiles C, C++, Objective-C, and Objective-C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

### Getting the Source Code and Building LLVM

The LLVM Getting Started documentation may be out of date.  The [Clang
Getting Started](http://clang.llvm.org/get_started.html) page might have more
accurate information.

This is an example work-flow and configuration to get and build the LLVM source:

1. Checkout LLVM (including related sub-projects like Clang):

     * ``git clone https://github.com/llvm/llvm-project.git``

     * Or, on windows, ``git clone --config core.autocrlf=false
    https://github.com/llvm/llvm-project.git``

2. Configure and build LLVM and Clang:

     * ``cd llvm-project``

     * ``cmake -S llvm -B build -G <generator> [options]``

        Some common build system generators are:

        * ``Ninja`` --- for generating [Ninja](https://ninja-build.org)
          build files. Most llvm developers use Ninja.
        * ``Unix Makefiles`` --- for generating make-compatible parallel makefiles.
        * ``Visual Studio`` --- for generating Visual Studio projects and
          solutions.
        * ``Xcode`` --- for generating Xcode projects.

        Some Common options:

        * ``-DLLVM_ENABLE_PROJECTS='...'`` --- semicolon-separated list of the LLVM
          sub-projects you'd like to additionally build. Can include any of: clang,
          clang-tools-extra, libcxx, libcxxabi, libunwind, lldb, compiler-rt, lld,
          polly, or debuginfo-tests.

          For example, to build LLVM, Clang, libcxx, and libcxxabi, use
          ``-DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi"``.

        * ``-DCMAKE_INSTALL_PREFIX=directory`` --- Specify for *directory* the full
          path name of where you want the LLVM tools and libraries to be installed
          (default ``/usr/local``).

        * ``-DCMAKE_BUILD_TYPE=type`` --- Valid options for *type* are Debug,
          Release, RelWithDebInfo, and MinSizeRel. Default is Debug.

        * ``-DLLVM_ENABLE_ASSERTIONS=On`` --- Compile with assertion checks enabled
          (default is Yes for Debug builds, No for all other build types).

      * ``cmake --build build [-- [options] <target>]`` or your build system specified above
        directly.

        * The default target (i.e. ``ninja`` or ``make``) will build all of LLVM.

        * The ``check-all`` target (i.e. ``ninja check-all``) will run the
          regression tests to ensure everything is in working order.

        * CMake will generate targets for each tool and library, and most
          LLVM sub-projects generate their own ``check-<project>`` target.

        * Running a serial build will be **slow**.  To improve speed, try running a
          parallel build.  That's done by default in Ninja; for ``make``, use the option
          ``-j NNN``, where ``NNN`` is the number of parallel jobs, e.g. the number of
          CPUs you have.

      * For more information see [CMake](https://llvm.org/docs/CMake.html)

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-started-with-llvm)
page for detailed information on configuring and compiling LLVM. You can visit
[Directory Layout](https://llvm.org/docs/GettingStarted.html#directory-layout)
to learn about the layout of the source code tree.
