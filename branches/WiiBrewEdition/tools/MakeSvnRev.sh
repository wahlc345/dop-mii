set REV=`svnversion -n ./`
echo $REV
touch ./include/svnrev.h
cat > ./include/svnrev.h <<EOF
#define SVN_REV $REV
#define SVN_REV_STR "$REV"
EOF
