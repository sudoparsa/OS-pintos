# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(cache-write) begin
(cache-write) create "a" should not fail.
(cache-write) File creation completed.
(cache-write) open "a" returned fd which should be greater than 2.
(cache-write) write completed.
(cache-write) Block write count should be less than 134.
(cache-write) Block read count should be less than acceptable error 6.
(cache-write) Removed "a".
(cache-write) end
cache-write: exit(0)
EOF
pass;