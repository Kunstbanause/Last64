BUILD_DIR=build_test
T3D_INST=$(shell realpath ..)

include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

N64_CXXFLAGS += -std=gnu++20 -Os -fno-exceptions

PROJECT_NAME=textureTest

src = src/tests/textureTest.cpp src/render/textureUtils.cpp

all: $(PROJECT_NAME).z64

$(BUILD_DIR)/$(PROJECT_NAME).elf: $(src:%.cpp=$(BUILD_DIR)/%.o)

$(PROJECT_NAME).z64: N64_ROM_TITLE="Texture Test"

clean:
	rm -rf $(BUILD_DIR) *.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean