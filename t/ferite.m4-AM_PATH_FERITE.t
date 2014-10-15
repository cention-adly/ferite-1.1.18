#!/bin/sh

test_dir=$0.d
ntest=0
nfailed=0
test_request_version() {
	request=$1
	installed_version=$3
	expect=$4

	echo "echo $installed_version" > ferite-config &&
	echo "AC_INIT(foo, 1.0.0)
AM_PATH_FERITE($request,[echo Yay],[echo Nay])
AC_OUTPUT" > configure.ac &&
	rm -rf aclocal.m4 config.log configure autom4te.cache config.status &&
	aclocal -I../.. &&
	autoconf &&
	ntest=$((ntest + 1)) &&
	msg="$ntest - $installed_version satisfies $request: $expect" &&
	log="../$0.$ntest.failed" &&
	echo "Fake installed ferite version is '$installed_version'" > $log &&
	if PATH=$PWD:$PATH ./configure|tee -a $log | grep -q -w "$expect"; then
		rm -f ../$0.$ntest.failed
		echo "ok $msg"
	else
		nfailed=$((nfailed + 1))
		echo "not ok $msg"
		exit 1
	fi
}

setup() {
	rm -rf $test_dir
	mkdir $test_dir
	cd $test_dir
	touch ferite-config
	chmod +x ferite-config
}

cleanup() {
	if [ $nfailed -eq 0 ]; then
		rm -rf $test_dir
	fi
}

setup

echo "1..15" # No. of tests we expect to run:
test_request_version "1.0.0"    metBy "1.0.0" Yay
test_request_version "1.0.0"    metBy "0.0.1" Nay
test_request_version "1.1.18"   metBy "1.1.18" Yay
test_request_version "1.1.18-1" metBy "1.1.18" Nay
test_request_version "1.1.18"   metBy "1.1.17" Nay
test_request_version "1.1.17"   metBy "1.1.18" Yay
test_request_version "1.1.17"   metBy "1.1.17-100" Yay
test_request_version "1.1.18-1" metBy "1.1.18-2" Yay
test_request_version "1.1.18-1" metBy "1.1.18-20" Yay
test_request_version "1.1.18"   metBy "1.1.18-20" Yay
test_request_version "1.1.18"   metBy "1.2.0" Yay
test_request_version "1.1.18-40"   metBy "1.1.18-40" Yay
test_request_version "1.1.18.40"   metBy "1.1.18-40" Yay
test_request_version "1.1.18-40"   metBy "1.1.18.40" Yay
test_request_version "1.1.18.40"   metBy "1.1.18.40" Yay

cleanup
