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
void vk::plugins::UISystemDefault::record_imgui() {
    std::print("Recording ImGui frame\n");
}
