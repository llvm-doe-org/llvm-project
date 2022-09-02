// Test that the driver produces reasonable linker invocations with -fopenacc.
//
// This test was roughly modeled after ../OpenMP/linking.c.
//
// FIXME: Replace DEFAULT_OPENMP_LIB below with the value chosen at configure time.

// DEFINE: %{check}( TARGET %, FC_PRE %) =                                     \
// DEFINE:   %clang -no-canonical-prefixes %s -### -o %t.o                     \
// DEFINE:     -fopenacc -target %{TARGET} -rtlib=platform > %t.out 2>&1 &&    \
// DEFINE:   FileCheck -check-prefix=%{FC_PRE} -input-file=%t.out %s

// RUN: %{check}( i386-pc-linux-gnu   %, LD           %)
// RUN: %{check}( x86_64-pc-linux-gnu %, LD           %)
// RUN: %{check}( x86_64-msvc-win32   %, MSVC-LINK-64 %)

// LD: "{{.*}}ld{{(.exe)?}}"
// LD: "-l[[DEFAULT_OPENMP_LIB:[^"]*]]"
// LD: "-lpthread" "-lc"

// MSVC-LINK-64:      link.exe
// MSVC-LINK-64-SAME: -nodefaultlib:vcomp.lib
// MSVC-LINK-64-SAME: -nodefaultlib:vcompd.lib
// MSVC-LINK-64-SAME: -libpath:{{.+}}/../lib
// MSVC-LINK-64-SAME: -defaultlib:libomp.lib
