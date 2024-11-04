set(LOADER_EXTERNAL_SRC
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/gltf_loader.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/gltf_loader_def.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/pugixml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/hydraxml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/cmesh4.cpp)

set(CROSSRT_EXTERNAL_SRC
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonRT.cpp)

set(NRENDER_SRC    
        main.cpp
        nbvh.cpp
        nbvh_host.cpp)

set(MODULE_NAME nrender)

set(MODULE_SOURCES
    ${NRENDER_SRC}
    ${LOADER_EXTERNAL_SRC}
    ${CROSSRT_EXTERNAL_SRC}
)

set(MODULE_LIBS

)
