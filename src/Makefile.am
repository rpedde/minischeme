BUILT_SOURCES = test-definitions.h grammar.c grammar.h
AM_YFLAGS = -d

CLEANFILES = test-definitions.h

noinst_LTLIBRARIES = libminischeme.la

bin_PROGRAMS = minischeme
TESTS = selfcheck
check_PROGRAMS = selfcheck

libminischeme_la_SOURCES = tokenizer.l primitives.h primitives.c \
	murmurhash.h murmurhash.c builtins.h builtins.c grammar.c \
	lisp-types.h lisp-types.c redblack.h redblack.c ports.h ports.c \
	char.h char.c math.c math.h parser.c parser.h

libminischeme_la_LIBADD = -lgc -lgmp -lmpfr

minischeme_SOURCES = main.c
minischeme_LDADD = -lreadline libminischeme.la

selfcheck_SOURCES = selfcheck.c
selfcheck_LDADD = libminischeme.la
nodist_selfcheck_SOURCES = test-definitions.h

EXTRA_DIST = gentests.sh test_parser.c test_builtins.c test_primitives.c

# build rule for test-definitions.h
.FORCE:
test-definitions.h: .FORCE
	./gentests.sh

grammar.c: grammar.y
	lemon grammar.y
