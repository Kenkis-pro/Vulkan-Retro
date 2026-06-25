#include "vulkan_retro_icd.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

struct VkInstance_T {
    uint32_t api_version = VK_API_VERSION_1_0;
};

struct VkPhysicalDevice_T {
    VkInstance instance = nullptr;
};

struct VkDevice_T {
    VkPhysicalDevice physical_device = nullptr;
};

struct VkQueue_T {
    VkDevice device = nullptr;
    uint32_t family = 0;
    uint32_t index = 0;
};

struct VkCommandPool_T { VkDevice device = nullptr; uint32_t queue_family = 0; };
struct VkCommandBuffer_T { VkCommandPool pool = nullptr; bool recording = false; bool executable = false; };
struct VkFence_T { bool signaled = false; };
struct VkSemaphore_T { bool signaled = false; };
struct VkBuffer_T { VkDeviceSize size = 0; VkDeviceMemory memory = nullptr; VkDeviceSize memory_offset = 0; };
struct VkImage_T { VkDevice device = nullptr; };
struct VkDeviceMemory_T { std::vector<uint8_t> bytes; bool mapped = false; };

namespace {

std::mutex g_mutex;
std::vector<std::unique_ptr<VkInstance_T>> g_instances;
std::vector<std::unique_ptr<VkPhysicalDevice_T>> g_physical_devices;
std::vector<std::unique_ptr<VkDevice_T>> g_devices;
std::vector<std::unique_ptr<VkQueue_T>> g_queues;
std::vector<std::unique_ptr<VkCommandPool_T>> g_command_pools;
std::vector<std::unique_ptr<VkCommandBuffer_T>> g_command_buffers;
std::vector<std::unique_ptr<VkFence_T>> g_fences;
std::vector<std::unique_ptr<VkSemaphore_T>> g_semaphores;
std::vector<std::unique_ptr<VkBuffer_T>> g_buffers;
std::vector<std::unique_ptr<VkDeviceMemory_T>> g_memories;

constexpr std::array<const char*, 0> kInstanceExtensions{};
constexpr std::array<const char*, 0> kDeviceExtensions{};

void copy_string(char* dst, std::size_t dst_size, std::string_view src) {
    if (dst_size == 0) {
        return;
    }
    const std::size_t n = std::min(dst_size - 1, src.size());
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

bool has_unsupported_names(uint32_t count, const char* const* names) {
    return count != 0 && names != nullptr;
}

bool strict_submit_mode() {
    const char* value = std::getenv("VULKAN_RETRO_STRICT_SUBMIT");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

template <typename T>
void erase_handle(std::vector<std::unique_ptr<T>>& handles, T* handle) {
    handles.erase(std::remove_if(handles.begin(), handles.end(), [handle](const auto& item) {
        return item.get() == handle;
    }), handles.end());
}

template <typename T, std::size_t N>
VkResult enumerate_names(const std::array<const char*, N>& names, uint32_t* count, T* properties) {
    if (count == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (properties == nullptr) {
        *count = static_cast<uint32_t>(names.size());
        return VK_SUCCESS;
    }

    const uint32_t requested = *count;
    const uint32_t written = std::min<uint32_t>(requested, static_cast<uint32_t>(names.size()));
    for (uint32_t i = 0; i < written; ++i) {
        std::memset(&properties[i], 0, sizeof(T));
        copy_string(properties[i].extensionName, sizeof(properties[i].extensionName), names[i]);
        properties[i].specVersion = 1;
    }
    *count = written;
    return requested < names.size() ? VK_INCOMPLETE : VK_SUCCESS;
}

VkPhysicalDevice ensure_physical_device(VkInstance instance) {
    auto device = std::make_unique<VkPhysicalDevice_T>();
    device->instance = instance;
    VkPhysicalDevice raw = device.get();
    g_physical_devices.emplace_back(std::move(device));
    return raw;
}

PFN_vkVoidFunction get_proc_addr(std::string_view name);

} // namespace

VKR_EXPORT VkResult VKR_CALL vkCreateInstance(
    const VkInstanceCreateInfo* create_info,
    const VkAllocationCallbacks*,
    VkInstance* instance) {
    if (instance == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (create_info != nullptr) {
        if (has_unsupported_names(create_info->enabledLayerCount, create_info->ppEnabledLayerNames)) {
            return VK_ERROR_LAYER_NOT_PRESENT;
        }
        if (has_unsupported_names(create_info->enabledExtensionCount, create_info->ppEnabledExtensionNames)) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    auto handle = std::make_unique<VkInstance_T>();
    if (create_info != nullptr && create_info->pApplicationInfo != nullptr && create_info->pApplicationInfo->apiVersion != 0) {
        const uint32_t requested_major = create_info->pApplicationInfo->apiVersion >> 22U;
        if (requested_major > 1) {
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }
        handle->api_version = std::min(create_info->pApplicationInfo->apiVersion, VK_API_VERSION_1_0);
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    *instance = handle.get();
    g_instances.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_instances.erase(std::remove_if(g_instances.begin(), g_instances.end(), [instance](const auto& item) {
        return item.get() == instance;
    }), g_instances.end());
}

VKR_EXPORT VkResult VKR_CALL vkEnumerateInstanceExtensionProperties(
    const char*, uint32_t* property_count, VkExtensionProperties* properties) {
    return enumerate_names(kInstanceExtensions, property_count, properties);
}

VKR_EXPORT VkResult VKR_CALL vkEnumerateInstanceLayerProperties(uint32_t* property_count, VkLayerProperties*) {
    if (property_count == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *property_count = 0;
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* physical_device_count,
    VkPhysicalDevice* physical_devices) {
    if (instance == nullptr || physical_device_count == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (physical_devices == nullptr) {
        *physical_device_count = 1;
        return VK_SUCCESS;
    }
    if (*physical_device_count == 0) {
        return VK_INCOMPLETE;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    physical_devices[0] = ensure_physical_device(instance);
    *physical_device_count = 1;
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice,
    VkPhysicalDeviceProperties* properties) {
    if (properties == nullptr) {
        return;
    }
    std::memset(properties, 0, sizeof(*properties));
    properties->apiVersion = VK_API_VERSION_1_0;
    properties->driverVersion = VKR_DRIVER_VERSION;
    properties->vendorID = VKR_VENDOR_ID_INTEL;
    properties->deviceID = VKR_DEVICE_ID_HD4000;
    properties->deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    copy_string(properties->deviceName, sizeof(properties->deviceName), "Vulkan Retro Intel HD Graphics 4000 Compatibility Device");
    properties->pipelineCacheUUID[0] = 'V';
    properties->pipelineCacheUUID[1] = 'K';
    properties->pipelineCacheUUID[2] = 'R';
    properties->limits.maxImageDimension1D = 8192;
    properties->limits.maxImageDimension2D = 8192;
    properties->limits.maxImageDimension3D = 2048;
    properties->limits.maxImageDimensionCube = 8192;
    properties->limits.maxImageArrayLayers = 2048;
    properties->limits.maxTexelBufferElements = 128 * 1024 * 1024;
    properties->limits.maxUniformBufferRange = 64 * 1024;
    properties->limits.maxStorageBufferRange = 128 * 1024 * 1024;
    properties->limits.maxPushConstantsSize = 128;
    properties->limits.maxMemoryAllocationCount = 4096;
    properties->limits.maxSamplerAllocationCount = 4000;
    properties->limits.bufferImageGranularity = 64 * 1024;
    properties->limits.maxBoundDescriptorSets = 4;
    properties->limits.maxPerStageDescriptorSamplers = 16;
    properties->limits.maxPerStageDescriptorUniformBuffers = 12;
    properties->limits.maxPerStageDescriptorStorageBuffers = 4;
    properties->limits.maxPerStageDescriptorSampledImages = 16;
    properties->limits.maxPerStageDescriptorStorageImages = 4;
    properties->limits.maxPerStageResources = 128;
}

VKR_EXPORT void VKR_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice, VkPhysicalDeviceFeatures* features) {
    if (features == nullptr) {
        return;
    }
    std::memset(features, 0, sizeof(*features));
    features->robustBufferAccess = 1;
    features->fullDrawIndexUint32 = 1;
    features->imageCubeArray = 1;
    features->independentBlend = 1;
    features->geometryShader = 1;
    features->samplerAnisotropy = 1;
}

VKR_EXPORT void VKR_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice,
    uint32_t* queue_family_property_count,
    VkQueueFamilyProperties* queue_family_properties) {
    if (queue_family_property_count == nullptr) {
        return;
    }
    if (queue_family_properties == nullptr) {
        *queue_family_property_count = 1;
        return;
    }
    if (*queue_family_property_count == 0) {
        return;
    }
    std::memset(&queue_family_properties[0], 0, sizeof(VkQueueFamilyProperties));
    queue_family_properties[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    queue_family_properties[0].queueCount = 1;
    queue_family_properties[0].timestampValidBits = 36;
    queue_family_properties[0].minImageTransferGranularity = {1, 1, 1};
    *queue_family_property_count = 1;
}

VKR_EXPORT void VKR_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice,
    VkPhysicalDeviceMemoryProperties* memory_properties) {
    if (memory_properties == nullptr) {
        return;
    }
    std::memset(memory_properties, 0, sizeof(*memory_properties));
    memory_properties->memoryHeapCount = 1;
    memory_properties->memoryHeaps[0].size = 512ULL * 1024ULL * 1024ULL;
    memory_properties->memoryHeaps[0].flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
    memory_properties->memoryTypeCount = 1;
    memory_properties->memoryTypes[0].heapIndex = 0;
    memory_properties->memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

VKR_EXPORT VkResult VKR_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice,
    const char*,
    uint32_t* property_count,
    VkExtensionProperties* properties) {
    return enumerate_names(kDeviceExtensions, property_count, properties);
}

VKR_EXPORT VkResult VKR_CALL vkCreateDevice(
    VkPhysicalDevice physical_device,
    const VkDeviceCreateInfo* create_info,
    const VkAllocationCallbacks*,
    VkDevice* device) {
    if (physical_device == nullptr || device == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (create_info != nullptr) {
        if (has_unsupported_names(create_info->enabledLayerCount, create_info->ppEnabledLayerNames)) {
            return VK_ERROR_LAYER_NOT_PRESENT;
        }
        if (has_unsupported_names(create_info->enabledExtensionCount, create_info->ppEnabledExtensionNames)) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        for (uint32_t i = 0; i < create_info->queueCreateInfoCount; ++i) {
            if (create_info->pQueueCreateInfos[i].queueFamilyIndex != 0 || create_info->pQueueCreateInfos[i].queueCount > 1) {
                return VK_ERROR_INITIALIZATION_FAILED;
            }
        }
    }

    auto handle = std::make_unique<VkDevice_T>();
    handle->physical_device = physical_device;
    std::lock_guard<std::mutex> lock(g_mutex);
    *device = handle.get();
    g_devices.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_devices.erase(std::remove_if(g_devices.begin(), g_devices.end(), [device](const auto& item) {
        return item.get() == device;
    }), g_devices.end());
}

VKR_EXPORT void VKR_CALL vkGetDeviceQueue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index, VkQueue* queue) {
    if (queue == nullptr || device == nullptr || queue_family_index != 0 || queue_index != 0) {
        return;
    }
    auto handle = std::make_unique<VkQueue_T>();
    handle->device = device;
    handle->family = queue_family_index;
    handle->index = queue_index;
    std::lock_guard<std::mutex> lock(g_mutex);
    *queue = handle.get();
    g_queues.emplace_back(std::move(handle));
}

VKR_EXPORT VkResult VKR_CALL vkDeviceWaitIdle(VkDevice device) {
    return device == nullptr ? VK_ERROR_INITIALIZATION_FAILED : VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkQueueWaitIdle(VkQueue queue) {
    return queue == nullptr ? VK_ERROR_INITIALIZATION_FAILED : VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkQueueSubmit(VkQueue queue, uint32_t, const VkSubmitInfo* submit_infos, VkFence fence) {
    if (queue == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (strict_submit_mode() && submit_infos != nullptr) {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    if (fence != nullptr) {
        fence->signaled = true;
    }
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkCreateCommandPool(
    VkDevice device, const VkCommandPoolCreateInfo* create_info, const VkAllocationCallbacks*, VkCommandPool* pool) {
    if (device == nullptr || create_info == nullptr || pool == nullptr || create_info->queueFamilyIndex != 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    auto handle = std::make_unique<VkCommandPool_T>();
    handle->device = device;
    handle->queue_family = create_info->queueFamilyIndex;
    std::lock_guard<std::mutex> lock(g_mutex);
    *pool = handle.get();
    g_command_pools.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkDestroyCommandPool(VkDevice, VkCommandPool pool, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    erase_handle(g_command_pools, pool);
}

VKR_EXPORT VkResult VKR_CALL vkAllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo* allocate_info, VkCommandBuffer* command_buffers) {
    if (device == nullptr || allocate_info == nullptr || allocate_info->commandPool == nullptr || command_buffers == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    for (uint32_t i = 0; i < allocate_info->commandBufferCount; ++i) {
        auto handle = std::make_unique<VkCommandBuffer_T>();
        handle->pool = allocate_info->commandPool;
        command_buffers[i] = handle.get();
        g_command_buffers.emplace_back(std::move(handle));
    }
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkFreeCommandBuffers(VkDevice, VkCommandPool, uint32_t count, const VkCommandBuffer* command_buffers) {
    if (command_buffers == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    for (uint32_t i = 0; i < count; ++i) {
        erase_handle(g_command_buffers, command_buffers[i]);
    }
}

VKR_EXPORT VkResult VKR_CALL vkBeginCommandBuffer(VkCommandBuffer command_buffer, const VkCommandBufferBeginInfo*) {
    if (command_buffer == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    command_buffer->recording = true;
    command_buffer->executable = false;
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkEndCommandBuffer(VkCommandBuffer command_buffer) {
    if (command_buffer == nullptr || !command_buffer->recording) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    command_buffer->recording = false;
    command_buffer->executable = true;
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkResetCommandBuffer(VkCommandBuffer command_buffer, VkFlags) {
    if (command_buffer == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    command_buffer->recording = false;
    command_buffer->executable = false;
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkCreateFence(VkDevice device, const VkFenceCreateInfo* create_info, const VkAllocationCallbacks*, VkFence* fence) {
    if (device == nullptr || fence == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    auto handle = std::make_unique<VkFence_T>();
    handle->signaled = create_info != nullptr && (create_info->flags & VK_FENCE_CREATE_SIGNALED_BIT) != 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    *fence = handle.get();
    g_fences.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkDestroyFence(VkDevice, VkFence fence, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    erase_handle(g_fences, fence);
}

VKR_EXPORT VkResult VKR_CALL vkWaitForFences(VkDevice device, uint32_t count, const VkFence* fences, VkBool32, uint64_t timeout) {
    if (device == nullptr || (count != 0 && fences == nullptr)) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (fences[i] != nullptr && !fences[i]->signaled && timeout == 0) {
            return VK_TIMEOUT;
        }
        if (fences[i] != nullptr) {
            fences[i]->signaled = true;
        }
    }
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkResetFences(VkDevice device, uint32_t count, const VkFence* fences) {
    if (device == nullptr || (count != 0 && fences == nullptr)) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (fences[i] != nullptr) {
            fences[i]->signaled = false;
        }
    }
    return VK_SUCCESS;
}

VKR_EXPORT VkResult VKR_CALL vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo*, const VkAllocationCallbacks*, VkSemaphore* semaphore) {
    if (device == nullptr || semaphore == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    auto handle = std::make_unique<VkSemaphore_T>();
    std::lock_guard<std::mutex> lock(g_mutex);
    *semaphore = handle.get();
    g_semaphores.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkDestroySemaphore(VkDevice, VkSemaphore semaphore, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    erase_handle(g_semaphores, semaphore);
}

VKR_EXPORT VkResult VKR_CALL vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* create_info, const VkAllocationCallbacks*, VkBuffer* buffer) {
    if (device == nullptr || create_info == nullptr || buffer == nullptr || create_info->size == 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    auto handle = std::make_unique<VkBuffer_T>();
    handle->size = create_info->size;
    std::lock_guard<std::mutex> lock(g_mutex);
    *buffer = handle.get();
    g_buffers.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkDestroyBuffer(VkDevice, VkBuffer buffer, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    erase_handle(g_buffers, buffer);
}

VKR_EXPORT void VKR_CALL vkGetBufferMemoryRequirements(VkDevice, VkBuffer buffer, VkMemoryRequirements* requirements) {
    if (buffer == nullptr || requirements == nullptr) {
        return;
    }
    requirements->size = (buffer->size + 255ULL) & ~255ULL;
    requirements->alignment = 256;
    requirements->memoryTypeBits = 1;
}

VKR_EXPORT VkResult VKR_CALL vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* allocate_info, const VkAllocationCallbacks*, VkDeviceMemory* memory) {
    if (device == nullptr || allocate_info == nullptr || memory == nullptr || allocate_info->memoryTypeIndex != 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    auto handle = std::make_unique<VkDeviceMemory_T>();
    try {
        handle->bytes.resize(static_cast<std::size_t>(allocate_info->allocationSize));
    } catch (...) {
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    *memory = handle.get();
    g_memories.emplace_back(std::move(handle));
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkFreeMemory(VkDevice, VkDeviceMemory memory, const VkAllocationCallbacks*) {
    std::lock_guard<std::mutex> lock(g_mutex);
    erase_handle(g_memories, memory);
}

VKR_EXPORT VkResult VKR_CALL vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkFlags, void** data) {
    if (device == nullptr || memory == nullptr || data == nullptr || offset > memory->bytes.size()) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    const VkDeviceSize map_size = size == ~0ULL ? memory->bytes.size() - offset : size;
    if (offset + map_size > memory->bytes.size()) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    memory->mapped = true;
    *data = memory->bytes.data() + offset;
    return VK_SUCCESS;
}

VKR_EXPORT void VKR_CALL vkUnmapMemory(VkDevice, VkDeviceMemory memory) {
    if (memory != nullptr) {
        memory->mapped = false;
    }
}

VKR_EXPORT VkResult VKR_CALL vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset) {
    if (device == nullptr || buffer == nullptr || memory == nullptr || offset >= memory->bytes.size()) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    buffer->memory = memory;
    buffer->memory_offset = offset;
    return VK_SUCCESS;
}

VKR_EXPORT PFN_vkVoidFunction VKR_CALL vkGetInstanceProcAddr(VkInstance, const char* name) {
    return name == nullptr ? nullptr : get_proc_addr(name);
}

VKR_EXPORT PFN_vkVoidFunction VKR_CALL vkGetDeviceProcAddr(VkDevice, const char* name) {
    return name == nullptr ? nullptr : get_proc_addr(name);
}

VKR_EXPORT PFN_vkVoidFunction VKR_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* name) {
    return vkGetInstanceProcAddr(instance, name);
}

VKR_EXPORT VkResult VKR_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* supported_version) {
    if (supported_version == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *supported_version = std::min(*supported_version, 5U);
    return VK_SUCCESS;
}

namespace {

PFN_vkVoidFunction get_proc_addr(std::string_view name) {
#define VKR_PROC(fn) if (name == #fn) return reinterpret_cast<PFN_vkVoidFunction>(&fn)
    VKR_PROC(vkCreateInstance);
    VKR_PROC(vkDestroyInstance);
    VKR_PROC(vkEnumerateInstanceExtensionProperties);
    VKR_PROC(vkEnumerateInstanceLayerProperties);
    VKR_PROC(vkEnumeratePhysicalDevices);
    VKR_PROC(vkGetPhysicalDeviceProperties);
    VKR_PROC(vkGetPhysicalDeviceFeatures);
    VKR_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
    VKR_PROC(vkGetPhysicalDeviceMemoryProperties);
    VKR_PROC(vkEnumerateDeviceExtensionProperties);
    VKR_PROC(vkCreateDevice);
    VKR_PROC(vkDestroyDevice);
    VKR_PROC(vkGetDeviceQueue);
    VKR_PROC(vkDeviceWaitIdle);
    VKR_PROC(vkQueueWaitIdle);
    VKR_PROC(vkQueueSubmit);
    VKR_PROC(vkCreateCommandPool);
    VKR_PROC(vkDestroyCommandPool);
    VKR_PROC(vkAllocateCommandBuffers);
    VKR_PROC(vkFreeCommandBuffers);
    VKR_PROC(vkBeginCommandBuffer);
    VKR_PROC(vkEndCommandBuffer);
    VKR_PROC(vkResetCommandBuffer);
    VKR_PROC(vkCreateFence);
    VKR_PROC(vkDestroyFence);
    VKR_PROC(vkWaitForFences);
    VKR_PROC(vkResetFences);
    VKR_PROC(vkCreateSemaphore);
    VKR_PROC(vkDestroySemaphore);
    VKR_PROC(vkCreateBuffer);
    VKR_PROC(vkDestroyBuffer);
    VKR_PROC(vkGetBufferMemoryRequirements);
    VKR_PROC(vkAllocateMemory);
    VKR_PROC(vkFreeMemory);
    VKR_PROC(vkMapMemory);
    VKR_PROC(vkUnmapMemory);
    VKR_PROC(vkBindBufferMemory);
    VKR_PROC(vkGetInstanceProcAddr);
    VKR_PROC(vkGetDeviceProcAddr);
    VKR_PROC(vk_icdGetInstanceProcAddr);
    VKR_PROC(vk_icdNegotiateLoaderICDInterfaceVersion);
#undef VKR_PROC
    return nullptr;
}

} // namespace
