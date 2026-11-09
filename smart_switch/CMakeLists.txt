cmake_minimum_required(VERSION 3.16.0)

set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/../external/esp-rainmaker)
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../components
    ${RMAKER_PATH}/components
    ${RMAKER_PATH}/examples/common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smart_switch)