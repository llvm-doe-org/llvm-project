# RUN: not %{lit} -j 1 -vv %{inputs}/shtest-pdata-pfor > %t.out
# RUN: FileCheck --input-file %t.out %s
# END.


# CHECK: Testing: 101 tests


# Test errors within %data but not within iteration
# -------------------------------------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-eof.txt
# CHECK: Test has unclosed %data 'foo', opened on line 2


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err0-nested-pdata.txt
# CHECK:      ValueError: in %data 'foo' opened on line 1, found nested %data
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err0-nested-pfor.txt
# CHECK:      ValueError: in %data 'foo' opened on line 1, found nested %for
# CHECK-NEXT: in RUN: directive on test line 3


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err1-itr1-no-lparen.txt
# CHECK:      ValueError: in %data 'foo', expected '(' to open first iteration
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err1-itr2-no-lparen.txt
# CHECK:      ValueError: in %data 'foo', expected '}' to close %data or '(' to open iteration 2
# CHECK-NEXT: in RUN: directive on test line 3


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err2-itr0-no-eol.txt
# CHECK:      ValueError: after %data 'foo' closing '}', expected end of line
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err2-itr1-no-eol.txt
# CHECK:      ValueError: after %data 'foo' closing '}', expected end of line
# CHECK-NEXT: in RUN: directive on test line 3


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-err3-no-iterations.txt
# CHECK:      ValueError: %data 'foo' has no iterations
# CHECK-NEXT: in RUN: directive on test line 2


# Test errors within %data iteration but not within value
# -------------------------------------------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-eof.txt
# CHECK: Test has unclosed %data 'foo', opened on line 6


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err0-itr1-nested-pdata.txt
# CHECK:      ValueError: in %data 'foo' iteration 1 opened on line 2, found nested %data
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err0-itr1-nested-pfor.txt
# CHECK:      ValueError: in %data 'foo' iteration 1 opened on line 2, found nested %for
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err0-itr3-nested-pdata.txt
# CHECK:      ValueError: in %data 'foo' iteration 3 opened on line 4, found nested %data
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err1-field1-id-equals.txt
# CHECK:       ValueError: in %data 'foo' iteration 2, expected first field identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err1-field1-id-lparen.txt
# CHECK:       ValueError: in %data 'foo' iteration 1, expected first field identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err1-field1-id-percent.txt
# CHECK:       ValueError: in %data 'foo' iteration 2, expected first field identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err1-field2-id-hash.txt
# CHECK:       ValueError: in %data 'foo' iteration 1, expected either ')' to close iteration or field 2 identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err1-field3-id-equals.txt
# CHECK:       ValueError: in %data 'foo' iteration 2, expected either ')' to close iteration or field 3 identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err1-field3-id-lparen.txt
# CHECK:       ValueError: in %data 'foo' iteration 1, expected either ')' to close iteration or field 3 identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 3


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err2-immediate-rbrace.txt
# CHECK:      ValueError: after %data 'foo' iteration 2 closing ')', expected end of line
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err2-itr1-no-eol.txt
# CHECK:      ValueError: after %data 'foo' iteration 1 closing ')', expected end of line
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err2-itr2-no-eol.txt
# CHECK:      ValueError: after %data 'foo' iteration 2 closing ')', expected end of line
# CHECK-NEXT: in RUN: directive on test line 4


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err3-itr1-no-fields.txt
# CHECK:      ValueError: %data 'foo' iteration 1 has no fields
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err3-itr3-no-fields.txt
# CHECK:      ValueError: %data 'foo' iteration 3 has no fields
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err3-missing-field1.txt
# CHECK:      ValueError: %data 'foo' iteration 2 is missing the following fields, which are in previous iterations: 'c'
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err3-missing-field2.txt
# CHECK:      ValueError: %data 'foo' iteration 3 is missing the following fields, which are in previous iterations: 'b'
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err3-missing-fields.txt
# CHECK:      ValueError: %data 'foo' iteration 5 is missing the following fields, which are in previous iterations: 'def', 'jkl', 'mno'
# CHECK-NEXT: in RUN: directive on test line 6


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err4-dollar-not-assign.txt
# CHECK:       ValueError: after %data 'bar' iteration 2 field identifier 'c', expected '='
# CHECK-NEXT: in RUN: directive on test line 8

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err4-eol-not-assign.txt
# CHECK:       ValueError: after %data 'bar' iteration 2 field identifier 'b', expected '='
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err4-rparen-not-assign.txt
# CHECK:       ValueError: after %data 'bar' iteration 1 field identifier 'b3', expected '='
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err4-space-not-assign.txt
# CHECK:       ValueError: after %data 'foo.bar' iteration 6 field identifier 'b', expected '='
# CHECK-NEXT: in RUN: directive on test line 7


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err5-extra-field1.txt
# CHECK:      ValueError: %data 'foo' iteration 2 has field 'd', which does not exist in previous iterations
# CHECK-NEXT: in RUN: directive on test line 3

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err5-extra-field2.txt
# CHECK:      ValueError: %data 'foo' iteration 3 has field 'b', which does not exist in previous iterations
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err5-extra-fields.txt
# CHECK:      ValueError: %data 'foo' iteration 5 has field 'mno', which does not exist in previous iterations
# CHECK-NEXT: in RUN: directive on test line 6

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err5-extra-missing-fields.txt
# CHECK:      ValueError: %data 'foo' iteration 5 has field 'mno', which does not exist in previous iterations
# CHECK-NEXT: in RUN: directive on test line 6


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err6-field-first-second.txt
# CHECK:      ValueError: %data '' iteration 1 field 'a' defined multiple times
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-itr-err6-field-multiple-later.txt
# CHECK:      ValueError: %data '' iteration 2 field 'b' defined multiple times
# CHECK-NEXT: in RUN: directive on test line 3


