#include "chip8.hpp"

#include <SDL.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>

Chip8::Chip8()
  : opcode(0)
  , i(0)
  , pc(0x200)
  , delay_timer(0)
  , sound_timer(0)
  , sp(0)
  , draw_flag(true)
  , halted(false)
  , cycles(0)
{
  for (auto i = 0; i < 80; ++i)
    memory[i] = fontset[i];
  clear_display();
  key.fill(0);
}

bool
Chip8::load_rom(const char* path)
{
  std::ifstream rom_file(path, std::ios::binary | std::ios::ate);
  if (!rom_file)
    return false;

  std::size_t fsize = rom_file.tellg();
  if (fsize > sizeof memory[0] * (0x1000 - 0x200))
    return false;

  rom_file.seekg(0);
  rom_file.read(reinterpret_cast<char*>(memory.data() + 0x200), fsize);
  return true;
}

void
Chip8::clear_display()
{
  framebuffer.fill(Color::BLACK);
}

static void
unknown_instruction(const std::uint16_t opcode)
{
  std::cerr << "unknown instruction: " << opcode << '\n';
}

static std::uint8_t
get_x(const std::uint16_t opcode)
{
  return (opcode & 0x0f00) >> 8;
}

static std::uint8_t
get_y(const std::uint16_t opcode)
{
  return (opcode & 0x00f0) >> 4;
}

