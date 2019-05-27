Clacc is a project to add [OpenACC](https://www.openacc.org/) support
to [Clang](http://clang.llvm.org/) and [LLVM](http://llvm.org/).
Clacc is currently funded by the [U.S. Exascale Computing Project
(ECP)](https://www.exascaleproject.org) and is maintained by the
[Future Technologies Group](https://ft.ornl.gov) at [Oak Ridge
National Laboratory](https://www.ornl.gov/).  Please contact [Joel
E. Denny](mailto:dennyje@ornl.gov) with any questions.

Sharing
=======

There is currently no public repo for Clacc, and we ask that you do
not redistribute Clacc.  We'll take it public when ready, hopefully in
the coming months.  Eventually, we'll offer it to the upstream LLVM
project, and we already make a habit of upstreaming LLVM changes that
are not specific to OpenACC.

Git Repo
========

The official Clacc git repo is:

> <https://code.ornl.gov/jum/Clacc>

This repo is a fork of [LLVM's
monorepo](https://github.com/llvm-project/llvm-project-20170507).  We
will eventually update this to LLVM's more recent monorepo.

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

Building and Testing
====================

Clacc should be built in the same manner as upstream Clang when OpenMP
support is desired.  At minimum, you must build the `clang` and
`openmp` subprojects of LLVM.  For example, depending on your system
configuration, the following might prove sufficient:

```
$ cd $LLVM_GIT_DIR/..
$ mkdir llvm-build && cd llvm-build
$ cmake -G Ninja \
        -DCMAKE_INSTALL_PREFIX=../llvm-install \
        -DLLVM_ENABLE_PROJECTS='clang;openmp' \
        $LLVM_GIT_DIR
$ ninja
```

For further details on building OpenMP support, including another
build stage that improves offloading support, see the following blog:

> <https://www.hahnjo.de/blog/2018/10/08/clang-7.0-openmp-offloading-nvidia.html>

The Clang OpenACC test suite currently builds and runs OpenACC test
programs.  Normally it builds them without offloading to avoid
requiring specific hardware.  To specify hardware architectures
available on your system so that it tests offloading to them as well,
specify one or more of the following when running `cmake`:

```
-DCLANG_ACC_TEST_EXE_X86_64=True
-DCLANG_ACC_TEST_EXE_NVPTX64=True
```

The Clang OpenACC test suite can be run by itself or as part of larger
test suites as follows:

```
$ ninja check-clang-openacc
$ ninja check-clang
$ ninja check-all
```

Using
=====

Clacc is still under development and requires significant manual
intervention for any real application or benchmark (see the
Documentation section below).  Currently, Clacc only supports OpenACC
programs with C as the base language.

Clacc's compiler is the `clang` executable in the `bin` subdirectory
of the LLVM build directory.  The most relevant command-line options
are as follows:

* OpenACC traditional compilation mode:
    * `-fopenacc` enables OpenACC support.  Unless source-to-source
      mode is enabled as discussed below, Clacc translates OpenACC to
      OpenMP and then compiles the OpenMP.
    * `-fopenmp-*` adjusts various OpenMP features when compiling the
      OpenMP translation.  So far, only `-fopenmp-targets=<triples>`
      to specify desired offloading targets has been tested.
* OpenACC source-to-source mode:
    * `-fopenacc-print=omp` enables OpenACC support and prints the
      OpenMP translation instead of performing traditional
      compilation.
    * `-fopenacc-print=omp-acc` is the same except it also prints the
      original OpenACC in comments next to the OpenMP.
    * `-fopenacc-print=acc-omp` reverses that.
    * `-fopenacc` is redundant with any of those options.
    * `-fopenmp-*` support has not yet been tested in OpenACC
      source-to-source mode.
* `-fopenmp` produces an error diagnostic when OpenACC support is
  enabled in either mode as Clacc currently does not support OpenACC
  and OpenMP in the same source.

For descriptions of all OpenACC-related and OpenMP-related
command-line options, run Clacc's `clang -help` and search for
`openacc` or `openmp`.

Documentation
=============

The following documentation is maintained in the Clacc git repo:

* `README.md` is this file.
* `clang/README-OpenACC-status.md` describes the current status of
  Clacc's support for OpenACC.
* `clang/README-OpenACC-design.md` describes the current design of
  Clacc.
