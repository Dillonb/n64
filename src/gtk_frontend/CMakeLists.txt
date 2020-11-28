FILE(GLOB_RECURSE
        resources
        "resources/*.ui"
        "resources/n64.gresource.xml"
)

SET(resources_dir   ${CMAKE_CURRENT_SOURCE_DIR}/resources)
SET(gresource       ${resources_dir}/n64.gresource.xml)
SET(resource_header ${CMAKE_CURRENT_BINARY_DIR}/resource.h)
SET(resource_source ${CMAKE_CURRENT_BINARY_DIR}/resource.c)

ADD_CUSTOM_COMMAND(
        COMMAND ${GLIB_COMPILE_RESOURCES_PROGRAM} ${gresource} --target ${resource_header} --generate-header
        OUTPUT ${resource_header}
        DEPENDS ${resources}
        WORKING_DIRECTORY ${resources_dir}
)

ADD_CUSTOM_COMMAND(
        COMMAND ${GLIB_COMPILE_RESOURCES_PROGRAM} ${gresource} --target ${resource_source} --generate-source
        OUTPUT ${resource_source}
        DEPENDS ${resources}
        WORKING_DIRECTORY ${resources_dir}
)

add_executable(${N64_GTK_TARGET} gtk_frontend.c ${resource_header} ${resource_source})
target_link_libraries(${N64_GTK_TARGET} PUBLIC ${GTK_LIBRARIES} core common)
target_include_directories(${N64_GTK_TARGET} PUBLIC ${GTK_INCLUDE_DIRS})