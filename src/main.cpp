#include "chip8.hpp"

#include <cstdio>

int
main(const int argc, const char* argv[])
{
  if (argc != 2)
    std::printf("usage\n");

  Chip8 chip8;

  // TODO: setup SDL
  // TODO: setup sound

  if (!chip8.load_rom(argv[1]))
    std::printf("error loading file: %s\n", argv[1]);

  for (auto i = 80; i < 0x200; ++i) {
    if (chip8.memory[i]) {
      std::puts("failed");
      return 1;
    }
  }
  for (auto i = 0; i < 80; ++i) {
    if (chip8.memory[i] != chip8.fontset[i]) {
      std::puts("fontset mismatch");
      return 1;
    }
  }

  // while (true) {
  //   chip8.step();
  //   if (chip8.draw_flag)
  //     chip8.update_screen();
  //   chip8.set_keys();
  // }
}
