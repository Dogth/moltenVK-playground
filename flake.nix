{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    pre-commits.url = "github:cachix/git-hooks.nix";
    utils.url = "github:numtide/flake-utils";
  };
  outputs =
    {
      self,
      nixpkgs,
      utils,
      pre-commits,
    }@inputs:
    utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      with pkgs;
      {
        checks = inputs.pre-commits.lib.${system}.run {
          src = ./.;
          default_stages = [
            "manual"
            "pre-push"
          ];

          hooks = {
            clang-format = {
              enable = true;
              types_or = lib.mkForce [
                "c"
                "c++"
              ];
            };
            compat = {
              enable = true;
              name = "CXX98 Compatibility Tests";
              entry = ''cmake -B build -DBUILD_TESTING=ON -DFAIL_ON_REGRESSION_FAILURE=ON --fresh'';
              language = "system";
              stages = [ "pre-push" ];
              pass_filenames = false;
            };
            unit-test = {
              enable = true;
              name = "Unit Tests";
              files = "\\.(cpp|hpp)$";
              entry = ''ctest --test-dir build --output-on-failure '';
              language = "system";
              stages = [ "pre-push" ];
              pass_filenames = false;
            };

            commitizen.enable = true;
          };
        };

        devShells.default = mkShell {
          name = "MoltenVK playground development Shell";
          inherit (self.checks.${system}) shellHook;

          buildInputs = self.checks.${system}.enabledPackages;

          packages = with pkgs; [
            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
          ];
        };
      }
    );
}
