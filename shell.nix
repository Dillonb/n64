{ pkgs ? import <nixpkgs> {} }:

let llvmPackage = pkgs.llvmPackages_17;
    stdenv = llvmPackage.libcxxStdenv;
    libcxx = llvmPackage.libraries.libcxx;
    libcxxabi = llvmPackage.libraries.libcxxabi;

  buildInputs = [
    pkgs.hello
    pkgs.cmake
    llvmPackage.libcxxClang
    pkgs.SDL2
    pkgs.capstone
    pkgs.ninja
    pkgs.shaderc
    pkgs.pkg-config
    pkgs.dbus
    pkgs.bzip2
    pkgs.gdb
    pkgs.vulkan-loader
    stdenv
    libcxx
    libcxxabi
  ];
in
pkgs.mkShell {
  buildInputs = buildInputs;
  shellHook = ''
    export CPATH=$CPATH:${libcxx.dev}/include/c++/v1
    export CPLUS_INCLUDE_PATH=$CPATH
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${libcxx}/lib:${libcxxabi}/lib
  '';
  CPATH = pkgs.lib.makeSearchPathOutput "dev" "include" (buildInputs);
}
