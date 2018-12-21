OUT_BIN_DIR = out/bin/
OUT_OBJ_DIR = out/obj/
INCLUDE_DIR = src/include/

CFLAGS = -std=c++17 -I$(VULKAN_SDK)/include -Isrc/include
LDFLAGS = -L$(VULKAN_SDK)/lib -lvulkan

TEMPLATE_SRC_DIR = src/template/
TEMPLATE_OBJECTS = $(OUT_OBJ_DIR)template.o $(OUT_OBJ_DIR)tools.o

build : $(TEMPLATE_OBJECTS)
	g++ -o $(OUT_BIN_DIR)template $(TEMPLATE_OBJECTS) $(LDFLAGS)

$(OUT_OBJ_DIR)tools.o : $(INCLUDE_DIR)tools.cpp
	g++ -c $(CFLAGS) $< -o $@ $(LDFLAGS)

$(OUT_OBJ_DIR)template.o : $(TEMPLATE_SRC_DIR)template.cpp
	g++ -c $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f out/bin/*
	rm -f out/obj/*.o