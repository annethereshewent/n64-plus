#pragma once

#include <cstdint>
#include <iostream>
#include "../bus/Bus.cpp"
#include "CPU.hpp"

typedef unsigned __int128 u128;
typedef __int128 i128;

void CPU::set(int i, uint64_t val) {
    if (i != 0) {
        r[i] = val;
    }
}

void CPU::lui(CPU* cpu, uint32_t instruction) {

    uint32_t immediate = getImmediate(instruction);

    int reg = getRt(instruction);

    uint64_t value = (int32_t)(int64_t)(uint64_t)(immediate << 16);

    cpu->r[reg] = value;
}

void CPU::addi(CPU* cpu, uint32_t instruction) {
    CPU::addiu(cpu, instruction);
}

void CPU::addiu(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    cpu->r[rt] = (int32_t)(int64_t)(uint64_t)cpu->r[rs] + immediate;
}

void CPU::andi(CPU* cpu, uint32_t instruction) {
    uint32_t immediate = getImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    cpu->r[rt] = cpu->r[rs] & immediate;
}
void CPU::beq(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint32_t immediate = getImmediate(instruction);

    if (cpu->r[rs] == cpu->r[rt]) {
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }

    cpu->inDelaySlot = true;
}
void CPU::beql(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint32_t immediate = getImmediate(instruction);
    if (cpu->r[rs] == cpu->r[rt]) {
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;

        cpu->inDelaySlot = true;
    } else {
        cpu->pc = cpu->nextPc;
        cpu->discarded = true;
    }
}
void CPU::bgtz(CPU* cpu, uint32_t instruction) {
    if ((int64_t)cpu->r[getRs(instruction)] > 0) {
        uint32_t immediate = getImmediate(instruction);
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }

    cpu->inDelaySlot = true;
}
void CPU::bgtzl(CPU* cpu, uint32_t instruction) {
    if ((int64_t)cpu->r[getRs(instruction)] > 0) {
        uint32_t immediate = getImmediate(instruction);
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
        cpu->inDelaySlot = true;
    } else {
        cpu->discarded = true;
        cpu->pc = cpu->nextPc;
    }
}
void CPU::blez(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    if ((int64_t)cpu->r[rs] <= 0) {
        uint32_t immediate = getImmediate(instruction);

        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }

    cpu->inDelaySlot = true;
}
void CPU::blezl(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    if ((int64_t)cpu->r[rs] <= 0) {
        uint32_t immediate = getImmediate(instruction);

        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;

        cpu->inDelaySlot = true;
    } else {
        cpu->discarded = true;
        cpu->pc = cpu->nextPc;
    }
}
void CPU::bne(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint32_t immediate = getImmediate(instruction);

    if (cpu->r[rs] != cpu->r[rt]) {
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }
    cpu->inDelaySlot = true;
}
void CPU::bnel(CPU* cpu, uint32_t instruction) {

    uint32_t immediate = getImmediate(instruction);
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    if (cpu->r[rs] != cpu->r[rt]) {
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;

        cpu->inDelaySlot = true;
    } else {
        cpu->pc = cpu->nextPc;
        cpu->discarded = true;
    }
}
void CPU::cache(CPU* cpu, uint32_t instruction) {
    uint32_t cacheOp = getRt(instruction);

    uint64_t offset = getSignedImmediate(instruction);
    uint32_t base = getRs(instruction);

    uint64_t address = cpu->r[base] + offset;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    switch (cacheOp) {
        case 0x0: {
            uint64_t line = (actualAddress >> 5) & 0x1ff;
            cpu->bus.icache[line].valid = false;
            break;
        }
        case 0x1: {
            // write-back invalidate of dcachce
            uint64_t line = (actualAddress >> 4) & 0x1ff;

            if (cpu->bus.dcache[line].dirty && cpu->bus.dcache[line].valid) {
                // do the writeback
                cpu->bus.dcacheWriteback(line);
            }
            cpu->bus.dcache[line].valid = false;

            break;
        }
        case 0x8: {
            uint64_t line = (actualAddress >> 5) & 0x1ff;
            bool isValid = (cpu->cop0.tagLo >> 7 & 0b1) == 1;
            uint32_t tag = (cpu->cop0.tagLo & 0xFFFFF00) << 4;

            cpu->bus.icache[line].valid = isValid;
            cpu->bus.icache[line].tag = tag;
            break;
        }
        case 0x9: {
            uint64_t line = (actualAddress >> 4) & 0x1ff;

            cpu->bus.dcache[line].valid = ((cpu->cop0.tagLo >> 7) & 0b1) == 1;
            cpu->bus.dcache[line].valid = ((cpu->cop0.tagLo >> 6) & 0b1) == 1;
            cpu->bus.dcache[line].tag = ((cpu->cop0.tagLo & 0xfffff00) << 4);
            break;
        }
        case 0xd: {
            uint64_t line = (actualAddress >> 4) & 0x1ff;

            if (!cpu->bus.dcacheHit(line, actualAddress) && cpu->bus.dcache[line].dirty) {
                cpu->bus.dcacheWriteback(line);
            }

            cpu->bus.dcache[line].tag = actualAddress & ~0xfff;
            cpu->bus.dcache[line].dirty = true;
            cpu->bus.dcache[line].valid = true;
            break;
        }
        case 0x10: {
            uint64_t line = (actualAddress >> 5) & 0x1ff;

            if (cpu->bus.icacheHit(line, actualAddress)) {
                cpu->bus.icache[line].valid = false;
            }
            break;
        }
        case 0x11: {
            uint64_t line = (actualAddress >> 4) & 0x1ff;

            if (cpu->bus.dcacheHit(line, actualAddress)) {
                cpu->bus.dcache[line].valid = false;
                cpu->bus.dcache[line].dirty = false;
            }
            break;
        }
        case 0x15: {
            uint64_t line = (actualAddress >> 4) & 0x1ff;

            if (cpu->bus.dcacheHit(line, actualAddress)) {
                cpu->bus.dcacheWriteback(line);

                cpu->bus.dcache[line].valid = false;
            }
            break;
        }
        case 0x19: {
            uint64_t line = (actualAddress >> 4) & 0x1ff;

            if (cpu->bus.dcacheHit(line, actualAddress) && cpu->bus.dcache[line].dirty) {
                cpu->bus.dcacheWriteback(line);
            }
            break;
        }
        default: {
            std::cout << "cache op not yet implemented: " << std::hex << cacheOp << "\n";
            throw std::runtime_error("");
            break;
        }
    }
}
void CPU::daddi(CPU* cpu, uint32_t instruction) {
    CPU::daddiu(cpu, instruction);
}
void CPU::daddiu(CPU* cpu, uint32_t instruction) {
    cpu->r[getRt(instruction)] = cpu->r[getRs(instruction)] + getSignedImmediate(instruction);
}
void CPU::j(CPU* cpu, uint32_t instruction) {
    uint32_t offset = (instruction & 0x3ffffff) << 2;

    uint32_t address = (cpu->pc & 0xfffffffff0000000) | offset;

    cpu->inDelaySlot = true;

    cpu->fastForwardAbsoluteLoop(address);

    cpu->nextPc = address;
}
void CPU::jal(CPU* cpu, uint32_t instruction) {
    cpu->r[31] = cpu->nextPc;

    uint64_t offset = ((uint64_t)instruction & 0x3ffffff) << 2;

    uint64_t address = (cpu->pc & 0xfffffffff0000000) | offset;

    cpu->inDelaySlot = true;

    cpu->nextPc = address;
}
void CPU::lb(CPU* cpu, uint32_t instruction) {
    uint64_t offset = getSignedImmediate(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t base = getRs(instruction);

    uint64_t address = cpu->r[base] + offset;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint64_t value = (int8_t)(int64_t)(uint64_t)cpu->bus.memRead8(actualAddress, cached);

    cpu->r[rt] = value;
}
void CPU::lbu(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    cpu->r[rt] = (uint64_t)cpu->bus.memRead8(actualAddress, cached);
}
void CPU::ld(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    cpu->r[rt] = cpu->bus.memRead64(actualAddress, cached);
}
void CPU::ldl(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    uint32_t shift = 8 * (address & 0x7);

    uint64_t mask = (((uint64_t)1 << shift) - 1);

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint64_t value = cpu->bus.memRead64(actualAddress & ~0x7, cached);

    cpu->r[rt] = (cpu->r[rt] & mask) | (value << shift);
}
void CPU::ldr(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint32_t shift = 8 * (7 - (actualAddress & 0x7));
    uint32_t maskShift = 8 * ((actualAddress & 0x7) + 1);

    uint64_t mask = (actualAddress & 0x7) == 7 ? 0 : ~(((uint64_t)1 << maskShift) - 1);

    uint64_t value = cpu->bus.memRead64(actualAddress & ~0x7, cached);

    cpu->r[rt] = (cpu->r[rt] & mask) | (value >> shift);
}
void CPU::lh(CPU* cpu, uint32_t instruction) {
    uint64_t offset = getSignedImmediate(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t base = getRs(instruction);

    uint32_t address = cpu->r[base] + offset;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint64_t value = (int16_t)(int64_t)(uint64_t)cpu->bus.memRead16(actualAddress, cached);

    cpu->r[rt] = value;
}
void CPU::lhu(CPU* cpu, uint32_t instruction) {
    uint64_t offset = getSignedImmediate(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t base = getRs(instruction);

    uint32_t address = cpu->r[base] + offset;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint64_t value = (uint64_t)cpu->bus.memRead16(actualAddress, cached);

    cpu->r[rt] = value;
}
void CPU::ll(CPU* cpu, uint32_t instruction) {
    cpu->llbit = true;

    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint32_t value = cpu->bus.memRead32(actualAddress, cached);

    cpu->r[rt] = (int32_t)(int64_t)(uint64_t)value;

    cpu->cop0.llAddress = actualAddress >> 4;
}
void CPU::lld(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: lld\n");
}
void CPU::lw(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint32_t value = cpu->bus.memRead32(actualAddress, cached);

    cpu->r[rt] = (int32_t)(int64_t)(uint64_t)value;
 }
void CPU::lwl(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint32_t shift = 8 * (actualAddress & 0x3);

    uint32_t mask = ((1 << shift) - 1);

    uint32_t value = cpu->bus.memRead32(actualAddress & ~0x3, cached);

    cpu->r[rt] = (int32_t)(int64_t)(uint64_t)(((uint32_t)cpu->r[rt] & mask) | (value << shift));
}
void CPU::lwr(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);
    uint32_t baseReg = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = cpu->r[baseReg] + immediate;

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address);

    if (error) {
        return;
    }

    uint32_t shift = 8 * (3 - (actualAddress & 0x3));
    uint32_t maskShift = 8 * ((actualAddress & 0x3) + 1);

    uint32_t mask = (actualAddress & 0x3) == 3 ? 0 : ~((1 << maskShift) - 1);

    uint32_t value = cpu->bus.memRead32(actualAddress & ~0x3, cached);

    cpu->r[rt] = (int32_t)(int64_t)(uint64_t)(((uint32_t)cpu->r[rt] & mask) | (value >> shift));
}
void CPU::lwu(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: lwu\n");
}
void CPU::ori(CPU* cpu, uint32_t instruction) {
    uint32_t imm = getImmediate(instruction);

    uint32_t rt = getRt(instruction);
    uint32_t rs = getRs(instruction);

    cpu->r[rt] = cpu->r[rs] | imm;
}
void CPU::reserved(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("Error: opcode reserved\n");

}
void CPU::sb(CPU* cpu, uint32_t instruction) {
    uint8_t byte = (uint8_t)cpu->r[getRt(instruction)];

    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    cpu->bus.memWrite8(actualAddress, byte, cached);
}
void CPU::sc(CPU* cpu, uint32_t instruction) {
    uint32_t rt = getRt(instruction);
    if (cpu->llbit) {
        cpu->llbit = false;

        uint64_t immediate = getSignedImmediate(instruction);
        uint32_t baseReg = getRs(instruction);

        uint64_t address = cpu->r[baseReg] + immediate;

        auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

        if (error) {
            return;
        }

        cpu->bus.memWrite32(actualAddress, (uint32_t)cpu->r[rt], cached);

        cpu->r[rt] = 1;
    } else {
        cpu->r[rt] = 0;
    }
}
void CPU::scd(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: scd\n");
}
void CPU::sd(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    cpu->bus.memWrite64(actualAddress, cpu->r[getRt(instruction)], cached);
}
void CPU::sdl(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    uint32_t shift = (actualAddress & 0x7) * 8;

    uint32_t maskShift = 8 * (8 - (actualAddress & 0x7));

    uint64_t mask = (actualAddress & 0x7) == 0 ? 0xffffffffffffffff : ((uint64_t)1 << maskShift) - 1;

    uint64_t value = (cpu->r[rt] >> shift);

    cpu->bus.memWrite64(actualAddress & ~0x7, value, cached, mask);
}
void CPU::sdr(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    uint32_t shift = 8 * (7 - (actualAddress & 0x7));

    uint64_t mask = ~(((uint64_t)1 << shift) - 1);

    uint32_t value = cpu->r[rt] << shift;

    cpu->bus.memWrite64(actualAddress & ~0x7, value, cached, mask);
}
void CPU::sh(CPU* cpu, uint32_t instruction) {
    uint16_t half = (uint16_t)cpu->r[getRt(instruction)];

    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    cpu->bus.memWrite16(actualAddress, half, cached);
}
void CPU::slti(CPU* cpu, uint32_t instruction) {
    int64_t immediate = (int16_t)(int64_t)getImmediate(instruction);

    uint64_t val = 0;
    if ((int64_t)cpu->r[getRs(instruction)] < immediate) {
        val = 1;
    }

    cpu->r[getRt(instruction)] = val;
}
void CPU::sltiu(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getImmediate(instruction);
    uint64_t val = 0;

    if (cpu->r[getRs(instruction)] < immediate) {
        val = 1;
    }
    cpu->r[getRt(instruction)] = val;
}
void CPU::sw(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    cpu->bus.memWrite32(actualAddress, (uint32_t)cpu->r[rt], cached);
}
void CPU::swl(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    uint32_t shift = (actualAddress & 0x3) * 8;

    uint32_t maskShift = 8 * (4 - (actualAddress & 0x3));

    uint32_t mask = (actualAddress & 0x3) == 0 ? -1 : (1 << maskShift) - 1;

    uint32_t value = (uint32_t)(cpu->r[rt] >> shift);

    cpu->bus.memWrite32(actualAddress & ~0x3, value, cached, false, mask);
}
void CPU::swr(CPU* cpu, uint32_t instruction) {
    uint64_t immediate = getSignedImmediate(instruction);

    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t address = immediate + cpu->r[rs];

    auto [actualAddress, error, cached] = cpu->bus.translateAddress(address, true);

    if (error) {
        return;
    }

    uint32_t shift = 8 * (3 - (actualAddress & 0x3));

    uint32_t mask = ~((1 << shift) - 1);

    uint32_t value = (uint32_t)(cpu->r[rt] << shift);

    cpu->bus.memWrite32(actualAddress & ~0x3, value, cached, false, mask);
}
void CPU::xori(CPU* cpu, uint32_t instruction) {
    cpu->r[getRt(instruction)] = cpu->r[getRs(instruction)] ^ (uint64_t)getImmediate(instruction);
}

void CPU::sll(CPU* cpu, uint32_t instruction) {
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);
    uint32_t shift = shiftAmount(instruction);

    cpu->r[rd] = (int32_t)(uint32_t)(uint64_t)((uint32_t)cpu->r[rt] << shift);
}
void CPU::srl(CPU* cpu, uint32_t instruction) {
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);
    uint32_t shift = shiftAmount(instruction);

    cpu->r[rd] = (int32_t)(uint32_t)(uint64_t)((uint32_t)cpu->r[rt] >> shift);
}
void CPU::sra(CPU* cpu, uint32_t instruction) {
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);
    uint32_t shift = shiftAmount(instruction);

    cpu->r[rd] = (int32_t)(int64_t)(uint64_t)((int64_t)cpu->r[rt] >> shift);
}
void CPU::sllv(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    uint32_t shift = cpu->r[rs] & 0x1f;


    cpu->r[rd] = (int32_t)(int64_t)(uint64_t)((uint32_t)cpu->r[rt] << shift);
}
void CPU::srlv(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    uint32_t shift = cpu->r[rs] & 0x1f;

    cpu->r[rd] = (int32_t)(int64_t)(uint64_t)((uint32_t)cpu->r[rt] >> shift);
}
void CPU::srav(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);
    uint32_t shift = cpu->r[rs] & 0x1f;

    cpu->r[rd] = (int32_t)(int64_t)(uint64_t)((int64_t)cpu->r[rt] >> shift);
}
void CPU::jr(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    cpu->inDelaySlot = true;
    cpu->nextPc = cpu->r[rs];
}
void CPU::jalr(CPU* cpu, uint32_t instruction) {
    cpu->r[getRd(instruction)] = cpu->nextPc;

    cpu->inDelaySlot = true;

    cpu->nextPc = cpu->r[getRs(instruction)];
}
void CPU::sync(CPU* cpu, uint32_t instruction) {
    // NOP
}
void CPU::mfhi(CPU* cpu, uint32_t instruction) {
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = cpu->hi;
}
void CPU::mthi(CPU* cpu, uint32_t instruction) {
    cpu->hi = cpu->r[getRs(instruction)];
}
void CPU::mflo(CPU* cpu, uint32_t instruction) {
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = cpu->lo;
}
void CPU::mtlo(CPU* cpu, uint32_t instruction) {
    cpu->lo = cpu->r[getRs(instruction)];
}
void CPU::dsllv(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    uint32_t shift = cpu->r[rs] & 0x3f;

    cpu->r[rd] = cpu->r[rt] << shift;
}
void CPU::dsrlv(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    uint32_t shift = cpu->r[rs] & 0x3f;

    cpu->r[rd] = cpu->r[rt] >> shift;
}
void CPU::dsrav(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    uint32_t shift = cpu->r[rs] & 0x3f;

    cpu->r[rd] = (uint64_t)((int64_t)cpu->r[rt] >> shift);
}
void CPU::mult(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    int64_t val1 = (int32_t)(int64_t)cpu->r[rs];
    int64_t val2 = (int32_t)(int64_t)cpu->r[rt];

    int64_t result = val1 * val2;

    cpu->lo = (int32_t)(int64_t)(uint64_t)result;

    cpu->hi = (int32_t)(int64_t)(uint64_t)(result >> 32);

    cpu->cop0.addCycles(4);
}
void CPU::multu(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);

    uint64_t val1 = (uint32_t)(uint64_t)cpu->r[rs];
    uint64_t val2 = (uint32_t)(uint64_t)cpu->r[rt];

    uint64_t result = val1 * val2;


    cpu->lo = (int32_t)(int64_t)(uint64_t)result;

    cpu->hi = (int32_t)(int64_t)(uint64_t)(result >> 32);

    cpu->cop0.addCycles(4);
}
void CPU::div(CPU* cpu, uint32_t instruction) {
    int32_t numerator = (int32_t)cpu->r[getRs(instruction)];
    int32_t denominator = (int32_t)cpu->r[getRt(instruction)];


    if (denominator != 0) {
        cpu->lo = (int64_t)(uint64_t)(numerator / denominator);
        cpu->hi = (int64_t)(uint64_t)(numerator % denominator);
    } else {
        if (numerator < 0) {
            cpu->lo = 1;
        } else {
            cpu->lo = 0xffffffffffffffff;
        }
        cpu->hi = numerator;
    }

    cpu->cop0.addCycles(36);
}
void CPU::divu(CPU* cpu, uint32_t instruction) {
    uint32_t numerator = (uint32_t)cpu->r[getRs(instruction)];
    uint32_t denominator = (uint32_t)cpu->r[getRt(instruction)];

    if (denominator != 0) {
        cpu->lo = (int32_t)(int64_t)(uint64_t)(numerator / denominator);
        cpu->hi = (int32_t)(int64_t)(uint64_t)(numerator % denominator);
    } else {
        cpu->lo = 0xffffffffffffffff;
        cpu->hi = (int32_t)(int64_t)(uint64_t)numerator;
    }

    cpu->cop0.addCycles(36);
}
void CPU::dmult(CPU* cpu, uint32_t instruction) {
    i128 val1 = (int64_t)(i128)cpu->r[getRs(instruction)];
    i128 val2 = (int64_t)(i128)cpu->r[getRt(instruction)];

    u128 result = (u128)(val1 * val2);

    cpu->lo = (uint64_t)result;
    cpu->hi = (uint64_t)(result >> 64);

    cpu->cop0.addCycles(7);
}
void CPU::dmultu(CPU* cpu, uint32_t instruction) {
    u128 result = (u128)cpu->r[getRs(instruction)] * (u128)cpu->r[getRt(instruction)];

    cpu->lo = (uint64_t)result;
    cpu->hi = (uint64_t)(result >> 64);

    cpu->cop0.addCycles(7);
}
void CPU::ddiv(CPU* cpu, uint32_t instruction) {
    int64_t numerator = (int64_t)cpu->r[getRs(instruction)];
    int64_t denominator = (int64_t)cpu->r[getRt(instruction)];

    if (denominator != 0) {
        cpu->lo = (uint64_t)(numerator / denominator);
        cpu->hi = (uint64_t)(numerator % denominator);
    } else {
        cpu->lo = numerator < 0 ? 1 : 0xffffffffffffffff;
        cpu->hi = numerator;
    }

    cpu->cop0.addCycles(68);
}
void CPU::ddivu(CPU* cpu, uint32_t instruction) {
    uint64_t numerator = cpu->r[getRs(instruction)];
    uint64_t denominator = cpu->r[getRt(instruction)];

    if (denominator != 0) {
        cpu->lo = numerator / denominator;
        cpu->hi = numerator % denominator;
    } else {
        cpu->lo = 0xffffffffffffffff;
        cpu->hi = numerator;
    }

    cpu->cop0.addCycles(68);
}
void CPU::add(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = (int32_t)(int64_t)(uint64_t)((uint32_t)cpu->r[rt] + (uint32_t)cpu->r[rs]);
}
void CPU::addu(CPU* cpu, uint32_t instruction) {
    CPU::add(cpu, instruction);
}
void CPU::sub(CPU* cpu, uint32_t instruction) {
    CPU::subu(cpu, instruction);
}
void CPU::subu(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = (int32_t)(int64_t)(uint64_t)((uint32_t)cpu->r[rs] - (uint32_t)cpu->r[rt]);
}
void CPU::and_(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = cpu->r[rs] & cpu->r[rt];
}
void CPU::or_(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = cpu->r[rs] | cpu->r[rt];
}
void CPU::xor_(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = cpu->r[rs] ^ cpu->r[rt];
}
void CPU::nor(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    cpu->r[rd] = ~(cpu->r[rs] | cpu->r[rt]);
}
void CPU::slt(CPU* cpu, uint32_t instruction) {
    uint64_t value = 0;

    if ((int64_t)cpu->r[getRs(instruction)] < (int64_t)cpu->r[getRt(instruction)]) {
        value = 1;
    }

    cpu->r[getRd(instruction)] = value;
}
void CPU::sltu(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);
    uint32_t rt = getRt(instruction);
    uint32_t rd = getRd(instruction);

    if (cpu->r[rs] < cpu->r[rt]) {
        cpu->r[rd] = 1;
    } else {
        cpu->r[rd] = 0;
    }
}
void CPU::dadd(CPU* cpu, uint32_t instruction) {
    cpu->r[getRd(instruction)] = cpu->r[getRs(instruction)] + cpu->r[getRt(instruction)];
}
void CPU::daddu(CPU* cpu, uint32_t instruction) {
    CPU::dadd(cpu, instruction);
}
void CPU::dsub(CPU* cpu, uint32_t instruction) {
    cpu->r[getRd(instruction)] = cpu->r[getRs(instruction)] - cpu->r[getRt(instruction)];
}
void CPU::dsubu(CPU* cpu, uint32_t instruction) {
    CPU::dsub(cpu, instruction);
}
void CPU::tge(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tge\n");
}
void CPU::tgeu(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tgeu\n");
}
void CPU::tlt(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tlt\n");
}
void CPU::tltu(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tltu\n");
}
void CPU::teq(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: teq\n");
}
void CPU::tne(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tne\n");
}
void CPU::dsll(CPU* cpu, uint32_t instruction) {
    uint32_t shift = shiftAmount(instruction);
    cpu->r[getRd(instruction)] = cpu->r[getRt(instruction)] << shift;
}
void CPU::dsrl(CPU* cpu, uint32_t instruction) {
    uint32_t shift = shiftAmount(instruction);
    cpu->r[getRd(instruction)] = cpu->r[getRt(instruction)] >> shift;
}
void CPU::dsra(CPU* cpu, uint32_t instruction) {
    uint32_t shift = shiftAmount(instruction);
    cpu->r[getRd(instruction)] = (uint64_t)((int64_t)cpu->r[getRt(instruction)] >> shift);
}
void CPU::dsll32(CPU* cpu, uint32_t instruction) {
    uint32_t shift = shiftAmount(instruction) + 32;
    cpu->r[getRd(instruction)] = cpu->r[getRt(instruction)] << shift;
}
void CPU::dsrl32(CPU* cpu, uint32_t instruction) {
    uint32_t shift = shiftAmount(instruction) + 32;
    cpu->r[getRd(instruction)] = cpu->r[getRt(instruction)] >> shift;
}
void CPU::dsra32(CPU* cpu, uint32_t instruction) {
    uint32_t shift = shiftAmount(instruction) + 32;
    cpu->r[getRd(instruction)] = (uint64_t)((int64_t)cpu->r[getRt(instruction)] >> shift);
}

