CFLAGS = -std=c++17
LDFLAGS = `pkg-config --static --libs glfw3` -lvulkan

INCLUDE = HelloTriangleApplication.hpp
SOURCE = HelloTriangleApplication.cpp main.cpp

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest $(SOURCE) -I.$(INCLUDE) $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	/usr/share/vulkan/explicit_layer.d ./VulkanTest

clean:
	rm -f VulkanTest

.PHONY: clang-format
clang-format:
	/bin/bash ./clang-format-wrapper.sh $(SOURCE) $(INCLUDE)
	clang-format -i -style=file $(SOURCE)