#!/bin/bash

if [[ $HOSTTYPE != "aarch64" ]] && [[ $HOSTTYPE != "arm64" ]] ; then
	echo wrong host type
	exit 255
fi

CXX=${CXX:-g++}

if [[ $1 == "debug" ]]; then
	OPT_FLAGS+=(
		-g
		-O0
	)
else
	OPT_FLAGS+=(
		-DNDEBUG
		-Ofast
	)
fi

as stringx.s -o stringx.o
${CXX} main.cpp -std=c++17 -Wno-switch -Wno-logical-op-parentheses -Wno-shift-op-parentheses -fno-rtti -fno-exceptions ${OPT_FLAGS[@]} -c -o main.o
${CXX} main.o stringx.o -o hello

if [ `which ctags` ]; then
	ctags --language-force=c++ --totals *{.h,.hpp,.cpp}
fi
