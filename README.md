# eldoria-game-compile
A RAYCAST CORP NAO PERMITE UTILIZAR O CODIGO DO JOGO PARA CRIAR UM CLONE, ESTE REPOSITORIO FOI CRIADO PARA COMPILAÇAO NO MACOS.

## Jogo nativo Eldoria 0.1.5

O código nativo em C99/raylib foi adicionado em [`native/`](native/) e é mantido separado do protótipo C# em [`Eldoria/`](Eldoria/). O workflow [`native-desktop.yml`](.github/workflows/native-desktop.yml) compila Linux, Windows x86_64/i686 e macOS Intel/Apple Silicon como artefatos do GitHub Actions.

As limitações de compatibilidade de macOS 10.6.5, Windows 7 e Windows ARM estão documentadas em [`native/BUGS.md`](native/BUGS.md). Builds legados exigem validação em hardware ou runners próprios; o GitHub Actions não fornece esses ambientes históricos.
