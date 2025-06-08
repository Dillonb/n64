{
  description = "Dillon's N64 Emulator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    n64-tools.url = "github:Dillonb/n64-tools.nix";
  };

  outputs = { self, nixpkgs, flake-utils, n64-tools }: flake-utils.lib.eachDefaultSystem (system:
    let
      shortRev = with self; if sourceInfo?dirtyShortRev then sourceInfo.dirtyShortRev else sourceInfo.shortRev;
      rev = with self; if sourceInfo?dirtyRev then sourceInfo.dirtyRev else sourceInfo.rev;
      pkgs = import nixpkgs { inherit system; };

      llvmPackages = pkgs.llvmPackages_19;

      devShellTools = [
        llvmPackages.clang-tools
        pkgs.git
        pkgs.rust-analyzer
      ];

      tools = [
        pkgs.gcc
        pkgs.cmake
        pkgs.ninja
        pkgs.shaderc
        pkgs.pkg-config
        pkgs.vulkan-loader
        pkgs.mold-wrapped
        pkgs.cargo
        pkgs.rustc
        pkgs.rust-cbindgen

        n64-tools.packages.${system}.bass
        n64-tools.packages.${system}.chksum64
      ];

      libs = [
        pkgs.SDL2
        pkgs.capstone
        pkgs.dbus
        pkgs.bzip2
      ] ++ pkgs.lib.optionals (!pkgs.stdenv.isDarwin) [
        pkgs.qt6.qtbase # TODO: Qt should work on Darwin too
        pkgs.qt6.wrapQtAppsHook
      ];
      stdenv =
        if pkgs.stdenv.isLinux then pkgs.stdenv
        else if pkgs.stdenv.isDarwin then llvmPackages.stdenv
        else throw "Unsupported platform";

      cargoDeps = pkgs.rustPlatform.importCargoLock {
        lockFile = ./src/jit/Cargo.lock;
      };
    in
    {
      packages.default = stdenv.mkDerivation
        {
          cargoDeps = cargoDeps;
          cargoRoot = "src/jit";
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
          nativeBuildInputs = tools ++ [ pkgs.makeWrapper pkgs.rustPlatform.cargoSetupHook ];
          buildInputs = libs;
          cmakeFlags = [
            (pkgs.lib.cmakeFeature "N64_GIT_COMMIT_HASH" rev) # Flakes do not have access to the .git dir, so we'll set this manually
            (pkgs.lib.cmakeFeature "CMAKE_BUILD_TYPE" "Release")
          ];
          passthru.exePath = "/bin/n64";
          postInstall =
            if pkgs.stdenv.isLinux then ''
              wrapProgram $out/bin/n64 --set LD_LIBRARY_PATH ${pkgs.vulkan-loader}/lib
              wrapProgram $out/bin/n64-qt --set LD_LIBRARY_PATH ${pkgs.vulkan-loader}/lib
            '' else if pkgs.stdenv.isDarwin then ''
              wrapProgram $out/bin/n64 --set DYLD_FALLBACK_LIBRARY_PATH ${pkgs.darwin.moltenvk}/lib
            '' else throw "Unsupported platform";

        };

      apps.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/n64";
      };

      apps.qt = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/n64-qt";
      };

      devShells.default = pkgs.mkShell.override { stdenv = stdenv; }
        {
          buildInputs = devShellTools ++ tools ++ libs;
          shellHook = let clang_path = "${llvmPackages.libclang.lib}/lib/clang/${pkgs.lib.versions.major (pkgs.lib.getVersion llvmPackages.clang)}"; in ''
            export LIBCLANG_PATH="${llvmPackages.libclang.lib}/lib";
            export BINDGEN_EXTRA_CLANG_ARGS="-isystem ${clang_path}/include -isystem ${pkgs.glibc.dev}/include";
          '' + (if stdenv.isLinux then ''
            export LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib";
          '' else if stdenv.isDarwin then ''
            # clangd needs to come from clang-tools. Because Darwin uses clang stdenv, this is the only way to ensure we use the right clangd.
            export PATH="${llvmPackages.clang-tools}/bin:$PATH";
            export DYLD_FALLBACK_LIBRARY_PATH="${pkgs.darwin.moltenvk}/lib";
          '' else throw "Unsupported platform");
        };
    }
  );
}
