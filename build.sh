#!/bin/bash

if [[ $HOSTTYPE != "aarch64" ]]; then
	echo wrong host type
	exit 255
fi

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
clang++-7 main.cpp -Wno-switch -Wno-logical-op-parentheses -Wno-shift-op-parentheses -fno-rtti -fno-exceptions ${OPT_FLAGS[@]} -c -o main.o
clang++-7 main.o stringx.o -o hello
