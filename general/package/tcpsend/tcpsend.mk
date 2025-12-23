TCPSEND_LICENSE = MIT
TCPSEND_LICENSE_FILES = LICENSE

define TCPSEND_EXTRACT_CMDS
	cp $(TCPSEND_PKGDIR)/src/tcpsend.c $(@D)/
endef

define TCPSEND_BUILD_CMDS
	(cd $(@D); $(TARGET_CC) -Os -s tcpsend.c -o tcpsend)
endef

define TCPSEND_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 -D $(@D)/tcpsend $(TARGET_DIR)/usr/bin/tcpsend
endef

$(eval $(generic-package))