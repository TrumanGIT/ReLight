#pragma once

#include <RE/Skyrim.h>
#include <RE/V/VirtualMachine.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

using namespace std::literals;
using namespace std;

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()

/*SKSEPluginInfo(
    .Version = REL::Version{ 5, 4, 0, 0 },
    .Name = "Relight",
    .Author = "Truman",
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
);*/