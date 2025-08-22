export OUT_DIR=buildDir
LIBUBOX_FULLPATH := ../../libubox_20180312.tar.gz
JSON_VER := jsonc_0.12
JSON_FULLPATH := ../../$(JSON_VER).tar.gz
JSON_PATCH_FULLPATH := ../../jsonc_senao.patch

CFLAGS := -ljson-c -lubox -lblobmsg_json -ljson_script 
CFLAGS += -I$(CURDIR)/$(OUT_DIR)/jsonc/include
CFLAGS += -I$(CURDIR)/$(OUT_DIR)/libubox/include
CFLAGS += -L$(CURDIR)/$(OUT_DIR)/jsonc/lib
CFLAGS += -L$(CURDIR)/$(OUT_DIR)/libubox/lib

#Steps
#  make -C testCases/test_libubox/ buildDir
#  make -C testCases/test_libubox/ check-libubox-file
#  make -C testCases/test_libubox/ check-jsonc-file
#  make -C testCases/test_libubox/ build-jsonc
#  make -C testCases/test_libubox/ build-libubox
#  make -C testCases/test_libubox/ all
#
#  LD_LIBRARY_PATH=$PWD/testCases/test_libubox/buildDir/libubox/lib:$PWD/testCases/test_libubox/buildDir/jsonc/lib:$LD_LIBRARY_PATH ./testCases/test_libubox/test
#
all:
	@gcc -o test main.c $(CFLAGS) 
	@gcc -o test2 main2.c $(CFLAGS) 
	@gcc -o test3 main3.c $(CFLAGS) 


check-libubox-file:
	if [ -f $(LIBUBOX_FULLPATH) ]; then \
		echo "$(LIBUBOX_FULLPATH) file exists"; \
		tar -zxvf $(LIBUBOX_FULLPATH); \
	fi

#
# Check Version : 
#   jsonc/json_c_version.h:17:#define JSON_C_VERSION "0.12"
#
check-jsonc-file:
	if [ ! -e $(JSON_VER) ]; then \
		if [ -f $(JSON_FULLPATH) ]; then \
			echo "$(JSON_VER) file exists"; \
			tar -zxvf $(JSON_FULLPATH); \
			if [ "jsonc_0.12" == "${JSON_VER}" ]; then \
				patch -p0 < $(JSON_PATCH_FULLPATH); \
			fi \
		else \
			echo "$(JSON_VER) file not exists"; \
		fi \
	fi

build-libubox:
	cd libubox; \
	sed -i 's:ProjectApp:$(CURDIR):g' CMakeLists.txt; \
	sed -i 's:ProjectInstall:$(CURDIR)/$(OUT_DIR):g' CMakeLists.txt; \
	sed -i 's:mips-linux-gcc:gcc:g' CMakeLists.txt; \
	sed -i 's:mips-linux-gcc:gcc:g' $(CURDIR)/libubox/toolchain.cmake; \
	cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_SYSTEM_NAME=Linux; \
	cmake -DBUILD_LUA=OFF -DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/libubox/toolchain.cmake -DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(OUT_DIR)/libubox \
	-DCMAKE_C_FLAGS=" -I$(CURDIR)/$(OUT_DIR)/jsonc/include  -L$(CURDIR)/$(OUT_DIR)/jsonc/lib" \
	-Dblobmsg_json_LIB_DEPENDS=$(CURDIR)/$(OUT_DIR)/jsonc/lib/libjson-c.so -Djson=$(CURDIR)/$(OUT_DIR)/jsonc/lib/libjson-c.so; \
	make; \
	make install;

printVar:
	@echo $(OUT_DIR)
	@echo $(PWD)
	@echo $(CURDIR)
#
# OUTPUT: libjson-c.la
#
build-jsonc:
	cd jsonc; \
	./autogen.sh; \
	ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ./configure CFLAGS=-w --prefix=$(CURDIR)/$(OUT_DIR)/jsonc; \
	make; \
	make install;



$(OUT_DIR):
	mkdir -p $@

distclean:
	@rm -f test
	@if [ -d libubox ]; then \
		rm -rf libubox; \
	fi
	@if [ -d jsonc ]; then \
		rm -rf jsonc; \
	fi


#		if [ $$i == $(UBOX_VER) ]; \
#			then make check-libubox-file; \
#			cd $(UBOX_VER); \
#			echo "Ubox"; \
#			sed -i 's:ProjectApp:$(APP_BASE_DIR):g' CMakeLists.txt; \
#			sed -i 's:ProjectInstall:$(APP_INSTALL_BASE_DIR):g' CMakeLists.txt; \
#			sed -i 's:mips-linux-gcc:$(CROSS_COMPILE)gcc:g' CMakeLists.txt; \
#			sed -i 's:mips-linux-gcc:$(CROSS_COMPILE)gcc:g' $(APP_BASE_DIR)/libubox/toolchain.cmake; \
#			cmake -DCMAKE_C_COMPILER=$(CROSS_COMPILE)gcc -DCMAKE_SYSTEM_NAME=Linux;\
#			if [ "json-c-0.18" == "${JSON_VER}" ]; then \
#				cmake -DBUILD_LUA=OFF -DCMAKE_TOOLCHAIN_FILE=$(APP_BASE_DIR)/libubox/toolchain.cmake -DCMAKE_INSTALL_PREFIX=$(APP_INSTALL_BASE_DIR)/libubox \
#            	-DCMAKE_C_FLAGS=" -I$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/include/json-c  -L$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/lib" \
#            	-Dblobmsg_json_LIB_DEPENDS=$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/lib/libjson-c.so -Djson=$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/lib/libjson-c.so; \
#			else \
#				cmake -DBUILD_LUA=OFF -DCMAKE_TOOLCHAIN_FILE=$(APP_BASE_DIR)/libubox/toolchain.cmake -DCMAKE_INSTALL_PREFIX=$(APP_INSTALL_BASE_DIR)/libubox \
#            	-DCMAKE_C_FLAGS=" -I$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/include  -L$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/lib" \
#            	-Dblobmsg_json_LIB_DEPENDS=$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/lib/libjson-c.so -Djson=$(APP_INSTALL_BASE_DIR)/$(JSON_VER)/lib/libjson-c.so; \
#			fi; \
#			cd ../ ; \
#		fi;\


