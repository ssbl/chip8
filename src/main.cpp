#include "chip8.hpp"

#include <SDL.h>

#include <cstdio>
#include <iostream>

int
main(const int argc, const char* argv[])
{
  if (argc != 2) {
    std::printf("usage: chip8 <path-to-rom-file>\n");
    return 1;
  }

  Chip8 chip8;

  if (!chip8.load_rom(argv[1])) {
    std::printf("error loading file: %s\n", argv[1]);
    return 1;
  }

  // setup SDL
  const int width = 640;
  const int height = 320;
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;

  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    std::cerr << "SDL error: " << SDL_GetError() << '\n';
    goto cleanup;
  }

  window = SDL_CreateWindow("CHIP-8 Emulator",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            width,
                            height,
                            SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "SDL error: " << SDL_GetError() << '\n';
    goto cleanup;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "SDL error: " << SDL_GetError() << '\n';
    goto cleanup;
  }

  texture = SDL_CreateTexture(
    renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
  if (!texture) {
    std::cerr << "SDL error: " << SDL_GetError() << '\n';
    goto cleanup;
  }

  while (!chip8.halted) {
    chip8.step();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto cleanup;
      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        const auto key = chip8.translate_keycode(event.key.keysym.sym);
        if (key != -1) {
          const auto keyval = event.type == SDL_KEYUP ? 0 : 1;
          chip8.key[key] = keyval;
        }
      }
    }

    if (chip8.draw_flag) {
      SDL_RenderClear(renderer);
      SDL_UpdateTexture(texture,
                        nullptr,
                        chip8.framebuffer.data(),
                        64 * sizeof(decltype(chip8.framebuffer)::value_type));
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);
    }

    chip8.update_timers();

    SDL_Delay(2);
  }

cleanup:
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
