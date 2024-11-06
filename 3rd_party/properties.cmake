
add_subdirectory(LiteMath)

set(LITENN_ENABLE_VULKAN ${ENABLE_VULKAN})
add_subdirectory(neural_core)
add_subdirectory(heavy_rt)
