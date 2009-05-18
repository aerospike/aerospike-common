# Citrusleaf Foundation
# Makefile

default: libcf
	@echo "done."

clean:
	rm -rf obj/x64/*
	rm -rf obj/i86/*
	rm -rf obj/native/*
	rm -f lib/*
	rm -f lib32/*
	rm -f lib64/*

%:
	$(MAKE) -f Makefile.native -C src $@
	$(MAKE) -f Makefile.32 -C src $@
	$(MAKE) -f Makefile.64 -C src $@
