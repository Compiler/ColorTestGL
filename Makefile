
UNAME_S := $(shell uname -s)
CXX ?= g++
CXXFLAGS += -std=c++17 -O2 -Wall -Wextra
TARGET := triangle

GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null)
GLFW_LIBS   := $(shell pkg-config --libs glfw3 2>/dev/null)
OCIO_CFLAGS := $(shell pkg-config --cflags OpenColorIO 2>/dev/null)
OCIO_LIBS   := $(shell pkg-config --libs OpenColorIO 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
	CXX := clang++
	CXXFLAGS += $(GLFW_CFLAGS) $(OCIO_CFLAGS)
	LDFLAGS  += $(GLFW_LIBS) $(OCIO_LIBS) -framework OpenGL
else
	CXXFLAGS += -DGLFW_INCLUDE_NONE $(GLFW_CFLAGS) $(OCIO_CFLAGS)
	ifneq ($(GLFW_LIBS),)
		LDFLAGS  += $(GLFW_LIBS) $(OCIO_LIBS)
	else
		ifeq ($(OS),Windows_NT)
			LDFLAGS += -lglfw3 -lopengl32 -lgdi32 $(OCIO_LIBS)
		else
			LDFLAGS += -lglfw -lGL -ldl -lpthread -lm $(OCIO_LIBS)
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

