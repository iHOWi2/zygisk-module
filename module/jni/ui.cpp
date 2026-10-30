#include "ui.h"

#include <cfloat>
#include <chrono>
#include <mutex>
#include <vector>

#include "il2cpp_dumper.h"
#include "imgui.h"

namespace ui {

struct TouchEv { float x, y; int action; };

static std::mutex g_mtx;
static std::vector<TouchEv> g_queue;
static float g_rect[2][4] = {};                // [0]=menu, [1]=toggle button: x,y,w,h (0 size = hidden)
static bool g_show = true;
static float g_scale = 2.5f;

bool touch(float x, float y, int action) {
    std::lock_guard<std::mutex> l(g_mtx);
    g_queue.push_back({x, y, action});
    return false; // Do not swallow touches; keep game controls smooth and active
}

void init_style() {
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    ImGuiStyle &st = ImGui::GetStyle();
    st.ScaleAllSizes(g_scale);
    st.FontScaleMain = g_scale;
    st.TouchExtraPadding = ImVec2(6.0f, 6.0f);
}

void begin_frame(int w, int h) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    if (dt > 0.1f) dt = 0.016f;
    io.DeltaTime = dt < 1e-4f ? 1e-4f : dt;
    last = now;

    std::vector<TouchEv> q;
    {
        std::lock_guard<std::mutex> l(g_mtx);
        q.swap(g_queue);
    }
    for (auto &e : q) {
        io.AddMousePosEvent(e.x, e.y);
        if (e.action == 0 || e.action == 5) {
            io.AddMouseButtonEvent(0, true);
        } else if (e.action == 1 || e.action == 3 || e.action == 6) {
            io.AddMouseButtonEvent(0, false);
        }
    }
}

void draw() {
    g_show = true; // Always keep overlay alive
    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.05f, io.DisplaySize.y * 0.05f),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.65f),
                             ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Universal Dumper (Unity / UE4)", nullptr, ImGuiWindowFlags_NoCollapse)) {
        auto &st = dumper::status();
        auto eng = st.engine.load();
        auto s = st.state.load();
        ImGui::Text("Engine: %s | State: %s", dumper::engine_name(eng), dumper::state_name(s));
        std::string detail = dumper::status_detail();
        if (!detail.empty()) {
            ImGui::TextWrapped("Stage: %s", detail.c_str());
        }
        if (st.il2cpp_base.load()) {
            ImGui::Text("Target Lib Base: 0x%llx", (unsigned long long)st.il2cpp_base.load());
        }
        std::string out = dumper::output_dir();
        if (!out.empty()) {
            ImGui::TextWrapped("Output: %s", out.c_str());
        }

        ImGui::Separator();
        if (ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
            std::string l = dumper::log_snapshot();
            ImGui::TextUnformatted(l.c_str());
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImVec2 p = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
    float menu[4] = { p.x, p.y, sz.x, sz.y };
    ImGui::End();

    std::lock_guard<std::mutex> l(g_mtx);
    memcpy(g_rect[0], menu, sizeof menu);
    memset(g_rect[1], 0, sizeof g_rect[1]);
}

}  // namespace ui
