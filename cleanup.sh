#!/bin/sh

# Use this to get rid of clutter you don't want in version control.
# autoreconf and configure are required after executing this script.
rm -f stamp-h1 *.h configure config.log config.status ltmain.sh
rm -f *.m4
rm -rf autom4te.cache/
rm -f m4/*.m4
rm -r libtool
rm -f test-driver

for dir in . hti-tcp-reflash man
do
    rm -f ${dir}/*.in
    rm -f ${dir}/Makefile
    rm -rf ${dir}/.deps
    rm -f ${dir}/*~
done