# Test errors within %data iteration but not within value
# -------------------------------------------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err0-eq-bs-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 4 field 'ghi' value, unexpected end of line after backslash
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err0-eq-chars-bs-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 6 field 'stu' value, unexpected end of line after backslash
# CHECK-NEXT: in RUN: directive on test line 7


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err1-eq-chars-sq-chars-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 1 field 'a' value, unexpected end of line within single-quotes: '() 	"\
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err1-eq-chars-sq-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 3 field 'b' value, unexpected end of line within single-quotes: '
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err1-eq-sq-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 2 field 'a' value, unexpected end of line within single-quotes: '
# CHECK-NEXT: in RUN: directive on test line 3


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err2-eq-chars-dq-chars-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 1 field 'a' value, unexpected end of line within double-quotes: "() 	'\"\\
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err2-eq-chars-dq-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 3 field 'b' value, unexpected end of line within double-quotes: "
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err2-eq-dq-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 2 field 'a' value, unexpected end of line within double-quotes: "
# CHECK-NEXT: in RUN: directive on test line 3


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err3-dq-bs-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 1 field 'b' value, unexpected end of line after backslash
# CHECK-NEXT: in RUN: directive on test line 2

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pdata-value-err3-dq-chars-bs-eol.txt
# CHECK:      ValueError: in %data 'foo' iteration 2 field 'b' value, unexpected end of line after backslash
# CHECK-NEXT: in RUN: directive on test line 3


# Test errors within %for
# -----------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-eof.txt
# CHECK: Test has unclosed %for 'foo', opened on line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err0-nested-pdata.txt
# CHECK:      ValueError: in %for 'foo' opened on line 6, found nested %data
# CHECK-NEXT: in RUN: directive on test line 7


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err1-empty-no-eol.txt
# CHECK:      ValueError: after %for 'foo' closing '}', expected end of line
# CHECK-NEXT: in RUN: directive on test line 6

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err1-no-eol.txt
# CHECK:      ValueError: after %for 'foo' closing '}', expected end of line
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err2-no-run-lines.txt
# CHECK:      ValueError: %for 'foo' has no run lines
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err3-first-unterm.txt
# CHECK:      ValueError: %for 'foo' has unterminated run lines (with '\')
# CHECK-NEXT: in RUN: directive on test line 6

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err3-later-unterm.txt
# CHECK:      ValueError: %for 'foo' has unterminated run lines (with '\')
# CHECK-NEXT: in RUN: directive on test line 9


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err4-undefined-field-brackets.txt
# CHECK:      ValueError: in command pipeline starting on line 5, %[c] is undefined
# CHECK-NEXT: in RUN: directive on test line 6

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err4-undefined-field-sq.txt
# CHECK:      ValueError: in command pipeline starting on line 7, %'d' is undefined
# CHECK-NEXT: in RUN: directive on test line 10

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err4-undefined-nested.txt
# CHECK:      ValueError: in command pipeline starting on line 18, %[e] is undefined
# CHECK-NEXT: in RUN: directive on test line 21


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-pfor-err5-ambiguous.txt
# CHECK:      ValueError: in command pipeline starting on line 24, %[a] is ambiguous
# CHECK-NEXT: defined in %data '2' opened on line 9
# CHECK-NEXT: defined in %data '0' opened on line 1
# CHECK-NEXT: in RUN: directive on test line 27


