REV=`svnversion -n ../`
echo $REV
cat > ./shared/svnrev.h <<EOF
#define SVN_REV $REV
#define SVN_REV_STR "$REV"
EOF
