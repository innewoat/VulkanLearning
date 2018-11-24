CFLAGS = -std=c++17 -I$(VULKAN_SDK)/include
LDFLAGS = -L$(VULKAN_SDK)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: main.o
	g++ $(CFLAGS) -o VulkanTest main.o $(LDFLAGS)

main.o: main.cpp
	g++ -g -c $(CFLAGS) main.cpp $(LDFLAGS)

.PHONY: clean

clean:
	rm -f VulkanTest