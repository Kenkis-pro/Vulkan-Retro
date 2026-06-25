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

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = 0;

    VkCommandPool command_pool = nullptr;
    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS || command_pool == nullptr) {
        std::cerr << "command-pool creation failed\n";
        return 1;
    }

    VkCommandBufferAllocateInfo command_buffer_info{};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandPool = command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = nullptr;
    if (vkAllocateCommandBuffers(device, &command_buffer_info, &command_buffer) != VK_SUCCESS || command_buffer == nullptr) {
        std::cerr << "command-buffer allocation failed\n";
        return 1;
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS || vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        std::cerr << "command-buffer recording failed\n";
        return 1;
    }

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = nullptr;
    if (vkCreateFence(device, &fence_info, nullptr, &fence) != VK_SUCCESS || fence == nullptr) {
        std::cerr << "fence creation failed\n";
        return 1;
    }

    VkSubmitInfo submit_info{};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    if (vkQueueSubmit(queue, 1, &submit_info, fence) != VK_SUCCESS || vkWaitForFences(device, 1, &fence, 1, 0) != VK_SUCCESS) {
        std::cerr << "no-op queue submit failed\n";
        return 1;
    }

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = 4096;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer = nullptr;
    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS || buffer == nullptr) {
        std::cerr << "buffer creation failed\n";
        return 1;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    VkMemoryAllocateInfo allocation_info{};
    allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation_info.allocationSize = requirements.size;
    allocation_info.memoryTypeIndex = 0;
    VkDeviceMemory memory = nullptr;
    if (vkAllocateMemory(device, &allocation_info, nullptr, &memory) != VK_SUCCESS ||
        vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
        std::cerr << "buffer memory setup failed\n";
        return 1;
    }
    void* mapped = nullptr;
    if (vkMapMemory(device, memory, 0, requirements.size, 0, &mapped) != VK_SUCCESS || mapped == nullptr) {
        std::cerr << "memory mapping failed\n";
        return 1;
    }
    vkUnmapMemory(device, memory);

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    vkDestroyCommandPool(device, command_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::cout << "Vulkan Retro ICD bootstrap smoke passed\n";
    return 0;
}
