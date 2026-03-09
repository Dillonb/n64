use std::{env, path::PathBuf};

fn main() {
    let bindings = bindgen::Builder::default()
        .header("../cpu/r4300i.h")
        .header("../cpu/rsp.h")
        .header("../cpu/dynarec/dynarec.h")
        .header("../cpu/dynarec/rsp_dynarec.h")
        .header("../cpu/tlb_instructions.h")
        .header("../cpu/dynarec/dynarec_memory_management.h")
        .header("../system/scheduler_utils.h")
        .header("../mem/n64bus.h")
        // Automatically generate the bindings if the C code changes
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Set some include paths
        .clang_arg("-I..")
        .clang_arg("-I../common")
        .clang_arg("-I../contrib/include")
        // These functions all use u128, which is not FFI-safe
        .blocklist_function("qecvt")
        .blocklist_function("qecvt_r")
        .blocklist_function("qfcvt")
        .blocklist_function("qfcvt_r")
        .blocklist_function("qgcvt")
        .blocklist_function("strtold")
        // NEON multi-vector struct types - these fail alignment checks because the type
        // aliases align to the _element_ in Rust, when they align to the whole thing in C.
        // I don't use these types in Rust and shouldn't ever need to, so just blocklist all of them.
        .blocklist_type(".*x\\d+x[234]_t");

    #[cfg(target_os = "windows")]
    let bindings = bindings.clang_arg("-DN64_WIN");

    #[cfg(target_arch = "aarch64")]
    let bindings = bindings.clang_arg("-DN64_USE_NEON");

    let bindings = bindings.generate().expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("c_bindings_generated.rs"))
        .expect("Couldn't write bindings!");
}
