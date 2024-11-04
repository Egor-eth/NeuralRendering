set(LOADER_EXTERNAL_SRC
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/gltf_loader.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/gltf_loader_def.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/pugixml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/hydraxml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/cmesh4.cpp
        )

include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/external)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/external/embree)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/external/nanort)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/external/tiny_gltf)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/core)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/core/common)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/core/builders)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/examples)
include_directories(${NREND_EXTERNAL_DIR}/heavy_rt/examples/loader)

set(HEAVY_RT_CORE_SRC
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/common/Image2d.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/common/Timer.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/common/qmc_sobol_niederreiter.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/FactoryRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/EmbreeRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BruteForceRT.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonRT.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonLoftRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonLoftRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonLoft16RT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonLoft16RT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonLRFT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonLRFT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonRTStacklessLBVH_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonRTStacklessLBVH.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH4CommonRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH4CommonRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH4HalfRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH4HalfRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2Stackless.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2Stackless_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2FatRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2FatRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2Fat16RT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2Fat16RT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2FatCompactRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2FatCompactRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/NanoRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/NonunifNodeStorage.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/ShtRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/ShtRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/ShtRT64.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/ShtRT64_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/Sht4NodesRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/Sht4NodesRT_host.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/Sht4NodesHalfRT.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/Sht4NodesHalfRT_host.cpp
        )

set(BVH_BUILDERS_SRC
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/lbvh.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/lbvh_host.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_fat.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_core.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_embree2.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_split.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/EXT_TriBoxOverlap.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_esc.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_esc2.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/cbvh_hlbvh.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/FatBVHReorder.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/FatBVHClusterize.cpp
        #${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/FatBVHPrint.cpp
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/builders/refitter.cpp
)       

set(NRENDER_SRC    
        main.cpp
        nbvh.cpp
        nbvh_host.cpp
        )

set(MODULE_NAME nrender)

set(MODULE_SOURCES
    ${NRENDER_SRC}
    ${LOADER_EXTERNAL_SRC}
    ${HEAVY_RT_CORE_SRC}
    ${BVH_BUILDERS_SRC}
)

set(MODULE_LIBS

)
