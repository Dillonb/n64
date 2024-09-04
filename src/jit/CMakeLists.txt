corrosion_import_crate(MANIFEST_PATH ./Cargo.toml)
corrosion_experimental_cbindgen(TARGET jit HEADER_NAME "jit_rs.h" FLAGS --config ${CMAKE_CURRENT_LIST_DIR}/cbindgen.toml)
