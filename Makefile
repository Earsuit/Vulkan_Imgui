CFLAGS = -std=c++17
LDFLAGS = `pkg-config --static --libs glfw3` -lvulkan

INC_DIR = ./inc ./imgui/inc
INC =$(foreach d, $(INC_DIR), -I$d)
HEADER = $(foreach d, $(INC_DIR), $(wildcard $d/*.h))
SOURCE = $(wildcard src/*.cpp imgui/src/*.cpp *.cpp)

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest $(SOURCE) $(INC) $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	/usr/share/vulkan/explicit_layer.d ./VulkanTest

clean:
	rm -f VulkanTest

.PHONY: clang-format
clang-format:
	/bin/bash ./clang-format-wrapper.sh $(SOURCE) $(HEADER)
	clang-format -i -style=file $(SOURCE) $(HEADER)