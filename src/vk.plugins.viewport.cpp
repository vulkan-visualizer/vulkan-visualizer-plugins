module;
#include <SDL3/SDL.h>
#include <array>
#include <fstream>
#include <imgui.h>
#include <print>
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
    this->fmt                      = caps.color_attachments.empty() ? VK_FORMAT_B8G8R8A8_UNORM : caps.color_attachments.front().format;
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
    const auto& target = frm.color_attachments.front();

    transition_image_layout(cmd, target, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    begin_rendering(cmd, target, frm.extent);
    draw_triangle(cmd, frm.extent);
    end_rendering(cmd);
    transition_image_layout(cmd, target, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
}
void vk::plugins::ViewportRenderer::create_pipeline_layout(const context::EngineContext& eng) {
    constexpr VkPipelineLayoutCreateInfo lci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VK_CHECK(vkCreatePipelineLayout(eng.device, &lci, nullptr, &layout));
}
void vk::plugins::ViewportRenderer::create_graphics_pipeline(const context::EngineContext& eng) {

    VkShaderModule vs;
    {
        std::ifstream f("shader/triangle.vert.spv", std::ios::binary | std::ios::ate);
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
        std::ifstream f("shader/triangle.frag.spv", std::ios::binary | std::ios::ate);
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
void vk::plugins::ViewportRenderer::draw_triangle(VkCommandBuffer cmd, VkExtent2D extent) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipe);

    VkViewport viewport{
        .width    = static_cast<float>(extent.width),
        .height   = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
void vk::plugins::ViewportRenderer::begin_rendering(VkCommandBuffer& cmd, const context::AttachmentView& target, VkExtent2D extent) {
    constexpr VkClearValue clear_value{.color = {{0.f, 0.f, 0.f, 1.0f}}};
    VkRenderingAttachmentInfo color_attachment{
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = target.view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = clear_value,
    };
    VkRenderingInfo render_info{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = {{0, 0}, extent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment,
    };
    vkCmdBeginRendering(cmd, &render_info);
}
void vk::plugins::ViewportRenderer::end_rendering(VkCommandBuffer& cmd) {
    vkCmdEndRendering(cmd);
}
void vk::plugins::ViewportRenderer::transition_image_layout(VkCommandBuffer& cmd, const context::AttachmentView& target, VkImageLayout old_layout, VkImageLayout new_layout) {
    auto [src_stage, dst_stage, src_access, dst_access] = [&]() -> std::array<std::uint64_t, 4> {
        if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            return {
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_MEMORY_WRITE_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            };

        return {
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        };
    }();

    VkImageMemoryBarrier2 barrier{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = src_stage,
        .srcAccessMask    = src_access,
        .dstStageMask     = dst_stage,
        .dstAccessMask    = dst_access,
        .oldLayout        = old_layout,
        .newLayout        = new_layout,
        .image            = target.image,
        .subresourceRange = {target.aspect, 0, 1, 0, 1},
    };
    const VkDependencyInfo depInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier,
    };
    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void vk::plugins::ViewportUI::create_imgui(const context::EngineContext& eng, VkFormat format, uint32_t n_swapchain_image) {
    std::array<VkDescriptorPoolSize, 11> pool_sizes{{
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    }};
    VkDescriptorPoolCreateInfo pool_info{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000u * static_cast<uint32_t>(pool_sizes.size()),
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes    = pool_sizes.data(),
    };
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool_));
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle& style                 = ImGui::GetStyle();
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(device, pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
        return false;
    }
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion          = VK_API_VERSION_1_3;
    init_info.Instance            = instance;
    init_info.PhysicalDevice      = physicalDevice;
    init_info.Device              = device;
    init_info.QueueFamily         = graphicsQueueFamily;
    init_info.Queue               = graphicsQueue;
    init_info.DescriptorPool      = pool_;
    init_info.MinImageCount       = swapchainImageCount;
    init_info.ImageCount          = swapchainImageCount;
    init_info.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator           = nullptr;
    init_info.CheckVkResultFn     = [](VkResult res) { VK_CHECK(res); };
    init_info.UseDynamicRendering = VK_TRUE;
    VkPipelineRenderingCreateInfo rendering_info{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext                   = nullptr,
        .viewMask                = 0,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &swapchainFormat,
        .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };
    init_info.PipelineRenderingCreateInfo = rendering_info;
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(device, pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
        return false;
    }
    color_format_ = swapchainFormat;
    initialized_  = true;
    return true;
}
void vk::plugins::ViewportUI::destroy_imgui(const context::EngineContext& eng) {}
void vk::plugins::ViewportUI::process_event(const SDL_Event& event) {}
void vk::plugins::ViewportUI::record_imgui(VkCommandBuffer& cmd, const context::FrameContext& frm) {}

void vk::plugins::ViewpoertPlugin::initialize() {
    std::println("Viewport Plugin initialized.");
}
void vk::plugins::ViewpoertPlugin::update() {
    // std::println("Viewpoint Plugin updated.");
}
void vk::plugins::ViewpoertPlugin::shutdown() {
    std::println("Viewport Plugin shutdown.");
}
