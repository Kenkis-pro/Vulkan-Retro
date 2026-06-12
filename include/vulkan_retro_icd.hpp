#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define VKR_EXPORT extern "C" __declspec(dllexport)
#define VKR_DECLARE extern "C"
#define VKR_CALL __stdcall
#else
#define VKR_EXPORT extern "C" __attribute__((visibility("default")))
#define VKR_DECLARE extern "C"
#define VKR_CALL
#endif

using VkFlags = uint32_t;
using VkBool32 = uint32_t;
using VkDeviceSize = uint64_t;
using VkSampleMask = uint32_t;
using PFN_vkVoidFunction = void (*)();

using VkInstance = struct VkInstance_T*;
using VkPhysicalDevice = struct VkPhysicalDevice_T*;
using VkDevice = struct VkDevice_T*;
using VkQueue = struct VkQueue_T*;
using VkCommandBuffer = struct VkCommandBuffer_T*;
using VkAllocationCallbacks = struct VkAllocationCallbacks_T;

using VkResult = int32_t;

constexpr VkResult VK_SUCCESS = 0;
constexpr VkResult VK_NOT_READY = 1;
constexpr VkResult VK_INCOMPLETE = 5;
constexpr VkResult VK_ERROR_OUT_OF_HOST_MEMORY = -1;
constexpr VkResult VK_ERROR_INITIALIZATION_FAILED = -3;
constexpr VkResult VK_ERROR_LAYER_NOT_PRESENT = -6;
constexpr VkResult VK_ERROR_EXTENSION_NOT_PRESENT = -7;
constexpr VkResult VK_ERROR_INCOMPATIBLE_DRIVER = -9;
constexpr VkResult VK_ERROR_FEATURE_NOT_PRESENT = -8;

constexpr uint32_t VK_MAKE_VERSION_COMPAT(uint32_t major, uint32_t minor, uint32_t patch) {
    return (major << 22U) | (minor << 12U) | patch;
}

constexpr uint32_t VK_API_VERSION_1_0 = VK_MAKE_VERSION_COMPAT(1, 0, 0);
constexpr uint32_t VKR_VENDOR_ID_INTEL = 0x8086;
constexpr uint32_t VKR_DEVICE_ID_HD4000 = 0x0166;
constexpr uint32_t VKR_DRIVER_VERSION = VK_MAKE_VERSION_COMPAT(0, 1, 0);

constexpr uint32_t VK_MAX_PHYSICAL_DEVICE_NAME_SIZE = 256;
constexpr uint32_t VK_UUID_SIZE = 16;
constexpr uint32_t VK_MAX_MEMORY_TYPES = 32;
constexpr uint32_t VK_MAX_MEMORY_HEAPS = 16;
constexpr uint32_t VK_QUEUE_GRAPHICS_BIT = 0x00000001;
constexpr uint32_t VK_QUEUE_COMPUTE_BIT = 0x00000002;
constexpr uint32_t VK_QUEUE_TRANSFER_BIT = 0x00000004;
constexpr uint32_t VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001;
constexpr uint32_t VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002;
constexpr uint32_t VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004;
constexpr uint32_t VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = 0x00000001;

using VkStructureType = int32_t;
constexpr VkStructureType VK_STRUCTURE_TYPE_APPLICATION_INFO = 0;
constexpr VkStructureType VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1;
constexpr VkStructureType VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2;
constexpr VkStructureType VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3;

using VkPhysicalDeviceType = int32_t;
constexpr VkPhysicalDeviceType VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1;

struct VkApplicationInfo {
    VkStructureType sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkDeviceQueueCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    const float* pQueuePriorities;
};

struct VkDeviceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    uint32_t queueCreateInfoCount;
    const VkDeviceQueueCreateInfo* pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const void* pEnabledFeatures;
};

struct VkExtensionProperties {
    char extensionName[256];
    uint32_t specVersion;
};

struct VkLayerProperties {
    char layerName[256];
    uint32_t specVersion;
    uint32_t implementationVersion;
    char description[256];
};

struct VkPhysicalDeviceLimits {
    uint32_t maxImageDimension1D;
    uint32_t maxImageDimension2D;
    uint32_t maxImageDimension3D;
    uint32_t maxImageDimensionCube;
    uint32_t maxImageArrayLayers;
    uint32_t maxTexelBufferElements;
    uint32_t maxUniformBufferRange;
    uint32_t maxStorageBufferRange;
    uint32_t maxPushConstantsSize;
    uint32_t maxMemoryAllocationCount;
    uint32_t maxSamplerAllocationCount;
    VkDeviceSize bufferImageGranularity;
    VkDeviceSize sparseAddressSpaceSize;
    uint32_t maxBoundDescriptorSets;
    uint32_t maxPerStageDescriptorSamplers;
    uint32_t maxPerStageDescriptorUniformBuffers;
    uint32_t maxPerStageDescriptorStorageBuffers;
    uint32_t maxPerStageDescriptorSampledImages;
    uint32_t maxPerStageDescriptorStorageImages;
    uint32_t maxPerStageDescriptorInputAttachments;
    uint32_t maxPerStageResources;
};

