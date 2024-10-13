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
        n64-tools.packages.${system}.bass
        n64-tools.packages.${system}.chksum64
      ];

      libs = [
        pkgs.SDL2
        pkgs.capstone
        pkgs.dbus
        pkgs.bzip2
      ] ++ pkgs.lib.optionals pkgs.stdenv.isDarwin [
        pkgs.darwin.apple_sdk.frameworks.Cocoa
      ] ++ pkgs.lib.optionals (!pkgs.stdenv.isDarwin) [
        pkgs.qt6.qtbase # TODO: Qt should work on Darwin too
        pkgs.qt6.wrapQtAppsHook
      ];
      stdenv = if pkgs.stdenv.isLinux then pkgs.stdenv
                else if pkgs.stdenv.isDarwin then pkgs.clang18Stdenv
                else throw "Unsupported platform";
    in
    {
      packages.default = stdenv.mkDerivation
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
          shellHook = if stdenv.isLinux then ''
            export LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib";
          '' else if stdenv.isDarwin then ''
            export DYLD_FALLBACK_LIBRARY_PATH="${pkgs.darwin.moltenvk}/lib";
          '' else throw "Unsupported platform";
        };
    }
  );
}
