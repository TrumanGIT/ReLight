#pragma once

#include "logger.hpp"

namespace raycast {

bool TESRayHitStatic(RE::bhkWorld* world, RE::NiPoint3 start, RE::NiPoint3 end);

bool HasAnythingBetween(RE::NiPoint3 start, RE::NiPoint3 end);

}