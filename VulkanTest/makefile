OUT_BIN_DIR = out/bin/
OUT_OBJ_DIR = out/obj/
INCLUDE_DIR = src/include/

CFLAGS = -std=c++17 -I$(VULKAN_SDK)/include -Isrc/include
LDFLAGS = -L$(VULKAN_SDK)/lib -lvulkan

TEMPLATE_SRC_DIR = src/template/
TEMPLATE_OBJECTS = $(OUT_OBJ_DIR)template.o $(OUT_OBJ_DIR)tools.o

TEXTURE_SRC_DIR = src/texture/
TEXTURE_OBJECTS = $(OUT_OBJ_DIR)texture.o $(OUT_OBJ_DIR)tools.o

ALL_OBJECTS = template texture

build : texture

texture : $(TEXTURE_OBJECTS)
	g++ $^ -o $(OUT_BIN_DIR)$@ $(LDFLAGS)

template : $(TEMPLATE_OBJECTS)
	g++ $^ -o $(OUT_BIN_DIR)$@ $(LDFLAGS)

$(OUT_OBJ_DIR)texture.o : $(TEXTURE_SRC_DIR)texture.cpp
	g++ -c $(CFLAGS) $< -o $@ $(LDFLAGS)

$(OUT_OBJ_DIR)template.o : $(TEMPLATE_SRC_DIR)template.cpp
	g++ -c $(CFLAGS) $< -o $@ $(LDFLAGS)

$(OUT_OBJ_DIR)tools.o : $(INCLUDE_DIR)tools.cpp
	g++ -c $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f out/bin/*
	rm -f out/obj/*.o