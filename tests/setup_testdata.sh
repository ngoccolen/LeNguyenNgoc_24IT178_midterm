#!/bin/sh
set -eu

mkdir -p tests/testdata/sub/inner
printf 'Hello NetBSD\n' > tests/testdata/a.txt
printf 'This is a somewhat longer file\n' > tests/testdata/b.txt
printf 'Nested file\n' > tests/testdata/sub/c.txt
printf 'Deep file\n' > tests/testdata/sub/inner/d.txt
touch tests/testdata/.hidden
printf '#!/bin/sh\necho test\n' > tests/testdata/run.sh
chmod +x tests/testdata/run.sh
ln -sf a.txt tests/testdata/link.txt
mkfifo tests/testdata/pipe 2>/dev/null || true
