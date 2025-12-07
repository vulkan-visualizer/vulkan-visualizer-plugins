module;
#include <SDL3/SDL.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
module vk.plugins.viewport;

// clang-format off
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _vk_check_res = (x); if (_vk_check_res != VK_SUCCESS) { throw std::runtime_error(std::string("Vulkan error ") + std::to_string(_vk_check_res) + " at " #x); } } while (false)
#endif
// clang-format on

void vk::plugins::ViewportRenderer::query_required_device_caps(context::RendererCaps& caps) {
    caps.allow_async_compute = false;
}
void vk::plugins::ViewportRenderer::get_capabilities(context::RendererCaps& caps) {
    caps                            = context::RendererCaps{};
    caps.presentation_mode          = context::PresentationMode::EngineBlit;
    caps.preferred_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    caps.color_attachments          = {context::AttachmentRequest{.name = "color", .format = VK_FORMAT_B8G8R8A8_UNORM, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, .samples = VK_SAMPLE_COUNT_1_BIT, .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .initial_layout = VK_IMAGE_LAYOUT_GENERAL}};
    caps.presentation_attachment    = "color";
}
void vk::plugins::ViewportRenderer::initialize(const context::EngineContext& eng, const context::RendererCaps& caps) {
    this->fmt = caps.color_attachments.empty() ? VK_FORMAT_B8G8R8A8_UNORM : caps.color_attachments.front().format;
    this->m_color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    this->m_dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    this->m_dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;

    this->create_pipeline_layout(eng);
    this->create_graphics_pipeline(eng);
}
void vk::plugins::ViewportRenderer::destroy(const context::EngineContext& eng) {
    vkDestroyPipeline(eng.device, pipe, nullptr);
    pipe = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(eng.device, layout, nullptr);
    layout = VK_NULL_HANDLE;
}
void vk::plugins::ViewportRenderer::record_graphics(VkCommandBuffer& cmd, const context::EngineContext& eng, const context::FrameContext& frm) {

}
void vk::plugins::ViewportRenderer::create_pipeline_layout(const context::EngineContext& eng) {
    constexpr VkPipelineLayoutCreateInfo lci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VK_CHECK(vkCreatePipelineLayout(eng.device, &lci, nullptr, &layout));
}
void vk::plugins::ViewportRenderer::create_graphics_pipeline(const context::EngineContext& eng) {

    VkShaderModule vs;
    {
        std::ifstream f("", std::ios::binary | std::ios::ate);
        const size_t s = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::vector<char> d(s);
        f.read(d.data(), s);

        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = d.size();
        ci.pCode    = reinterpret_cast<const uint32_t*>(d.data());
        VK_CHECK(vkCreateShaderModule(eng.device, &ci, nullptr, &vs));
    }

    VkShaderModule fs;
    {
        std::ifstream f("", std::ios::binary | std::ios::ate);
        const size_t s = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::vector<char> d(s);
        f.read(d.data(), s);

        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = d.size();
        ci.pCode    = reinterpret_cast<const uint32_t*>(d.data());
        VK_CHECK(vkCreateShaderModule(eng.device, &ci, nullptr, &fs));
    }

    const VkPipelineShaderStageCreateInfo stages[2] = {
        {
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vs,
            .pName  = "main",
        },
        {
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fs,
            .pName  = "main",
        },
    };

    this->m_graphics_pipeline.rendering_info = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &this->fmt,
    };
    this->m_graphics_pipeline.vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    this->m_graphics_pipeline.input_assembly_state = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    this->m_graphics_pipeline.viewport_state = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };
    this->m_graphics_pipeline.rasterization_state = {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode    = VK_CULL_MODE_NONE,
        .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth   = 1.0f,
    };
    this->m_graphics_pipeline.multisample_state = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    this->m_graphics_pipeline.color_blend_state = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &m_color_blend_attachment,
    };
    this->m_graphics_pipeline.dynamic_state = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates    = m_dynamic_states,
    };

    VkGraphicsPipelineCreateInfo pci{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &this->m_graphics_pipeline.rendering_info,
        .stageCount          = 2,
        .pStages             = stages,
        .pVertexInputState   = &this->m_graphics_pipeline.vertex_input_state,
        .pInputAssemblyState = &this->m_graphics_pipeline.input_assembly_state,
        .pViewportState      = &this->m_graphics_pipeline.viewport_state,
        .pRasterizationState = &this->m_graphics_pipeline.rasterization_state,
        .pMultisampleState   = &this->m_graphics_pipeline.multisample_state,
        .pColorBlendState    = &this->m_graphics_pipeline.color_blend_state,
        .pDynamicState       = &this->m_graphics_pipeline.dynamic_state,
        .layout              = layout,
    };
    VK_CHECK(vkCreateGraphicsPipelines(eng.device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipe));

    vkDestroyShaderModule(eng.device, vs, nullptr);
    vkDestroyShaderModule(eng.device, fs, nullptr);
}

void vk::plugins::ViewportUI::create_imgui(const context::EngineContext& eng, VkFormat format, uint32_t n_swapchain_image) {}
void vk::plugins::ViewportUI::destroy_imgui(const context::EngineContext& eng) {}
void vk::plugins::ViewportUI::process_event(const SDL_Event& event) {}
void vk::plugins::ViewportUI::record_imgui(VkCommandBuffer& cmd, const context::FrameContext& frm) {}
