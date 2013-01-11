
SUBDIRS=utils soft
SD_TARGETS= $(SUBDIRS:%=%-subdir)

all: $(SD_TARGETS)

.PHONY: all $(SD_TARGETS)

$(SD_TARGETS): %-subdir:
	$(MAKE) -C $*

.PHONY: go
go: utils-subdir
	$(MAKE) -C soft go

.PHONY: soft/%
soft/%:
	$(MAKE) -C soft $*
