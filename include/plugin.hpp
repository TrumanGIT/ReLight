#pragma once

#include <RE/Skyrim.h>
#include <RE/V/VirtualMachine.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

using namespace std::literals;
using namespace std;

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()

