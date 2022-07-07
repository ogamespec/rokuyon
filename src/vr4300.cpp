/*
    Copyright 2022 Hydr8gon

    This file is part of rokuyon.

    rokuyon is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    rokuyon is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with rokuyon. If not, see <https://www.gnu.org/licenses/>.
*/

#include <cstring>

#include "vr4300.h"
#include "cp0.h"
#include "log.h"
#include "memory.h"

namespace VR4300
{
    uint64_t registersR[33];
    uint64_t *registersW[32];
    uint64_t hi, lo;
    uint32_t programCounter;
    uint32_t nextOpcode;

    extern void (*immInstrs[])(uint32_t);
    extern void (*regInstrs[])(uint32_t);
    extern void (*extInstrs[])(uint32_t);

    void j(uint32_t opcode);
    void jal(uint32_t opcode);
    void beq(uint32_t opcode);
    void bne(uint32_t opcode);
    void blez(uint32_t opcode);
    void bgtz(uint32_t opcode);
    void addi(uint32_t opcode);
    void addiu(uint32_t opcode);
    void slti(uint32_t opcode);
    void sltiu(uint32_t opcode);
    void andi(uint32_t opcode);
    void ori(uint32_t opcode);
    void xori(uint32_t opcode);
    void lui(uint32_t opcode);
    void beql(uint32_t opcode);
    void bnel(uint32_t opcode);
    void blezl(uint32_t opcode);
    void bgtzl(uint32_t opcode);
    void daddi(uint32_t opcode);
    void daddiu(uint32_t opcode);
    void ldl(uint32_t opcode);
    void ldr(uint32_t opcode);
    void lb(uint32_t opcode);
    void lh(uint32_t opcode);
    void lwl(uint32_t opcode);
    void lw(uint32_t opcode);
    void lbu(uint32_t opcode);
    void lhu(uint32_t opcode);
    void lwr(uint32_t opcode);
    void lwu(uint32_t opcode);
    void sb(uint32_t opcode);
    void sh(uint32_t opcode);
    void swl(uint32_t opcode);
    void sw(uint32_t opcode);
    void sdl(uint32_t opcode);
    void sdr(uint32_t opcode);
    void swr(uint32_t opcode);
    void ld(uint32_t opcode);
    void sd(uint32_t opcode);

    void sll(uint32_t opcode);
    void srl(uint32_t opcode);
    void sra(uint32_t opcode);
    void sllv(uint32_t opcode);
    void srlv(uint32_t opcode);
    void srav(uint32_t opcode);
    void jr(uint32_t opcode);
    void jalr(uint32_t opcode);
    void mfhi(uint32_t opcode);
    void mthi(uint32_t opcode);
    void mflo(uint32_t opcode);
    void mtlo(uint32_t opcode);
    void dsllv(uint32_t opcode);
    void dsrlv(uint32_t opcode);
    void dsrav(uint32_t opcode);
    void mult(uint32_t opcode);
    void multu(uint32_t opcode);
    void div(uint32_t opcode);
    void divu(uint32_t opcode);
    void dmult(uint32_t opcode);
    void dmultu(uint32_t opcode);
    void ddiv(uint32_t opcode);
    void ddivu(uint32_t opcode);
    void add(uint32_t opcode);
    void addu(uint32_t opcode);
    void sub(uint32_t opcode);
    void subu(uint32_t opcode);
    void and_(uint32_t opcode);
    void or_(uint32_t opcode);
    void xor_(uint32_t opcode);
    void nor(uint32_t opcode);
    void slt(uint32_t opcode);
    void sltu(uint32_t opcode);
    void dadd(uint32_t opcode);
    void daddu(uint32_t opcode);
    void dsub(uint32_t opcode);
    void dsubu(uint32_t opcode);
    void dsll(uint32_t opcode);
    void dsrl(uint32_t opcode);
    void dsra(uint32_t opcode);
    void dsll32(uint32_t opcode);
    void dsrl32(uint32_t opcode);
    void dsra32(uint32_t opcode);

