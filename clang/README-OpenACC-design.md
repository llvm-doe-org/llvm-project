This document discusses the design of clacc's compiler front end,
which will be based on LLVM's clang.

Overview
========

Clacc is a new project that extends clang (the LLVM C/C++ compiler
frontend) with OpenACC support.  Clacc's approach is to translate
OpenACC (a descriptive language) to OpenMP (a prescriptive language),
which is already supported by clang.  An important objective of clacc
is to investigate potential mappings from OpenACC constructs to OpenMP
constructs and how such mappings might be controlled by system
administrators, application programmers, or automated performance
analyses.

Terminology
===========

The following terminology is used in this document:

* acc = OpenACC
* AST = abstract syntax tree
* LLVM IR = LLVM intermediate representation
* node = an AST node
* omp = OpenMP
* rt = runtime
* src = C source code
* tree = synonymous with AST
* TU = translation unit

High-Level Compiler Design
==========================

Design Alternatives
-------------------

We have considered several design alternatives for the clacc compiler:

```
1. acc src  --parser-->                     omp AST  --codegen-->  LLVM IR + omp rt calls
2. acc src  --parser-->  acc AST                     --codegen-->  LLVM IR + omp rt calls
3. acc src  --parser-->  acc AST  --ttx-->  omp AST  --codegen-->  LLVM IR + omp rt calls
```

In the above diagram:

* **acc src** = C source code containing acc constructs.
* **acc AST** = a clang AST in which acc constructs are represented by
  nodes with acc node types.  Of course, such node types do not
  already exist in clang's implementation.
* **omp AST** = a clang AST in which acc constructs have been lowered
  to omp constructs represented by nodes with omp node types.  Of
  course, such node types *do* already exist in clang's
  implementation.
* **parser** = the existing clang parser and semantic analyzer,
  extended to handle acc constructs.
* **codegen** = the existing clang backend that translates a clang AST
  to LLVM IR, extended if necessary (depending on which design
  is chosen) to perform codegen from acc nodes.
* **ttx** (tree transformer) = a new clang component that transforms
  acc to omp in clang ASTs.

Design Features
---------------

There are several features to consider when choosing among the designs
in the previous section:

1. **acc AST as an artifact** -- Because they create acc AST nodes,
   **designs 2 and 3** best facilitate the creation of additional acc
   source-level tools (such as pretty printers, analyzers, lint-like
   tools, and editor extensions).  Some of these tools, such as pretty
   printing, would be available immediately or as minor extensions of
   tools that already exist in clang's ecosystem.

2. **omp AST/source as an artifact** -- Because they create omp AST
   nodes, **designs 1 and 3** best facilitate the use of source-level
   tools to help an application developer discover how clacc has
   mapped his acc to omp, possibly in order to debug a mapping
   specification he has supplied.  With design 2 instead, an
   application developer has to examine low-level LLVM IR + omp rt
   calls.  Moreover, with designs 1 and 3, permanently migrating an
   application's acc source to omp source can be automated.

3. **omp AST for mapping implementation** -- **Designs 1 and 3** might
   also make it easier for the compiler developer to reason about and
   implement mappings from acc to omp.  That is, because acc and omp
   syntax is so similar, implementing the translation at the level of
   a syntactic representation is probably easier than translating to
   LLVM IR.

4. **omp AST for codegen** -- **Designs 1 and 3** simplify the
   compiler implementation by enabling reuse of clang's existing omp
   support for codegen.  In contrast, design 2 requires at least some
   extensions to clang codegen to support acc nodes.

5. **Full acc AST for mapping** -- **Designs 2 and 3** potentially
   enable the compiler to analyze the entire source (as opposed to
   just the acc construct currently being parsed) while choosing the
   mapping to omp.  It is not clear if this feature will prove useful,
   but it might enable more optimizations and compiler research
   opportunities.

6. **No acc node classes** -- **Design 1** simplifies the compiler
   implementation by eliminating the need to implement many acc node
   classes.  While we have so far found that implementing these
   classes is mostly mechanical, it does take a non-trivial amount of
   time.

7. **No omp mapping** -- **Design 2** does not require acc to be
   mapped to omp.  That is, it is conceivable that, for some acc
   constructs, there will prove to be no omp syntax to capture the
   semantics we wish to implement.  It is also conceivable that we
   might one day want to represent some acc constructs directly as
   extensions to LLVM IR, where some acc analyses or optimizations
   might be more feasible to implement.  This possibility dovetails
   with recent discussions in the LLVM community about developing LLVM
   IR extensions for various parallel programming models.

