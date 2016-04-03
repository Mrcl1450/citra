// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/arm/jit_x64/jit_x64.h"

namespace JitX64 {

void JitX64::USAD8() { CompileInterpretInstruction(); }
void JitX64::USADA8() { CompileInterpretInstruction(); }

}