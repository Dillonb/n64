corrosion_import_crate(MANIFEST_PATH ./Cargo.toml)
corrosion_experimental_cbindgen(TARGET jit HEADER_NAME "jit_rs.h" FLAGS --config ${CMAKE_CURRENT_LIST_DIR}/cbindgen.toml)

if(APPLE)
    # Corrosion hardcodes the CommandLineTools SDK lib dir, which pulls in a different libSystem than CMAKE_OSX_SYSROOT and breaks linking.
    set_property(TARGET jit-static PROPERTY INTERFACE_LINK_DIRECTORIES "")
endif()