Because of features 4 and 6, design 1 is likely the fastest design to
implement, at least at first while we focus on simple acc features and
simple mappings to omp.  However, we have so far found no advantage
that design 1 has but that design 3 does not have except for feature
6, which we see as the least important of the above features in the
long term.

The only advantage we have found that design 2 has but that design 3
does not have is feature 7.  It should be possible to choose design 3
as the default but, for certain acc constructs or scenarios where
feature 7 proves important (if any), incorporate design 2.  In other
words, if we decide not to map a particular acc construct to any omp
construct, ttx would leave it alone, and we would extend codegen to
handle it directly.

Conclusions
-----------

For the above reasons, and because design 3 offers the cleanest
separation of concerns, **we have chosen design 3 with the possibility
of incorporating design 2 where it proves useful**.

Traversing acc vs. omp Nodes
----------------------------

Design 3 enables both features 1 and 2.  For this reason, clang tools
and built-in source-level functionality need some way to choose
whether to examine acc nodes or the omp nodes to which they were
translated.  We are considering several mechanisms to make this
possible:

* `-ast-print` is an existing clang command-line option for printing
  the original source code (after C pre-processing) from the clang
  AST.  To enable source-to-source translation from the clang command
  line, we are implementing a new clang command-line option named
  `-fopenacc-print`.  It takes any of the following values:

    * `acc`: acc nodes are printed and omp nodes to which they were
      translated are ignored.  This is the default as most users
      likely expect `-ast-print` to print the original source code
      without regard to how it is implemented internally by the
      compiler.
    * `omp`: omp nodes are printed and the acc nodes from which they
      were translated are ignored.
    * `acc-omp`: acc nodes are printed and the omp nodes to which they
      were translated are printed in neighboring comments.
    * `omp-acc`: omp nodes are printed and the acc nodes to which they
      were translated are printed in neighboring comments.

