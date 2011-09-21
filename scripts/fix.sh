#! /bin/sh
#
# This script processes the filelists produced by DayDream and fixes
# the problems caused by the year 2000. Only short filelists (MSDOS-
# compliant filenames) are affected by this bug.
#
# Usage is simple: "fix.sh /home/bbs/daydream.cfg" or similar command.
#

function fail() {
    echo $1
    exit 1
}    

if [ "x$1" = "x" ]; then
    fail 'Usage: fix.sh <full pathname of daydream.cfg>'
fi

if [ ! -e $1 ]; then
    fail 'Usage: fix.sh <full pathname of daydream.cfg>'
fi   

DIRS=`sed -e '/CONF_PATH/ ! D' -e 's/^[[:space:]]*CONF_PATH\.*[[:space:]]\(.*\)$/\1/' $1 2>/dev/null` || fail "ERROR: sed(1) failed."
if [ "x$DIRS" = "x" ]; then
    fail "ERROR: no conference directories in $1."
fi    

for dir in $DIRS; do
    cd $dir/data 2>/dev/null || { echo warning: Can\'t chdir to $dir/data; continue; }
    LISTS=`ls directory.??? 2>/dev/null`
    if [ "x$LISTS" = "x" ]; then
        echo "Directory `pwd` doesn't contain filelists, skipping."
	continue
    fi
    for fl in $LISTS; do
        cp -f $fl $fl~ 2>/dev/null || { echo warning: Can\'t backup $fl, skipping.; continue; }
	sed -e 's/^\(.\{25\}[[:space:]][[:digit:]][[:digit:]]\.[[:digit:]][[:digit:]]\.\)1\([[:digit:]][[:digit:]].*\)$/\1\2/' $fl~ > $fl 2>/dev/null || { \
	    echo "warning: sed(1) failed processing `pwd`/$fl.";\
	    echo "         restoring the backup.";\
	    cp -f $fl~ $fl; continue;\
	}
	echo "Fixed successfully: `pwd`/$fl."
    done
done

echo
