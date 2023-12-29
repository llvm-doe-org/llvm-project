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

## Existing Deployments

Clacc has been deployed on several popular systems and can be loaded using the
instructions below.  In many cases, [TAU](http://www.tau.uoregon.edu/) has also
been deployed and can optionally be loaded in order to profile Clacc-compiled
OpenACC applications with a command like: `tau_exec -T serial -openacc ./a.out`

* ORNL's Frontier

    ```
    $ module load ums ums025 rocm clacc
    $ module help clacc
    $ module load ums ums002 tau/2.32.1-clang-acc # optional
    ```

* ANL's Polaris

    ```
    $ module use --append /home/jdenny/shared/polaris/modules
    $ module load clacc
    $ module help clacc
    $ module load tau-clacc # optional
    ```

## Building and Installing

If you are working on a system without an existing installation of Clacc, this
section describes two approaches for building and installing Clacc: (1) by
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
$ spack config add 'modules:prefix_inspections:lib64:[LD_LIBRARY_PATH]'
$ spack config add 'modules:prefix_inspections:lib:[LD_LIBRARY_PATH]'

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
$ module load nvhpc/22.11
$ PATH=/opt/nvidia/hpc_sdk/Linux_x86_64/22.11/cuda/11.8/bin:$PATH
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
$ module load llvm/14.0.0
$ . spack/share/spack/setup-env.sh
$ spack config add 'modules:prefix_inspections:lib64:[LD_LIBRARY_PATH]'
$ spack config add 'modules:prefix_inspections:lib:[LD_LIBRARY_PATH]'
$ spack install llvm-doe@develop.clacc % clang@14.0.0 +cuda cuda_arch=70 -libcxx -internal_unwind
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
$ spack config add 'modules:prefix_inspections:lib64:[LD_LIBRARY_PATH]'
$ spack config add 'modules:prefix_inspections:lib:[LD_LIBRARY_PATH]'
$ spack install llvm-doe@develop.clacc % clang@14.0.0 -libcxx -internal_unwind
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
$ module load cmake/3.19.2 gnu/9.2.0 nvhpc/22.11
$ PATH=/opt/nvidia/hpc_sdk/Linux_ppc64le/22.11/cuda/11.8/bin:$PATH
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
$ module load gnu/10.2.0
$ . spack/share/spack/setup-env.sh
$ spack config add 'modules:prefix_inspections:lib64:[LD_LIBRARY_PATH]'
$ spack config add 'modules:prefix_inspections:lib:[LD_LIBRARY_PATH]'
$ spack install llvm-doe@develop.clacc % gcc@10.2.0 +cuda cuda_arch=70 -internal_unwind
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
$ clang -fopenacc test.c
$ ./a.out
Hello World: 2
```

To compile and run for an NVIDIA GPU:

```
$ clang -fopenacc -fopenmp-targets=nvptx64-nvidia-cuda test.c
$ ./a.out
Hello World: 2
```

To compile and run for an AMD GPU:

```
$ clang -fopenacc -fopenmp-targets=amdgcn-amd-amdhsa test.c
$ ./a.out
Hello World: 2
```

If you see an error for any of the above examples, try extending the `clang`
command line with `-L` followed by the root `lib` directory of your Clacc
install directory.  For a spack install, `spack find -p llvm-doe@develop.clacc`
identifies the Clacc install directory.  The problem is that `clang` sometimes
tries to link systems libraries (e.g., `/usr/lib64/libomptarget.so`) instead of
the libraries that were built and installed for it.  This problem is inherited
from upstream LLVM and is not unique to Clacc.

## Usage from a Build Directory

Most Clacc users should work from the install directory, as described
in the previous section.  However, if you plan to modify Clacc, it
might be easier to work from the build directory instead.  Doing so
requires setting many environment variables and command-line options
to ensure `clang` finds its own libraries and header files.

For example, to compile and run with offloading to a GPU, where
`$CLACC_BUILD_DIR` is the `build` directory created when following [the above
build procedure from Git](#from-git):

* For an NVIDIA GPU:

    ```
    $ export PATH=$CLACC_BUILD_DIR/bin:$PATH
    $ export LD_LIBRARY_PATH=$CLACC_BUILD_DIR/lib:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src:$LD_LIBRARY_PATH
    $ clang -fopenacc -fopenmp-targets=nvptx64-nvidia-cuda \
      --libomptarget-nvptx-bc-path=$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
      -L $CLACC_BUILD_DIR/lib \
      -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
      -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
      -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
      -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
      -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
      test.c
    $ ./a.out
    ```

* For an AMD GPU:

    ```
    $ export PATH=$CLACC_BUILD_DIR/bin:$PATH
    $ export LD_LIBRARY_PATH=$CLACC_BUILD_DIR/lib:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget:$CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src:$LD_LIBRARY_PATH
    $ clang -fopenacc -fopenmp-targets=amdgcn-amd-amdhsa \
      -L $CLACC_BUILD_DIR/lib \
      -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
      -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libomptarget \
      -L $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
      -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/runtime/src \
      -isystem $CLACC_BUILD_DIR/runtimes/runtimes-bins/openmp/libacc2omp/src \
      test.c
    $ ./a.out
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
    * Without `-fopenmp-targets` or `--offload-arch`, the host is targeted.
    * `-fopenmp-targets=<triples>` specifies desired offloading
      targets.  So far, the following triples have been tested:
        * `x86_64-pc-linux-gnu` for x86_64.
        * `powerpc64le-ibm-linux-gnu` for Power9.
        * `nvptx64-nvidia-cuda` for NVIDIA GPUs.
        * `amdgcn-amd-amdhsa` for AMD GPUs.
    * `--offload-arch=<arch>` specifies a desired offloading target
      architecture, such as `sm_35`.  It is not necessary to specify both
      `-fopenmp-targets` and `--offload-arch` as one can be determined based on
      the other.
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
    * `--offload-arch` and options starting with `-fopenmp-` have not been
      tested in OpenACC source-to-source mode.
* `-fopenmp` produces an error diagnostic when OpenACC support is
  enabled in either mode as Clacc currently does not support OpenACC
  and OpenMP in the same source.

For brief descriptions of all OpenACC-related and OpenMP-related
command-line options, run Clacc's `clang -help` and search for
`openacc`, `openmp`, or `offload-arch`.  The section "Supported Features" in
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

### ALCF

This research used resources of the [Argonne Leadership Computing
Facility](https://alcf.anl.gov/), a U.S. Department of Energy (DOE) Office of
Science user facility at Argonne National Laboratory and is based on research
supported by the U.S. DOE Office of Science-Advanced Scientific Computing
Research Program, under Contract No.  DE-AC02-06CH11357.

# The LLVM Compiler Infrastructure

Welcome to the LLVM project!

This repository contains the source code for LLVM, a toolkit for the
construction of highly optimized compilers, optimizers, and run-time
environments.

The LLVM project has multiple components. The core of the project is
itself called "LLVM". This contains all of the tools, libraries, and header
files needed to process intermediate representations and convert them into
object files. Tools include an assembler, disassembler, bitcode analyzer, and
bitcode optimizer.

C-like languages use the [Clang](http://clang.llvm.org/) frontend. This
component compiles C, C++, Objective-C, and Objective-C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

## Getting the Source Code and Building LLVM

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm)
page for information on building and running LLVM.

For information on how to contribute to the LLVM project, please take a look at
the [Contributing to LLVM](https://llvm.org/docs/Contributing.html) guide.

## Getting in touch

Join the [LLVM Discourse forums](https://discourse.llvm.org/), [Discord
chat](https://discord.gg/xS7Z362), or #llvm IRC channel on
[OFTC](https://oftc.net/).

The LLVM project has adopted a [code of conduct](https://llvm.org/docs/CodeOfConduct.html) for
participants to all modes of communication within the project.
