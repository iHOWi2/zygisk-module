#pragma once
#include <jni.h>
#include <cstdint>

namespace input {
void install(JNIEnv *env);
void hook_unity(const char *path, uintptr_t base);
}
