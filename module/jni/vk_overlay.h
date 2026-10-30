#pragma once
namespace vkov {
// Installs hooks on libvulkan's vkGet{Instance,Device}ProcAddr so we can wrap
// vkCreateSwapchainKHR / vkQueuePresentKHR and draw ImGui on top of the game's frames.
void install();
}
