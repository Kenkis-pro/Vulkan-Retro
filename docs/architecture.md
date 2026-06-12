# Vulkan Retro architecture

Vulkan Retro is structured as an installable Vulkan ICD rather than a layer. The
loader discovers the ICD through the JSON manifests in `icd/`, then calls the
standard `vk_icdNegotiateLoaderICDInterfaceVersion` and
`vk_icdGetInstanceProcAddr` entry points exported by the shared library.

## Target hardware

The initial compatibility target is Intel HD Graphics 4000/Ivy Bridge era
hardware from 2012-2013. That generation does not expose the fixed-function and
memory model guarantees expected by a complete hardware Vulkan 1.x driver on all
platforms, so Vulkan Retro starts with a strict Vulkan 1.0 compatibility device
that reports conservative limits and only advertises features that are realistic
for that class of integrated GPU.

## Backend plan

1. **ICD bootstrap**: loader negotiation, instance/device enumeration, property
   queries, queue-family discovery, and manifest packaging.
2. **Memory/resources**: host-visible memory allocation, buffer/image metadata,
   and format capability tables tuned for Ivy Bridge.
3. **Command recording**: command pools, command buffers, render-pass metadata,
   barriers, and validation of unsupported paths.
4. **Execution backend**: Windows and Linux backend shims that translate the safe
   subset to the platform graphics stack available on the target machine.
5. **Conformance growth**: add Vulkan 1.0 commands incrementally with CTS-style
   smoke tests and never advertise capabilities before implementation.

## Current implementation contract

The current ICD is intentionally conservative. It is loader-discoverable, reports
one integrated GPU compatibility device, supports basic instance/device/queue
lifetime queries, and returns `VK_ERROR_FEATURE_NOT_PRESENT` for submitted work
until command execution exists. This prevents applications from mistaking the
prototype for a complete renderer while giving the project a buildable,
cross-platform driver foundation.