void CPU::bltz(CPU* cpu, uint32_t instruction) {
    if ((int64_t)cpu->r[getRs(instruction)] < 0) {
        uint32_t immediate = getImmediate(instruction);

        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }
    cpu->inDelaySlot = true;
}
void CPU::bgez(CPU* cpu, uint32_t instruction) {
    if ((int64_t)cpu->r[getRs(instruction)] >= 0) {
        uint32_t immediate = getImmediate(instruction);

        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }
    cpu->inDelaySlot = true;
}
void CPU::bltzl(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    if ((int64_t)cpu->r[rs] < 0) {
        uint32_t immediate = getImmediate(instruction);
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;

        cpu->inDelaySlot = true;
    } else {
        cpu->pc = cpu->nextPc;
        cpu->discarded = true;
    }
}
void CPU::bgezl(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    if ((int64_t)cpu->r[rs] >= 0) {
        uint32_t immediate = getImmediate(instruction);
        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;

        cpu->inDelaySlot = true;
    } else {
        cpu->pc = cpu->nextPc;
        cpu->discarded = true;
    }
}

void CPU::tgei(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    int64_t immediate = (int64_t)getSignedImmediate(instruction);
    int64_t value = (int64_t)cpu->r[rs];

    if (value > immediate) {
        // throw an exception here!
        throw std::runtime_error("TODO: throw exception\n");

    }
}
void CPU::tgeiu(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tgeiu\n");
}
void CPU::tlti(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tlti\n");
}
void CPU::tltiu(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tltiu\n");
}
void CPU::teqi(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: teqi\n");
}

void CPU::tnei(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: tnei\n");
}

void CPU::bltzal(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    cpu->r[31] = cpu->nextPc;
    if ((int64_t)cpu->r[rs] < 0) {
        uint32_t immediate = getImmediate(instruction);

        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }

    cpu->inDelaySlot = true;
}
void CPU::bgezal(CPU* cpu, uint32_t instruction) {
    uint32_t rs = getRs(instruction);

    uint64_t nextPc = cpu->nextPc;
    if ((int64_t)cpu->r[rs] >= 0) {
        uint32_t immediate = getImmediate(instruction);

        uint64_t amount = (int16_t)(int64_t)(uint64_t)(immediate << 2);

        cpu->fastForwardRelativeLoop((int16_t)amount);

        cpu->nextPc = cpu->pc + amount;
    }

    cpu->r[31] = nextPc;

    cpu->inDelaySlot = true;
}
void CPU::bltzall(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: bltzall\n");
}
void CPU::bgezall(CPU* cpu, uint32_t instruction) {
    throw std::runtime_error("TODO: bgezall\n");
}