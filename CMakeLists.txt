cmake_minimum_required(VERSION 3.16)

project(VulkanRetro VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
find_package(Python3 COMPONENTS Interpreter)

option(VULKAN_RETRO_BUILD_TESTS "Build Vulkan Retro smoke tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vulkan_retro_icd SHARED
    src/vulkan_retro_icd.cpp
)

target_include_directories(vulkan_retro_icd
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(vulkan_retro_icd
    PRIVATE
        VULKAN_RETRO_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
    target_compile_definitions(vulkan_retro_icd PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    set_target_properties(vulkan_retro_icd PROPERTIES OUTPUT_NAME "vulkan_retro")
else()
    target_compile_definitions(vulkan_retro_icd PRIVATE VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_WAYLAND_KHR)
    set_target_properties(vulkan_retro_icd PROPERTIES OUTPUT_NAME "vulkan_retro")
endif()

include(GNUInstallDirs)
install(TARGETS vulkan_retro_icd
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES icd/vulkan_retro_icd.linux.json DESTINATION ${CMAKE_INSTALL_DATADIR}/vulkan/icd.d)

if(VULKAN_RETRO_BUILD_TESTS)
    add_executable(icd_bootstrap_smoke tests/icd_bootstrap_smoke.cpp)
    target_include_directories(icd_bootstrap_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(icd_bootstrap_smoke PRIVATE vulkan_retro_icd)
    add_test(NAME icd_bootstrap_smoke COMMAND icd_bootstrap_smoke)

    if(Python3_Interpreter_FOUND)
        add_test(NAME source_export_smoke
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/source_export_smoke.py ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()
