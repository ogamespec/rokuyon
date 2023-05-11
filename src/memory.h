/*
    Copyright 2022-2023 Hydr8gon

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

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

namespace Memory
{
    void reset();
    void getEntry(uint32_t index, uint32_t &entryLo0, uint32_t &entryLo1, uint32_t &entryHi, uint32_t &pageMask);
    void setEntry(uint32_t index, uint32_t  entryLo0, uint32_t  entryLo1, uint32_t  entryHi, uint32_t  pageMask);

    template <typename T> T read(uint32_t address);
    template <typename T> void write(uint32_t address, T value);
}

#endif // MEMORY_H
