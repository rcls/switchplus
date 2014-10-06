
SUBDIRS=utils soft #module

all:
	$(MAKE) -C soft all
	$(MAKE) -C utils all

.PHONY: all

$(SD_TARGETS): %-subdir:
	$(MAKE) -C $*

.PHONY: go clean
go: soft/go

clean:
	$(MAKE) -C soft clean
	$(MAKE) -C utils clean

.PHONY: force
force:
soft/%: force
	$(MAKE) -C soft $*
