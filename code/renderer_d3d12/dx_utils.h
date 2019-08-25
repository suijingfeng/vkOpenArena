#pragma once

#include "DXGI1_4.h"
void printAvailableAdapters_f(void);
unsigned int getNumberOfAvailableAdapters(IDXGIFactory2* const pHardwareFactory);
void printWideStr(wchar_t * const WStr);

void printOutputInfo(IDXGIAdapter1* const pAdapter);