set(LOADER_EXTERNAL_SRC
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/gltf_loader.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/loader_utils/gltf_loader_def.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/pugixml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/hydraxml.cpp
        ${NREND_EXTERNAL_DIR}/LiteScene/cmesh4.cpp
        )

set(MODULE_NAME nrender)

set(MODULE_SOURCES
        main.cpp
        nbvh.cpp
        nbvh_host.cpp
    ${LOADER_EXTERNAL_SRC}
)

set(MODULE_LIBS
        HeavyRT
        LiteMath
        LiteNN
)
