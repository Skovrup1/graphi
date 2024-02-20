#include "vk_engine.h"
#include <fmt/core.h>

int main() {
    VulkanEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();

    return 0;
}
