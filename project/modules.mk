
###############################################################################
##  MSGPACK MODULE                                                           ##
###############################################################################

ifndef MSGPACK
$(warning ***************************************************************)
$(warning *)
$(warning *  MSGPACK is not defined. )
$(warning *  MSGPACK should be set to a valid path. )
$(warning *)
$(warning ***************************************************************)
$(error )
endif

ifeq ($(wildcard $(MSGPACK)/configure.in),) 
$(warning ***************************************************************)
$(warning *)
$(warning *  MSGPACK is '$(MSGPACK)')
$(warning *  MSGPACK doesn't contain 'configure'. )
$(warning *  MSGPACK should be set to a valid path. )
$(warning *)
$(warning ***************************************************************)
$(error )
endif

.PHONY: MSGPACK-build
MSGPACK-build: $(MSGPACK)/src/.libs/libmsgpackc.a

.PHONY: MSGPACK-prepare
MSGPACK-prepare: 
	$(noop)

PHONY: MSGPACK-clean
MSGPACK-clean:
	@if [ -e "$(MSGPACK)/Makefile" ]; then \
		$(MAKE) -e -C $(MSGPACK) clean; \
		$(MAKE) -e -C $(MSGPACK) distclean; \
	fi
	@if [ -e "$(MSGPACK)/configure" ]; then \
		rm -f $(MSGPACK)/configure; \
	fi

$(MSGPACK)/configure: $(MSGPACK)/configure.in
	cd $(MSGPACK) && autoreconf -v --force

$(MSGPACK)/Makefile: $(MSGPACK)/configure
	cd $(MSGPACK) && ./configure CFLAGS="-fPIC"

$(MSGPACK)/src/.libs/libmsgpackc.a: $(MSGPACK)/Makefile
	cd $(MSGPACK) && $(MAKE) CFLAGS="-fPIC"