    void bltz(uint32_t opcode);
    void bgez(uint32_t opcode);
    void bltzl(uint32_t opcode);
    void bgezl(uint32_t opcode);
    void bltzal(uint32_t opcode);
    void bgezal(uint32_t opcode);
    void bltzall(uint32_t opcode);
    void bgezall(uint32_t opcode);

    void eret(uint32_t opcode);
    void mfc0(uint32_t opcode);
    void mtc0(uint32_t opcode);

    void cop(uint32_t opcode);
    void unk(uint32_t opcode);
}

// Immediate-type instruction lookup table, using opcode bits 26-31
void (*VR4300::immInstrs[0x40])(uint32_t) =
{
    nullptr, nullptr, j,    jal,   beq,  bne,  blez,  bgtz,  // 0x00-0x07
    addi,    addiu,   slti, sltiu, andi, ori,  xori,  lui,   // 0x08-0x0F
    cop,     unk,     unk,  unk,   beql, bnel, blezl, bgtzl, // 0x10-0x17
    daddi,   daddiu,  ldl,  ldr,   unk,  unk,  unk,   unk,   // 0x18-0x1F
    lb,      lh,      lwl,  lw,    lbu,  lhu,  lwr,   lwu,   // 0x20-0x27
    sb,      sh,      swl,  sw,    sdl,  sdr,  swr,   unk,   // 0x28-0x2F
    unk,     unk,     unk,  unk,   unk,  unk,  unk,   ld,    // 0x30-0x37
    unk,     unk,     unk,  unk,   unk,  unk,  unk,   sd     // 0x38-0x3F
};

// Register-type instruction lookup table, using opcode bits 0-5
void (*VR4300::regInstrs[0x40])(uint32_t) =
{
    sll,  unk,   srl,  sra,  sllv,   unk,    srlv,   srav,  // 0x00-0x07
    jr,   jalr,  unk,  unk,  unk,    unk,    unk,    unk,   // 0x08-0x0F
    mfhi, mthi,  mflo, mtlo, dsllv,  unk,    dsrlv,  dsrav, // 0x10-0x17
    mult, multu, div,  divu, dmult,  dmultu, ddiv,   ddivu, // 0x18-0x1F
    add,  addu,  sub,  subu, and_,   or_,    xor_,   nor,   // 0x20-0x27
    unk,  unk,   slt,  sltu, dadd,   daddu,  dsub,   dsubu, // 0x28-0x2F
    unk,  unk,   unk,  unk,  unk,    unk,    unk,    unk,   // 0x30-0x37
    dsll, unk,   dsrl, dsra, dsll32, unk,    dsrl32, dsra32 // 0x38-0x3F
};

// Extra-type instruction lookup table, using opcode bits 16-20
void (*VR4300::extInstrs[0x20])(uint32_t) =
{
    bltz,   bgez,   bltzl,   bgezl,   unk, unk, unk, unk, // 0x00-0x07
    unk,    unk,    unk,     unk,     unk, unk, unk, unk, // 0x08-0x0F
    bltzal, bgezal, bltzall, bgezall, unk, unk, unk, unk, // 0x10-0x17
    unk,    unk,    unk,     unk,     unk, unk, unk, unk  // 0x18-0x1F
};

void VR4300::reset()
{
    // Map the writable registers so that writes to r0 are redirected
    registersW[0] = &registersR[32];
    for (int i = 1; i < 32; i++)
        registersW[i] = &registersR[i];

    // Reset the interpreter to its initial state
    memset(registersR, 0, sizeof(registersR));
    hi = lo = 0;
    programCounter = 0xBFC00000;
    nextOpcode = Memory::read<uint32_t>(programCounter);
}

void VR4300::runOpcode()
{
    // Move an opcode through the pipeline
    uint32_t opcode = nextOpcode;
    nextOpcode = Memory::read<uint32_t>(programCounter += 4);

    // Look up and execute an instruction
    switch (opcode >> 26)
    {
        default: return (*immInstrs[opcode >> 26])(opcode);
        case 0:  return (*regInstrs[opcode & 0x3F])(opcode);
        case 1:  return (*extInstrs[(opcode >> 16) & 0x1F])(opcode);
    }
}

