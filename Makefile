CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-missing-field-initializers -Ivendor/raylib-5.5_linux_amd64/include -Isrc
LDFLAGS  := vendor/raylib-5.5_linux_amd64/lib/libraylib.a /usr/lib64/libGL.so.1 /usr/lib64/libX11.so.6 -lm -lpthread -ldl -lrt

TARGET   := phantom_forces
SRCS     := src/main.cpp

all: $(TARGET)

$(TARGET): $(SRCS) src/math_springs.hpp src/audio_synth.hpp src/weapons.hpp src/maps.hpp src/entities.hpp src/game_world.hpp src/hud_and_menu.hpp
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

test: $(TARGET)
	./$(TARGET) --self-test

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
