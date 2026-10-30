#pragma once
// Backend-independent ImGui menu + touch handling (shared by the GLES and Vulkan paths).
namespace ui {

// Call once after ImGui::CreateContext().
void init_style();
// Call before ImGui::NewFrame(): sets display size / delta time and applies queued touches.
void begin_frame(int w, int h);
// Builds the menu widgets (between NewFrame and Render).
void draw();

// Touch entry point (thread-safe, called from the game's UI thread).
// action: android MotionEvent action masked (0 down, 1 up, 2 move, 3 cancel).
// Returns true if the event was consumed by the overlay and must not reach the game.
bool touch(float x, float y, int action);

}  // namespace ui
