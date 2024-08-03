{
  description = "Dillon's N64 Emulator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    systems.url = "github:nix-systems/default";
  };

  outputs = { self, nixpkgs, systems }:
    let
      eachSystem = nixpkgs.lib.genAttrs (import systems);
      get_deps = (pkgs:
        let
          llvmPackage = pkgs.llvmPackages_18;
          libcxx = llvmPackage.libraries.libcxx;
          clang = llvmPackage.libcxxClang;
        in
        {
          clang = clang;
          libcxx = libcxx;
          tools = [
            pkgs.cmake
            clang
            pkgs.ninja
            pkgs.shaderc
            pkgs.pkg-config
            pkgs.gdb
            pkgs.vulkan-loader
            pkgs.mold-wrapped
          ];

          libs = [
            pkgs.SDL2
            pkgs.capstone
            pkgs.dbus
            pkgs.bzip2
            libcxx
          ];
        });
    in
    {
      devShells = eachSystem
        (system:
          let
            pkgs = nixpkgs.legacyPackages.${system};
            deps = (get_deps pkgs);
            lib_cpath = pkgs.lib.makeSearchPathOutput "dev" "include" (deps.libs);
            # Taken from CMake's ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
            extra_cpath = "${deps.libcxx.dev}/include/c++/v1:${deps.clang}/resource-root/include:${pkgs.glibc.dev}/include";
          in
          {
            default = pkgs.mkShell
              {
                buildInputs = deps.tools ++ deps.libs;
                CPATH = "${lib_cpath}:${extra_cpath}";
                LD_LIBRARY_PATH = "${pkgs.vulkan-loader}/lib";
              };
          });
    };
}
