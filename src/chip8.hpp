#ifndef CHIP8_HPP_INCLUDED
#define CHIP8_HPP_INCLUDED

#include <SDL.h>

#include <array>
#include <cstdint>

enum class Color : std::uint32_t
{
  BLACK = 0xff000000,
  WHITE = 0xffffffff,
};

struct Chip8
{
  std::uint16_t opcode;
  std::array<std::uint8_t, 4096> memory;
  std::array<std::uint8_t, 16> V;
  std::uint16_t i;
  std::uint16_t pc;
  std::array<Color, 64 * 32> framebuffer;
  std::uint8_t delay_timer;
  std::uint8_t sound_timer;
  std::array<std::uint8_t, 16> key;
  std::array<std::uint16_t, 16> stack;
  std::uint16_t sp;

  bool draw_flag;
  bool halted;
  int cycles;

  // 540 Hz emulated clock rate, 60 fps
  static const int cycles_per_frame = 540 / 60;

  static constexpr std::array<std::uint8_t, 80> fontset = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  Chip8();
  bool load_rom(const char* path);
  void step();
  void clear_display();
  void update_timers();
  int translate_keycode(const SDL_Keycode keycode);
};

#endif // CHIP8_HPP_INCLUDED
