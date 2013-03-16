#!/bin/sh
# Convert git log to GNU-style ChangeLog file.

git log --date-order --date=short --since="Sat Apr 21 13:05:11 2012 +0800" | \
  sed -e '/^commit.*$/d' | \
  awk '/^Author/ {sub(/\\$/,""); getline t; print $0 t; next}; 1' | \
  sed -e 's/^Author: //g' | \
  sed -e 's/>Date:   \([0-9]*-[0-9]*-[0-9]*\)/>\t\1/g' | \
  sed -e 's/^\(.*\) \(<.*>\)\t\(.*\)/\3  \1 \2/g' -e 's/^    /\t/g' > ChangeLog

make distcheck
make srpm
