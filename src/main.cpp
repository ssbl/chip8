#include "chip8.hpp"

#include <SDL.h>

#include <cstdio>
#include <iostream>

int
main(const int argc, const char* argv[])
{
  if (argc != 2) {
    std::printf("usage\n");
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
        const auto keyval = event.type == SDL_KEYUP ? 0 : 1;
        switch (event.key.keysym.sym) {
          case SDLK_1: // 1
            chip8.key[0x1] = keyval;
            break;
          case SDLK_2: // 2
            chip8.key[0x2] = keyval;
            break;
          case SDLK_3: // 3
            chip8.key[0x3] = keyval;
            break;
          case SDLK_4: // C
            chip8.key[0xc] = keyval;
            break;
          case SDLK_q: // 4
            chip8.key[0x4] = keyval;
            break;
          case SDLK_w: // 5
            chip8.key[0x5] = keyval;
            break;
          case SDLK_e: // 6
            chip8.key[0x6] = keyval;
            break;
          case SDLK_r: // D
            chip8.key[0xd] = keyval;
            break;
          case SDLK_a: // 7
            chip8.key[0x7] = keyval;
            break;
          case SDLK_s: // 8
            chip8.key[0x8] = keyval;
            break;
          case SDLK_d: // 9
            chip8.key[0x9] = keyval;
            break;
          case SDLK_f: // E
            chip8.key[0xe] = keyval;
            break;
          case SDLK_z: // A
            chip8.key[0xa] = keyval;
            break;
          case SDLK_x: // 0
            chip8.key[0x0] = keyval;
            break;
          case SDLK_c: // B
            chip8.key[0xb] = keyval;
            break;
          case SDLK_v: // F
            chip8.key[0xf] = keyval;
            break;
          default:
            break;
        }
      }
    }

    if (chip8.draw_flag) {
      SDL_RenderClear(renderer);
      auto copy = chip8.framebuffer;
      SDL_UpdateTexture(texture, nullptr, copy.data(), 64 * sizeof(unsigned));
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
