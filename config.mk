export BUILD_ROOT = $(shell pwd)

export INCLUDE_PATH = $(BUILD_ROOT)/_include

BUILD_DIR = $(BUILD_ROOT)/signal/ \
			$(BUILD_ROOT)/app/

# Whether to generate debug information during compilation.
export TARGET = debug 