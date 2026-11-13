Esta pasta contém o projeto Android nativo do Eldoria 0.1.5. O jogo continua sendo compilado em C99 com raylib 5.5; o módulo Android usa uma Activity nativa customizada, CMake e o Android NDK. Os assets do jogo ficam em `app/src/main/assets/assets` para serem empacotados no APK.

## Recursos Android da 0.1.5

A build inclui quatro pets alternáveis, atlases de materiais/personagens, perfil de qualidade alto, shader OpenGL ES opcional e multiplayer TCP direto. A permissão `android.permission.INTERNET` está declarada no manifesto. O host abre a porta escolhida no menu e o convidado informa o IPv4 e a porta.

Em uma mesma rede, use o IPv4 privado do host. Entre redes diferentes, permita a porta TCP no firewall e faça encaminhamento de porta no roteador quando necessário; o convidado deve usar o IPv4 público do host. A versão atual suporta um convidado por host, sem matchmaking ou relay.

## Controles touch

O canto inferior esquerdo contém o joystick virtual de movimento. O lado direito superior aceita arrasto para controlar a câmera em primeira pessoa. No lado direito ficam os botões de ataque, interação, construção, inventário, crafting, troca do pet e corrida. O menu inicial, a tela multiplayer e a tela de opções aceitam toque.

A entrada desktop permanece disponível: WASD movimenta, mouse gira a câmera, F ou clique ataca, E interage, B constrói, C abre o crafting, P troca o pet, TAB abre o inventário e Shift ativa corrida.

## Importar shaders pelo gerenciador de arquivos

Em **Opções**, use a ação de importar shader. A `EldoriaActivity` abre o seletor `ACTION_OPEN_DOCUMENT`; o arquivo escolhido é copiado para o armazenamento privado como `eldoria_imported_shader.fs`. O jogo combina esse fragment shader com o vertex shader OpenGL ES compatível e mantém o fallback quando o arquivo é inválido ou o usuário cancela.

## Requisitos de compilação

Instale Android Studio com Android SDK, Android NDK, CMake e Gradle. O projeto foi configurado para `compileSdk 35`, `minSdk 23`, `targetSdk 35`, NDK `26.3.11579264` e as ABIs `arm64-v8a` e `armeabi-v7a`. Para uma build reproduzível, configure `ANDROID_HOME` ou `ANDROID_SDK_ROOT` e use uma instalação que contenha as versões exigidas pelo Android Gradle Plugin.

## APK e AAB

A partir de `native`, execute:

```bash
./packaging/android/build-android.sh
```

O script gera:

- `dist/android/Eldoria-Android-0.1.5.apk` para instalação direta em um aparelho Android;
- `dist/android/Eldoria-Android-0.1.5.aab` para publicação na Google Play;
- `dist/android/SHA256SUMS` com os hashes dos dois artefatos.

O APK release usa assinatura debug apenas para gerar uma instalação local. Para a Google Play, substitua a configuração por um keystore de release próprio.
