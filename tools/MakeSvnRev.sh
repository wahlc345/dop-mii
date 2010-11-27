if [ ! -f ./include/svnrev.h ]; then
  REV=`svnversion -n ./`
  if [ "$REV" == "exported" ]; then
    # try GIT
    REV=`git svn info |grep '^Revision:' | sed -e 's/^Revision: //'`
  fi
  touch ./include/svnrev.h
  cat > ./include/svnrev.h <<EOF
#define SVN_REV $REV
#define SVN_REV_STR "$REV"
EOF
fi
