/******************************************************************************************
*	Chili Direct3D Engine																  *
*	Copyright 2018 PlanetChili <http://www.planetchili.net>								  *
*																						  *
*	This file is part of Chili Direct3D Engine.											  *
*																						  *
*	Chili Direct3D Engine is free software: you can redistribute it and/or modify		  *
*	it under the terms of the GNU General Public License as published by				  *
*	the Free Software Foundation, either version 3 of the License, or					  *
*	(at your option) any later version.													  *
*																						  *
*	The Chili Direct3D Engine is distributed in the hope that it will be useful,		  *
*	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
*	GNU General Public License for more details.										  *
*																						  *
*	You should have received a copy of the GNU General Public License					  *
*	along with The Chili Direct3D Engine.  If not, see <http://www.gnu.org/licenses/>.    *
******************************************************************************************/

// target Windows 7 or later

// The following #defines disable a bunch of unused windows stuff. If you 
// get weird errors when trying to do some windows stuff, try removing some
// (or all) of these defines (it will increase build time though).

#ifndef FULL_WINTARD
/*
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOKERNEL
#define NONLS
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE
*/
#endif
#define NOMINMAX
#define STRICT
#pragma once
#include <windows.h>
#include <minwindef.h>
#include <winnt.h>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <cstdint>
#include <sdkddkver.h>



#ifndef DLL
#ifdef DESCDLL
#define DLL __declspec( dllexport )
#else
#define DLL __declspec( dllimport )
#endif
#endif

#define IDI_ICON1 101

#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

//#define IMGUI_DISABLE

#include <wrl.h>
#ifdef _USEDXC
#include <Support/Errorcodes.h>
#endif

#include <mutex>
#include <atomic>
struct thRect {
    std::atomic<bool> Updated;
    RECT wr = {};
    std::mutex Mtx;
};

#ifdef __MINGW32__
constexpr uint8_t nybble_from_hex(char c) {
  return ((c >= '0' && c <= '9')
              ? (c - '0')
              : ((c >= 'a' && c <= 'f')
                     ? (c - 'a' + 10)
                     : ((c >= 'A' && c <= 'F') ? (c - 'A' + 10)
                                               : /* Should be an error */ -1)));
}

constexpr uint8_t byte_from_hex(char c1, char c2) {
  return nybble_from_hex(c1) << 4 | nybble_from_hex(c2);
}

constexpr uint8_t byte_from_hexstr(const char str[2]) {
  return nybble_from_hex(str[0]) << 4 | nybble_from_hex(str[1]);
}

constexpr GUID guid_from_string(const char str[37]) {
  return GUID{static_cast<uint32_t>(byte_from_hexstr(str)) << 24 |
                  static_cast<uint32_t>(byte_from_hexstr(str + 2)) << 16 |
                  static_cast<uint32_t>(byte_from_hexstr(str + 4)) << 8 |
                  byte_from_hexstr(str + 6),
              static_cast<uint16_t>(
                  static_cast<uint16_t>(byte_from_hexstr(str + 9)) << 8 |
                  byte_from_hexstr(str + 11)),
              static_cast<uint16_t>(
                  static_cast<uint16_t>(byte_from_hexstr(str + 14)) << 8 |
                  byte_from_hexstr(str + 16)),
              {byte_from_hexstr(str + 19), byte_from_hexstr(str + 21),
               byte_from_hexstr(str + 24), byte_from_hexstr(str + 26),
               byte_from_hexstr(str + 28), byte_from_hexstr(str + 30),
               byte_from_hexstr(str + 32), byte_from_hexstr(str + 34)}};
}
/*
#ifndef CROSS_PLATFORM_UUIDOF
#define CROSS_PLATFORM_UUIDOF(interface, spec)                                 \
  struct interface;                                                            \
  template <> inline const GUID &__mingw_uuidof<interface>() {       \
    static const IID _IID = guid_from_string(spec);                            \
    return _IID;                                                               \
}
#endif

*/

#endif


