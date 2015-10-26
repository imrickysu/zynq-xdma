ifndef PETALINUX
$(error "Error: PETALINUX environment variable not set.  Change to the root of your PetaLinux install, and source the settings.sh file")
endif

include libs.common.mk

LIB=libxdma

# Add any other object files to this list below
LIB_OBJS = libxdma.o
LIB_HEADERS=libxdma.h

INCLUDES := -I../../modules/xdma-dev

all: build install

.PHONY: build
build: $(LIB).a $(LIB).so
	# Install libraries
	mkdir -p $(STAGEDIR)/usr/lib
	rsync -a *.a $(STAGEDIR)/usr/lib/
	rsync -a *.so $(STAGEDIR)/usr/lib/
	rsync -a  *.so.* $(STAGEDIR)/usr/lib/
	
	# Install headers
	mkdir -p $(STAGEDIR)/usr/include
	for h in $(LIB_HEADERS); do \
		rsync -a $$h $(STAGEDIR)/usr/include; \
	done

$(LIB).a:$(LIB_OBJS)
	$(AR) rcs $@ $(LIB_OBJS)

$(LIB).so:$(LIB_OBJS)
	$(CC) -shared -o $@.1.0.1 $(LIB_OBJS)
	rm -f $@.1 $@
	ln -s $@.1.0.1 $@.1
	ln -s $@.1.0.1 $@

%.o: %.c
	$(CC) $(INCLUDES) -c $(CFLAGS) -o $@ $<
	
install: $(LIB).so
	# Install libraries template
	# Install lib files
	$(TARGETINST) -d $<.1.0.1 /lib/$<.1.0.1
	# Install lib symlink
	$(TARGETINST) -s $<.1.0.1 /lib/$<.1
	$(TARGETINST) -s $<.1.0.1 /lib/$<

clean:
	rm -rf *.o *.so *.so.* *.a

help:
	@echo ""
	@echo "Quick reference for various supported build targets for $(INSTANCE)."
	@echo "----------------------------------------------------"
	@echo "  clean                  clean out build objects"
	@echo "  all                    build $(INSTANCE) and install to rootfs host copy"
	@echo "  build                  build subsystem"
	@echo "  install                install built objects to rootfs host copy"
