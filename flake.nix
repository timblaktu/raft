{
  description = "C implementation of the Raft consensus protocol";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # Python environment for testing (virtraft2, log_fuzzer, cffi bindings)
        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          cffi
          hypothesis
          colorama
          coloredlogs
          docopt
          terminaltables
        ]);

        # CLinkedListQueue - test dependency fetched by `make download-contrib`
        clinkedlistqueue = pkgs.fetchFromGitHub {
          owner = "willemt";
          repo = "CLinkedListQueue";
          rev = "9f6ca72d0e168178b0faf1b35fb39941fcf7e2c6";
          hash = "sha256-snvw3nNF1+dEfqhtuU7eMSFS75PveCtfTiiOQzXK3/M=";
        };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            # Build toolchain
            gcc
            gnumake

            # Testing
            pythonEnv
            valgrind

            # Code coverage
            lcov

            # Static analysis (optional)
            # infer
          ];

          shellHook = ''
            # Symlink CLinkedListQueue if not already present
            if [ ! -d CLinkedListQueue ]; then
              ln -sf ${clinkedlistqueue} CLinkedListQueue
            fi
          '';
        };

        packages.default = pkgs.stdenv.mkDerivation {
          pname = "raft";
          version = "0-unstable";
          src = ./.;

          buildPhase = ''
            make static shared
          '';

          installPhase = ''
            mkdir -p $out/lib $out/include
            cp libraft.a $out/lib/
            cp libraft.so $out/lib/ 2>/dev/null || cp libraft.dylib $out/lib/ 2>/dev/null || true
            cp include/raft.h include/raft_types.h include/raft_log.h $out/include/
          '';
        };
      }
    );
}