# Test errors outside %data or %for
# ---------------------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-run-err0-undefined-field-brackets.txt
# CHECK:      ValueError: outside %for, %[a] is undefined
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: in-run-err0-undefined-field-sq.txt
# CHECK:      ValueError: outside %for, %'xyz' is undefined
# CHECK-NEXT: in RUN: directive on test line 1


# Test errors in %data opening line
# ---------------------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err0-extra-chars.txt
# CHECK:      ValueError: in line containing '%data', '%data' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err0-no-percent.txt
# CHECK:      ValueError: in line containing '%data', '%data' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err0-no-post-space.txt
# CHECK:      ValueError: in line containing '%data', '%data' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err0-pdata-in-cmd.txt
# CHECK:      ValueError: in line containing '%data', '%data' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err0-pre-pdata.txt
# CHECK:      ValueError: in line containing '%data', '%data' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 1


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err1-id-bad-char.txt
# CHECK:      ValueError: after '%data', expected opening '{' or identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err1-no-pre-space.txt
# CHECK:      ValueError: after '%data', expected opening '{' or identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 1


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err2-id-bad-char0.txt
# CHECK:      ValueError: after '%data f', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err2-id-bad-char1.txt
# CHECK:      ValueError: after '%data ba', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err2-no-rbrace.txt
# CHECK:      ValueError: after '%data foo', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err2-two-ids.txt
# CHECK:      ValueError: after '%data foo', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 1


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err3-all-one-line.txt
# CHECK:      ValueError: after %data 'foo' opening '{', expected end of line
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err3-empty-all-one-line.txt
# CHECK:      ValueError: after %data 'foo' opening '{', expected end of line
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err3-nested-pdata.txt
# CHECK:      ValueError: after %data 'foo' opening '{', expected end of line
# CHECK-NEXT: in RUN: directive on test line 1


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err4-earlier-line-unterm.txt
# CHECK:      ValueError: %data 'foo' opened after unterminated run line
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err4-prev-line-unterm.txt
# CHECK:      ValueError: %data 'foo' opened after unterminated run line
# CHECK-NEXT: in RUN: directive on test line 2


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err5-redefined-empty-string.txt
# CHECK:      ValueError: %data '' already defined on line 1
# CHECK-NEXT: in RUN: directive on test line 4

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err5-redefined.txt
# CHECK:      ValueError: %data 'foo' already defined on line 4
# CHECK-NEXT: in RUN: directive on test line 10


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err6-es-id-first.txt
# CHECK:      ValueError: %data '' must be the only %data in the file
# CHECK-NEXT: %data '' defined on line 2
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err6-es-id-later.txt
# CHECK:      ValueError: %data '' must be the only %data in the file
# CHECK-NEXT: %data 'bar' defined on line 4
# CHECK-NEXT: in RUN: directive on test line 10

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pdata-open-err6-es-id-second.txt
# CHECK:      ValueError: %data '' must be the only %data in the file
# CHECK-NEXT: %data 'foo' defined on line 1
# CHECK-NEXT: in RUN: directive on test line 4


# Test errors in %for opening line
# --------------------------------


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err0-extra-chars.txt
# CHECK:      ValueError: in line containing '%for', '%for' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err0-no-percent.txt
# CHECK:      ValueError: in line containing '%for', '%for' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err0-no-post-space.txt
# CHECK:      ValueError: in line containing '%for', '%for' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err0-pfor-in-cmd.txt
# CHECK:      ValueError: in line containing '%for', '%for' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err0-pre-pdata.txt
# CHECK:      ValueError: in line containing '%for', '%for' expected immediately after 'RUN:'
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err1-id-bad-char.txt
# CHECK:      ValueError: after '%for', expected opening '{' or identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err1-no-pre-space.txt
# CHECK:      ValueError: after '%for', expected opening '{' or identifier consisting of only dashes, underscores, periods, letters, and digits
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err2-id-bad-char0.txt
# CHECK:      ValueError: after '%for fo', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err2-id-bad-char1.txt
# CHECK:      ValueError: after '%for foo', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err2-no-rbrace.txt
# CHECK:      ValueError: after '%for foo', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err2-two-ids.txt
# CHECK:      ValueError: after '%for foo', expected opening '{'
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err3-all-one-line.txt
# CHECK:      ValueError: after %for 'foo' opening '{', expected end of line
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err3-empty-all-one-line.txt
# CHECK:      ValueError: after %for 'foo' opening '{', expected end of line
# CHECK-NEXT: in RUN: directive on test line 5

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err3-nested-pdata.txt
# CHECK:      ValueError: after %for 'foo' opening '{', expected end of line
# CHECK-NEXT: in RUN: directive on test line 5


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err4-earlier-line-unterm.txt
# CHECK:      ValueError: %for 'foo' opened after unterminated run line
# CHECK-NEXT: in RUN: directive on test line 11

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err4-prev-line-unterm.txt
# CHECK:      ValueError: %for 'foo' opened after unterminated run line
# CHECK-NEXT: in RUN: directive on test line 7


# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err5-no-pdata.txt
# CHECK:      ValueError: %for specifies undefined %data identifier 'foo'
# CHECK-NEXT: in RUN: directive on test line 1

# CHECK-LABEL: UNRESOLVED: shtest-pdata-pfor :: pfor-open-err5-unknown-pdata.txt
# CHECK:      ValueError: %for specifies undefined %data identifier 'f'
# CHECK-NEXT: in RUN: directive on test line 13


# Test running with correct usage of %data and %for
# -------------------------------------------------


# CHECK-LABEL: FAIL: shtest-pdata-pfor :: run-fail.txt
# CHECK:      Script:
# CHECK-NEXT: --
# CHECK-NEXT: 'RUN: at line 16';  echo start

# CHECK-NEXT: 'RUN: at line 18, data at line 8';    echo foo start
# CHECK-NEXT: 'RUN: at line 20, data at lines 8, 12';      echo abc start
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 12, 2';        echo foo abc
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 12, 3';        true foo abc
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 12, 4';        false foo abc
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 12, 5';        echo foo abc
# CHECK-NEXT: 'RUN: at line 24, data at lines 8, 12';      echo abc end
# CHECK-NEXT: 'RUN: at line 20, data at lines 8, 13';      echo xyz start
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 13, 2';        echo foo xyz
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 13, 3';        true foo xyz
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 13, 4';        false foo xyz
# CHECK-NEXT: 'RUN: at line 22, data at lines 8, 13, 5';        echo foo xyz
# CHECK-NEXT: 'RUN: at line 24, data at lines 8, 13';      echo xyz end
# CHECK-NEXT: 'RUN: at line 26, data at line 8';    echo foo end

# CHECK-NEXT: 'RUN: at line 18, data at line 9';    echo bar start
# CHECK-NEXT: 'RUN: at line 20, data at lines 9, 12';      echo abc start
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 12, 2';        echo bar abc
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 12, 3';        true bar abc
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 12, 4';        false bar abc
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 12, 5';        echo bar abc
# CHECK-NEXT: 'RUN: at line 24, data at lines 9, 12';      echo abc end
# CHECK-NEXT: 'RUN: at line 20, data at lines 9, 13';      echo xyz start
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 13, 2';        echo bar xyz
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 13, 3';        true bar xyz
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 13, 4';        false bar xyz
# CHECK-NEXT: 'RUN: at line 22, data at lines 9, 13, 5';        echo bar xyz
# CHECK-NEXT: 'RUN: at line 24, data at lines 9, 13';      echo xyz end
# CHECK-NEXT: 'RUN: at line 26, data at line 9';    echo bar end

# CHECK-NEXT: 'RUN: at line 28';  echo end
# CHECK-NEXT: --
# CHECK:      Command Output (stdout):
# CHECK-NEXT: --
# CHECK-NEXT: $ ":" "RUN: at line 16"
# CHECK-NEXT: $ "echo" "start"
# CHECK-NEXT: # command output:
# CHECK-NEXT: start
# CHECK:      $ ":" "RUN: at line 18, data at line 8"
# CHECK-NEXT: $ "echo" "foo" "start"
# CHECK-NEXT: # command output:
# CHECK-NEXT: foo start
# CHECK:      $ ":" "RUN: at line 20, data at lines 8, 12"
# CHECK-NEXT: $ "echo" "abc" "start"
# CHECK-NEXT: # command output:
# CHECK-NEXT: abc start
# CHECK:      $ ":" "RUN: at line 22, data at lines 8, 12, 2"
# CHECK-NEXT: $ "echo" "foo" "abc"
# CHECK-NEXT: # command output:
# CHECK-NEXT: foo abc
# CHECK:      $ ":" "RUN: at line 22, data at lines 8, 12, 3"
# CHECK-NEXT: $ "true" "foo" "abc"
# CHECK-NEXT: $ ":" "RUN: at line 22, data at lines 8, 12, 4"
# CHECK-NEXT: $ "false" "foo" "abc"
# CHECK-NEXT: note: command had no output on stdout or stderr
# CHECK-NEXT: error: command failed with exit status: 1
# CHECK-NOT:  RUN


# CHECK-LABEL: PASS: shtest-pdata-pfor :: run-success-pdata-es-id.txt
# CHECK-LABEL: PASS: shtest-pdata-pfor :: run-success.txt
