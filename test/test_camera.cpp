import vk.engine;
import vk.plugins.camera;
import vk.plugins.renderer;
import vk.plugins.ui;

int main() {
    vk::engine::VulkanEngine engine;
    vk::plugins::RendererTriangle renderer;
    vk::plugins::UISystemDefault ui_system;
    engine.init(renderer, ui_system);
    engine.run(renderer, ui_system);
    engine.cleanup();
    return 0;
}
