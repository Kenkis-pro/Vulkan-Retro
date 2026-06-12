#include "vulkan_retro_icd.hpp"

#include <algorithm>
#include <array>
#include <cstring>
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

namespace {

std::mutex g_mutex;
std::vector<std::unique_ptr<VkInstance_T>> g_instances;
std::vector<std::unique_ptr<VkPhysicalDevice_T>> g_physical_devices;
std::vector<std::unique_ptr<VkDevice_T>> g_devices;
std::vector<std::unique_ptr<VkQueue_T>> g_queues;

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

VKR_EXPORT VkResult VKR_CALL vkQueueSubmit(VkQueue queue, uint32_t submit_count, const void*, void*) {
    if (queue == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return submit_count == 0 ? VK_SUCCESS : VK_ERROR_FEATURE_NOT_PRESENT;
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
    VKR_PROC(vkGetInstanceProcAddr);
    VKR_PROC(vkGetDeviceProcAddr);
    VKR_PROC(vk_icdGetInstanceProcAddr);
    VKR_PROC(vk_icdNegotiateLoaderICDInterfaceVersion);
#undef VKR_PROC
    return nullptr;
}

} // namespace
