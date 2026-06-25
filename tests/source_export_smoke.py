#!/usr/bin/env python3
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
source = (root / "src" / "vulkan_retro_icd.cpp").read_text(encoding="utf-8")
required_exports = [
    "vk_icdNegotiateLoaderICDInterfaceVersion",
    "vk_icdGetInstanceProcAddr",
    "vkCreateInstance",
    "vkEnumeratePhysicalDevices",
    "vkGetPhysicalDeviceProperties",
    "vkCreateDevice",
    "vkQueueSubmit",
    "vkCreateCommandPool",
    "vkAllocateCommandBuffers",
    "vkCreateBuffer",
    "vkAllocateMemory",
    "vkMapMemory",
]
missing = [name for name in required_exports if name not in source]
if missing:
    raise SystemExit(f"missing required ICD exports: {', '.join(missing)}")
print("Vulkan Retro ICD exports are present")