void
Chip8::step()
{
  opcode = (memory[pc] << 8) | memory[pc + 1];
  draw_flag = false;
  ++cycles;

  switch (opcode & 0xf000) {
    case 0x0000: {
      switch (opcode & 0x00ff) {
        case 0xe0: // cls
          clear_display();
          draw_flag = true;
          pc += 2;
          break;
        case 0xee: // ret
          assert(sp > 0);
          pc = stack[--sp];
          pc += 2;
          break;
        default: // sys addr (0x0nnn)
          pc = opcode & 0x0fff;
          break;
      }
    } break;
    case 0x1000: { // jp addr
      pc = opcode & 0x0fff;
    } break;
    case 0x2000: { // call addr
      assert(sp <= 0xf);
      stack[sp++] = pc;
      pc = opcode & 0x0fff;
    } break;
    case 0x3000: { // se Vx,byte
      pc += (V[get_x(opcode)] == (opcode & 0x00ff)) ? 4 : 2;
    } break;
    case 0x4000: { // sne Vx,byte
      pc += (V[get_x(opcode)] != (opcode & 0x00ff)) ? 4 : 2;
    } break;
    case 0x5000: { // se Vx,vy
      pc += (V[get_x(opcode)] == V[get_y(opcode)]) ? 4 : 2;
    } break;
    case 0x6000: { // ld Vx,byte
      V[get_x(opcode)] = opcode & 0x00ff;
      pc += 2;
    } break;
    case 0x7000: { // add Vx,byte
      V[get_x(opcode)] += opcode & 0x00ff;
      pc += 2;
    } break;
    case 0x8000: {
      const auto x = get_x(opcode);
      const auto y = get_y(opcode);

      switch (opcode & 0x000f) {
        case 0x0: { // ld Vx,Vy
          V[x] = V[y];
        } break;
        case 0x1: { // or Vx,Vy
          V[x] |= V[y];
        } break;
        case 0x2: { // and Vx,Vy
          V[x] &= V[y];
        } break;
        case 0x3: { // xor Vx,Vy
          V[x] ^= V[y];
        } break;
        case 0x4: { // add Vx,Vy
          const auto sum = V[x] + V[y];
          V[0xf] = sum > 0xff;
          V[x] = sum;
        } break;
        case 0x5: { // sub Vx,Vy
          V[0xf] = !(V[y] > V[x]);
          V[x] -= V[y];
        } break;
        case 0x6: { // shr Vx
          V[0xf] = V[x] & 1;
          V[x] >>= 1;
        } break;
        case 0x7: { // subn Vx,Vy
          V[0xf] = !(V[x] > V[y]);
          V[x] = V[y] - V[x];
        } break;
        case 0xe: { // shl Vx
          V[0xf] = V[x] >> 7;
          V[x] <<= 1;
        } break;
        default:
          unknown_instruction(opcode);
          return;
      }
      pc += 2;
    } break;
    case 0x9000: {
      switch (opcode & 0x000f) {
        case 0x0: { // sne Vx,Vy
          pc += (V[get_x(opcode)] != V[get_y(opcode)]) ? 4 : 2;
        } break;
        default:
          unknown_instruction(opcode);
          return;
      }
    } break;
    case 0xa000: { // ld i,addr
      i = opcode & 0x0fff;
      pc += 2;
    } break;
    case 0xb000: { // jp V0,addr
      pc = (opcode & 0x0fff) + V[0x0];
    } break;
    case 0xc000: { // rnd Vx,byte
      V[get_x(opcode)] = (std::rand() % 256) & (opcode & 0x00ff);
      pc += 2;
    } break;
    case 0xd000: { // drw Vx,Vy,nibble
      const auto x = get_x(opcode);
      const auto y = get_y(opcode);
      const auto n = opcode & 0x000f;

      V[0xf] = 0;
      for (auto sy = 0; sy < n; ++sy) {
        assert(i + sy < static_cast<int>(memory.size()));
        const auto pixel = memory[i + sy];
        for (auto bit = 0; bit < 8; ++bit) {
          const auto index =
            (V[x] + bit + (V[y] + sy) * 64) % framebuffer.size();
          // check if any bit in the new pixel has changed from 1 to 0
          if ((pixel & (0x80 >> bit)) != 0) {
            if (framebuffer[index] == Color::WHITE) {
              V[0xf] = 1;
              framebuffer[index] = Color::BLACK;
            } else {
              framebuffer[index] = Color::WHITE;
            }
          }
        }
      }

      draw_flag = true;
      pc += 2;
    } break;
    case 0xe000: {
      switch (opcode & 0x00ff) {
        case 0x9e: { // skp Vx
          assert(V[get_x(opcode)] <= 0x0f);
          pc += (key[V[get_x(opcode)]] == 1) ? 4 : 2;
        } break;
        case 0xa1: { // sknp Vx
          assert(V[get_x(opcode)] <= 0x0f);
          pc += (key[V[get_x(opcode)]] == 0) ? 4 : 2;
        } break;
        default:
          unknown_instruction(opcode);
          return;
      }
    } break;
    case 0xf000: {
      const auto x = get_x(opcode);
      switch (opcode & 0x00ff) {
        case 0x07: { // ld Vx,dt
          V[x] = delay_timer;
          pc += 2;
        } break;
        case 0x0a: { // ld Vx,k
          SDL_Event event;
          while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
              halted = true;
              return;
            }
            if (event.type == SDL_KEYDOWN) {
              const auto k = translate_keycode(event.key.keysym.sym);
              if (k == -1) {
                // Try again
                --cycles;
                return;
              }

              V[x] = k;
              pc += 2;
            }
          }
        } break;
        case 0x15: { // ld dt,Vx
          delay_timer = V[x];
          pc += 2;
        } break;
        case 0x18: { // ld st,Vx
          sound_timer = V[x];
          pc += 2;
        } break;
        case 0x1e: { // add i,Vx
          const auto sum = i + V[x];
          V[0xf] = sum > 0xff;
          i = sum;
          pc += 2;
        } break;
        case 0x29: { // ld f,Vx
          assert(V[x] <= 0x0f);
          i = V[x] * 5;
          pc += 2;
        } break;
        case 0x33: { // ld b,Vx
          assert(i + 2 < static_cast<std::uint16_t>(memory.size()));
          memory[i + 2] = V[x] % 10;
          memory[i + 1] = (V[x] / 10) % 10;
          memory[i] = V[x] / 100;
          pc += 2;
        } break;
        case 0x55: { // ld [i],Vx
          assert(i + x < memory.size());
          for (auto reg = 0; reg <= x; ++reg)
            memory[i + reg] = V[reg];
          i += x + 1;
          pc += 2;
        } break;
        case 0x65: { // ld Vx,[i]
          assert(i + x < memory.size());
          for (auto byte = 0; byte <= x; ++byte)
            V[byte] = memory[i + byte];
          i += x + 1;
          pc += 2;
        } break;
        default:
          unknown_instruction(opcode);
          return;
      }
    } break;
    default:
      unknown_instruction(opcode);
      return;
  }
}

void
Chip8::update_timers()
{
  if (cycles < cycles_per_frame)
    return;

  cycles -= cycles_per_frame;
  if (delay_timer > 0)
    --delay_timer;
  if (sound_timer > 0)
    --sound_timer;
}

int
Chip8::translate_keycode(const SDL_Keycode keycode)
{
  switch (keycode) {
    case SDLK_1:
      return 0x1;
    case SDLK_2:
      return 0x2;
    case SDLK_3:
      return 0x3;
    case SDLK_4:
      return 0xc;
    case SDLK_q:
      return 0x4;
    case SDLK_w:
      return 0x5;
    case SDLK_e:
      return 0x6;
    case SDLK_r:
      return 0xd;
    case SDLK_a:
      return 0x7;
    case SDLK_s:
      return 0x8;
    case SDLK_d:
      return 0x9;
    case SDLK_f:
      return 0xe;
    case SDLK_z:
      return 0xa;
    case SDLK_x:
      return 0x0;
    case SDLK_c:
      return 0xb;
    case SDLK_v:
      return 0xf;
    default:
      return -1;
  }
}
