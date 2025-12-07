module;
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
module vk.plugins.viewport;

void vk::plugins::ViewportRenderer::query_required_device_caps(context::RendererCaps& caps) {caps.allow_async_compute = false;}
void vk::plugins::ViewportRenderer::get_capabilities(context::RendererCaps& caps) {
    caps                            = context::RendererCaps{};
    caps.presentation_mode          = context::PresentationMode::EngineBlit;
    caps.preferred_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    caps.color_attachments          = {context::AttachmentRequest{.name = "color", .format = VK_FORMAT_B8G8R8A8_UNORM, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, .samples = VK_SAMPLE_COUNT_1_BIT, .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .initial_layout = VK_IMAGE_LAYOUT_GENERAL}};
    caps.presentation_attachment    = "color";
}
void vk::plugins::ViewportRenderer::initialize(const context::EngineContext& eng, const context::RendererCaps& caps, const context::FrameContext& frm) {}
void vk::plugins::ViewportRenderer::destroy(const context::EngineContext& eng, const context::RendererCaps& caps) {}
void vk::plugins::ViewportRenderer::record_graphics(VkCommandBuffer& cmd, const context::EngineContext& eng, const context::FrameContext& frm) {}

void vk::plugins::ViewportUI::create_imgui(const context::EngineContext& eng, VkFormat format, uint32_t n_swapchain_image) {}
void vk::plugins::ViewportUI::destroy_imgui(const context::EngineContext& eng) {}
void vk::plugins::ViewportUI::process_event(const SDL_Event& event) {}
void vk::plugins::ViewportUI::record_imgui(VkCommandBuffer& cmd, const context::FrameContext& frm) {}
