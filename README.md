# Vulkan Retro

Vulkan Retro is a cross-platform Vulkan ICD project for retro and low-end GPUs,
with Intel HD Graphics 4000/Ivy Bridge era hardware (2012-2013) as the first
compatibility target.

The repository currently contains a buildable Vulkan 1.0 ICD foundation for
Windows and Linux:

- Vulkan loader entry points (`vk_icdNegotiateLoaderICDInterfaceVersion`,
  `vk_icdGetInstanceProcAddr`).
- Instance, physical-device, logical-device, queue, and capability-query stubs.
- Command pool, command buffer, fence, semaphore, buffer, memory, map, and bind stubs
  that let more loader/probe paths complete without immediate `VK_ERROR_*` exits.
- Conservative Intel HD Graphics 4000-like device properties and memory limits.
- Linux and Windows ICD manifest templates.
- A smoke test that verifies required ICD exports remain present.

> Status: bootstrap/prototype. The ICD now defaults to a safe no-op submission
> mode so simple probes do not immediately fail with `VK_ERROR_FEATURE_NOT_PRESENT`.
> It still does not execute GPU work or render real frames yet. Set
> `VULKAN_RETRO_STRICT_SUBMIT=1` to restore strict failure for submitted work.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Linux loader usage

After building, point the Vulkan loader at the manifest and library:

```bash
export VK_ICD_FILENAMES="$PWD/icd/vulkan_retro_icd.linux.json"
export LD_LIBRARY_PATH="$PWD/build:${LD_LIBRARY_PATH}"
vulkaninfo --summary
```

For installation:

```bash
cmake --install build --prefix /usr/local
```

## Windows loader usage

Build with a Windows CMake generator, then register or pass the manifest from
`icd/vulkan_retro_icd.windows.json` according to the Vulkan loader ICD manifest
rules. The shared library output name is `vulkan_retro.dll`.

## Roadmap

See [`docs/architecture.md`](docs/architecture.md) for the implementation plan.
The next milestone is image/swapchain support and a backend that can translate
recorded command streams into real rendering work for a minimal Vulkan 1.0
triangle path.
