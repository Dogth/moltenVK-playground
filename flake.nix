{
  description = "Vulkan (MoltenVK) development shell for macOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    pre-commit-hooks.url = "github:cachix/pre-commit-hooks.nix";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      pre-commit-hooks,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };

        vulkanPackages =
          with pkgs;
          [
            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
            vulkan-tools
            moltenvk
            shaderc
            glslang
            spirv-tools
            spirv-cross
          ]
          ++ pkgs.lib.optionals (!pkgs.stdenv.isDarwin) [
            vulkan-extension-layer
          ];
        buildTools = with pkgs; [
          cmake
          ninja
          pkg-config
          glfw3
          glm
        ];

        testTools = with pkgs; [
          catch2_3
        ];

        devTools = with pkgs; [
          commitizen
          git
        ];
      in
      {
        checks = {
          pre-commit-check = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              commitizen.enable = true;
              clang-format.enable = true;
            };
          };
        };

        devShells.default = pkgs.mkShell {
          buildInputs = vulkanPackages ++ buildTools ++ testTools ++ devTools;

          shellHook = ''
            ${self.checks.${system}.pre-commit-check.shellHook}

            export VULKAN_SDK="${pkgs.vulkan-headers}"
            export VK_ICD_FILENAMES="${pkgs.moltenvk}/share/vulkan/icd.d/MoltenVK_icd.json"
            export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
            export DYLD_LIBRARY_PATH="${pkgs.moltenvk}/lib''${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"

            echo "Vulkan (MoltenVK) shell ready :3"
          '';
        };
      }
    );
}
