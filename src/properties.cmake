set(LOADER_EXTERNAL_SRC
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/load_gltf.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/pugixml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/hydraxml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/cmesh4.cpp)

set(CROSSRT_EXTERNAL_SRC
        ${NREND_EXTERNAL_DIR}/heavy_rt/core/BVH2CommonRT.cpp)

set(NRENDER_SRC    
        main.cpp
        build_bvh.cpp)

set(MODULE_NAME nrender)

set(MODULE_SOURCES
    ${NRENDER_SRC}
    ${LOADER_EXTERNAL_SRC}
    ${CROSSRT_EXTERNAL_SRC}
)

set(MODULE_LIBS

)
