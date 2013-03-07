LIB = frontier
PROG = frontier

# Using 'scons' for building (make clean|release|debug|profile)
#
#
# To build serially (helps get the error messages right): make debug SCONS_FLAGS=""

# SCONS_FLAGS=-j 4
SCONS_FLAGS=

# Installation directories

prosessor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(prosessor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

includedir = $(PREFIX)/include/smartmet
objdir = obj

ifeq ($(origin BINDIR), undefined)
  bindir = $(PREFIX)/bin
else
  bindir = $(BINDIR)
endif

# rpm variables

#rpmsourcedir=/tmp/$(shell whoami)/rpmbuild
rpmsourcedir=${HOME}/rpmbuild
rpmerr = "There's no spec file ($(LIB).spec). RPM wasn't created. Please make a spec file or copy and rename it into $(LIB).spec"

rpmversion := $(shell grep "^Version:" $(LIB).spec  | cut -d\  -f 2 | tr . _)
rpmrelease := $(shell grep "^Release:" $(LIB).spec  | cut -d\  -f 2 | tr . _)

# What to install

LIBFILE = libsmartmet_$(LIB).a

# How to install

INSTALL_PROG = install -m 775
INSTALL_DATA = install -m 664

.PHONY: test rpm

#
# The rules
#
SCONS_FLAGS += objdir=$(objdir)

all release:
	scons $(SCONS_FLAGS)

debug:
	scons $(SCONS_FLAGS) debug=1

profile:
	scons $(SCONS_FLAGS) profile=1

clean:
	@#scons -c objdir=$(objdir)
	-rm -f $(LIBFILE) *~ source/*~ include/*~ *.o $(PROG) .sconsign.dblite
	-rm -rf $(objdir)

install:
	@mkdir -p $(includedir)/$(LIB)
	@list=`cd include && ls -1 *.h`; \
	for hdr in $$list; do \
	  echo $(INSTALL_DATA) include/$$hdr $(includedir)/$(LIB)/$$hdr; \
	  $(INSTALL_DATA) include/$$hdr $(includedir)/$(LIB)/$$hdr; \
	done
	@mkdir -p $(libdir)
	$(INSTALL_DATA) $(LIBFILE) $(libdir)/$(LIBFILE)
	mkdir -p $(bindir)
	@list='$(PROG)'; \
	for prog in $$list; do \
	  echo $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	  $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	done

test:
	cd test && make test

html:
	mkdir -p /data/local/html/lib/$(LIB)
	doxygen $(LIB).dox

rpm: clean
	if [ -e $(LIB).spec ]; \
	then \
	  tar --exclude-vcs -C ../ -cf $(rpmsourcedir)/smartmet-$(LIB).tar $(LIB) ; \
	  gzip -f $(rpmsourcedir)/smartmet-$(LIB).tar ; \
	  TAR_OPTIONS=--wildcards rpmbuild -ta $(rpmsourcedir)/smartmet-$(LIB).tar.gz ; \
	else \
	  echo $(rpmerr); \
	fi;

tag:
	cvs -f tag '$(LIB)_$(rpmversion)-$(rpmrelease)' .

headertest:
	@echo "Checking self-sufficiency of each header:"
	@echo
	@for hdr in $(HDRS); do \
	echo $$hdr; \
	echo "#include \"$$hdr\"" > /tmp/$(LIB).cpp; \
	echo "int main() { return 0; }" >> /tmp/$(LIB).cpp; \
	$(CC) $(CFLAGS) $(INCLUDES) -o /dev/null /tmp/$(LIB).cpp $(LIBS); \
	done
