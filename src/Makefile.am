BUILT_SOURCES = parser.h test-definitions.h
AM_YFLAGS = -d

CLEANFILES = test-definitions.h

noinst_LTLIBRARIES = libminischeme.la

bin_PROGRAMS = minischeme
TESTS = selfcheck
check_PROGRAMS = selfcheck

libminischeme_la_SOURCES = parser.y tokenizer.l primitives.h primitives.c murmurhash.h murmurhash.c
libminischeme_la_LIBADD = -lgc

minischeme_SOURCES = main.c
minischeme_LDADD = -lreadline libminischeme.la

selfcheck_SOURCES = selfcheck.c
selfcheck_LDADD = libminischeme.la
nodist_selfcheck_SOURCES = test-definitions.h

EXTRA_DIST = gentests.sh test_parser.c

# build rule for test-definitions.h
.FORCE:
test-definitions.h: .FORCE
	./gentests.sh










