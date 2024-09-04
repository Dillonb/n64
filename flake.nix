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

      devShellTools = [
        pkgs.clang-tools
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
        pkgs.rust-analyzer
        pkgs.rust-cbindgen

        n64-tools.packages.${system}.bass
        n64-tools.packages.${system}.chksum64
      ];

      libs = [
        pkgs.SDL2
        pkgs.capstone
        pkgs.dbus
        pkgs.bzip2
      ];
    in
    {
      packages.default = pkgs.stdenv.mkDerivation
        {
          cargoDeps = pkgs.rustPlatform.importCargoLock {
            lockFile = ./src/jit/Cargo.lock;
          };
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
          buildInputs = devShellTools ++ tools ++ libs;
          LD_LIBRARY_PATH = "${pkgs.vulkan-loader}/lib";
        };
    }
  );
}
