# Copyright (c) 2009 Red Hat, Inc.
# 
# Authors: Andrew Beekhof
#	   Steven Dake (sdake@redhat.com)
#
# This software licensed under BSD license, the text of which follows:
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# - Neither the name of the MontaVista Software, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

MAINTAINERCLEANFILES    = Makefile.in

AM_CFLAGS	= -fPIC
INCLUDES       	= -I$(top_builddir)/include  -I$(top_srcdir)/include
DARWIN_OPTS	= -dynamiclib -bind_at_load -current_version 2.0.0 -compatibility_version 2.0.0

LCRSO		= 
LCR_SRC		= objdb.c vsf_ykd.c coroparse.c quorum.c vsf_quorum.c
TOTEM_SRC	= coropoll.c totemip.c totemnet.c totemrrp.c totemsrp.c totemmrp.c totempg.c crypto.c wthread.c
LOGSYS_SRC	= wthread.c logsys.c

lib_LIBRARIES	= libtotem_pg.a liblogsys.a
sbin_PROGRAMS	= corosync
SHARED_LIBS	=

libtotem_pg_a_SOURCES	= $(TOTEM_SRC)
liblogsys_a_SOURCES	= $(LOGSYS_SRC)

corosync_SOURCES = main.c mempool.c util.c sync.c apidef.c service.c ipc.c \
		timer.c totemconfig.c mainconfig.c quorum.c ../lcr/lcr_ifact.c
corosync_LDADD	  = -ltotem_pg -llogsys
corosync_DEPENDENCIES = libtotem_pg.so.2.0.0 liblogsys.so.2.0.0
corosync_LDFLAGS = $(DYFLAGS) -L./

TOTEM_OBJS	= $(TOTEM_SRC:%.c=%.o)
LOGSYS_OBJS	= $(LOGSYS_SRC:%.c=%.o)

if BUILD_DYNAMIC
SHARED_LIBS	+= $(lib_LIBRARIES:%.a=%.so.2.0.0)
SHARED_LIBS_SO	= $(SHARED_LIBS:%.so.2.0.0=%.so)
SHARED_LIBS_SO_TWO = $(SHARED_LIBS:%.so.2.0.0=%.so.2)
LCRSO		+= $(LCR_SRC:%.c=%.lcrso)
else
corosync_SOURCES += $(LCR_SRC)
endif

if BUILD_DARWIN
%.lcrso: %.o
	$(CC) $(CFLAGS) -bundle -bind_at_load -bundle_loader ./corosync $^ -o $@

libtotem_pg.so.2.0.0: $(TOTEM_OBJS)
	$(CC) $(LDFLAGS) $(DARWIN_OPTS) $(TOTEM_OBJS) -o $@

liblogsys.so.2.0.0: $(LOGSYS_OBJS)
	$(CC) $(LDFLAGS) $(DARWIN_OPTS) $(LOGSYS_OBJS) -o $@

else

%.lcrso: %.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,$@ $^ -o $@

libtotem_pg.so.2.0.0: $(TOTEM_OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libtotem_pg.so.2 $^ -o $@
	ln -sf libtotem_pg.so.2.0.0 libtotem_pg.so
	ln -sf libtotem_pg.so.2.0.0 libtotem_pg.so.2

liblogsys.so.2.0.0: $(LOGSYS_OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,liblogsys.so.2 $^ -o $@
	ln -sf liblogsys.so.2.0.0 liblogsys.so
	ln -sf liblogsys.so.2.0.0 liblogsys.so.2
endif

lint:
	-splint $(LINT_FLAGS) $(CFLAGS) *.c

all-local: $(LCRSO) $(SHARED_LIBS)
	@echo Built corosync Executive

install-exec-local:
	for lib in $(SHARED_LIBS) $(SHARED_LIBS_SO) $(SHARED_LIBS_SO_TWO); do $(INSTALL) -m 644 $$lib $(DESTDIR)/$(libdir); done
	for lib in $(LCRSO); do	$(INSTALL) -m 644 $$lib $(DESTDIR)/$(LCRSODIR); done

clean-local:
	rm -f corosync *.o *.lcrso gmon.out *.da *.bb *.bbg *.so*

