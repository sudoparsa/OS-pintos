# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(cache-hitrate) begin
(cache-hitrate) create "a" should not fail.
(cache-hitrate) open "a" returned fd which should be greater than 2.
(cache-hitrate) write should write all the data to file.
(cache-hitrate) File creation completed.
(cache-hitrate) First run cache hit rate calculated.
(cache-hitrate) Second run cumulative cache hit rate calculated.
(cache-hitrate) Hit rate should grow.
(cache-hitrate) Removed "a".
(cache-hitrate) end
cache-hitrate: exit(0)
EOF
pass;