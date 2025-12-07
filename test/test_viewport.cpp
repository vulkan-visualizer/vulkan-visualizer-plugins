import vk.engine;
import vk.plugins.viewport;

int main() {
    vk::engine::VulkanEngine engine;
    vk::plugins::ViewportRenderer renderer;
    vk::plugins::ViewportUI ui_system;

    engine.init(renderer, ui_system);
    engine.run(renderer, ui_system);
    engine.cleanup();
    return 0;
}