void VR4300::exception()
{
    // Disable further exceptions and jump to the exception handler
    // TODO: support non-interrupt exceptions
    CP0::write(12, CP0::read(12) | 0x2); // EXL
    CP0::write(14, programCounter);
    programCounter = 0x80000180 - 4;
    nextOpcode = 0;
}

void VR4300::j(uint32_t opcode)
{
    // Jump to an immediate value
    programCounter = ((programCounter & 0xF0000000) | ((opcode & 0x3FFFFFF) << 2)) - 4;
}

void VR4300::jal(uint32_t opcode)
{
    // Save the return address and jump to an immediate value
    *registersW[31] = programCounter + 4;
    programCounter = ((programCounter & 0xF0000000) | ((opcode & 0x3FFFFFF) << 2)) - 4;
}

void VR4300::beq(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if two registers are equal
    if (registersR[(opcode >> 21) & 0x1F] == registersR[(opcode >> 16) & 0x1F])
        programCounter += ((int16_t)opcode << 2) - 4;
}

void VR4300::bne(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if two registers aren't equal
    if (registersR[(opcode >> 21) & 0x1F] != registersR[(opcode >> 16) & 0x1F])
        programCounter += ((int16_t)opcode << 2) - 4;
}

void VR4300::blez(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is less or equal to zero
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] <= 0)
        programCounter += ((int16_t)opcode << 2) - 4;
}

void VR4300::bgtz(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is greater than zero
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] > 0)
        programCounter += ((int16_t)opcode << 2) - 4;
}

