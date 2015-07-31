#!/bin/csh -f
#
# Copyright (C): dGB Beheer B. V.
# $Id$
#
# Wrapper script to run a test
#

set datadir = ""
set wdir = ""
set plf = ""
set config = ""
set cmd = ""
set args = ""
set qtdir = ""
set expret = 0
set valgrind = ""
set gensuppressions="all"

parse_args:
if ( "$1" == "--command" ) then
    set cmd=$2
    shift
else if ( "$1" == "--datadir" ) then
    set datadir=$2
    shift
else if ( "$1" == "--plf" ) then
    set plf=$2
    shift
else if ( "$1" == "--quiet" ) then
    set args="${args} --quiet"
    set gensuppressions="no"
else if ( "$1" == "--valgrind" ) then
    set valgrind=$2
    shift
else if ( "$1" == "--expected-result" ) then
    set expret=$2
    shift
else if ( "$1" == "--wdir" ) then
    set wdir=$2
    shift
else if ( "$1" == "--config" ) then
    set config=$2
    shift
else if ( "$1" == "--parfile" ) then
    set args="${args} $2"
    shift
else if ( "$1" == "--qtdir" ) then
    set qtdir=$2
    shift
else if ( "$1" == "--help" ) then
    echo "Usage: run_tst [option]"
    echo " --command cmd"
    echo " --ldpath ldpath"
    echo " --datadir datadir"
    echo " [--parfile parfile]"
    exit 0
else
    goto do_it
endif

shift
goto parse_args

do_it:

if ( "$cmd" == "" ) then
    echo "Command not specified"
    exit 1
endif

if ( "$wdir" == "" ) then
    echo "--wdir not specified"
    exit 1
endif

if ( "$plf" == "" ) then
    echo "--plf not specified"
    exit 1
endif

if ( "$config" == "" ) then
    echo "--config not specified"
    exit 1
endif

if ( $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH ${qtdir}/lib:${wdir}/bin/${plf}/${config}:${LD_LIBRARY_PATH}
else
    setenv LD_LIBRARY_PATH ${qtdir}/lib:${wdir}/bin/${plf}/${config}
endif

if ( $?DYLD_LIBRARY_PATH ) then
    setenv DYLD_LIBRARY_PATH ${qtdir}/lib:${wdir}/bin/${plf}/${config}:${DYLD_LIBRARY_PATH}
else
    setenv DYLD_LIBRARY_PATH ${qtdir}/lib:${wdir}/bin/${plf}/${config}
endif


if ( "$datadir" != "" ) then
    set args = "${args} --datadir ${datadir}"
endif

if ( "${valgrind}" != "" ) then
    set suppression = `ls testing/suppressions/*.supp | awk '{ print "--suppressions" "=" $1 }'`
    ${valgrind} \
	${suppression} \
	--gen-suppressions=${gensuppressions} \
	"-q" \
	"--tool=memcheck" \
	"--leak-check=full" \
	"--show-reachable=no" \
	"--workaround-gcc296-bugs=yes" \
	"--num-callers=50" \
	"--track-origins=yes" \
	"--error-exitcode=1" \
	"${wdir}/bin/${plf}/${config}/${cmd}" ${args} --quiet
    set result = ${status}
    if ( "${result}" != "${expret}" ) then
	echo "Test program ${cmd} failed memory test".
	exit 1
    endif
else
    "${wdir}/bin/${plf}/${config}/${cmd}" ${args}
    set result = ${status}
    if ( "${result}" != "${expret}" ) then
	echo "Test program ${cmd} retured ${result}, while ${expret} was expected"
	exit 1
    endif
endif

exit 0