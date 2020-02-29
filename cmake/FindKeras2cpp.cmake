set(KERAS2CPP_HEADERS
        ${KERAS2CPP_SOURCE_DIR}/model.h
        ${KERAS2CPP_SOURCE_DIR}/tensor.hh
        ${KERAS2CPP_SOURCE_DIR}/utils.h
        ${KERAS2CPP_SOURCE_DIR}/baseLayer.h
        ${KERAS2CPP_INCLUDE_DIR}/activation.h
        ${KERAS2CPP_INCLUDE_DIR}/conv1d.h
        ${KERAS2CPP_INCLUDE_DIR}/conv2d.h
        ${KERAS2CPP_INCLUDE_DIR}/dense.h
        ${KERAS2CPP_INCLUDE_DIR}/elu.h
        ${KERAS2CPP_INCLUDE_DIR}/embedding.h
        ${KERAS2CPP_INCLUDE_DIR}/flatten.h
        ${KERAS2CPP_INCLUDE_DIR}/lstm.h
        ${KERAS2CPP_INCLUDE_DIR}/locally1d.h
        ${KERAS2CPP_INCLUDE_DIR}/locally2d.h
        ${KERAS2CPP_INCLUDE_DIR}/maxPooling2d.h
        ${KERAS2CPP_INCLUDE_DIR}/batchNormalization.h
        )

find_path(LIBKERAS2CPP_INCLUDE_DIR ${KERAS2CPP_HEADERS}
        PATHS "${PROJECT_SOURCE_DIR}/external/keras2cpp")

find_library(LIBKERAS2CPP_LIBRARY
        NAMES keras2cpp
        PATHS "${PROJECT_SOURCE_DIR}/lib")