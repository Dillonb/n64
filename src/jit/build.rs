use std::{env, path::PathBuf};

fn main() {
    let bindings = bindgen::Builder::default()
        .header("../cpu/r4300i.h")

        // Automatically generate the bindings if the C code changes
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))

        // Set some include paths
        .clang_arg("-I..")
        .clang_arg("-I../common")

        .allowlist_type("r4300i_t")

        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("c_bindings_generated.rs"))
        .expect("Couldn't write bindings!");
}
