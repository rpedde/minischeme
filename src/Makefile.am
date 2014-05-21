BUILT_SOURCES = parser.h
AM_YFLAGS = -d

bin_PROGRAMS = minischeme
TESTS = selfcheck
check_PROGRAMS = selfcheck

minischeme_SOURCES = parser.y tokenizer.l main.c primitives.h primitives.c
minischeme_LDADD = -lreadline -lgc

selfcheck_SOURCES = test_parser.c parser.y tokenizer.l primitives.h primitives.c
selfcheck_LDADD = -lreadline -lgc











