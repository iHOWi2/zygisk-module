#include <fcntl.h>
#include <jni.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "config.h"
#include "il2cpp_dumper.h"
#include "overlay.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

static bool read_all(int fd, void *buf, size_t n) {
    auto *p = static_cast<char *>(buf);
    while (n) {
        ssize_t r = read(fd, p, n);
        if (r <= 0) return false;
        p += r;
        n -= r;
    }
    return true;
}

static void write_all(int fd, const void *buf, size_t n) {
    auto *p = static_cast<const char *>(buf);
    while (n) {
        ssize_t r = write(fd, p, n);
        if (r <= 0) return;
        p += r;
        n -= r;
    }
}

class DumperModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *name = env_->GetStringUTFChars(args->nice_name, nullptr);
        std::string proc = name ? name : "";
        if (name) env_->ReleaseStringUTFChars(args->nice_name, name);

        // Ask the root companion for the config text (app process cannot read /data/adb).
        std::string text;
        int fd = api_->connectCompanion();
        if (fd >= 0) {
            uint32_t len = 0;
            if (read_all(fd, &len, sizeof len) && len < (1u << 20)) {
                text.resize(len);
                if (len && !read_all(fd, text.data(), len)) text.clear();
            }
            close(fd);
        }
        cfg_ = parse_config(text);
        if (cfg_.matches(proc)) {
            target_ = true;
            proc_ = proc;
        } else {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (!target_) return;
        dumper::start(cfg_, proc_);
        overlay::install(env_);
    }

private:
    Api *api_ = nullptr;
    JNIEnv *env_ = nullptr;
    Config cfg_;
    std::string proc_;
    bool target_ = false;
};

static void companion_handler(int client) {
    std::string text;
    int fd = open(kConfigPath, O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        char buf[4096];
        ssize_t r;
        while ((r = read(fd, buf, sizeof buf)) > 0) text.append(buf, r);
        close(fd);
    }
    uint32_t len = static_cast<uint32_t>(text.size());
    write_all(client, &len, sizeof len);
    write_all(client, text.data(), len);
}

REGISTER_ZYGISK_MODULE(DumperModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
