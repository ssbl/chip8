#include "chip8.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

Chip8::Chip8()
  : opcode(0)
  , i(0)
  , pc(0x200)
  , delay_timer(0)
  , sound_timer(0)
  , sp(0)
  , draw_flag(false)
{
  for (auto i = 0; i < 80; ++i)
    memory[i] = fontset[i];
  clear_display();
}

bool
Chip8::load_rom(const char* path)
{
  std::ifstream rom_file(path, std::ios::binary | std::ios::ate);
  if (!rom_file)
    return false;

  auto fsize = static_cast<std::size_t>(rom_file.tellg());
  if (fsize > sizeof memory[0] * (0x1000 - 0x200))
    return false;
  rom_file.seekg(0);
  rom_file.read(reinterpret_cast<char*>(memory.data() + 0x200), fsize);

  return true;
}

void
Chip8::clear_display()
{
  std::fill_n(framebuffer.begin(), 64 * 32, Color::BLACK);
}

static void
unknown_instruction(const unsigned short opcode)
{
  std::cerr << "unknown instruction: " << opcode << '\n';
}

static unsigned char
get_x(const unsigned short opcode)
{
  return (opcode & 0x0f00) >> 8;
}

static unsigned char
get_y(const unsigned short opcode)
{
  return (opcode & 0x00f0) >> 4;
}

void
Chip8::step()
{
  opcode = (memory[pc] << 8) | memory[pc + 1];
  // std::cout << std::hex << opcode << '\n';
  draw_flag = false;

  switch (opcode & 0xf000) {
    case 0x0000: {
      switch (opcode & 0x00ff) {
        case 0xe0: // cls
          clear_display();
          pc += 2;
          break;
        case 0xee: // ret
          pc = stack[--sp];
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
          auto sum = V[x] + V[y];
          V[0xf] = sum > 0xff;
          V[x] = sum;
        } break;
        case 0x5: { // sub Vx,Vy
          V[0xf] = V[x] >= V[y];
          V[x] -= V[y];
        } break;
        case 0x6: { // shr Vx
          V[0xf] = V[x] & 1;
          V[x] /= 2;
        } break;
        case 0x7: { // subn Vx,Vy
          V[0xf] = V[y] > V[x];
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
      pc = (opcode & 0x0fff) + V[0];
    } break;
    case 0xc000: { // rnd Vx,byte
      const auto x = (opcode & 0x0f00) >> 8;
      V[x] = 99 & (opcode & 0x00ff);
      pc += 2;
    } break;
    case 0xd000: { // drw Vx,Vy,nibble
      // TODO: bounds checking?
      const auto x = get_x(opcode);
      const auto y = get_y(opcode);
      const auto n = opcode & 0x000f;

      V[0xf] = 0;
      for (auto sy = 0; sy < n; ++sy) {
	const auto pixel = memory[i + sy];
        for (auto bit = 0; bit < 8; ++bit) {
	  const auto index = x + bit + (y + sy) * 64;
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

        draw_flag = true;
        pc += 2;
      }
    } break;
    case 0xe000: {
      switch (opcode & 0x00ff) {
        case 0x9e: { // skp Vx
          assert(V[get_x(opcode)] <= 0x0f);
          pc += (key[V[get_x(opcode)]] == 1) ? 4 : 2;
        } break;
        case 0xa1: { // sknp Vx
          pc += (key[V[get_x(opcode)]] == 0) ? 4 : 2;
        } break;
        default:
          unknown_instruction(opcode);
          return;
      }
    } break;
    case 0xf000: {
      const auto x = (opcode & 0x0f00) >> 8;
      switch (opcode & 0x00ff) {
        case 0x07: { // ld Vx,dt
          V[x] = delay_timer;
          pc += 2;
        } break;
        case 0x0a: { // ld Vx,k
	  // TODO: store the correct key in Vx here
          std::cin >> V[x];
          pc += 2;
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
          i += V[x];
          pc += 2;
        } break;
        case 0x29: { // ld f,Vx
          assert(V[x] <= 0x0f);
          i = V[x] * 5;
          pc += 2;
        } break;
        case 0x33: { // ld b,Vx
          assert(i + 2 < 0x1000);
          memory[i + 2] = V[x] % 10;
          memory[i + 1] = (V[x] / 10) % 10;
          memory[i] = V[x] / 100;
          pc += 2;
        } break;
        case 0x55: { // ld [i],Vx
          assert(i + x < 0x1000);
          for (auto reg = 0; reg <= x; ++reg)
            memory[i + reg] = V[reg];
          pc += 2;
        } break;
        case 0x65: { // ld Vx,[i]
          assert(i + x < 0x1000);
          for (auto byte = 0; byte <= x; ++byte)
            V[byte] = memory[i + byte];
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

  if (delay_timer > 0) {
    --delay_timer;
  }
  if (sound_timer > 0) {
    // TODO: beep
    --sound_timer;
  }
}
