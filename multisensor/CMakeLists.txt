cmake_minimum_required(VERSION 3.16.0)

set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/../external/esp-rainmaker)

list(APPEND EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../components
    ${RMAKER_PATH}/components
    ${RMAKER_PATH}/examples/common/rmaker_app_network
    ${RMAKER_PATH}/examples/common/rmaker_app_insights
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(multisensor)
