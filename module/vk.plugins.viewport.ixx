module;
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
export module vk.plugins.viewport;
import vk.engine;
import vk.context;

namespace vk::plugins {
    export class ViewportRenderer {
    public:
        void query_required_device_caps(context::RendererCaps& caps);
        void get_capabilities(context::RendererCaps& caps);
        void initialize(const context::EngineContext& eng, const context::RendererCaps& caps, const context::FrameContext& frm);
        void destroy(const context::EngineContext& eng, const context::RendererCaps& caps);
        void record_graphics(VkCommandBuffer& cmd, const context::EngineContext& eng, const context::FrameContext& frm);
    };
    export class ViewportUI {
    public:
        void create_imgui(const context::EngineContext& eng, VkFormat format, uint32_t n_swapchain_image);
        void destroy_imgui(const context::EngineContext& eng);
        void process_event(const SDL_Event& event);
        void record_imgui(VkCommandBuffer& cmd, const context::FrameContext& frm);
    };
} // namespace vk::plugins
