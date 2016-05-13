// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "common/bit_set.h"
#include "emitter.h"

// x64 ABI:s, and helpers to help follow them when JIT-ing code.
// All convensions return values in EAX (+ possibly EDX).

// Windows 64-bit
// * 4-reg "fastcall" variant, very new-skool stack handling
// * Callee moves stack pointer, to make room for shadow regs for the biggest function _it itself calls_
// * Parameters passed in RCX, RDX, ... further parameters are MOVed into the allocated stack space.
// Scratch:      RAX RCX RDX R8 R9 R10 R11
// Callee-save:  RBX RSI RDI RBP R12 R13 R14 R15
// Parameters:   RCX RDX R8 R9, further MOV-ed

// Linux 64-bit
// * 6-reg "fastcall" variant, old skool stack handling (parameters are pushed)
// Scratch:      RAX RCX RDX RSI RDI R8 R9 R10 R11
// Callee-save:  RBX RBP R12 R13 R14 R15
// Parameters:   RDI RSI RDX RCX R8 R9

#define ABI_ALL_FPRS BitSet32(0xffff0000)
#define ABI_ALL_GPRS BitSet32(0x0000ffff)

#ifdef _WIN32 // 64-bit Windows - the really exotic calling convention

#define ABI_PARAM1 ::Gen::RCX
#define ABI_PARAM2 ::Gen::RDX
#define ABI_PARAM3 ::Gen::R8
#define ABI_PARAM4 ::Gen::R9

// xmm0-xmm15 use the upper 16 bits in the functions that push/pop registers.
#define ABI_ALL_CALLER_SAVED \
    (BitSet32 { ::Gen::RAX, ::Gen::RCX, ::Gen::RDX, ::Gen::R8, ::Gen::R9, ::Gen::R10, ::Gen::R11, \
                ::Gen::XMM0+16, ::Gen::XMM1+16, ::Gen::XMM2+16, ::Gen::XMM3+16, ::Gen::XMM4+16, ::Gen::XMM5+16 })
#else //64-bit Unix / OS X

#define ABI_PARAM1 ::Gen::RDI
#define ABI_PARAM2 ::Gen::RSI
#define ABI_PARAM3 ::Gen::RDX
#define ABI_PARAM4 ::Gen::RCX
#define ABI_PARAM5 ::Gen::R8
#define ABI_PARAM6 ::Gen::R9

// TODO: Avoid pushing all 16 XMM registers when possible. Most functions we call probably
// don't actually clobber them.
#define ABI_ALL_CALLER_SAVED \
    (BitSet32 { ::Gen::RAX, ::Gen::RCX, ::Gen::RDX, ::Gen::RDI, ::Gen::RSI, ::Gen::R8, ::Gen::R9, ::Gen::R10, ::Gen::R11 } | \
     ABI_ALL_FPRS)
#endif // WIN32

#define ABI_ALL_CALLEE_SAVED (~ABI_ALL_CALLER_SAVED)

#define ABI_RETURN ::Gen::RAX
