BUILT_SOURCES = parser.h
AM_YFLAGS = -d

noinst_LTLIBRARIES = libminischeme.la

bin_PROGRAMS = minischeme
TESTS = selfcheck
check_PROGRAMS = selfcheck

libminischeme_la_SOURCES = parser.y tokenizer.l primitives.h primitives.c 

minischeme_SOURCES = main.c
minischeme_LDADD = -lreadline -lgc libminischeme.la

selfcheck_SOURCES = test_parser.c 
selfcheck_LDADD = -lreadline -lgc libminischeme.la











