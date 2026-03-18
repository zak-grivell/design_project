{
  description = "Mbed OS development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ] (system:
      let
        tool_chain = "gcc_arm";
        mbed_target = "LPC1768";

        pkgs = import nixpkgs {
          inherit system;
          overlays = [ self.overlays.default ];
        };

        mbed-build = pkgs.writeShellScriptBin "mbed-build" ''
          exec mbed-tools compile -t ${tool_chain} -m ${mbed_target} "$@"
        '';

        mbed-flash = pkgs.writeShellScriptBin "mbed-flash" ''
          exec mbed-tools compile -t ${tool_chain} -m ${mbed_target} -f "$@"
        '';

        mbed-compile-db = pkgs.writeShellScriptBin "mbed-compile-db" ''
          set -e
          mbed-tools compile -t ${tool_chain} -m ${mbed_target}
          BUILD_DIR="cmake_build/${mbed_target}/develop/GCC_ARM/"
          
          # CHANGE HERE: Scrub ALL host include environment variables
          unset NIX_CFLAGS_COMPILE NIX_CFLAGS_COMPILE_FOR_TARGET CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH
          
          cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$BUILD_DIR"
          ln -sf "$BUILD_DIR/compile_commands.json" compile_commands.json
          echo "compile_commands.json ready"
        '';
              # Unset NIX_CFLAGS_COMPILE before starting clangd so nix does not
        # inject host libcxx headers into every translation unit clangd sees.
        clangd-arm = pkgs.writeShellScriptBin "clangd" ''
          # CHANGE HERE: Scrub ALL host include environment variables
          unset NIX_CFLAGS_COMPILE NIX_CFLAGS_COMPILE_FOR_TARGET CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH
          exec ${pkgs.clang-tools}/bin/clangd \
            --query-driver="${pkgs.gcc-arm-embedded}/bin/arm-none-eabi-*" \
            "$@"
        '';
        in {
        devShells.default = pkgs.mkShellNoCC {
          packages = [
            pkgs.clang-tools

            (pkgs.python311.withPackages (ps: with ps; [
              future
              ninja
              prettytable
              intelhex
              pip
            ]))

            pkgs.mbed-tools
            pkgs.gcc-arm-embedded
            pkgs.cmake

            mbed-build
            mbed-flash
            mbed-compile-db
            clangd-arm
          ];

          shellHook = ''
cat > .clangd <<'EOF'
CompileFlags:
  Add:
    - "--target=arm-none-eabi"
    # CHANGE HERE: Set the bare-metal sysroot and disable all standard includes
    - "--sysroot=${pkgs.gcc-arm-embedded}/arm-none-eabi"
    - "-nostdinc"
    - "-nostdinc++"
    - "-xc++"
    - "-std=c++14"
    - "-isystem${pkgs.gcc-arm-embedded}/arm-none-eabi/include/c++/10.3.1"
    - "-isystem${pkgs.gcc-arm-embedded}/arm-none-eabi/include/c++/10.3.1/arm-none-eabi"
    - "-isystem${pkgs.gcc-arm-embedded}/arm-none-eabi/include"
    - "-isystem${pkgs.gcc-arm-embedded}/lib/gcc/arm-none-eabi/10.3.1/include"
    - "-isystem${pkgs.gcc-arm-embedded}/lib/gcc/arm-none-eabi/10.3.1/include-fixed"
  Remove:
    - "-mbranch-protection*"
  CompilationDatabase: "."
Diagnostics:
  SystemHeaders: false
EOF
            cat > .mbedignore <<'EOF'
.devenv
.direnv
.nix
result
.git
EOF
          '';
        };
      }
    ) // {
      overlays.default = final: prev: {

        # ── mbed-tools Python package ──────────────────────────────────────
        mbed-tools = prev.python311Packages.buildPythonPackage rec {
          pname = "mbed-tools";
          version = "7.58.0";

          src = prev.fetchPypi {
            inherit pname version;
            sha256 = "sha256-tFMpKkb1z86eYboQxPGbhVLBrUzx5xOVwXCieeXYHB0==";
          };

          pyproject = true;

          build-system = with prev.python311Packages; [
            setuptools
            setuptools-scm
          ];

          propagatedBuildInputs = with prev.python311Packages; [
            click
            pyserial
            intelhex
            prettytable
            packaging
            cmsis-pack-manager
            python-dotenv
            gitpython
            tqdm
            tabulate
            requests
            jinja2
            setuptools
            future
          ];

          doCheck = false;

          meta = with prev.lib; {
            description = "Arm Mbed command line tools";
            homepage = "https://github.com/ARMmbed/mbed-tools";
            license = licenses.asl20;
          };
        };

        # ── gcc-arm-embedded toolchain ─────────────────────────────────────
        gcc-arm-embedded = prev.stdenv.mkDerivation rec {
          pname = "gcc-arm-embedded";
          version = "10.3-2021.10";

          platform =
            {
              aarch64-darwin = "mac";
              aarch64-linux  = "aarch64";
              x86_64-linux   = "x86_64";
            }
            .${prev.stdenv.hostPlatform.system}
              or (throw "Unsupported system: ${prev.stdenv.hostPlatform.system}");

          src = prev.fetchurl {
            url = "https://developer.arm.com/-/media/files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-${platform}.tar.bz2";
            sha256 =
              {
                aarch64-darwin = "+2E9rLJRSfFA9z/p/2w4C7QzKOa/gTRzmG6RJ+K8KDs=";
                aarch64-linux  = "2d465847eb1d05f876270494f51034de9ace9abe87a4222d079f3360240184d3";
                x86_64-linux   = "8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd";
              }
              .${prev.stdenv.hostPlatform.system}
                or (throw "Unsupported system: ${prev.stdenv.hostPlatform.system}");
          };

          dontConfigure = true;
          dontBuild     = true;
          dontPatchELF  = true;
          dontStrip     = true;

          installPhase = ''
            mkdir -p $out
            cp -r * $out
            rm -f $out/bin/arm-none-eabi-gdb-py \
                  $out/bin/arm-none-eabi-gdb-add-index-py
          '';

          preFixup = prev.lib.optionalString prev.stdenv.hostPlatform.isLinux ''
            find $out -type f | while read f; do
              patchelf "$f" > /dev/null 2>&1 || continue
              patchelf --set-interpreter \
                $(cat ${prev.stdenv.cc}/nix-support/dynamic-linker) "$f" || true
              patchelf --set-rpath ${
                prev.lib.makeLibraryPath [
                  "$out"
                  prev.stdenv.cc.cc
                  prev.ncurses6
                  prev.libxcrypt-legacy
                  prev.xz
                  prev.zstd
                ]
              } "$f" || true
            done
          '';
        };
      };
    };
}
