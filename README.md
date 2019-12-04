# Clacc

Clacc is a project to add [OpenACC](https://www.openacc.org/) support
to [Clang](http://clang.llvm.org/) and [LLVM](http://llvm.org/).
Clacc is currently funded by the [U.S. Exascale Computing Project
(ECP)](https://www.exascaleproject.org) and is maintained by the
[Future Technologies Group](https://ft.ornl.gov) at [Oak Ridge
National Laboratory](https://www.ornl.gov/).  Please contact [Joel
E. Denny](mailto:dennyje@ornl.gov) with any questions.

## Sharing

There is currently no public repo for Clacc, and we ask that you do
not redistribute Clacc.  We'll take it public when ready, hopefully in
the coming months.  Eventually, we'll offer it to the upstream LLVM
project, and we already make a habit of upstreaming LLVM changes that
are not specific to OpenACC.

## Git Repo

The official Clacc git repo is:

> <https://code.ornl.gov/jum/Clacc>

This repo is a fork of [LLVM's github
repo](https://github.com/llvm/llvm-project).

The branches and tags are as follows:

* [`clacc`](https://code.ornl.gov/jum/Clacc/tree/clacc) branch: This
  contains our most recent stable work.

* [`clacc-2018-llvm-hpc`](https://code.ornl.gov/jum/Clacc/tree/clacc-2018-llvm-hpc)
  tag: This points to the version of Clacc we used for the evaluation
  in our [LLVM-HPC 2018
  paper](https://csmd.ornl.gov/index.php/node/362).

* [`master`](https://code.ornl.gov/jum/Clacc/tree/master) branch and
  other branches and tags whose names do not start with `clacc`: These
  are from upstream LLVM and do not contain Clacc.

So far, we use this repo just to share our source with others who have
reached out to us.  Eventually, we plan to set it up for CI, etc.  If
you see something missing that would be helpful now, let us know, and
we'll raise the priority.

## Building and Testing

For a brief guide to getting started with LLVM, see the section [The
LLVM Compiler Infrastructure](#the-llvm-compiler-infrastructure)
below, which contains the contents of the upstream LLVM `README.md`.

Clacc should be built in the same manner as upstream LLVM when Clang
and OpenMP support are desired.  At minimum, you must build the
`clang` and `openmp` subprojects of LLVM.  For example, depending on
your system configuration, the following might prove sufficient:

```
$ cd $LLVM_GIT_DIR/..
$ mkdir llvm-build && cd llvm-build
$ cmake -DCMAKE_INSTALL_PREFIX=../llvm-install \
        -DLLVM_ENABLE_PROJECTS='clang;openmp' \
        $LLVM_GIT_DIR
$ make
```

For further details on building OpenMP support, including another
build stage that improves offloading support, see the following blog:

> <https://www.hahnjo.de/blog/2018/10/08/clang-7.0-openmp-offloading-nvidia.html>

Clacc's Clang OpenACC test suite currently builds and runs OpenACC
test programs.  Normally it builds them without offloading to avoid
requiring specific hardware.  To specify hardware architectures
available on your system so that it tests offloading to them as well,
specify one or more of the following when running `cmake`:

```
-DCLANG_ACC_TEST_EXE_X86_64=True   # x86_64-unknown-linux-gnu
-DCLANG_ACC_TEST_EXE_NVPTX64=True  # nvptx64-nvidia-cuda
```

Clacc's Clang OpenACC test suite can be run by itself or as part of larger
test suites as follows:

```
$ make check-clang-openacc
$ make check-clang
$ make check-all
```

## Using

Clacc is still under development and requires significant manual
intervention for any real application or benchmark (see the
Documentation section below).  Currently, Clacc only supports OpenACC
programs with C as the base language.

Clacc's compiler is the `clang` executable in the `bin` subdirectory
of the LLVM build directory.  Here's a simple example of using it:

```
$ export LD_LIBRARY_PATH=$CLACC_BUILD/lib
$ export PATH=$CLACC_BUILD/bin:$PATH
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

The most relevant command-line options are as follows:

* OpenACC traditional compilation mode:
    * `-fopenacc` enables OpenACC support.  Unless source-to-source
      mode is enabled as discussed below, Clacc translates OpenACC to
      OpenMP and then compiles the OpenMP.
    * Options starting with `-fopenmp-` adjust various OpenMP features
      when compiling the OpenMP translation.  So far, only
      `-fopenmp-targets=<triples>` to specify desired offloading
      targets has been tested.  `nvptx64-nvidia-cuda` is the triple
      for NVIDIA GPUs.
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

For descriptions of all OpenACC-related and OpenMP-related
command-line options, run Clacc's `clang -help` and search for
`openacc` or `openmp`.

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

This directory and its subdirectories contain source code for LLVM,
a toolkit for the construction of highly optimized compilers,
optimizers, and runtime environments.

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
component compiles C, C++, Objective C, and Objective C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

### Getting the Source Code and Building LLVM

The LLVM Getting Started documentation may be out of date.  The [Clang
Getting Started](http://clang.llvm.org/get_started.html) page might have more
accurate information.

This is an example workflow and configuration to get and build the LLVM source:

1. Checkout LLVM (including related subprojects like Clang):

     * ``git clone https://github.com/llvm/llvm-project.git``

     * Or, on windows, ``git clone --config core.autocrlf=false
    https://github.com/llvm/llvm-project.git``

2. Configure and build LLVM and Clang:

     * ``cd llvm-project``

     * ``mkdir build``

     * ``cd build``

     * ``cmake -G <generator> [options] ../llvm``

        Some common generators are:

        * ``Ninja`` --- for generating [Ninja](https://ninja-build.org)
          build files. Most llvm developers use Ninja.
        * ``Unix Makefiles`` --- for generating make-compatible parallel makefiles.
        * ``Visual Studio`` --- for generating Visual Studio projects and
          solutions.
        * ``Xcode`` --- for generating Xcode projects.

        Some Common options:

        * ``-DLLVM_ENABLE_PROJECTS='...'`` --- semicolon-separated list of the LLVM
          subprojects you'd like to additionally build. Can include any of: clang,
          clang-tools-extra, libcxx, libcxxabi, libunwind, lldb, compiler-rt, lld,
          polly, or debuginfo-tests.

          For example, to build LLVM, Clang, libcxx, and libcxxabi, use
          ``-DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi"``.

        * ``-DCMAKE_INSTALL_PREFIX=directory`` --- Specify for *directory* the full
          pathname of where you want the LLVM tools and libraries to be installed
          (default ``/usr/local``).

        * ``-DCMAKE_BUILD_TYPE=type`` --- Valid options for *type* are Debug,
          Release, RelWithDebInfo, and MinSizeRel. Default is Debug.

        * ``-DLLVM_ENABLE_ASSERTIONS=On`` --- Compile with assertion checks enabled
          (default is Yes for Debug builds, No for all other build types).

      * Run your build tool of choice!

        * The default target (i.e. ``ninja`` or ``make``) will build all of LLVM.

        * The ``check-all`` target (i.e. ``ninja check-all``) will run the
          regression tests to ensure everything is in working order.

        * CMake will generate build targets for each tool and library, and most
          LLVM sub-projects generate their own ``check-<project>`` target.

        * Running a serial build will be *slow*.  To improve speed, try running a
          parallel build. That's done by default in Ninja; for ``make``, use
          ``make -j NNN`` (NNN is the number of parallel jobs, use e.g. number of
          CPUs you have.)

      * For more information see [CMake](https://llvm.org/docs/CMake.html)

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-started-with-llvm)
page for detailed information on configuring and compiling LLVM. You can visit
[Directory Layout](https://llvm.org/docs/GettingStarted.html#directory-layout)
to learn about the layout of the source code tree.
