module;
#include <SDL3/SDL.h>
#include <array>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <print>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>
module vk.plugins.ui;

// clang-format off
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _vk_check_res = (x); if (_vk_check_res != VK_SUCCESS) { throw std::runtime_error(std::string("Vulkan error ") + std::to_string(_vk_check_res) + " at " #x); } } while (false)
#endif
// clang-format on

void vk::plugins::UISystemDefault::set_main_window_title(const char* title) {
    (void) title;
}
void vk::plugins::UISystemDefault::create_imgui(const context::EngineContext& e, VkFormat format, uint32_t n_swapchain_image) {
    color_format_ = format;
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
    VK_CHECK(vkCreateDescriptorPool(e.device, &pool_info, nullptr, &pool_));
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
    if (!ImGui_ImplSDL3_InitForVulkan(e.window)) {
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(e.device, pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
        throw std::runtime_error("ImGui_ImplSDL3_InitForVulkan failed");
    }
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion          = VK_API_VERSION_1_3;
    init_info.Instance            = e.instance;
    init_info.PhysicalDevice      = e.physical;
    init_info.Device              = e.device;
    init_info.QueueFamily         = e.graphics_queue_family;
    init_info.Queue               = e.graphics_queue;
    init_info.DescriptorPool      = pool_;
    init_info.MinImageCount       = n_swapchain_image;
    init_info.ImageCount          = n_swapchain_image;
    init_info.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator           = nullptr;
    init_info.CheckVkResultFn     = [](VkResult res) { VK_CHECK(res); };
    init_info.UseDynamicRendering = VK_TRUE;
    VkPipelineRenderingCreateInfo rendering_info{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext                   = nullptr,
        .viewMask                = 0,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &color_format_,
        .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };
    init_info.PipelineRenderingCreateInfo = rendering_info;
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(e.device, pool_, nullptr);
        pool_ = VK_NULL_HANDLE;
        throw std::runtime_error("ImGui_ImplVulkan_Init failed");
    }
}
void vk::plugins::UISystemDefault::destroy_imgui(const context::EngineContext& e) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(e.device, pool_, nullptr);
}
void vk::plugins::UISystemDefault::process_event(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN) {
        SDL_Keymod mods = static_cast<SDL_Keymod>(e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT));
        SDL_Keycode key = e.key.key;
        std::print("Key down: key={} mod={}\n", key, static_cast<int>(mods));
    }
    ImGui_ImplSDL3_ProcessEvent(&e);
}

void transition_to_color_attachment(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout) {
    VkImageMemoryBarrier2 barrier{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext            = nullptr,
        .srcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
        .oldLayout        = oldLayout,
        .newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image            = image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u},
    };
    VkDependencyInfo dep{
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount  = 1u,
        .pImageMemoryBarriers     = &barrier,
    };
    vkCmdPipelineBarrier2(cmd, &dep);
}

void transition_to_present(VkCommandBuffer cmd, VkImage image) {
    VkImageMemoryBarrier2 barrier{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask    = 0u,
        .oldLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image            = image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u},
    };
    VkDependencyInfo dep{
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount  = 1u,
        .pImageMemoryBarriers     = &barrier,
    };
    vkCmdPipelineBarrier2(cmd, &dep);
}

void vk::plugins::UISystemDefault::record_imgui(VkCommandBuffer &cmd, const context::FrameContext& frm) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello");
    ImGui::Text("ImGui + Vulkan + SDL3");
    ImGui::End();

    transition_to_color_attachment(cmd, frm.swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkRenderingAttachmentInfo color_attachment{
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = frm.swapchain_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo rendering_info{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = {{0, 0}, frm.extent},
        .layerCount           = 1u,
        .colorAttachmentCount = 1u,
        .pColorAttachments    = &color_attachment,
    };
    vkCmdBeginRendering(cmd, &rendering_info);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);

    transition_to_present(cmd, frm.swapchain_image);
}
