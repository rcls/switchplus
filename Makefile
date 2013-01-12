
SUBDIRS=utils soft module
SD_TARGETS= $(SUBDIRS:%=%-subdir)

all: $(SD_TARGETS)

.PHONY: all $(SD_TARGETS)

$(SD_TARGETS): %-subdir:
	$(MAKE) -C $*

.PHONY: go
go: soft/go

.PHONY: force
force:
soft/%: force
	$(MAKE) -C soft $*
