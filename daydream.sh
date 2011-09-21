DAYDREAM=/home/bbs
export DAYDREAM

if test "x$LD_LIBRARY_PATH" = "x"; then
    LD_LIBRARY_PATH=`pwd`/lib
else
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib
fi   
export LD_LIBRARY_PATH
