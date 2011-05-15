PKG_CONFIG_PATH := ${PKG_CONFIG_PATH}:${HOME}/lib/pkgconfig

CFLAGS=-pedantic  -pedantic-errors -w  -Wextra  -Wall \
       -Waggregate-return -Wno-attributes \
       -Wcast-align  -Wcast-qual -Wclobbered \
       -Wconversion  -Wcoverage-mismatch  -Wno-deprecated \
       -Wno-div-by-zero -Wempty-body -Wno-endif-labels \
       -Werror  -Wfatal-errors  -Wfloat-equal \
       -Wno-format-contains-nul -Wno-format-extra-args \
       -Wformat-nonliteral -Wformat-security  -Wformat-y2k \
       -Wignored-qualifiers -Wimplicit \
       -Woverlength-strings \
       -Wpacked -Wpadded \
       -Wno-pointer-to-int-cast \
       -Wredundant-decls  -Wshadow \
       -Wunreachable-code \
       -Wunused \
		-g -std=c99


LDFLAGS=

all: clickr

clickr: clickr.c
	gcc $(CFLAGS) clickr.c -o clickr \
	`pkg-config --cflags --libs libconfig libcrypto libssl libcurl`

clean:
	rm -f clickr
