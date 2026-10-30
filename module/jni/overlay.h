#pragma once
#include <jni.h>
// Installs the touch hook and the GLES / Vulkan rendering hooks, then renders the ImGui menu.
namespace overlay {
void install(JNIEnv *env);
}
