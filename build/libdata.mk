# The data library contains data files such as the license file, the
# authors list and others

DATA_RESOURCES = \
	$(TEXT_COMPRESSED) \
	Data/other/egm96s.dem
ifneq ($(TARGET_IS_ESP32),y)
# on the ESP32, translations are loaded from the data directory
DATA_RESOURCES += $(MO_FILES)
endif
DATA_SOURCES += $(foreach file,$(DATA_RESOURCES),$(DATA)/$(notdir $(file)).c)

define add-data-file
generate:: $(DATA)/$(notdir $(1)).c
ifeq ($(TARGET_IS_ESP32),y)
$(DATA)/$(notdir $(1)).c: export RESOURCE_SECTION:=.section .rodata
endif
$(DATA)/$(notdir $(1)).c: $(1) $(topdir)/tools/BinToC.pl $(topdir)/tools/BinToC.pm | $$(DATA)/dirstamp
	@$$(NQ)echo "  GEN     $$@"
	$(Q)$(PERL) $(topdir)/tools/BinToC.pl $$< $$@
endef

$(foreach file,$(DATA_RESOURCES),$(eval $(call add-data-file,$(file))))

$(eval $(call link-library,libdata,DATA))
