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

#ifndef RY_CANVAS_H
#define RY_CANVAS_H

#include <chrono>
#include <wx/wx.h>
#include <wx/glcanvas.h>

class ryFrame;

class ryCanvas: public wxGLCanvas
{
    public:
        ryCanvas(ryFrame *frame);

        void finish();

    private:
        ryFrame *frame;
        wxGLContext *context;

        int frameCount = 0;
        int swapInterval = 0;
        int refreshRate = 0;
        std::chrono::steady_clock::time_point lastRateTime;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t x = 0;
        uint32_t y = 0;

        uint8_t sizeReset = 0;
        bool fullScreen = false;
        bool finished = false;

        void draw(wxPaintEvent &event);
        void resize(wxSizeEvent &event);
        void pressKey(wxKeyEvent &event);
        void releaseKey(wxKeyEvent &event);

        wxDECLARE_EVENT_TABLE();
};

#endif // RY_CANVAS_H
