AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src

TESTS = test-engine
check_PROGRAMS = test-engine

test_engine_SOURCES = \
	test-engine.c \
	$(top_srcdir)/src/engine.c \
	$(top_srcdir)/src/zhuyin.c \
	$(NULL)

test_engine_CFLAGS = \
	@IBUS_CFLAGS@ \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-DPKGDATADIR="$(pkgdatadir)"

test_engine_LDFLAGS = \
	@IBUS_LIBS@ \
	@GTK_LIBS@ \
	@GLIB_LIBS@

