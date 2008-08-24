# Citrusleaf Foundation
# Makefile

default: libcf
	@echo "done."

clean:
	rm -rf obj/*

%:
	$(MAKE) -C src $@
