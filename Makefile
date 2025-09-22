UNAME_S := $(shell uname -s)
CXX ?= g++
CXXFLAGS += -std=c++17 -O2 -Wall -Wextra
TARGET := triangle

GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null)
GLFW_LIBS   := $(shell pkg-config --libs glfw3   2>/dev/null)

ifeq ($(UNAME_S),Darwin)
	CXX := clang++
	CXXFLAGS += $(GLFW_CFLAGS)
	LDFLAGS  += $(GLFW_LIBS) -framework OpenGL
else
	CXXFLAGS += -DGLFW_INCLUDE_NONE
	ifneq ($(GLFW_CFLAGS),)
		CXXFLAGS += $(GLFW_CFLAGS)
		LDFLAGS  += $(GLFW_LIBS)
	else
		ifeq ($(OS),Windows_NT)
			LDFLAGS += -lglfw3 -lopengl32 -lgdi32
		else
			LDFLAGS += -lglfw -lGL -ldl -lpthread -lm
		endif
	endif
endif

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: run clean
run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)

