#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include "config.h"

namespace dumper {

enum class Engine { Detecting, Unity, UE4 };
enum class State { WaitingForLib, WaitingForRuntime, Ready, Dumping, Done, Failed };

struct Status {
    std::atomic<Engine> engine{Engine::Detecting};
    std::atomic<State> state{State::WaitingForLib};
    std::atomic<uintptr_t> il2cpp_base{0};
    std::atomic<bool> request{false};
    std::string unity_version;  // guarded by log mutex / engine detail string
};

Status &status();

// Start background thread: auto-detects Unity (libil2cpp.so) or UE4 (libUE4.so) and dumps automatically.
void start(const Config &cfg, const std::string &package);

// Ask the worker to perform the dump.
void request_dump();

// Thread-safe log access for the overlay.
std::string log_snapshot();
void log(const char *fmt, ...);
// Current stage description shown in the overlay under the state line.
std::string status_detail();
void set_status_detail(const char *fmt, ...);
const char *state_name(State s);
const char *engine_name(Engine e);
std::string output_dir();
void set_output_dir(const std::string &dir);

// Kernel-validated memory read (never raises SIGSEGV/SIGBUS).
bool safe_read(uintptr_t addr, void *buf, size_t len);

}  // namespace dumper
