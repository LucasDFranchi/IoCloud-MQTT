# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

target_add_binary_data(${COMPONENT_TARGET} "static/index.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "static/styles.css" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "static/app.js" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "static/jquery-3.3.1.min.js" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "static/favicon.ico"TEXT)