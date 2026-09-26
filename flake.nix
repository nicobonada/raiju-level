{
  description = "Print SDL's battery reading for a gamepad";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          raiju-level = pkgs.stdenv.mkDerivation {
            pname = "raiju-level";
            version = "0.1.0";
            src = ./src;
            nativeBuildInputs = [ pkgs.pkg-config ];
            buildInputs = [ pkgs.sdl3 ];
            buildPhase = ''
              runHook preBuild
              cc -O2 -o raiju-level raiju-level.c $(pkg-config --cflags --libs sdl3)
              runHook postBuild
            '';
            installPhase = ''
              runHook preInstall
              install -Dm755 raiju-level $out/bin/raiju-level
              runHook postInstall
            '';
            meta = {
              description = "Print SDL's battery reading for a connected gamepad";
              mainProgram = "raiju-level";
              license = pkgs.lib.licenses.mit;
            };
          };
        in
        {
          default = raiju-level;
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              gcc
              pkg-config
              sdl3
            ];
          };
        }
      );
    };
}
