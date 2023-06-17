export BUILD_ROOT = $(shell pwd)

export INCLUDE_PATH = $(BUILD_ROOT)/_include
export INCLUDE_PROTO = $(BUILD_ROOT)/generated

BUILD_DIR = $(BUILD_ROOT)/signal/ \
			$(BUILD_ROOT)/proc/ \
			$(BUILD_ROOT)/net/ \
			$(BUILD_ROOT)/misc/ \
			$(BUILD_ROOT)/generated/ \
			$(BUILD_ROOT)/business/ \
			$(BUILD_ROOT)/app/

# Whether to generate debug information during compilation.
export TARGET = debug 