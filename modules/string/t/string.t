#!/bin/sh

gdb=
dir=$(dirname $0)
topdir=$dir/../../..

module_dir=$dir/..
native_module_dir=$module_dir/.libs
ferite_bin=$topdir/test/ferite
if [ -n "$GDB" ]; then
	if [ "$GDB" = 1 ]; then
		GDB=
	fi
	gdb="gdb $GDB -ex run --args "
	ferite_bin=$topdir/test/.libs/ferite
fi

echo $gdb $ferite_bin --include $module_dir --native-inc $native_module_dir ${0%.t}.fe || exit 1
$gdb $ferite_bin --include $module_dir --native-inc $native_module_dir ${0%.t}.fe || exit 1
