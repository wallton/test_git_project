
# 定義多個目錄
DIR_OUTPUT := buildL
PREFIX := $(CURDIR)/$(DIR_OUTPUT)
UCLIBC_DIR := uclibc-ng
GIT_URL := https://github.com/wbx-github/uclibc-ng.git
KERNEL_INC_DIR := /tmp/kernel-headers
KERNEL_VER_MAJOR := $(shell uname -r | cut -d. -f1)
KERNEL_VER := $(shell uname -r | cut -d. -f1-2)

.PHONY: all config build install clean distclean
all: $(DIR_OUTPUT) build



# -nostdlib：避免使用系統的 glibc。
# -I 和 -L ：指定 uClibc-ng 的頭文件和庫路徑。
# -Wl,--dynamic-linker：指定動態鏈接器。
#
test:
	gcc -o $@ test.c -I$(PREFIX)/uclibc-ng/dev/include -L$(PREFIX)/uclibc-ng/dev/lib -lc -nostdlib -Wl,--dynamic-linker=$(PREFIX)/uclibc-ng/run/lib/ld64-uClibc.so.0


demo:
	@echo PREFIX = $(PREFIX)
	@echo KERNEL_VER = $(KERNEL_VER)
	@echo KERNEL_VER_MAJOR = $(KERNEL_VER_MAJOR)


# 檢查並創建目錄
$(DIR_OUTPUT):
	@mkdir -p $@

# @@@安裝全過程 https://github.com/wbx-github/uclibc-ng/blob/master/INSTALL
# 
# wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.tar.xz
# tar -xf linux-5.15.tar.xz
# cd linux-5.15
# make headers_install INSTALL_HDR_PATH=/tmp/kernel-headers

# 
# git clone https://github.com/wbx-github/uclibc-ng.git
# cd uclibc-ng
# make menuconfig
# make
# make install




# kernel.org 的源代碼位於 https://cdn.kernel.org/pub/linux/kernel/vX.x/，其中 X.x 是主版本號（如 v5.x）。
# 文件名格式為 linux-<version>.tar.xz，需要動態替換版本號。
prerequisite:
	@if [ ! -f linux-$(KERNEL_VER).tar.xz ]; then \
		echo "Download linux-$(KERNEL_VER).tar.xz"; \
		wget https://cdn.kernel.org/pub/linux/kernel/v$(KERNEL_VER_MAJOR).x/linux-$(KERNEL_VER).tar.xz; \
	fi;
	@if [ ! -d linux-$(KERNEL_VER) ]; then \
		tar -xf linux-$(KERNEL_VER).tar.xz; \
	fi;
	@if [ ! -d $(KERNEL_INC_DIR) ]; then \
		cd linux-$(KERNEL_VER); \
		echo "Copy headers to $(KERNEL_INC_DIR)"; \
		make headers_install INSTALL_HDR_PATH=$(KERNEL_INC_DIR); \
	fi;



# Download from github
#
#$(UCLIBC_DIR):
#	@echo "cloning uClibc-ng repository..."
#	@git clone $(GIT_URL) $(UCLIBC_DIR)
#	@cd $(UCLIBC_DIR)

# Download from https://downloads.uclibc-ng.org/releases/1.0.52/
#
$(UCLIBC_DIR):
	@echo "untar uClibc-ng repository..."
	tar Jxvf uClibc-ng-1.0.52.tar.xz ; \
	mv -i uClibc-ng-1.0.52 $(UCLIBC_DIR)

# 配置 uClibc-ng

#.config [ KERNEL_HEADERS="/tmp/kernel-headers/include" ]
#
#    Target Architecture Features and Options  --->
#        ()  Linux kernel header location

#.config [ ]
#
#
#
menuconfig: $(UCLIBC_DIR)
	@echo "Configuring uClibc-ng..."
	@cd $(UCLIBC_DIR) && make menuconfig
	@echo "Please configure the target architecture and save the config."

# 編譯 uClibc-ng
build: $(UCLIBC_DIR) prerequisite
	@echo "Building uClibc-ng..."
	@cd $(UCLIBC_DIR) && make CFLAGS="-I$(KERNEL_INC_DIR)/include -I$(CURDIR)/$(UCLIBC_DIR)/include"

# 安裝 uClibc-ng 到指定路徑
#  DEVEL_PREFIX  ：開發文件（頭文件、靜態庫）的安裝路徑。
#  RUNTIME_PREFIX：運行時文件（動態庫）的安裝路徑
install: $(PREFIX)
	@echo "Installing uClibc-ng to $(PREFIX)..."
	@cd $(UCLIBC_DIR) && make PREFIX=$(PREFIX) DEVEL_PREFIX=/uclibc-ng/dev/ RUNTIME_PREFIX=/uclibc-ng/run/ install

# 清理
clean:
	@echo "Cleaning uClibc-ng build..."
	@cd $(UCLIBC_DIR) && make clean
	@for dir in $(DIR_OUTPUT); do \
		if test -d $$dir; then \
			rm -r $$dir; \
			echo "Removed $$dir"; \
		fi; \
	done


# 完全移除
distclean:
	@echo "Removing uClibc-ng directory..."
	@if [ -n "$(UCLIBC_DIR)" ]; then \
		rm -rf $(UCLIBC_DIR); \
		echo "Removed: $(UCLIBC_DIR)"; \
	fi

