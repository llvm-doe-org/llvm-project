// Test that the driver produces reasonable linker invocations with -fopenacc.
//
// This test was roughly modeled after ../OpenMP/linking.c.
//
// FIXME: Replace DEFAULT_OPENMP_LIB below with the value chosen at configure time.

// RUN: %data {
// RUN:   (target=i386-pc-linux-gnu   pre=LD)
// RUN:   (target=x86_64-pc-linux-gnu pre=LD)
// RUN:   (target=x86_64-msvc-win32   pre=MSVC-LINK-64)
// RUN: }
// RUN: %for {
// RUN:   %clang -no-canonical-prefixes %s -### -o %t.o 2>&1 \
// RUN:          -fopenacc -target %[target] -rtlib=platform \
// RUN:   | FileCheck -check-prefix=%[pre] %s
// RUN: }

// LD: "{{.*}}ld{{(.exe)?}}"
// LD: "-l[[DEFAULT_OPENMP_LIB:[^"]*]]"
// LD: "-lpthread" "-lc"

// MSVC-LINK-64:      link.exe
// MSVC-LINK-64-SAME: -nodefaultlib:vcomp.lib
// MSVC-LINK-64-SAME: -nodefaultlib:vcompd.lib
// MSVC-LINK-64-SAME: -libpath:{{.+}}/../lib
// MSVC-LINK-64-SAME: -defaultlib:libomp.lib
