# Clacc

Clacc is a project to add [OpenACC](https://www.openacc.org/) support
to [Clang](http://clang.llvm.org/) and [LLVM](http://llvm.org/).
Clacc is funded by the [Exascale Computing Project
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

## Building and Installing

This section describes two approaches for building and installing Clacc: (1) by
directly cloning Clacc from its git repo, running cmake, etc., or (2) by using
Spack.  The first approach enables maximum control over the build, is regularly
tested during Clacc development, and is recommended for anyone who wishes to
modify Clacc.  However, the second approach might be more convenient for some
Clacc users, especially those already using Spack.

After generally describing both approaches, this section presents a series of
examples that apply these approaches to build Clacc on real machines.  Even if
none of those examples fit your requirements exactly, they might reveal options
you should investigate.

### From Git

Clacc should be built in the same manner as upstream LLVM when Clang
and OpenMP support are desired.  At minimum, you must build the
`clang` and `openmp` subprojects of LLVM.  For example:

```
$ git clone -b clacc/main https://github.com/llvm-doe-org/llvm-project.git
$ cd llvm-project
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=../install \
        -DLLVM_ENABLE_PROJECTS=clang      \
        -DLLVM_ENABLE_RUNTIMES=openmp     \
        ../llvm
$ make
$ make install
$ export PATH=`pwd`/../install/bin:$PATH
$ export LD_LIBRARY_PATH=`pwd`/../install/lib:$LD_LIBRARY_PATH
```

Building LLVM successfully can be challenging, and the above minimal
procedure might not be sufficient for your system, or you might wish
to tweak some options.  It is not practical to capture all possible
issues here.  However, there are several resources that can help:

* See the example Clacc build procedures later in this section.
* For further details on building OpenMP support, see the following FAQ:

    > <https://openmp.llvm.org/docs/SupportAndFAQ.html>

* For a brief guide to getting started with LLVM, see the section [The
  LLVM Compiler Infrastructure](#the-llvm-compiler-infrastructure)
  below, which contains the contents of the upstream LLVM `README.md`.
* Contact the Clacc maintainers.  See the [contact info
  above](#clacc).

### From Spack

[Spack](https://spack.io/) is a package manager that has become popular in HPC,
but it can also be used on, for example, a Linux laptop.  If you're not familiar
with Spack, see the [Spack
documentation](https://spack.readthedocs.io/en/latest/).

Spack's Clacc package builds from `clacc/main` in the LLVM DOE Fork.  The
package specification is:

```
llvm-doe@develop.clacc
```

For example, the following minimal procedure installs Spack and then uses it to
install Clacc:

```
$ git clone -c feature.manyFiles=true https://github.com/spack/spack.git
$ . spack/share/spack/setup-env.sh # different shells require different scripts

$ spack install llvm-doe@develop.clacc # might take hours to build
$ spack load llvm-doe@develop.clacc # adjusts PATH, LD_LIBRARY_PATH, etc.
$ clang --version # reveals the Clacc git commit hash
```

As in the examples below, additional options might be required for some systems.
For example, `cuda` options are required to enable NVIDIA GPU offloading.

### Example Build: ORNL ExCL's equinox

[System details](https://excl.ornl.gov/excl-systems/): x86_64, 4
NVIDIA V100 GPUs

From Git:

```
$ git clone -b clacc/main https://github.com/llvm-doe-org/llvm-project.git
$ cd llvm-project
$ mkdir build && cd build
$ module load nvhpc/21.7
$ PATH=/opt/nvidia/hpc_sdk/Linux_x86_64/21.7/cuda/11.4/bin:$PATH
$ cmake -DCMAKE_INSTALL_PREFIX=../install    \
        -DCMAKE_BUILD_TYPE=Release           \
        -DLLVM_ENABLE_PROJECTS=clang         \
        -DLLVM_ENABLE_RUNTIMES=openmp        \
        -DLLVM_TARGETS_TO_BUILD="host;NVPTX" \
        -DCMAKE_C_COMPILER=gcc               \
        -DCMAKE_CXX_COMPILER=g++             \
        ../llvm
$ make
$ make install
$ export PATH=`pwd`/../install/bin:$PATH
$ export LD_LIBRARY_PATH=`pwd`/../install/lib:$LD_LIBRARY_PATH
```

From Spack:

```
$ git clone -c feature.manyFiles=true https://github.com/spack/spack.git
$ . spack/share/spack/setup-env.sh
$ module load llvm/13.0.1
$ spack install llvm-doe@develop.clacc % clang@13.0.1 +cuda cuda_arch=70
$ spack load llvm-doe@develop.clacc
```

### Example Build: ORNL ExCL's explorer

[System details](https://excl.ornl.gov/excl-systems/): AMD EPYC 7272,
2 AMD MI60 Instinct GPUs

From Git:

```
$ git clone -b clacc/main https://github.com/llvm-doe-org/llvm-project.git
$ cd llvm-project
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=../install     \
        -DCMAKE_BUILD_TYPE=Release            \
        -DLLVM_ENABLE_PROJECTS="clang;lld"    \
        -DLLVM_ENABLE_RUNTIMES=openmp         \
        -DLLVM_TARGETS_TO_BUILD="host;AMDGPU" \
        -DCMAKE_C_COMPILER=gcc                \
        -DCMAKE_CXX_COMPILER=g++              \
        ../llvm
$ make
$ make install
$ export PATH=`pwd`/../install/bin:$PATH
$ export LD_LIBRARY_PATH=`pwd`/../install/lib:$LD_LIBRARY_PATH
```

From Spack:

```
$ git clone -c feature.manyFiles=true https://github.com/spack/spack.git
$ . spack/share/spack/setup-env.sh
$ spack install llvm-doe@develop.clacc % clang@10.0.0
$ spack load llvm-doe@develop.clacc
```

### Example Build: ORNL ExCL's leconte

[System details](https://excl.ornl.gov/excl-systems/): 2 POWER9 CPUs,
6 NVIDIA V100 GPUs

From Git:

```
$ git clone -b clacc/main https://github.com/llvm-doe-org/llvm-project.git
$ cd llvm-project
$ mkdir build && cd build
$ module load cmake/3.19.2 gnu/9.2.0 nvhpc/22.2
$ PATH=/opt/nvidia/hpc_sdk/Linux_ppc64le/22.2/cuda/11.6/bin:$PATH
$ cmake -DCMAKE_INSTALL_PREFIX=../install    \
        -DCMAKE_BUILD_TYPE=Release           \
        -DLLVM_ENABLE_PROJECTS=clang         \
        -DLLVM_ENABLE_RUNTIMES=openmp        \
        -DLLVM_TARGETS_TO_BUILD="host;NVPTX" \
        -DCMAKE_C_COMPILER=gcc               \
        -DCMAKE_CXX_COMPILER=g++             \
        -DGCC_INSTALL_PREFIX=$GCC_DIR        \
        ../llvm
$ make
$ make install
$ export PATH=`pwd`/../install/bin:$PATH
$ export LD_LIBRARY_PATH=`pwd`/../install/lib:$LD_LIBRARY_PATH
```

From Spack:

```
$ git clone -c feature.manyFiles=true https://github.com/spack/spack.git
$ . spack/share/spack/setup-env.sh
$ module load gnu/10.2.0
$ cp spack/var/spack/repos/builtin/packages/llvm/llvm14-hwloc-ompd.patch \
     spack/var/spack/repos/builtin/packages/llvm-doe
# Open spack/var/spack/repos/builtin/packages/llvm-doe/package.py in an editor.
# Add the following lines after the existing calls to the 'patch' function,
# making sure to indent the same amount as those calls:
#
#   # patch for missing hwloc.h include for libompd
#   patch('llvm14-hwloc-ompd.patch', when="@develop.clacc")
$ spack install llvm-doe@develop.clacc % gcc@10.2.0 +cuda cuda_arch=70
$ spack load llvm-doe@develop.clacc
```

## Basic Usage

Clacc's compiler is the `clang` executable in the `bin` subdirectory of the
install directory.  Here's a simple example of using Clacc after installing it
and after setting environment variables as described in the previous section:

```
$ cat test.c
#include <stdio.h>
int main() {
  int x = 0;
  #pragma acc parallel num_gangs(2) reduction(+:x)
  x += 1;
  printf("Hello World: %d\n", x);
  return 0;
}
```

To compile and run only for host:

```
$ clang -fopenacc test.c && ./a.out
Hello World: 2
```

To compile and run for an NVIDIA GPU:

```
$ clang -fopenacc -fopenmp-targets=nvptx64-nvidia-cuda test.c && ./a.out
Hello World: 2
```

To compile and run for an AMD GPU:

```
$ clang -fopenacc -fopenmp-targets=amdgcn-amd-amdhsa test.c && ./a.out
Hello World: 2
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

## Usage from a Build Directory

Most Clacc users should work from the install directory, as described
in the previous section.  However, if you plan to modify Clacc, it
might be easier to work from the build directory instead.  Doing so
requires setting many environment variables and command-line options
to ensure `clang` finds its own libraries and header files regardless
of any like-named files that happen to be installed in system
directories.  Not setting those can produce unexpected behavior at
compile time or run time.  This complexity is inherited from LLVM
upstream and is not unique to Clacc.

For example, to compile and run for an NVIDIA GPU, where `$CLACC_BUILD_DIR` is
the `build` directory created when following [the above build procedure from
Git](#from-git):

```
$ export PATH=$CLACC_BUILD_DIR/bin:$PATH
$ export LD_LIBRARY_PATH=$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src:$LD_LIBRARY_PATH
$ clang -fopenacc -fopenmp-targets=nvptx64-nvidia-cuda \
  --libomptarget-nvptx-bc-path=$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
  -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
  -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
  -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
  -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
  -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
  test.c
$ ./a.out
```

For an AMD GPU, change the Clang command line to:

```
$ clang -fopenacc -fopenmp-targets=amdgcn-amd-amdhsa \
  --libomptarget-amdgcn-bc-path=$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
  -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
  -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
  -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
  -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
  -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
  test.c
```

## Testing

Test suites checking Clacc's OpenACC support can be run from Clacc's build
directory, such as  the `build` directory created when following [the above
build procedure from Git](#from-git).  They can be run by themselves or as part
of larger test suites, as follows:

```
$ make check-clang-openacc # OpenACC directives
$ make check-clang         # all of Clang including OpenACC directives
$ make check-libacc2omp    # OpenACC runtime
$ make check-openmp        # OpenMP and OpenACC runtime
$ make check-all           # all LLVM subprojects
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
        * `x86_64-pc-linux-gnu` for x86_64.
        * `powerpc64le-ibm-linux-gnu` for Power9.
        * `nvptx64-nvidia-cuda` for NVIDIA GPUs.
        * `amdgcn-amd-amdhsa` for AMD GPUs.
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

The [next top-level section](#the-llvm-compiler-infrastructure)
contains the contents of the upstream LLVM `README.md`.

## Acknowledgement

Clacc development depends on funding and resources from several
organizations.

### ECP

This research was supported by the [Exascale Computing
Project](https://www.exascaleproject.org) (17-SC-20-SC), a joint
project of the U.S. Department of Energy’s Office of Science and
National Nuclear Security Administration, responsible for delivering a
capable exascale ecosystem, including software, applications, and
hardware technology, to support the nation’s exascale computing
imperative.

### ExCL

This research used resources of the [Experimental Computing
Laboratory](https://excl.ornl.gov/) (ExCL) at the Oak Ridge National
Laboratory, which is supported by the Office of Science of the U.S.
Department of Energy under Contract No. DE-AC05-00OR22725.

### OLCF

This research used resources of the [Oak Ridge Leadership Computing
Facility](https://www.olcf.ornl.gov/) at the Oak Ridge National
Laboratory, which is supported by the Office of Science of the U.S.
Department of Energy under Contract No. DE-AC05-00OR22725.

# The LLVM Compiler Infrastructure

This directory and its sub-directories contain the source code for LLVM,
a toolkit for the construction of highly optimized compilers,
optimizers, and run-time environments.

The README briefly describes how to get started with building LLVM.
For more information on how to contribute to the LLVM project, please
take a look at the
[Contributing to LLVM](https://llvm.org/docs/Contributing.html) guide.

## Getting Started with the LLVM System

Taken from [here](https://llvm.org/docs/GettingStarted.html).

### Overview

Welcome to the LLVM project!

The LLVM project has multiple components. The core of the project is
itself called "LLVM". This contains all of the tools, libraries, and header
files needed to process intermediate representations and convert them into
object files. Tools include an assembler, disassembler, bitcode analyzer, and
bitcode optimizer. It also contains basic regression tests.

C-like languages use the [Clang](http://clang.llvm.org/) frontend. This
component compiles C, C++, Objective-C, and Objective-C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

### Getting the Source Code and Building LLVM

The LLVM Getting Started documentation may be out of date. The [Clang
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

        Some common options:

        * ``-DLLVM_ENABLE_PROJECTS='...'`` and ``-DLLVM_ENABLE_RUNTIMES='...'`` ---
          semicolon-separated list of the LLVM sub-projects and runtimes you'd like to
          additionally build. ``LLVM_ENABLE_PROJECTS`` can include any of: clang,
          clang-tools-extra, cross-project-tests, flang, libc, libclc, lld, lldb,
          mlir, openmp, polly, or pstl. ``LLVM_ENABLE_RUNTIMES`` can include any of
          libcxx, libcxxabi, libunwind, compiler-rt, libc or openmp. Some runtime
          projects can be specified either in ``LLVM_ENABLE_PROJECTS`` or in
          ``LLVM_ENABLE_RUNTIMES``.

          For example, to build LLVM, Clang, libcxx, and libcxxabi, use
          ``-DLLVM_ENABLE_PROJECTS="clang" -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi"``.

        * ``-DCMAKE_INSTALL_PREFIX=directory`` --- Specify for *directory* the full
          path name of where you want the LLVM tools and libraries to be installed
          (default ``/usr/local``). Be careful if you install runtime libraries: if
          your system uses those provided by LLVM (like libc++ or libc++abi), you
          must not overwrite your system's copy of those libraries, since that
          could render your system unusable. In general, using something like
          ``/usr`` is not advised, but ``/usr/local`` is fine.

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

        * Running a serial build will be **slow**. To improve speed, try running a
          parallel build. That's done by default in Ninja; for ``make``, use the option
          ``-j NNN``, where ``NNN`` is the number of parallel jobs to run.
          In most cases, you get the best performance if you specify the number of CPU threads you have.
          On some Unix systems, you can specify this with ``-j$(nproc)``.

      * For more information see [CMake](https://llvm.org/docs/CMake.html).

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-started-with-llvm)
page for detailed information on configuring and compiling LLVM. You can visit
[Directory Layout](https://llvm.org/docs/GettingStarted.html#directory-layout)
to learn about the layout of the source code tree.

## Getting in touch

Join [LLVM Discourse forums](https://discourse.llvm.org/), [discord chat](https://discord.gg/xS7Z362) or #llvm IRC channel on [OFTC](https://oftc.net/).

The LLVM project has adopted a [code of conduct](https://llvm.org/docs/CodeOfConduct.html) for
participants to all modes of communication within the project.
