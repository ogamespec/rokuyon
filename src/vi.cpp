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

#include <atomic>
#include <cstddef>
#include <cstring>
#include <queue>
#include <mutex>

#include "vi.h"
#include "core.h"
#include "log.h"
#include "memory.h"
#include "mi.h"
#include "rdp.h"

namespace VI
{
    std::queue<_Framebuffer*> framebuffers;
    std::atomic<bool> ready;
    std::mutex mutex;

    uint32_t control;
    uint32_t origin;
    uint32_t width;
    uint32_t hVideo;
    uint32_t vVideo;
    uint32_t xScale;
    uint32_t yScale;

    void drawFrame();
}

_Framebuffer *VI::getFramebuffer()
{
    // Wait until a new frame is ready
    if (!ready.load())
        return nullptr;

    // Get the next frame in the queue
    mutex.lock();
    _Framebuffer *fb = framebuffers.front();
    framebuffers.pop();
    ready.store(!framebuffers.empty());
    mutex.unlock();
    return fb;
}

void VI::reset()
{
    // Reset the VI to its initial state
    control = 0;
    origin = 0;
    width = 0;
    hVideo = 0;
    vVideo = 0;
    xScale = 0;
    yScale = 0;

    // Schedule the first frame to be drawn
    Core::schedule(drawFrame, (93750000 / 60) * 2);
}

uint32_t VI::read(uint32_t address)
{
    // Read from an I/O register if one exists at the given address
    switch (address)
    {
        default:
            LOG_WARN("Unknown VI register read: 0x%X\n", address);
            return 0;
    }
}

void VI::write(uint32_t address, uint32_t value)
{
    // Write to an I/O register if one exists at the given address
    switch (address)
    {
        case 0x4400000: // VI_CONTROL
            // Set the VI control register
            // TODO: actually use bits other than type
            control = (value & 0x1FBFF);
            return;

        case 0x4400004: // VI_ORIGIN
            // Set the framebuffer address
            origin = 0x80000000 | (value & 0xFFFFFF);
            return;

        case 0x4400008: // VI_WIDTH
            // Set the framebuffer width in pixels
            width = (value & 0xFFF);
            return;

        case 0x4400010: // VI_V_CURRENT
            // Acknowledge a VI interrupt instead of writing a value
            MI::clearInterrupt(3);
            return;

        case 0x4400024: // VI_H_VIDEO
        {
            // Set the range of visible horizontal pixels
            uint32_t start = (value >> 16) & 0x3FF;
            uint32_t end   = (value >>  0) & 0x3FF;
            hVideo = (end - start);
            return;
        }

        case 0x4400028: // VI_V_VIDEO
        {
            // Set the range of visible vertical pixels
            uint32_t start = (value >> 16) & 0x3FF;
            uint32_t end   = (value >>  0) & 0x3FF;
            vVideo = (end - start) / 2;
            return;
        }

        case 0x4400030: // VI_X_SCALE
            // Set the framebuffer X-scale
            // TODO: actually use offset value
            xScale = (value & 0xFFF);
            return;

        case 0x4400034: // VI_Y_SCALE
            // Set the framebuffer Y-scale
            // TODO: actually use offset value
            yScale = (value & 0xFFF);
            return;

        default:
            LOG_WARN("Unknown VI register write: 0x%X\n", address);
            return;
    }
}

void VI::drawFrame()
{
    // Ensure the RDP thread has finished drawing
    RDP::finishThread();

    // Allow up to 2 framebuffers to be queued, to preserve frame pacing if emulation runs ahead
    if (framebuffers.size() < 2)
    {
        // Create a new framebuffer
        _Framebuffer *fb = new _Framebuffer();
        fb->width  = ((xScale ? xScale : 0x200) * hVideo) >> 10;
        fb->height = ((yScale ? yScale : 0x200) * vVideo) >> 10;
        fb->data = new uint32_t[fb->width * fb->height];

        // Clear the screen if there's nothing to display
        if (fb->width == 0 || fb->height == 0)
        {
            fb->width = 8;
            fb->height = 8;
            delete[] fb->data;
            fb->data = new uint32_t[fb->width * fb->height];
            goto clear;
        }

        // Read the framebuffer from N64 memory
        switch (control & 0x3) // Type
        {
            case 0x3: // 32-bit
                // Translate pixels from RGB_8888 to ARGB8888
                for (uint32_t y = 0; y < fb->height; y++)
                {
                    for (uint32_t x = 0; x < fb->width; x++)
                    {
                        uint32_t color = Memory::read<uint32_t>(origin + ((y * width + x) << 2));
                        uint8_t r = (color >> 24) & 0xFF;
                        uint8_t g = (color >> 16) & 0xFF;
                        uint8_t b = (color >>  8) & 0xFF;
                        fb->data[y * fb->width + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
                    }
                }
                break;

            case 0x2: // 16-bit
                // Translate pixels from RGB_5551 to ARGB8888
                for (uint32_t y = 0; y < fb->height; y++)
                {
                    for (uint32_t x = 0; x < fb->width; x++)
                    {
                        uint16_t color = Memory::read<uint16_t>(origin + ((y * width + x) << 1));
                        uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
                        uint8_t g = ((color >>  6) & 0x1F) * 255 / 31;
                        uint8_t b = ((color >>  1) & 0x1F) * 255 / 31;
                        fb->data[y * fb->width + x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
                    }
                }
                break;

            default:
            clear:
                // Don't show anything
                memset(fb->data, 0, fb->width * fb->height * sizeof(uint32_t));
                break;
        }

        // Add the frame to the queue
        mutex.lock();
        framebuffers.push(fb);
        ready.store(true);
        mutex.unlock();
    }

    // Finish the frame and request a VI interrupt
    // TODO: request interrupt at the proper time
    MI::setInterrupt(3);

    // Schedule the next frame to be drawn
    Core::schedule(drawFrame, (93750000 / 60) * 2);
    Core::countFrame();
}
