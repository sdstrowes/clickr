CFLAGS=-pedantic  -pedantic-errors -w  -Wextra  -Wall \
       -Waggregate-return -Wno-attributes \
       -Wno-builtin-macro-redefined \
       -Wcast-align  -Wcast-qual -Wclobbered \
       -Wconversion  -Wcoverage-mismatch  -Wno-deprecated \
       -Wno-div-by-zero -Wempty-body -Wno-endif-labels \
       -Werror  -Wfatal-errors  -Wfloat-equal \
       -Wno-format-contains-nul -Wno-format-extra-args \
       -Wformat-nonliteral -Wformat-security  -Wformat-y2k \
       -Wignored-qualifiers -Wimplicit \
       -Woverlength-strings \
       -Wpacked  -Wpacked-bitfield-compat  -Wpadded \
       -Wno-pointer-to-int-cast \
       -Wredundant-decls  -Wshadow \
       -Wunreachable-code \
       -Wunused \
		-g

LDFLAGS=

all: clickr

clickr: clickr.c
	gcc $(CFLAGS) clickr.c -o clickr \
	`pkg-config --cflags --libs libconfig libcrypto libssl libcurl`

clean:
	rm -f clickr
