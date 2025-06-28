use std::{env, path::PathBuf};

fn main() {
    let bindings = bindgen::Builder::default()
        .header("../cpu/r4300i.h")
        .header("../cpu/dynarec/dynarec.h")
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
        .blocklist_function("strtold");

    #[cfg(target_os = "windows")]
    let bindings = bindings.clang_arg("-DN64_WIN");

    let bindings = bindings.generate().expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("c_bindings_generated.rs"))
        .expect("Couldn't write bindings!");
}