* AST traversals (using clang's `RecursiveASTVisitor`) visit acc nodes
  but skip the omp nodes to which they are translated.  Like most
  `-ast-print` users, most AST traversal developers and users likely
  expect for traversals to visit an AST representing the original
  source code.  However, while visiting an acc node, a visitor can be
  written to call `getOMPNode` to access the omp node.

* Any tool that wishes to operate on only the omp AST to which the acc
  AST is translated can pass `-ast-print -fopenacc-print=omp` to clang
  and then run clang again to parse the output.  However, in such
  cases, it might be more efficient and convenient to have a mechanism
  that can adjust all AST traversals (by adjusting
  `RecursiveASTVisitor`) to skip acc nodes and visit only omp nodes.
  It would be especially convenient if existing tools require no
  modification (other than perhaps an update to the latest clang) to
  enable this mechanism.  More exploration will be necessary to
  determine if such a mechanism is feasible (we have not yet
  identified a simple way to pass clang command-line options to the
  base `RecursiveASTVisitor` implementation, so an environment
  variable might be required) and worthwhile (we do not have a list of
  specific use cases yet).  If we implement such a mechanism as a
  command-line option, it might be best to merge it with
  `-fopenacc-print` but renamed to something more general like
  `-fopenacc-ast`.

* Such mechanisms would not affect codegen: design 3 requires that
  codegen operate on omp nodes, and design 2 requires that codegen
  operate on acc nodes.

* Such mechanisms would also likely not affect clang's command-line
  options `-ast-dump` and `-ast-view`, which are meant for debugging
  and thus should probably always faithfully render the actual
  internal AST structure: the omp node is shown as a component of the
  acc node or not shown at all.

* Of course, any omp nodes that are not associated with acc nodes
  (because they originate from original source-level omp directives)
  are irrelevant to our design and thus would not be affected by any
  of the above mechanisms.

ttx Design
==========

Background
----------

A key issue in transforming acc to omp in clang ASTs is that clang
ASTs are designed to be immutable once constructed.  This issue might
at first seem to make our ttx component impossible to implement, but
it does not.  Here are a few AST transformation techniques that are
already in use in clang:

1. Clang has a `Rewrite` facility for annotating an AST with textual
   modifications and printing the resulting source code, which could
   then be reparsed as new source code into a new AST.

2. The initial construction of a clang AST involves attaching new
   nodes to existing nodes, so modifying an AST by extending it is
   permitted by design.

3. A special case of #2 is clang's `TreeTransform` facility, which is
   currently used to transform C++ templates for the sake of
   instantiating them:
    1. **Behavior**: When the parser reaches a template instantiation,
       `TreeTransform` builds a transformed copy of the AST node that
       represents the template, and it inserts the copy into the
       syntactic context of the template instantiation.
    2. **Extensible**: Fortunately, `TreeTransform` is designed to be
       extensible: it is a class template employing CRTP for static
       polymorphism.
    3. **Caveat 1: Transitory semantic data**: In order to build new
       nodes, `TreeTransform` runs many of the same semantic actions
       that the parser normally runs.  Those semantic actions require
       the transitory semantic data (stored in clang's `Sema` object)
       that has been built by the time the parser reaches the
       syntactic context where new nodes are to be inserted, but the
       parser gradually discards some of that semantic metadata as the
       parser progresses on to other syntactic contexts.  Thus,
       `TreeTransform` cannot be run on arbitrary nodes in the AST at
       arbitrary times.  For example, to run `TreeTransform` on
       arbitrary nodes in a TU after the parse of that TU has
       completed, it might be necessary to transform the TU's entire
       AST in order to rebuild all of the necessary transitory
       semantic metadata.
    4. **Caveat 2: Permanent semantic data**: Currently, the original
       parse permanently associates semantic data with C++ template
       nodes in a way that's compatible with later runs of
       `TreeTransform` for C++ template instantiation.  However,
       there's no guarantee that semantic data permanently associated
       with any arbitrary node will be compatible with any arbitrary
       extension of `TreeTransform`.  For example, we have already
       noticed that, if we write a simple `TreeTransform` extension
       that merely duplicates an OpenMP region immediately after that
       region's node is constructed, the default `TreeTransform`
       implementation does not update the declaration contexts for
       variable declarations that are local to the duplicate region
       (see `TreeTransform::TransformDecl`), so those duplicate
       variables appear to be declared in the original region,
       resulting in spurious compiler diagnostics.  It appears that
       this particular problem can be overcome by overriding more of
       the `TreeTransform` functionality, but the effort does not
       appear to be trivial even though it seems like boilerplate that
       is necessary for many conceivable extensions of
       `TreeTransform`.
    5. **Caveat 3: Unknown limitations**: Because `TreeTransform` is
       designed primarily for C++ template instantiation, it is not
       clear at this point what other limitations an attempt to extend
       it might expose.

Design Alternatives
-------------------

1. Alternatives for how ttx performs transformations:

    1. **Annotate** acc nodes with omp as text (as in `Rewrite`),
       print the entire AST, and parse it again.  This approach is
       likely bad for compilation performance, and we imagine the
       clang community would see that performance hit as a big
       negative.
    2. **Replace** acc nodes with omp nodes.  While this approach
       seems to directly contradict the clang AST design, at least one
       clang project managed to get away with it for some older
       version of clang (snucl -- Jungwon Kim).  Even if it still
       manages to work somehow, because it contradicts the design, it
       might break as clang evolves, and it probably would not be
       acceptable to the clang community.
    3. For each acc node, **add a second hidden parent**, an omp node,
       for the same children.  Technically, the acc node would point
       to the omp node as if it were a special direct child whose
       children happen to be the acc node's original children.  The
       acc node would delegate codegen, printing, etc. to the omp node
       as necessary, but the omp node would be skipped by traversals
       related to source-level analysis.  One caveat we have already
       noticed when prototyping this approach is that the acc node's
       children constructed during parsing must be compatible with the
       omp node that will be added sometime after the acc construct is
       parsed, so the decision of which omp node to map to cannot
       always be fully deferred until after parsing.  Specifically,
       when trying to translate `acc parallel` to `omp target teams`
       using this approach, we had to construct one captured region
       for each of `omp target` and `omp teams`.
    4. For each acc node, **add a hidden subtree** rooted at an omp
       node.  This approach would likely use `TreeTransform`.  We have
       not prototyped this approach yet, but it seems feasible.
       Similar to the previous alternative, the acc node would point
       to the new subtree as if it were a special direct subtree to
       which it would delegate codegen, printing, etc. as necessary,
       but the subtree would be skipped by traversals related to
       source-level analysis.  This approach overcomes the caveat of
       the previous approach but likely requires the additional effort
       of extending `TreeTransform`.
    5. For each TU, **add a new TU** with omp nodes instead of acc
       nodes.  This approach would likely use `TreeTransform`.

2. Alternatives for when ttx performs transformations:

    1. Immediately after each acc node is constructed.  This approach
       is nonsensical for 1.5 above.
    2. At the end of each TU or all TUs.  For 1.4 above, this approach
       is likely not feasible due to the first `TreeTransform` caveat
       mentioned in the previous section.  If using `TreeTransform`,
       1.5 seems necessary to overcome that caveat.

Conclusions
-----------

For now, **we plan to employ design 1.3 combined with 2.1 where
possible and 2.2 where necessary** for our design of ttx.  These
design alternatives seem:

* **Most compatible**: Unlike 1.2, 1.3 doesn't attempt to violate
  clang AST immutability.
* **Most efficient**: Unlike 1.1 or 1.5, 1.3 doesn't require
  rebuilding the entire AST.
* **Most flexible**: Unlike 1.4 or 1.5, it appears that 1.3 can be
  combined with either 2.1 for simplicity or 2.2 when full TU
  visibility is required for a transformation.

However, if additional rebuilding proves necessary in the subtree of
an acc node, as in the caveat mentioned above for 1.3, **we might
employ 1.4 (with 2.1) or 1.5 (with 2.2)**, depending on whether full
TU visibility is required.

OpenACC to OpenMP Mapping
=========================

This section lists mappings that we have implemented or plan to
implement from OpenACC directives and clauses to OpenMP directives and
clauses.  If a directive or clause does not appear in this section, we
simply haven't planned it yet.

Notation
--------

For clauses and data attributes, we use the following notations:

* *pre* labels a data attribute that is predetermined and not
  specified by an explicit clause.
* *imp* labels a data attribute or clause that is implicitly
  determined and not specified by an explicit clause.
* *exp* labels a clause (possibly specifying a data attribute) that is
  explicitly specified.
* *not* labels a clause that is not explicitly specified.
* Mappings for data attributes and for clauses that are specified per
  variable should be interpreted per variable.
* Mappings for other clauses should be interpreted per directive.
* The notation `lab1 clause1 -> lab2 clause2` specifies that OpenACC
  `clause1` under condition `lab1` maps to OpenMP `clause2` under
  condition `lab2`.
* The notation `lab1|lab2 clause1 -> lab3 clause2` specifies both of
  the following mappings:
    * `lab1 clause1 -> lab3 clause2`
    * `lab2 clause1 -> lab3 clause2`
* The notation `lab1|lab2 clause1 -> lab3|lab4 clause2` specifies both
  of the following mappings:
    * `lab1 clause1 -> lab3 clause2`
    * `lab2 clause1 -> lab4 clause2`

Mappings
--------

For now, we implement a prescriptive interpretation of OpenACC without
loop analysis and thus with only a safe mapping to OpenMP.

* `acc parallel` -> `omp target teams`:
    * Data sharing semantics:
        * It is an error if a variable has more than one of exp
          `firstprivate`, exp `private`, or exp `reduction`.  Notes:
            * These have contradictory specifications for
              initialization of the local copy of the variable.
            * Relative to `firstprivate` and `private`, `reduction`
              has a contradictory specification for storing data back
              to the original variable.
        * exp `firstprivate`, exp `private`, or exp `reduction` for a
          variable of incomplete type is an error:
            * Note that a local copy must be allocated in each of
              these case, but allocation is impossible for incomplete
              types.
        * exp `private` or `reduction` for a `const` variable is an
          error.  Notes:
            * The local copy of a const private variable would remain
              uninitialized throughout its lifetime.
            * A reduction intends to assign to both the original
              variable and a local copy after its initialization, and
              that's impossible if it's const.
            * `firstprivate` is fine for a `const` variable.  The
              local copy will have the original variable's value
              throughout its lifetime.
        * OpenACC's rules to add imp `shared` and imp `firstprivate`
          are ignored for a variable `v` if the following rule
          produces an imp `reduction` for `v` on `acc parallel`.
        * If (1) neither exp `firstprivate` nor exp `private` for a
          variable `v` that is declared outside this `acc parallel`,
          and (2) on any contained gang-partitioned `acc loop`, exp
          `reduction` with a reduction operator `op` for `v`, then
          this `acc parallel` has imp `reduction` with `op` for `v`.
            * Note that we are not applying to an `acc parallel` any
              loop-specified gang reduction if exp `firstprivate` or
              exp `private` for that variable on `acc parallel`.  gcc
              7.2.0 and pgcc 18.4-0 also appear to have this behavior.
              Our justification is simple: where the reduction is
              specified (at `acc loop`), only the local copy of the
              variable is visible, so it's a gang-local variable.
        * It is an error if, on this `acc parallel`, there exist
          multiple imp|exp `reduction` with different reduction
          operators for a single variable `v`.
        * Variable type restrictions for `reduction` are specified in
          README-OpenACC-status.md as that is a highly user-visible
          issue.
    * Data sharing mapping:
        * imp `shared` -> exp `shared`
        * imp|exp `firstprivate` -> exp `firstprivate`
        * exp `private` -> exp `private`
        * imp|exp `reduction` -> exp `reduction`
    * exp `num_gangs` -> exp `num_teams`
    * if exp `num_workers` with a constant-expression argument, and if
      there is a contained worker-partitioned `acc loop`, then exp
      `num_workers` -> wrap the `acc parallel` in a compound statement
      and declare a local const variable with the same type and value
      as the exp `num_workers` argument
    * else, exp `num_workers` is discarded here during translation
        * Note that a constant-expression argument here might be used
          by a nested worker-partitioned `acc loop`.
    * exp `vector_length` is discarded here during translation
        * Notes:
            * Clacc does not support non-constant expressions for
              `vector_length`.  See README-OpenACC-status.md for
              details.
            * The constant expression argument here might be used by a
              nested vector-partitioned `acc loop`.
* `acc loop` within `acc parallel`:
    * if exp `seq`, then:
        * The directive and the following clauses are discarded during
          translation:
            * `seq`, `independent`, `auto`
            * `gang`, `worker`, `vector`
            * `collapse`
        * Data sharing semantics:
            * If the loop control variable is declared in the init of
              the attached `for` loop, the loop control variable is
              pre `private`.
            * If the loop control variable is just assigned instead of
              declared in the init of the attached `for` loop, the
              loop control variable is imp `shared`.
            * For any other variable referenced within the loop but
              declared outside the loop, the variable is imp `shared`.
            * exp `reduction` is not permitted on a loop control
              variable.
            * For exp `firstprivate`, exp `private`, and exp
              `reduction`, the restrictions for more than one of these
              per variable, for variables of incomplete type, and for
              `const` variables are the same as on `acc parallel`.
        * Data sharing mapping:
            * pre `private` and imp `shared` are discarded during
              translation.
            * exp `private` -> wrap associated loop in a compound
              statement and declare an uninitialized local copy of the
              variable.
            * exp `reduction` is discarded during translation.
        * Notes:
            * This is gang-redundant, worker-single, vector-single
              mode.  Thus, as far as partitioning is concerned, a
              simple C `for` loop is sufficient.
            * pre `private` is only for a loop control variable that
              is declared in the init of the attached `for` loop, so a
              private copy is already made for the one thread
              executing the loop.
            * imp `shared` is only for variables referenced within the
              loop but declared outside the loop, and these are
              already shared by the simple C `for` loop.
            * exp `private` just needs to be local to the one thread
              executing the loop, and so creating a new local variable
              is sufficient.
            * There are a few considerations related to the decision
              to drop the `reduction` clause:
                * `reduction` implies a private copy of the variable
                  per thread, but in this case there's only one thread
                  involved in the reduction.  Thus, the only reason to
                  create a private copy would be to avoid races with
                  other threads for other redundant gangs or for
                  partitioned ancestor loops.  However, the final
                  value must be copied back to the original at the end
                  of this loop, so there's a race anyway, so privacy
                  doesn't help.
                * If the specified `reduction` operator is not the
                  same as the operator that the loop body effectively
                  performs on the variable, then the result is
                  affected by dropping the clause.  However, in the
                  case of non-sequential loops for which we don't drop
                  the clause, such a discrepancy means that the way in
                  which the compiler or runtime choose to partition
                  the loop also affects the result.  Thus, in general,
                  the app developer shouldn't create such `reduction`
                  discrepancies, even though the OpenACC 2.6 spec
                  doesn't appear to say so.  For example, assume we
                  have `x=2` initially, `reduction(*:x)`, loop has 2
                  iterations, and loop body performs `++x`.  The final
                  `x` is 4, 6, or 8 when dropping the reduction,
                  partitioning with 1 thread (sequential execution),
                  or partitioning with 2 threads.
                * If we have such a `reduction` discrepancy, the
                  result value from my experiments with pgcc 18.4-0
                  and gcc 7.2.0 seem to indicate that they drop the
                  `reduction` clause in the case of sequential loops
                  except pgcc when targeting multicore.  We choose to
                  go with the majority here and just drop it.
            * The decision not to permit `reduction` on a loop control
              variable is for consistency with the non-sequential loop
              case, for which a `reduction` isn't discarded.
            * We considered mapping `acc loop seq` to various OpenMP
              directives so that `private` and `reduction` clauses
              could simply be translated to OpenMP clauses.  To
              understand the desired properties of the chosen mapping
              described above, it's important to understand why each
              of those considered mappings failed to behave correctly:
                * `omp parallel for num_threads(1)` does not behave as
                  a sequential loop in at least two ways:
                    * It predetermines the loop control variable to be
                      private, but that's not how a sequential loop
                      treats a loop control variable that is assigned
                      but not declared in the init of the attached
                      `for` loop.
                    * When the loop control variable is modified in
                      the body of the loop, behavior is not defined
                      because the init, cond, and incr expressions
                      alone must determine the number of iterations.
                      In my experiments, the current clang OpenMP
                      implementation observes only those expressions
                      and ignores even the simplest modification in
                      the body.  OpenACC 2.6 sec. 2.9 lines 1357-1358
                      describes the related OpenACC requirement, which
                      does not apply in the case of `seq`, so the
                      mapping shouldn't impose this restriction.
                * `omp parallel num_threads(1)` (drops the `for` from
                  the above directive) does not have the above
                  problems.  However, for either directive, `acc loop
                  seq` cannot be nested outside `acc loop gang` or
                  inside `acc loop vector` because `omp parallel`
                  cannot be nested outside `omp distribute` or inside
                  `omp simd`.
    * else if exp `auto`, then same mapping as for exp `seq`.
    * else, exp|imp `independent`, so:
        * Note that compilers are permitted to apply `gang`, `worker`,
          or `vector` to loops where they were not specified.  (See
          the 2018/06/20 thread on technical@openacc.org for how to
          interpret the spec on this point.)  This requires loop
          analysis, so we will try that after getting this initial
          safe mapping implemented.
        * if not `gang`, not `worker`, and not `vector`, then same
          mapping as for exp `seq`
        * else, -> `omp`:
            * exp `gang` -> exp `distribute`
                * This is currently the only case of a
                  gang-partitioned `acc loop`.
            * exp `worker` -> exp `parallel for`
                * This is currently the only case of a
                  worker-partitioned `acc loop`.
            * if, neither this nor any ancestor `acc loop` is
              gang-partitioned or worker-partitioned, then -> exp
              `parallel for` and exp `num_threads(1)`.  Notes:
                * We add `parallel for` for this case because OpenMP
                  does not permit `omp simd` directly inside `omp
                  target teams`.  An alternative might be to translate
                  to `omp simd` directly inside `omp parallel`, but
                  OpenMP does not have a combined `omp parallel simd`
                  directive, leading us to question the semantics.
                * We are assuming that, if worker parallelism is not
                  specified but vector parallelism is, then vector
                  operations within a single worker are the intention.
            * exp `vector` -> exp `simd`
                * This is currently the only case of a
                  vector-partitioned `acc loop`.
            * The output `distribute`, `parallel for`, and `simd`
              OpenMP directive components are sorted in the above
              order before all clauses (including the above
              `num_threads(1)`) regardless of the input OpenACC clause
              order.
            * if exp `worker`, then exp `num_workers` from ancestor
              `acc parallel` -> exp `num_threads` where the argument
              is either:
                * the original exp `num_workers` argument if it is a
                  constant expression
                * an expression containing only a reference to the
                  local const variable generated for that exp
                  `num_workers`
                    * Note that on the ancestor `acc parallel` and on
                      all OpenACC directives nested between it and
                      this `acc loop`, the OpenMP data sharing
                      attribute for the local const variable for
                      `num_workers` is implicit.  Because the variable
                      is const, private copies are not useful, so
                      sharing is probably most efficient, but not all
                      OpenMP directives permit an exp `shared` clause.
                      Thus, relying on the implicit data sharing
                      attribute throughout simplifies the
                      implementation.  (For variables appearing in the
                      original OpenACC source, we make a greater
                      effort to be explicit about data sharing.)
            * if exp `vector`, then exp `vector_length` from ancestor
              `acc parallel` -> exp `simdlen` (with the exp
              `vector_length` constant-expression argument)
            * `collapse` -> `collapse`
            * Data sharing semantics:
                * Whether just assigned or declared in the init of the
                  attached `for` loop, the loop control variable is:
                    * pre `private`
                    * Notes:
                        * This is dictated by OpenACC 2.6 sec. 2.6.1.
                        * This choice is consistent with OpenMP 4.5's
                          choice for `distribute` (`gang`) and
                          `parallel for` (`worker`) (sec. 2.15.1.1
                          p. 179 lines 24-25).
                        * This choice is not consistent with OpenMP
                          4.5's choice for `simd` (`vector`)
                          (sec. 2.15.1.1 p. 179 lines 26-27), which
                          specifies pre `linear` instead.
                        * OpenMP 4.5 (sec. 2.15.1.1 p. 181 lines 1-2)
                          also permits exp `lastprivate` in the
                          `distribute` and `parallel for` cases.
                          (Unfortunately, OpenACC currently has no exp
                          `lastprivate`.  Maybe one day.)
                        * The advantage of `lastprivate` here would be
                          that it achieves behavior closer to that of
                          (1) the sequential loop and (2) the case of
                          `simd` (`linear` has `lastprivate`-like
                          semantics).  It might also mimic behavior
                          from the PGI OpenACC compiler.
                        * The disadvantage of `lastprivate` here is
                          that, with gang partitioning, `lastprivate`
                          can cause a data race on accesses to the
                          variable, as described in OpenMP 4.5
                          sec. 2.10.8 p. 118 lines 21-24 and
                          sec. 2.15.3.5 p. 199 lines 13-16.  `linear`
                          has the same problem due to its
                          `lastprivate`-like behavior.  `private` does
                          not.
                        * The potential for that data race doesn't end
                          until the end of the enclosing `acc
                          parallel` construct, so `lastprivate` isn't
                          really useful if the variable is private to
                          the enclosing `acc parallel` construct.
                          Indeed, gcc 7.2.0 compiling the generated
                          OpenMP complains about `lastprivate` in that
                          case.  As of this writing, clang master does
                          not complain.
                        * `shared` is not permitted and is nonsensical
                          on a partitioned loop's control variable.
                * For any other variable referenced within the loop
                  but declared outside the loop, the variable is:
                    * pre|imp `shared`
                    * Notes:
                        * We make this choice for consistency with
                          OpenMP 4.5's choice (sec. 2.15.1.1 p. 180
                          line 16 and p. 182 lines 1-4, but I'm not
                          quite clear on why lines 1-4 don't just say
                          shared).
                        * In comparison to the loop control variable,
                          these variables are (1) simpler in that
                          `linear` is not dictated by OpenMP for the
                          case of exp `simd` and (2) more complex in
                          that, for gang partitioning, like the data
                          race discussed above for outgoing values
                          (when `shared` or `lastprivate`), there is
                          also a potential data race for incoming
                          values (when `shared` or `firstprivate`).
                          With `firstprivate`, both clang (as of this
                          writing) and gcc 7.2.0 complain when the
                          variable is private to the `acc parallel`
                          (that is, `omp target teams`).
                * exp `reduction` is not permitted on a loop control
                  variable.  Notes:
                    * We have not identified anything in the OpenACC
                      2.6 specification that requires this constraint.
                      However, it's hard to imagine what the effect of
                      a reduction on a loop control variable should
                      be.  For example, given one thread per iteration
                      and a `+` reduction operator, the local copy
                      should always have initial value 1 when treated
                      as a reduction variable, but its value should
                      vary across iterations when treated as a loop
                      control variable.
                    * Clang's OpenMP implementation also enforces this
                      constraint.  That makes sense by the OpenMP spec
                      because an OpenMP `reduction` is a data sharing
                      attribute and a loop control variable has a
                      different predetermined data sharing attribute.
                    * For OpenACC, gcc 7.2.0 also enforces this
                      constraint, but pgcc 18.4-0 does not enforce it.
                * For exp `firstprivate`, exp `private`, and exp
                  `reduction`, the restrictions for more than one of
                  these per variable, for variables of incomplete
                  type, and for `const` variables are the same as on
                  `acc parallel`.
            * Data sharing mapping:
                * if exp `worker` or, this and every ancestor `acc
                  loop` until the ancestor `acc parallel` is not
                  gang-partitioned and not worker-partitioned, then
                  pre|imp `shared` -> exp `shared`
                * else, pre|imp `shared` -> imp `shared`
                    * Note that OpenMP `distribute` or `simd` without
                      `parallel for` (which we add for `worker` or to
                      be able to nest `simd` directly within `target
                      teams`) does not support a `shared` clause, so
                      we must rely on OpenMP implicit data sharing
                      rules then.
                * pre `private` for a loop control variable that is
                  declared in the init of the attached `for` loop ->
                  pre `private`
                    * Note that exp `private` (in OpenACC or OpenMP)
                      is impossible for such a variable because such
                      an exp `private` would indicate a variable from
                      the enclosing scope (or be an error if none).
                * if exp `vector`, then pre|exp `private` for a loop
                  control variable that is just assigned instead of
                  declared in the init of the attached `for` loop:
                    * -> pre `linear`
                    * Wrap loop in a compound statement and declare an
                      uninitialized local copy of the loop control
                      variable.
                    * Notes:
                        * For `simd`, OpenMP 4.5 specifies pre
                          `linear` here (sec. 2.15.1.1 p. 179 lines
                          26-27), so we cannot translate to exp
                          `private`.
                        * We don't attempt to translate to exp
                          `linear` because (1) the OpenMP spec says
                          the step must be the increment from the
                          attached loop, (2) the OpenMP spec says the
                          default step for an exp `linear` is 1, and
                          (3) we don't want to have to implement
                          extracting the increment from the attached
                          loop when we can just rely on the behavior
                          of pre `linear` (and thus on clang's or some
                          other target compiler's OpenMP
                          implementation to extract it for us).
                * in all other cases, pre|exp `private` -> exp
                  `private`
                * if exp `worker` or exp `vector`, then exp
                  `reduction` -> exp `reduction`
                * else, exp `reduction` is discarded here during
                  translation.  Notes:
                    * A gang reduction for a gang-local variable is
                      useless and so is discarded during translation.
                    * Gang reductions for other variables are
                      addressed in the data sharing semantics on `acc
                      parallel`.
* Combined directives:
    * The translation of combined directives is performed in two
      stages:
        * Translation from the combined OpenACC directive to separate
          OpenACC directives: This stage is performed during the parse
          by sema.  Sema builds the effective AST subtree containing
          the separate OpenACC directives, and then it stores a
          pointer to the AST node for the outermost of those
          directives as a member (not a normal AST child) of the
          combined directive's AST node.
        * Translation from the separate OpenACC directives to OpenMP:
          This stage is performed using the `TreeTransform` facility
          just as it normally would be for the separate directives.
    * Mappings from the combined OpenACC directives to the separate
      OpenACC directives are as follows:
        * `acc parallel loop` -> `acc parallel`, whose associated
          statement is an `acc loop`, whose associated statement is
          the associated statement from the `acc parallel loop`:
            * `private` -> `private` on the `acc loop`
            * `reduction` -> `reduction` on both the `acc parallel`
              and `acc loop`
            * Each remaining clause is permitted on only one of the
              separate OpenACC directives, and so it is mapped to that
              directive.

Unmappable Features
-------------------

It might prove to be impossible to map some OpenACC features to
standard OpenMP.  For such features, our plan is to map to OpenMP
language or runtime extensions, which we will implement as necessary.

When source-to-source translation (as opposed to normal full
compilation) is enabled, we will implement diagnostics to identify
uses of such features unless those diagnostics are inexact and prove
to have too many false positives.  In the worst cases, we would
identify such features at run time.

The following is a list of OpenACC features we have identified that
might not be possible to map to OpenMP, but we are still investigating
possible solutions:

* `vector_length` with a non-constant expression argument because
  `simdlen`, to which `vector_length` is translated, requires a
  constant expression.
* If a variable is gang-shared within an `acc parallel` due to, for
  example, a `copy` clause there, and if that variable is also
  involved in a gang reduction specified on a nested `acc loop`, that
  variable becomes gang-private throughout that `acc parallel` when
  translated to OpenMP, where the reduction must be specified at the
  level of the `acc parallel` (`omp target teams`):
    * This discrepancy is mostly relevant to accesses to that variable
      within that `acc parallel` before that `acc loop`.  Within that
      `acc parallel` after that `acc loop`, it's probably not
      reasonable to access the variable in either version because the
      reduction doesn't happen until the end of the `acc parallel`.
    * It's possible we could fix this by generating some other
      gang-reduced variable that replaces the original variable within
      the `acc loop gang`, but the original gang-shared variable
      remains elsewhere in the `acc parallel`.  The gang-reduced
      variable would be initialized as all its private copies are
      initialized (e.g., 1 in the case of `*`), and we would need some
      way to combine (just one multiply in the case of `*`) its
      reduced value back into the shared copy.  To declare the
      gang-reduced variable and combine it back into the gang-shared
      variable, perhaps we'd break `omp target teams` into two
      directives and add the additional code outside the `omp teams`.
    * Anyway, to get this right, we first have to understand exactly
      what OpenACC says about how a gang-shared variable interacts
      with multiple `acc loop` directives that have gang-reductions
      for that variable within a single `acc parallel`.  Does it
      correspond to the above behavior?  Experimenting with pgcc
      18.4-0, it doesn't seem to.  Actually, with -ta:tesla, it seems
      to do exactly what clacc does now when there's no `copy` clause
      specified: there's a gang reduction implied on `acc parallel`,
      and the variable is gang-private within.
    * Because we have not yet implemented explicit or implicit `copy`
      clauses, and because OpenACC 2.6 only permits reductions for
      scalars, such a reduction variable will be `firstprivate` or
      `private` and thus gang-private, so this issue doesn't affect us
      yet.
* A gang reduction specified on an orphaned `acc loop` directive
  because the enclosing compute construct to which the reduction would
  normally be applied during translation is not statically visible.
* `acc loop` directive that observes `num_workers` and `vector_length`
  because the enclosing compute construct from which those clauses
  would normally be applied during translation is not statically
  visible.
* Multiple reference counters because OpenMP has just one reference
  counter.
