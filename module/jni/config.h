#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Plain-text config (key=value, '#' comments). Read by the Zygisk companion (root)
// from the module dir and sent to the app process.
//   package=com.example.game      (repeatable)
//   output=/sdcard/Download/il2cpp_dump
//   dump_metadata=1 / dump_so=1 / dump_script_json=1
//
// UE4 (optional). 0 = auto-detect. Values may be RVA from libUE4.so base
// or an absolute VA (auto-distinguished).
//   ue4_gnames=0x4B7C3C0          GNames object/pointer or Blocks[] array
//   ue4_guobjectarray=0x4C2E1A0   GUObjectArray / TUObjectArray
//   ue4_gnames_mode=fnamepool | tnameentryarray
//   ue4_fnamepool_stride=2        entry stride in bytes (2 or 4)
//   ue4_fnamepool_shift=6         header >> shift == length
//   ue4_guobj_mode=chunked | fixed
//   ue4_item_size=24              sizeof(FUObjectItem)
//   ue4_elements_per_chunk=65536
//   ue4_num_elements=0            object count (overrides header value)
//   ue4_off_internal_index / ue4_off_class / ue4_off_name / ue4_off_outer
//   ue4_off_super / ue4_off_children / ue4_off_child_props
//   ue4_off_properties_size / ue4_off_prop_offset
//   ue4_use_ffield=0 | 1
struct Config {
    std::vector<std::string> packages;
    std::string output = "auto";  // auto = /sdcard/Android/data/<pkg>/files/il2cpp_dump
    bool dump_cs = true;
    bool dump_metadata = true;
    bool dump_so = true;
    bool dump_script_json = true;

    // UE4 manual offsets / structure hints (0 or empty = auto-detect).
    uint64_t ue4_gnames = 0;
    uint64_t ue4_guobjectarray = 0;
    int ue4_gnames_mode = 0;      // 0 auto, 1 FNamePool, 2 TNameEntryArray
    int ue4_guobj_mode = 0;       // 0 auto, 1 Chunked, 2 Fixed
    uint32_t ue4_fnamepool_stride = 0;
    uint32_t ue4_fnamepool_shift = 0;
    uint32_t ue4_item_size = 0;
    uint32_t ue4_elements_per_chunk = 0;
    uint32_t ue4_num_elements = 0;
    uint32_t ue4_off_internal_index = 0;
    uint32_t ue4_off_class = 0;
    uint32_t ue4_off_name = 0;
    uint32_t ue4_off_outer = 0;
    uint32_t ue4_off_super = 0;
    uint32_t ue4_off_children = 0;
    uint32_t ue4_off_child_props = 0;
    uint32_t ue4_off_properties_size = 0;
    uint32_t ue4_off_prop_offset = 0;
    int ue4_use_ffield = -1;      // -1 auto, 0 UField, 1 FField

    // Parse diagnostics (invalid values, unknown ue4_* keys) - logged on start.
    std::vector<std::string> notes;

    bool matches(const std::string &proc) const;
};

constexpr const char *kConfigPath = "/data/adb/modules/zygisk_il2cpp_dumper/config.txt";

Config parse_config(const std::string &text);
