#
# Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
#

.PHONY: $(EXEC) $(LIB)

#
# Rule to build executable
#
ifdef EXEC
$(EXEC): $(BUILD)/$(EXEC)

$(BUILD)/$(EXEC): $(CSTYLES) $(OBJS) \
    $(foreach lib,$(ADA_LIBS),$(SRC)/ayla/lib/$(ARCH)/lib$(lib).a)
	$(QUIET)echo "LINK $(notdir $@)"; \
		$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) -Wl,-Map=$@.map
endif
#
# Rule to make a static library.
#
ifdef LIB
$(LIB): $(BUILD)/$(LIB)

$(BUILD)/$(LIB): $(CSTYLES) $(OBJS)
	$(QUIET)echo "AR $(notdir $@)"; \
		$(AR) r $@ $(OBJS)
endif
#
# Rule to make dependencies files
#
$(BUILD)/%.d: %.c Makefile
	$(QUIET)echo "DEP $<"; \
		(mkdir -p $(dir $@); $(CC) -MM $(CPPFLAGS) $(CFLAGS) $< | \
		sed 's,\($*\)\.o[ :]*,$(BUILD)/\1.o $@: ,g' > $@) || rm -f $@

-include $(DEPS)

#
# Object file rules
#
$(BUILD)/%.o: %.c Makefile
	$(QUIET)echo "CC $<" ; mkdir -p $(dir $@) ; $(CC) -c $(CFLAGS) -o $@ $<

#
# Style check rules
#
$(BUILD)/%.cs: %.c
	$(QUIET)echo "CSTYLE $<"; \
	$(CSTYLE) --strict --terse --summary-file --no-tree -f $< \
	&& mkdir -p $(dir $@) && touch $@

$(BUILD)/%.hcs: %.h
	$(QUIET)echo "CSTYLE $<"; \
	$(CSTYLE) --strict --terse --summary-file --no-tree -f $< \
	&& mkdir -p $(dir $@) && touch $@

#
# Generate build directory
#
$(BUILD):
	$(QUIET)mkdir -p $@

clean:
	$(QUIET)rm -f $(OBJS) $(DEPS) $(CSTYLES)
	$(QUIET)cd $(BUILD); rm -f $(EXEC) $(LIB)
