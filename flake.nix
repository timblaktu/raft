{
  description = "willemt/raft - C implementation of Raft consensus";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        clinkedlistqueue-src = pkgs.fetchFromGitHub {
          owner = "willemt";
          repo = "CLinkedListQueue";
          rev = "9f6ca72d0e168178b0faf1b35fb39941fcf7e2c6";
          sha256 = "sha256-snvw3nNF1+dEfqhtuU7eMSFS75PveCtfTiiOQzXK3/M=";
        };
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.gcc
            pkgs.gnumake
            pkgs.valgrind
          ];

          shellHook = ''
            if [ ! -d CLinkedListQueue ] || [ -L CLinkedListQueue ]; then
              ln -sfn ${clinkedlistqueue-src} CLinkedListQueue
            fi
          '';
        };
      }
    );
}
