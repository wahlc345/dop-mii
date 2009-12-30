#!/bin/sh
REV=`svnversion -n ../`
echo $REV
touch ./source/svnrev.h
cat > ./source/svnrev.h <<EOF
#define SVN_REV $REV
#define SVN_REV_STR "$REV"
EOF
