CXX = x86_64-w64-mingw32-gcc
override CPPFLAGS = -I./detours/src
WINRES ?= x86_64-w64-mingw32-windres
BITS ?= -m64
TARGET= fix_scrollbug.dll
TARGET_DEPS = fix_scrollbug.o \
		fix_scrollbug_versioninfo.o

all: $(TARGET)

%.o : %.rc
	$(WINRES) $< -o $@

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $< -o $@

$(TARGET): $(TARGET_DEPS)
	$(CXX) $< -o $@
