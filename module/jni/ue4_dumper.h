#pragma once
#include <cstdint>
#include <string>
#include "config.h"

namespace ue4 {

// Runs the full Unreal Engine 4 auto-detection and dump pipeline.
bool dump_ue4(const Config &cfg, const std::string &package, uintptr_t lib_base, size_t lib_size);

}  // namespace ue4
