{
  description = "Dillon's N64 Emulator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    systems.url = "github:nix-systems/default";
  };

  outputs = { self, nixpkgs, systems }:
    let
      eachSystem = nixpkgs.lib.genAttrs (import systems);
      shortRev = with self; if sourceInfo?dirtyShortRev then sourceInfo.dirtyShortRev else sourceInfo.shortRev;
      rev = with self; if sourceInfo?dirtyRev then sourceInfo.dirtyRev else sourceInfo.rev;
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
            pkgs.vulkan-loader
            pkgs.mold-wrapped
          ];

          devShellTools = [
            pkgs.gdb
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
                buildInputs = deps.tools ++ deps.libs ++ deps.devShellTools;
                CPATH = "${lib_cpath}:${extra_cpath}";
                LD_LIBRARY_PATH = "${pkgs.vulkan-loader}/lib";
              };
          });

      packages = eachSystem
        (system:
          let
            pkgs = nixpkgs.legacyPackages.${system};
            deps = (get_deps pkgs);
          in
          {
            default = pkgs.stdenv.mkDerivation
              {
                pname = "dgb-n64";
                version = "0.0.1-${shortRev}";
                src = pkgs.lib.fileset.toSource {
                  root = ./.;
                  fileset = pkgs.lib.fileset.unions [
                    ./src
                    ./CMakeLists.txt
                    ./cmake
                    ./tests
                  ];
                };
                nativeBuildInputs = deps.tools ++ [ pkgs.makeWrapper ];
                buildInputs = deps.libs;
                cmakeFlags = [
                  (pkgs.lib.cmakeFeature "N64_GIT_COMMIT_HASH" rev) # Flakes do not have access to the .git dir, so we'll set this manually
                  (pkgs.lib.cmakeFeature "CMAKE_BUILD_TYPE" "Release")
                ];
                passthru.exePath = "/bin/n64";
                postInstall = ''
                  wrapProgram $out/bin/n64 --set LD_LIBRARY_PATH ${pkgs.vulkan-loader}/lib
                '';
              };
          });

      apps = eachSystem
        (system: {
          default = {
            type = "app";
            program = "${self.packages.${system}.default}/bin/n64";
          };
        });
    };
}
