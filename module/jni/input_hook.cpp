#include "input_hook.h"

#include <android/input.h>
#include <android/log.h>
#include <jni.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "il2cpp_dumper.h"
#include "shadowhook.h"
#include "ui.h"

namespace input {

using InjectFn = jboolean (*)(JNIEnv *, jobject, jobject);
static InjectFn orig_inject = nullptr;

static jclass g_motion_cls = nullptr;
static jmethodID g_get_action = nullptr, g_get_x = nullptr, g_get_y = nullptr;

static jboolean my_inject(JNIEnv *env, jobject thiz, jobject ev) {
    if (ev) {
        if (!g_motion_cls) {
            jclass c = env->FindClass("android/view/MotionEvent");
            if (c) {
                g_motion_cls = (jclass)env->NewGlobalRef(c);
                g_get_action = env->GetMethodID(c, "getActionMasked", "()I");
                g_get_x = env->GetMethodID(c, "getX", "()F");
                g_get_y = env->GetMethodID(c, "getY", "()F");
                env->DeleteLocalRef(c);
            }
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        if (g_motion_cls && env->IsInstanceOf(ev, g_motion_cls)) {
            int action = env->CallIntMethod(ev, g_get_action);
            float x = env->CallFloatMethod(ev, g_get_x);
            float y = env->CallFloatMethod(ev, g_get_y);
            if (env->ExceptionCheck()) env->ExceptionClear();
            else {
                bool hit = ui::touch(x, y, action);
                if (hit) return JNI_TRUE;
            }
        }
    }
    return orig_inject ? orig_inject(env, thiz, ev) : JNI_FALSE;
}

// -----------------------------------------------------------------------------
// Universal AInputQueue hook for Unreal Engine and native games
// -----------------------------------------------------------------------------
static int32_t (*orig_getEvent)(AInputQueue *, AInputEvent **) = nullptr;

static int32_t hook_getEvent(AInputQueue *q, AInputEvent **out) {
    int32_t res = orig_getEvent ? orig_getEvent(q, out) : -1;
    if (res >= 0 && out && *out) {
        int32_t type = AInputEvent_getType(*out);
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(*out) & AMOTION_EVENT_ACTION_MASK;
            float x = AMotionEvent_getX(*out, 0);
            float y = AMotionEvent_getY(*out, 0);
            if (ui::touch(x, y, action)) {
                AInputQueue_finishEvent(q, *out, 1);
                return -1;
            }
        }
    }
    return res;
}

static uintptr_t find_native_inject_offset(const char *path) {
    if (!path || !path[0]) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 150 * 1024 * 1024) { fclose(f); return 0; }

    std::vector<uint8_t> buf(sz);
    if (fread(buf.data(), 1, sz, f) != (size_t)sz) { fclose(f); return 0; }
    fclose(f);

    const char target[] = "nativeInjectEvent";
    const uint8_t *p = buf.data();
    uintptr_t str_pos = 0;
    for (size_t i = 0; i + sizeof(target) <= (size_t)sz; ++i) {
        if (!memcmp(p + i, target, sizeof(target))) {
            str_pos = i;
            break;
        }
    }
    if (!str_pos) return 0;

    for (size_t i = 0; i + 24 <= (size_t)sz; i += 8) {
        uint64_t p1 = *reinterpret_cast<const uint64_t *>(p + i);
        if (p1 == str_pos) {
            uint64_t p2 = *reinterpret_cast<const uint64_t *>(p + i + 8);
            if (p2 < (uint64_t)sz && strstr(reinterpret_cast<const char *>(p + p2), "InputEvent")) {
                uint64_t p3 = *reinterpret_cast<const uint64_t *>(p + i + 16);
                dumper::log("Found nativeInjectEvent in ELF: fn_offset=0x%lx", (unsigned long)p3);
                return p3;
            }
        }
    }
    return 0;
}

void hook_unity(const char *path, uintptr_t base) {
    if (orig_inject) return;

    uintptr_t fn_off = find_native_inject_offset(path);
    if (!fn_off) {
        fn_off = 0x3ca8b4; // Fallback offset for CounterFlame / Unity 2021.3 ARM64
    }

    void *fn_addr = reinterpret_cast<void *>(base + fn_off);
    void *stub = shadowhook_hook_func_addr(fn_addr,
                                           reinterpret_cast<void *>(my_inject),
                                           reinterpret_cast<void **>(&orig_inject));
    dumper::log("Unity nativeInjectEvent hook: addr=%p, stub=%p, err=%d",
                fn_addr, stub, shadowhook_get_errno());
}

void install(JNIEnv *env) {
    shadowhook_hook_sym_name("libandroid.so", "AInputQueue_getEvent",
                             reinterpret_cast<void *>(hook_getEvent),
                             reinterpret_cast<void **>(&orig_getEvent));
}

}  // namespace input
