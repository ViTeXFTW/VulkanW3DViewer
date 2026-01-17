#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

class VulkanApp {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    vk::Instance instance;

    static constexpr uint32_t WIDTH = 800;
    static constexpr uint32_t HEIGHT = 600;

    void initWindow() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan App", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
    }

    void initVulkan() {
        createInstance();
    }

    void createInstance() {
        vk::ApplicationInfo appInfo{
            "Vulkan App",
            VK_MAKE_VERSION(1, 0, 0),
            "No Engine",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_3
        };

        // Get required GLFW extensions
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions) {
            throw std::runtime_error("Failed to get required GLFW extensions");
        }

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Validation layers
        std::vector<const char*> layers;
#ifdef ENABLE_VULKAN_VALIDATION
        layers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        vk::InstanceCreateInfo createInfo{
            {},
            &appInfo,
            layers,
            extensions
        };

        instance = vk::createInstance(createInfo);
        std::cout << "Vulkan instance created\n";
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        instance.destroy();

        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
};

int main() {
    VulkanApp app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
