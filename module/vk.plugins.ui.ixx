module;
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
export module vk.plugins.ui;
import vk.context;


namespace vk::plugins {
    export class UISystemDefault {
    public:
        void set_main_window_title(const char* title);
        void create_imgui(const context::EngineContext& e, VkFormat format, uint32_t n_swapchain_image);
        void destroy_imgui(const context::EngineContext& e);
        void process_event(const SDL_Event& e);
        VkDescriptorPool pool_{VK_NULL_HANDLE};
        VkFormat color_format_{};
    };
} // namespace vk::plugins
