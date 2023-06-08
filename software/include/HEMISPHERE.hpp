#pragma once
// Copyright (c) 2018, Jason Justian
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "oc/apps.h"
#include "oc/ui.h"
#include "hemisphere/manager.hpp"

static hemisphere::Manager manager;

void ReceiveManagerSysEx();

////////////////////////////////////////////////////////////////////////////////
//// O_C App Functions
////////////////////////////////////////////////////////////////////////////////

// App stubs
void HEMISPHERE_init();
size_t HEMISPHERE_storageSize();
size_t HEMISPHERE_save(void *storage);
size_t HEMISPHERE_restore(const void *storage);
void FASTRUN HEMISPHERE_isr();
void HEMISPHERE_handleAppEvent(oc::AppEvent event);
void HEMISPHERE_loop(); // Essentially deprecated in favor of ISR
void HEMISPHERE_menu();
void HEMISPHERE_screensaver(); // Deprecated in favor of screen blanking
void HEMISPHERE_handleButtonEvent(const UI::Event &event);
void HEMISPHERE_handleEncoderEvent(const UI::Event &event);
