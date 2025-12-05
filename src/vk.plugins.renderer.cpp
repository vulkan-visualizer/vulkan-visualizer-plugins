module;
#include <vulkan/vulkan.h>
module vk.plugins.renderer;
import vk.context;

void vk::plugins::RendererTriangle::query_required_device_caps(context::RendererCaps& c) {
    c.allow_async_compute = false;
}
void vk::plugins::RendererTriangle::get_capabilities(context::RendererCaps& c) {
    c                            = context::RendererCaps{};
    c.presentation_mode          = context::PresentationMode::EngineBlit;
    c.preferred_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    c.color_attachments          = {context::AttachmentRequest{.name = "color", .format = VK_FORMAT_B8G8R8A8_UNORM, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, .samples = VK_SAMPLE_COUNT_1_BIT, .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .initial_layout = VK_IMAGE_LAYOUT_GENERAL}};
    c.presentation_attachment    = "color";
}
void vk::plugins::RendererTriangle::initialize(const context::EngineContext& e, const context::RendererCaps& c, const context::FrameContext&) {
    initialize_device_state(e, c);
    initialize_pipeline_resources();
}
void vk::plugins::RendererTriangle::destroy(const context::EngineContext& e, const context::RendererCaps&) {
    destroy_pipeline_resources();
    dev = VK_NULL_HANDLE;
}
void vk::plugins::RendererTriangle::record_graphics(VkCommandBuffer cmd, const context::EngineContext&, const context::FrameContext& f) {
    if (!is_ready_to_render(f)) return;

    const auto& target = f.color_attachments.front();

    prepare_for_rendering(cmd, target);
    execute_rendering(cmd, target, f.extent);
    finalize_rendering(cmd, target);
}
