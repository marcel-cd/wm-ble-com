# change this to match your environment
arm_toolchain=${HOME}/_bin/gcc-arm-none-eabi-10-2020-q4-major/bin/

# Boards compatible with this app
TARGET_BOARDS := pca10100
# we neede a custom flash layout for uploading the scratchpad
INI_FILE_APP = $(APP_SRCS_PATH)pca10100_scratchpad.ini

default_applib_log_level ?= 3
use_debug_print_module=yes

default_network_address ?= 0x000042
default_network_channel ?= 3

app_specific_area_id=0x1bde21
app_major=1
app_minor=1
app_maintenance=0
app_development=0
