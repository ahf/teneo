#!/bin/bash

set -o errexit

git archive --format tar.gz --output /tmp/teneo_1.0.0.orig.tar.gz HEAD
gbp buildpackage -uc -us --git-ignore-new --git-tarball-dir=/tmp
lintian --profile debian --color=auto -iI ../teneo*.changes
echo Debian pkg seems a-okay
ls -d -- ../teneo*
