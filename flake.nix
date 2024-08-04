{
  description = "Dillon's N64 Emulator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system:
    let
      shortRev = with self; if sourceInfo?dirtyShortRev then sourceInfo.dirtyShortRev else sourceInfo.shortRev;
      rev = with self; if sourceInfo?dirtyRev then sourceInfo.dirtyRev else sourceInfo.rev;
      pkgs = import nixpkgs { inherit system; };
      llvmPackage = pkgs.llvmPackages_18;
      libcxx = llvmPackage.libraries.libcxx;
      clang = llvmPackage.libcxxClang;
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

      lib_cpath = pkgs.lib.makeSearchPathOutput "dev" "include" (libs);
      # Taken from CMake's ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
      extra_cpath = "${libcxx.dev}/include/c++/v1:${clang}/resource-root/include:${pkgs.glibc.dev}/include";
    in
    {
      packages.default = pkgs.stdenv.mkDerivation
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
          nativeBuildInputs = tools ++ [ pkgs.makeWrapper ];
          buildInputs = libs;
          cmakeFlags = [
            (pkgs.lib.cmakeFeature "N64_GIT_COMMIT_HASH" rev) # Flakes do not have access to the .git dir, so we'll set this manually
            (pkgs.lib.cmakeFeature "CMAKE_BUILD_TYPE" "Release")
            (pkgs.lib.cmakeFeature "CMAKE_C_COMPILER" "${clang}/bin/clang")
            (pkgs.lib.cmakeFeature "CMAKE_CXX_COMPILER" "${clang}/bin/clang++")
          ];
          passthru.exePath = "/bin/n64";
          postInstall = ''
            wrapProgram $out/bin/n64 --set LD_LIBRARY_PATH ${pkgs.vulkan-loader}/lib
          '';
        };

      apps.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/n64";
      };

      devShells.default = pkgs.mkShell
        {
          buildInputs = tools ++ libs ++ devShellTools;
          CPATH = "${lib_cpath}:${extra_cpath}";
          LD_LIBRARY_PATH = "${pkgs.vulkan-loader}/lib";
          shellHook = ''
            export CC="${clang}/bin/clang"
            export CXX="${clang}/bin/clang++"
          '';
        };
    }
  );
}
