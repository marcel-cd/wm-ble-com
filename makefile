SHELL := /bin/bash
.PHONY: build flash clean_all
.PHONY: copyright-add copyright-remove

####################################################################################################################
# Settings build
APP_NAME ?= app

##### change this settings
MKDIR  = mkdir -p
COPY  = cp -a
CLEANUP = rm -f
RC ?= /usr/bin/rc

BOARD ?= pca10100
JLINK_SERIAL1 :=  685689088


####################################################################################################################
# Wirepas SDK Setting
#
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=6
APP_PERSISTENT=yes

SHARED_DATA=yes
SHARED_APP_CONFIG=yes
SHARED_APP_CONFIG_FILTERS=1
SHARED_OFFLINE=no
SHARED_OFFLINE_MODULES=0
SHARED_NEIGHBORS=yes
SHARED_NEIGHBORS_CBS=1
SHARED_BEACON=yes
# App need it to register for stop stack event
STACK_STATE_LIB=yes
STACK_STATE_CBS=1

# Wirepas HAL #################################################################
HAL_LED=yes
HAL_BUTTON=yes
HAL_UART=yes
UART_USE_DMA=yes
UART_USE_USB=no

# Settings compilation
COPYRIGHT_CMD ?= $(HOME)/.local/share/gem/ruby/3.0.0/bin/copyright-header

# Adding ble scanner module
SRCS += \
    $(SRCS_PATH)src/app.c \
    $(SRCS_PATH)src/app_app.c \
    $(SRCS_PATH)src/app_settings.c \
    $(SRCS_PATH)src/sm.c \
    $(SRCS_PATH)src/otap.c \
    $(SRCS_PATH)src/mod/fsm.c \
    $(SRCS_PATH)src/mod/ble.c \

# needed for UART
SRCS += $(UTIL_PATH)syscalls.c

INCLUDES += -I$(SRCS_PATH)/src \
			-I$(SRCS_PATH)/src/mod \

# debug applib
CFLAGS += -DDEBUG_APP_LOG_MAX_LEVEL=$(default_applib_log_level)

# Network settings
CFLAGS += -DCONF_NETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DCONF_NETWORK_CHANNEL=$(default_network_channel)
CFLAGS += -DCONF_ROLE=$(default_role)

#Device class & mode
CFLAGS += -DAPP_DEVICE_MODE=$(default_app_device_mode)

# CFLAGS += -DDEBUG_LOG_UART_BAUDRATE=115200
CFLAGS += -DDEBUG_LOG_UART_BAUDRATE=460800

# Persistent memory
ifeq ($(use_debug_print_module),yes)
CFLAGS += -DUSE_DEBUG_PRINT_MODULE
APP_PRINTING=yes
USE_DEBUG_PRINT_MODULE=yes
endif


WMSDK_BASE := wm-sdk/
BUILDDIR := build/$(BOARD)/$(APP_NAME)

ifndef WMSDK_BASE
$(error The WMSDK_BASE environment variable must be set)
endif

ifeq ($(development_mode),yes)
CFLAGS += -DDEVELOPMENT_MODE
endif

# Copyright
# ---------------------------------------------------------------------------
SHELL := /bin/bash

COPYRIGHT_LICENSE ?= license_header
COPYRIGHT_OUTPUT_DIR ?= .
COPYRIGHT_WORD_WRAP ?= 120
COPYRIGHT_PATHS ?= src:test

copyright-remove:
	$(COPYRIGHT_CMD) \
	  --license-file $(COPYRIGHT_LICENSE)  \
	  --remove-path $(COPYRIGHT_PATHS) \
	  --guess-extension \
	  --word-wrap $(COPYRIGHT_WORD_WRAP) \
	  --output-dir $(COPYRIGHT_OUTPUT_DIR)
copyright-add:
	$(COPYRIGHT_CMD) \
	  --license-file $(COPYRIGHT_LICENSE)  \
	  --add-path $(COPYRIGHT_PATHS) \
	  --guess-extension \
	  --word-wrap $(COPYRIGHT_WORD_WRAP) \
	  --output-dir $(COPYRIGHT_OUTPUT_DIR)


# targets
# ---------------------------------------------------------------------------
build: cleanall
	cd ${WMSDK_BASE} && touch .license_accepted && mkdir -p ${BUILDDIR} && bear --output source/${APP_NAME}/compile_commands.json --	make app_name=${APP_NAME} target_board=${BOARD}

cleanall:
	cd ${WMSDK_BASE} &&	make clean_all target_board=${BOARD} app_name=${APP_NAME}

flash:
	nrfjprog -f NRF52 --recover --snr ${JLINK_SERIAL1}
	nrfjprog -f NRF52 --snr ${JLINK_SERIAL1} --program ${WMSDK_BASE}/${BUILDDIR}/final_image_$(APP_NAME).hex --chiperase --reset --verify

