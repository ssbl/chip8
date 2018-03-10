#include "SDL2/SDL.h"
#include "chip8.hpp"

#include <cstdio>
#include <iostream>

int
main(const int argc, const char* argv[])
{
  if (argc != 2)
    std::printf("usage\n");

  Chip8 chip8;

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
                            SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
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

  // TODO: setup sound

  if (!chip8.load_rom(argv[1]))
    std::printf("error loading file: %s\n", argv[1]);

  while (true) {
    chip8.step();

    // update display
    if (chip8.draw_flag) {
      SDL_RenderClear(renderer);
      // the nullptrs indicate a full update of the texture
      SDL_UpdateTexture(texture, nullptr, chip8.framebuffer.data(), 4 * 64);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);
    }

    // TODO: set key presses
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto cleanup;
    }

    SDL_Delay(50);
  }

cleanup:
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