void VR4300::addi(uint32_t opcode)
{
    // Add a signed 16-bit immediate to a register and store the lower result
    // TODO: overflow exception
    int32_t value = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::addiu(uint32_t opcode)
{
    // Add a signed 16-bit immediate to a register and store the lower result
    int32_t value = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::slti(uint32_t opcode)
{
    // Check if a signed register is less than a signed 16-bit immediate, and store the result
    bool value = (int64_t)registersR[(opcode >> 21) & 0x1F] < (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::sltiu(uint32_t opcode)
{
    // Check if a register is less than a signed 16-bit immediate, and store the result
    bool value = registersR[(opcode >> 21) & 0x1F] < (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::andi(uint32_t opcode)
{
    // Bitwise and a register with a 16-bit immediate and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] & (opcode & 0xFFFF);
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::ori(uint32_t opcode)
{
    // Bitwise or a register with a 16-bit immediate and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] | (opcode & 0xFFFF);
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::xori(uint32_t opcode)
{
    // Bitwise exclusive or a register with a 16-bit immediate and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] ^ (opcode & 0xFFFF);
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::lui(uint32_t opcode)
{
    // Load a 16-bit immediate into the upper 16 bits of a register
    *registersW[(opcode >> 16) & 0x1F] = (int16_t)opcode << 16;
}

void VR4300::beql(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if two registers are equal
    // Otherwise, discard the delay slot opcode
    if (registersR[(opcode >> 21) & 0x1F] == registersR[(opcode >> 16) & 0x1F])
        programCounter += ((int16_t)opcode << 2) - 4;
    else
        nextOpcode = 0;
}

void VR4300::bnel(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if two registers aren't equal
    // Otherwise, discard the delay slot opcode
    if (registersR[(opcode >> 21) & 0x1F] != registersR[(opcode >> 16) & 0x1F])
        programCounter += ((int16_t)opcode << 2) - 4;
    else
        nextOpcode = 0;
}

void VR4300::blezl(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is less or equal to zero
    // Otherwise, discard the delay slot opcode
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] <= 0)
        programCounter += ((int16_t)opcode << 2) - 4;
    else
        nextOpcode = 0;
}

void VR4300::bgtzl(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is greater than zero
    // Otherwise, discard the delay slot opcode
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] > 0)
        programCounter += ((int16_t)opcode << 2) - 4;
    else
        nextOpcode = 0;
}

void VR4300::daddi(uint32_t opcode)
{
    // Add a signed 16-bit immediate to a register and store the result
    // TODO: overflow exception
    uint64_t value = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::daddiu(uint32_t opcode)
{
    // Add a signed 16-bit immediate to a register and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = value;
}

void VR4300::ldl(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the most significant byte of a misaligned 64-bit word
    // The aligned 64-bit word is shifted left so its bytes misalign to match the address
    // The shifted value is read to a register, but empty bytes are left unchanged
    // This allows LDL and LDR to be used in succession to read misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint64_t value = Memory::read<uint64_t>(address) << ((address & 7) * 8);
    uint64_t *reg = registersW[(opcode >> 16) & 0x1F];
    *reg = (*reg & ~((uint64_t)-1 << ((address & 7) * 8))) | value;
}

void VR4300::ldr(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the most significant byte of a misaligned 64-bit word
    // The aligned 64-bit word is shifted right so its bytes misalign to match the address
    // The shifted value is read to a register, but empty bytes are left unchanged
    // This allows LDL and LDR to be used in succession to read misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint64_t value = Memory::read<uint64_t>(address) >> ((7 - (address & 7)) * 8);
    uint64_t *reg = registersW[(opcode >> 16) & 0x1F];
    *reg = (*reg & ~((uint64_t)-1 >> ((7 - (address & 7)) * 8))) | value;
}

void VR4300::lb(uint32_t opcode)
{
    // Load a signed byte from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = (int8_t)Memory::read<uint8_t>(address);
}

void VR4300::lh(uint32_t opcode)
{
    // Load a signed half-word from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = (int16_t)Memory::read<uint16_t>(address);
}

void VR4300::lwl(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the most significant byte of a misaligned 32-bit word
    // The aligned 32-bit word is shifted left so its bytes misalign to match the address
    // The shifted value is read to a register, but empty bytes are left unchanged
    // This allows LWL and LWR to be used in succession to read misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint32_t value = Memory::read<uint32_t>(address) << ((address & 3) * 8);
    uint64_t *reg = registersW[(opcode >> 16) & 0x1F];
    *reg = (int32_t)(((uint32_t)*reg & ~((uint32_t)-1 << ((address & 7) * 8))) | value);
}

void VR4300::lw(uint32_t opcode)
{
    // Load a signed word from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = (int32_t)Memory::read<uint32_t>(address);
}

void VR4300::lbu(uint32_t opcode)
{
    // Load a byte from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = Memory::read<uint8_t>(address);
}

void VR4300::lhu(uint32_t opcode)
{
    // Load a half-word from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = Memory::read<uint16_t>(address);
}

void VR4300::lwr(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the most significant byte of a misaligned 32-bit word
    // The aligned 32-bit word is shifted right so its bytes misalign to match the address
    // The shifted value is read to a register, but empty bytes are left unchanged
    // This allows LWL and LWR to be used in succession to read misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint32_t value = Memory::read<uint32_t>(address) >> ((3 - (address & 3)) * 8);
    uint64_t *reg = registersW[(opcode >> 16) & 0x1F];
    *reg = (int32_t)(((uint32_t)*reg & ~((uint32_t)-1 >> ((3 - (address & 3)) * 8))) | value);
}

void VR4300::lwu(uint32_t opcode)
{
    // Load a word from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = Memory::read<uint32_t>(address);
}

void VR4300::sb(uint32_t opcode)
{
    // Store a byte to memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    Memory::write<uint8_t>(address, registersR[(opcode >> 16) & 0x1F]);
}

void VR4300::sh(uint32_t opcode)
{
    // Store a half-word to memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    Memory::write<uint16_t>(address, registersR[(opcode >> 16) & 0x1F]);
}

void VR4300::swl(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the most significant byte of a misaligned 32-bit word
    // A register value is shifted right so its bytes misalign to match the address
    // The shifted value is written to the aligned memory address, but empty bytes are left unchanged
    // This allows SWL and SWR to be used in succession to write misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint32_t rVal = (uint32_t)registersR[(opcode >> 16) & 0x1F] >> ((address & 3) * 8);
    uint32_t mVal = Memory::read<uint32_t>(address) & ~((uint32_t)-1 >> ((address & 3) * 8));
    Memory::write<uint32_t>(address, rVal | mVal);
}

void VR4300::sw(uint32_t opcode)
{
    // Store a word to memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    Memory::write<uint32_t>(address, registersR[(opcode >> 16) & 0x1F]);
}

void VR4300::sdl(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the most significant byte of a misaligned 64-bit word
    // A register value is shifted right so its bytes misalign to match the address
    // The shifted value is written to the aligned memory address, but empty bytes are left unchanged
    // This allows SDL and SDR to be used in succession to write misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint64_t rVal = registersR[(opcode >> 16) & 0x1F] >> ((address & 7) * 8);
    uint64_t mVal = Memory::read<uint64_t>(address) & ~((uint64_t)-1 >> ((address & 7) * 8));
    Memory::write<uint64_t>(address, rVal | mVal);
}

void VR4300::sdr(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the least significant byte of a misaligned 64-bit word
    // A register value is shifted left so its bytes misalign to match the address
    // The shifted value is written to the aligned memory address, but empty bytes are left unchanged
    // This allows SDL and SDR to be used in succession to write misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint64_t rVal = registersR[(opcode >> 16) & 0x1F] << ((7 - (address & 7)) * 8);
    uint64_t mVal = Memory::read<uint64_t>(address) & ~((uint64_t)-1 << ((7 - (address & 7)) * 8));
    Memory::write<uint64_t>(address, rVal | mVal);
}

void VR4300::swr(uint32_t opcode)
{
    // This instruction is confusing, but I'll try my best to explain
    // The address points to the least significant byte of a misaligned 32-bit word
    // A register value is shifted left so its bytes misalign to match the address
    // The shifted value is written to the aligned memory address, but empty bytes are left unchanged
    // This allows SWL and SWR to be used in succession to write misaligned words
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    uint32_t rVal = (uint32_t)registersR[(opcode >> 16) & 0x1F] << ((3 - (address & 3)) * 8);
    uint32_t mVal = Memory::read<uint32_t>(address) & ~((uint32_t)-1 << ((3 - (address & 3)) * 8));
    Memory::write<uint32_t>(address, rVal | mVal);
}

void VR4300::ld(uint32_t opcode)
{
    // Load a double-word from memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    *registersW[(opcode >> 16) & 0x1F] = Memory::read<uint64_t>(address);
}

void VR4300::sd(uint32_t opcode)
{
    // Store a double-word to memory at base register plus immeditate offset
    uint32_t address = registersR[(opcode >> 21) & 0x1F] + (int16_t)opcode;
    Memory::write<uint64_t>(address, registersR[(opcode >> 16) & 0x1F]);
}

void VR4300::sll(uint32_t opcode)
{
    // Shift a register left by a 5-bit immediate and store the lower result
    int32_t value = registersR[(opcode >> 16) & 0x1F] << ((opcode >> 6) & 0x1F);
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::srl(uint32_t opcode)
{
    // Shift a register right by a 5-bit immediate and store the lower result
    int32_t value = (uint32_t)registersR[(opcode >> 16) & 0x1F] >> ((opcode >> 6) & 0x1F);
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::sra(uint32_t opcode)
{
    // Shift a register right by a 5-bit immediate and store the lower signed result
    int32_t value = (int32_t)registersR[(opcode >> 16) & 0x1F] >> ((opcode >> 6) & 0x1F);
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::sllv(uint32_t opcode)
{
    // Shift a register left by a register and store the lower result
    int32_t value = registersR[(opcode >> 16) & 0x1F] << registersR[(opcode >> 21) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::srlv(uint32_t opcode)
{
    // Shift a register right by a register and store the lower result
    int32_t value = (uint32_t)registersR[(opcode >> 16) & 0x1F] >> registersR[(opcode >> 21) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::srav(uint32_t opcode)
{
    // Shift a register right by a register and store the lower signed result
    int32_t value = (int32_t)registersR[(opcode >> 16) & 0x1F] >> registersR[(opcode >> 21) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::jr(uint32_t opcode)
{
    // Jump to an address stored in a register
    programCounter = registersR[(opcode >> 21) & 0x1F] - 4;
}

void VR4300::jalr(uint32_t opcode)
{
    // Save the return address and jump to an address stored in a register
    *registersW[(opcode >> 11) & 0x1F] = programCounter + 4;
    programCounter = registersR[(opcode >> 21) & 0x1F] - 4;
}

void VR4300::mfhi(uint32_t opcode)
{
    // Copy the high word of the mult/div result to a register
    *registersW[(opcode >> 11) & 0x1F] = hi;
}

void VR4300::mthi(uint32_t opcode)
{
    // Copy a register to the high word of the mult/div result
    hi = registersR[(opcode >> 21) & 0x1F];
}

void VR4300::mflo(uint32_t opcode)
{
    // Copy the low word of the mult/div result to a register
    *registersW[(opcode >> 11) & 0x1F] = lo;
}

void VR4300::mtlo(uint32_t opcode)
{
    // Copy a register to the low word of the mult/div result
    lo = registersR[(opcode >> 21) & 0x1F];
}

void VR4300::dsllv(uint32_t opcode)
{
    // Shift a register left by a register and store the result
    uint64_t value = registersR[(opcode >> 16) & 0x1F] << registersR[(opcode >> 21) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsrlv(uint32_t opcode)
{
    // Shift a register right by a register and store the result
    uint64_t value = registersR[(opcode >> 16) & 0x1F] >> registersR[(opcode >> 21) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsrav(uint32_t opcode)
{
    // Shift a register right by a register and store the signed result
    uint64_t value = (int64_t)registersR[(opcode >> 16) & 0x1F] >> registersR[(opcode >> 21) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::mult(uint32_t opcode)
{
    // Multiply two 32-bit signed registers and set the 64-bit result
    int64_t value = (int64_t)(int32_t)registersR[(opcode >> 16) & 0x1F] *
        (int32_t)registersR[(opcode >> 21) & 0x1F];
    hi = value >> 32;
    lo = (uint32_t)value;
}

void VR4300::multu(uint32_t opcode)
{
    // Multiply two 32-bit unsigned registers and set the 64-bit result
    uint64_t value = (uint64_t)(uint32_t)registersR[(opcode >> 16) & 0x1F] *
        (uint32_t)registersR[(opcode >> 21) & 0x1F];
    hi = value >> 32;
    lo = (uint32_t)value;
}

void VR4300::div(uint32_t opcode)
{
    // Divide two 32-bit signed registers and set the 32-bit result and remainder
    // TODO: handle edge cases
    int32_t divisor = registersR[(opcode >> 16) & 0x1F];
    int32_t value = registersR[(opcode >> 21) & 0x1F];
    if (divisor && !(divisor == -1 && value == (1 << 31)))
    {
        hi = value % divisor;
        lo = value / divisor;
    }
}

void VR4300::divu(uint32_t opcode)
{
    // Divide two 32-bit unsigned registers and set the 32-bit result and remainder
    // TODO: handle edge cases
    if (uint32_t divisor = registersR[(opcode >> 16) & 0x1F])
    {
        uint32_t value = registersR[(opcode >> 21) & 0x1F];
        hi = value % divisor;
        lo = value / divisor;
    }
}

void VR4300::dmult(uint32_t opcode)
{
    // Multiply two signed 64-bit registers and set the 128-bit result
    __uint128_t value = (__int128_t)(int64_t)registersR[(opcode >> 16) & 0x1F] *
        (int64_t)registersR[(opcode >> 21) & 0x1F];
    hi = value >> 64;
    lo = value;
}

void VR4300::dmultu(uint32_t opcode)
{
    // Multiply two unsigned 64-bit registers and set the 128-bit result
    __uint128_t value = (__uint128_t)registersR[(opcode >> 16) & 0x1F] *
        registersR[(opcode >> 21) & 0x1F];
    hi = value >> 64;
    lo = value;
}

void VR4300::ddiv(uint32_t opcode)
{
    // Divide two 64-bit signed registers and set the 64-bit result and remainder
    // TODO: handle edge cases
    int64_t divisor = registersR[(opcode >> 16) & 0x1F];
    int64_t value = registersR[(opcode >> 21) & 0x1F];
    if (divisor && !(divisor == -1 && value == (1L << 63)))
    {
        hi = value % divisor;
        lo = value / divisor;
    }
}

void VR4300::ddivu(uint32_t opcode)
{
    // Divide two 64-bit unsigned registers and set the 64-bit result and remainder
    // TODO: handle edge cases
    if (uint64_t divisor = registersR[(opcode >> 16) & 0x1F])
    {
        uint64_t value = registersR[(opcode >> 21) & 0x1F];
        hi = value % divisor;
        lo = value / divisor;
    }
}

void VR4300::add(uint32_t opcode)
{
    // Add a register to a register and store the lower result
    // TODO: overflow exception
    int32_t value = registersR[(opcode >> 21) & 0x1F] + registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::addu(uint32_t opcode)
{
    // Add a register to a register and store the result
    int32_t value = registersR[(opcode >> 21) & 0x1F] + registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::sub(uint32_t opcode)
{
    // Add a register to a register and store the lower result
    // TODO: overflow exception
    int32_t value = registersR[(opcode >> 21) & 0x1F] - registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::subu(uint32_t opcode)
{
    // Add a register to a register and store the result
    int32_t value = registersR[(opcode >> 21) & 0x1F] - registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::and_(uint32_t opcode)
{
    // Bitwise and a register with a register and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] & registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::or_(uint32_t opcode)
{
    // Bitwise or a register with a register and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] | registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::xor_(uint32_t opcode)
{
    // Bitwise exclusive or a register with a register and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] ^ registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::nor(uint32_t opcode)
{
    // Bitwise or a register with a register and store the negated result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] | registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = ~value;
}

void VR4300::slt(uint32_t opcode)
{
    // Check if a signed register is less than another signed register, and store the result
    bool value = (int64_t)registersR[(opcode >> 21) & 0x1F] < (int64_t)registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::sltu(uint32_t opcode)
{
    // Check if a register is less than another register, and store the result
    bool value = registersR[(opcode >> 21) & 0x1F] < registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dadd(uint32_t opcode)
{
    // Add a register to a register and store the result
    // TODO: overflow exception
    uint64_t value = registersR[(opcode >> 21) & 0x1F] + registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::daddu(uint32_t opcode)
{
    // Add a register to a register and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] + registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsub(uint32_t opcode)
{
    // Subtract a register from a register and store the result
    // TODO: overflow exception
    uint64_t value = registersR[(opcode >> 21) & 0x1F] - registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsubu(uint32_t opcode)
{
    // Subtract a register from a register and store the result
    uint64_t value = registersR[(opcode >> 21) & 0x1F] - registersR[(opcode >> 16) & 0x1F];
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsll(uint32_t opcode)
{
    // Shift a register left by a 5-bit immediate and store the result
    uint64_t value = registersR[(opcode >> 16) & 0x1F] << ((opcode >> 6) & 0x1F);
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsrl(uint32_t opcode)
{
    // Shift a register right by a 5-bit immediate and store the result
    uint64_t value = registersR[(opcode >> 16) & 0x1F] >> ((opcode >> 6) & 0x1F);
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsra(uint32_t opcode)
{
    // Shift a register right by a 5-bit immediate and store the signed result
    uint64_t value = (int64_t)registersR[(opcode >> 16) & 0x1F] >> ((opcode >> 6) & 0x1F);
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsll32(uint32_t opcode)
{
    // Shift a register left by a 5-bit immediate plus 32 and store the result
    uint64_t value = registersR[(opcode >> 16) & 0x1F] << (32 + ((opcode >> 6) & 0x1F));
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsrl32(uint32_t opcode)
{
    // Shift a register right by a 5-bit immediate plus 32 and store the result
    uint64_t value = registersR[(opcode >> 16) & 0x1F] >> (32 + ((opcode >> 6) & 0x1F));
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::dsra32(uint32_t opcode)
{
    // Shift a register right by a 5-bit immediate plus 32 and store the signed result
    uint64_t value = (int64_t)registersR[(opcode >> 16) & 0x1F] >> (32 + ((opcode >> 6) & 0x1F));
    *registersW[(opcode >> 11) & 0x1F] = value;
}

void VR4300::bltz(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is less than zero
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] < 0)
        programCounter += ((int16_t)opcode << 2) - 4;
}

void VR4300::bgez(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is greater or equal to zero
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] >= 0)
        programCounter += ((int16_t)opcode << 2) - 4;
}

void VR4300::bltzl(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is less than zero
    // Otherwise, discard the delay slot opcode
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] < 0)
        programCounter += ((int16_t)opcode << 2) - 4;
    else
        nextOpcode = 0;
}

void VR4300::bgezl(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is greater or equal to zero
    // Otherwise, discard the delay slot opcode
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] >= 0)
        programCounter += ((int16_t)opcode << 2) - 4;
    else
        nextOpcode = 0;
}

void VR4300::bltzal(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is less than zero
    // Also, save the return address
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] < 0)
    {
        *registersW[31] = programCounter + 4;
        programCounter += ((int16_t)opcode << 2) - 4;
    }
}

void VR4300::bgezal(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is greater or equal to zero
    // Also, save the return address
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] >= 0)
    {
        *registersW[31] = programCounter + 4;
        programCounter += ((int16_t)opcode << 2) - 4;
    }
}

void VR4300::bltzall(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is less than zero
    // Also, save the return address; otherwise, discard the delay slot opcode
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] < 0)
    {
        *registersW[31] = programCounter + 4;
        programCounter += ((int16_t)opcode << 2) - 4;
    }
    else
    {
        nextOpcode = 0;
    }
}

void VR4300::bgezall(uint32_t opcode)
{
    // Add a 16-bit offset to the program counter if a register is greater or equal to zero
    // Also, save the return address; otherwise, discard the delay slot opcode
    if ((int64_t)registersR[(opcode >> 21) & 0x1F] >= 0)
    {
        *registersW[31] = programCounter + 4;
        programCounter += ((int16_t)opcode << 2) - 4;
    }
    else
    {
        nextOpcode = 0;
    }
}

void VR4300::eret(uint32_t opcode)
{
    // Return from an exception and re-enable them
    programCounter = CP0::read(14) - 4;
    nextOpcode = 0;
    CP0::write(12, CP0::read(12) & ~0x2); // EXL
}

void VR4300::mfc0(uint32_t opcode)
{
    // Copy a CP0 register value to a CPU register
    *registersW[(opcode >> 16) & 0x1F] = CP0::read((opcode >> 11) & 0x1F);
}

void VR4300::mtc0(uint32_t opcode)
{
    // Copy a CPU register value to a CP0 register
    CP0::write((opcode >> 11) & 0x1F, registersR[(opcode >> 16) & 0x1F]);
}

void VR4300::cop(uint32_t opcode)
{
    // Look up coprocessor instructions that weren't worth making a table for
    if (opcode == 0x42000018)
        return eret(opcode);
    else if ((opcode & 0xFFE007FF) == 0x40000000)
        return mfc0(opcode);
    else if ((opcode & 0xFFE007FF) == 0x40800000)
        return mtc0(opcode);
}

void VR4300::unk(uint32_t opcode)
{
    // Warn about unknown instructions
    LOG_CRIT("Unknown opcode: 0x%08X @ 0x%X\n", opcode, programCounter - 4);
}
