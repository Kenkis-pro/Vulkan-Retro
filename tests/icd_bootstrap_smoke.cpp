#include "vulkan_retro_icd.hpp"

#include <cstring>
#include <iostream>

int main() {
    uint32_t loader_version = 5;
    if (vk_icdNegotiateLoaderICDInterfaceVersion(&loader_version) != VK_SUCCESS || loader_version != 5) {
        std::cerr << "loader negotiation failed\n";
        return 1;
    }

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    VkInstance instance = nullptr;
    if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS || instance == nullptr) {
        std::cerr << "instance creation failed\n";
        return 1;
    }

    uint32_t physical_count = 0;
    if (vkEnumeratePhysicalDevices(instance, &physical_count, nullptr) != VK_SUCCESS || physical_count != 1) {
        std::cerr << "physical-device count failed\n";
        return 1;
    }

    VkPhysicalDevice physical_device = nullptr;
    if (vkEnumeratePhysicalDevices(instance, &physical_count, &physical_device) != VK_SUCCESS || physical_device == nullptr) {
        std::cerr << "physical-device enumeration failed\n";
        return 1;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    if (properties.vendorID != VKR_VENDOR_ID_INTEL || properties.deviceID != VKR_DEVICE_ID_HD4000) {
        std::cerr << "unexpected compatibility device IDs\n";
        return 1;
    }
    if (std::strstr(properties.deviceName, "HD Graphics 4000") == nullptr) {
        std::cerr << "unexpected compatibility device name\n";
        return 1;
    }

    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    const float priority = 1.0f;
    queue_info.pQueuePriorities = &priority;

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;

    VkDevice device = nullptr;
    if (vkCreateDevice(physical_device, &device_info, nullptr, &device) != VK_SUCCESS || device == nullptr) {
        std::cerr << "logical-device creation failed\n";
        return 1;
    }

    VkQueue queue = nullptr;
    vkGetDeviceQueue(device, 0, 0, &queue);
    if (queue == nullptr || vkQueueWaitIdle(queue) != VK_SUCCESS || vkDeviceWaitIdle(device) != VK_SUCCESS) {
        std::cerr << "queue/device idle failed\n";
        return 1;
    }

    if (vkQueueSubmit(queue, 1, nullptr, nullptr) != VK_ERROR_FEATURE_NOT_PRESENT) {
        std::cerr << "implemented queue submit must not be advertised yet\n";
        return 1;
    }

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::cout << "Vulkan Retro ICD bootstrap smoke passed\n";
    return 0;
}
