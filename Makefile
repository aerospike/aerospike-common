# Citrusleaf Foundation
# Makefile

default: libcf
	@echo "done."

clean:
	rm -rf obj/*
	rm -f lib/*

%:
	$(MAKE) -f Makefile -C src $@