struct VkPhysicalDeviceSparseProperties {
    VkBool32 residencyStandard2DBlockShape;
    VkBool32 residencyStandard2DMultisampleBlockShape;
    VkBool32 residencyStandard3DBlockShape;
    VkBool32 residencyAlignedMipSize;
    VkBool32 residencyNonResidentStrict;
};

struct VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    VkPhysicalDeviceType deviceType;
    char deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    uint8_t pipelineCacheUUID[VK_UUID_SIZE];
    VkPhysicalDeviceLimits limits;
    VkPhysicalDeviceSparseProperties sparseProperties;
};

struct VkPhysicalDeviceFeatures {
    VkBool32 robustBufferAccess;
    VkBool32 fullDrawIndexUint32;
    VkBool32 imageCubeArray;
    VkBool32 independentBlend;
    VkBool32 geometryShader;
    VkBool32 tessellationShader;
    VkBool32 sampleRateShading;
    VkBool32 dualSrcBlend;
    VkBool32 logicOp;
    VkBool32 multiDrawIndirect;
    VkBool32 drawIndirectFirstInstance;
    VkBool32 depthClamp;
    VkBool32 depthBiasClamp;
    VkBool32 fillModeNonSolid;
    VkBool32 depthBounds;
    VkBool32 wideLines;
    VkBool32 largePoints;
    VkBool32 alphaToOne;
    VkBool32 multiViewport;
    VkBool32 samplerAnisotropy;
};

struct VkQueueFamilyProperties {
    VkFlags queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    struct { uint32_t width; uint32_t height; uint32_t depth; } minImageTransferGranularity;
};

struct VkMemoryType {
    VkFlags propertyFlags;
    uint32_t heapIndex;
};

struct VkMemoryHeap {
    VkDeviceSize size;
    VkFlags flags;
};

struct VkPhysicalDeviceMemoryProperties {
    uint32_t memoryTypeCount;
    VkMemoryType memoryTypes[VK_MAX_MEMORY_TYPES];
    uint32_t memoryHeapCount;
    VkMemoryHeap memoryHeaps[VK_MAX_MEMORY_HEAPS];
};

VKR_DECLARE VkResult VKR_CALL vkCreateInstance(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
VKR_DECLARE void VKR_CALL vkDestroyInstance(VkInstance, const VkAllocationCallbacks*);
VKR_DECLARE VkResult VKR_CALL vkEnumerateInstanceExtensionProperties(const char*, uint32_t*, VkExtensionProperties*);
VKR_DECLARE VkResult VKR_CALL vkEnumerateInstanceLayerProperties(uint32_t*, VkLayerProperties*);
VKR_DECLARE VkResult VKR_CALL vkEnumeratePhysicalDevices(VkInstance, uint32_t*, VkPhysicalDevice*);
VKR_DECLARE void VKR_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice, VkPhysicalDeviceProperties*);
VKR_DECLARE void VKR_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice, VkPhysicalDeviceFeatures*);
VKR_DECLARE void VKR_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*);
VKR_DECLARE void VKR_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);
VKR_DECLARE VkResult VKR_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*);
VKR_DECLARE VkResult VKR_CALL vkCreateDevice(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*);
VKR_DECLARE void VKR_CALL vkDestroyDevice(VkDevice, const VkAllocationCallbacks*);
VKR_DECLARE void VKR_CALL vkGetDeviceQueue(VkDevice, uint32_t, uint32_t, VkQueue*);
VKR_DECLARE VkResult VKR_CALL vkDeviceWaitIdle(VkDevice);
VKR_DECLARE VkResult VKR_CALL vkQueueWaitIdle(VkQueue);
VKR_DECLARE VkResult VKR_CALL vkQueueSubmit(VkQueue, uint32_t, const void*, void*);
VKR_DECLARE PFN_vkVoidFunction VKR_CALL vkGetInstanceProcAddr(VkInstance, const char*);
VKR_DECLARE PFN_vkVoidFunction VKR_CALL vkGetDeviceProcAddr(VkDevice, const char*);
VKR_DECLARE PFN_vkVoidFunction VKR_CALL vk_icdGetInstanceProcAddr(VkInstance, const char*);
VKR_DECLARE VkResult VKR_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t*);
