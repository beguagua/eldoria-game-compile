Eldoria é um jogo 3D de mundo aberto escrito em **C99** com **raylib**. A direção é inspirada em princípios de sandbox-RPG, exploração, criação e aventura, mas o universo, a narrativa, as criaturas, os materiais, a interface e a tecnologia são próprios.

## Estado do desenvolvimento

A versão **0.1.5** contém a cena 3D de baixo custo, terreno procedural determinístico, câmera em primeira pessoa, coleta, inventário, construção, combate, salvamento binário, NPCs com rotina e áudio opcional. A expansão **Ecos das Três Fendas** mantém as Grutas de Basalto, a Fenda de Cobalto e a Fornalha Rubra, com minérios exclusivos, inimigos avançados, dungeons, portais e bosses.

A 0.1.5 adiciona quatro pets companheiros, um perfil de qualidade alta, shader realista opcional, atlases visuais novos e multiplayer direto TCP para uma sessão com host e um convidado. O multiplayer não usa servidor externo: o dispositivo do host abre a porta escolhida e o convidado informa o IPv4 e a porta.

## Requisitos desktop

O projeto requer um compilador C99, raylib 5.x ou compatível, CMake 3.10 ou superior e as dependências gráficas e de áudio da plataforma. O perfil compatível utiliza OpenGL e evita dependência obrigatória de APIs modernas específicas.

| Plataforma | Estratégia | Observação |
|---|---|---|
| macOS 10.13 | raylib estática + frameworks Cocoa, IOKit, OpenGL e CoreVideo | Compilar com deployment target compatível e validar em hardware Intel. |
| Windows 10+ | MinGW-w64/W64Devkit ou Visual Studio + raylib | A build liga Winsock para o multiplayer e inclui os arquivos de dados. |
| Linux | GCC/Clang + raylib + bibliotecas OpenGL/X11/ALSA | CMake facilita a adaptação entre distribuições. |
| Android 6.0+ | NativeActivity customizada + NDK + OpenGL ES | `minSdk 23`, `compileSdk 35`, `arm64-v8a` e `armeabi-v7a`. |

## Compilação desktop

Com CMake e a raylib vendorizada no source:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

O executável é escrito em `build/bin/eldoria`. O CMake copia a pasta `assets` para `build/assets`.

## Compilação Android

A partir de `native`, configure `ANDROID_HOME` ou `ANDROID_SDK_ROOT` com Android SDK, NDK 26.3.11579264, CMake 3.22.1 e Gradle compatível com Android Gradle Plugin 8.5.2. Depois execute:

```bash
./packaging/android/build-android.sh
```

A build da 0.1.5 gera `dist/android/Eldoria-Android-0.1.5.apk`, `dist/android/Eldoria-Android-0.1.5.aab` e `dist/android/SHA256SUMS`. O APK release usa a assinatura debug apenas para instalação local; para distribuição na Google Play, configure um keystore próprio.

## Controles

No desktop, `WASD` move o personagem em primeira pessoa e o mouse controla a câmera. `F` ou clique esquerdo executa um ataque, `E` coleta recursos ou interage com portais, lanternas e bancada, `B` constrói, `C` abre a bancada de crafting, `1` a `8` escolhem receitas, `TAB` abre o inventário, `P` troca o pet e `F5` salva a partida. `ESC` encerra a execução quando está no jogo.

No Android, o canto inferior esquerdo contém o joystick virtual. O lado direito superior aceita arrasto para controlar a câmera; os botões permitem atacar, interagir, construir, abrir inventário, abrir crafting, trocar o pet e correr. O menu inicial aceita toque.

## Pets

O pet ativo acompanha o jogador com movimento suave e fica visível na cena e no HUD. Pressione `P` ou toque no botão **PET** para alternar entre a **Raposa de Aurora**, a **Coruja de Musgo**, a **Salamandra Brasa** e a **Lebre de Cobalto**. As silhuetas são procedurais e os novos atlases em `assets/textures/` complementam os materiais e os personagens.

## Shader realista e importação

Em **Opções**, pressione `1` para ativar/desativar o shader realista, `2` para alternar o perfil de qualidade e `3` para abrir o gerenciador de arquivos. No desktop, o seletor usa `zenity` quando disponível. No Android, a Activity abre `ACTION_OPEN_DOCUMENT`, copia o arquivo escolhido para o armazenamento privado do jogo e recarrega o shader.

O pacote padrão fica como arquivo separado em `assets/shaders/`, com vertex e fragment shaders para OpenGL 3.3. O Android inclui variantes OpenGL ES. Um pacote ausente ou inválido não impede o jogo de iniciar: o renderer retorna automaticamente à iluminação compatível.

## Multiplayer direto

Abra **MULTIPLAYER ONLINE** no menu inicial. O host informa a porta e escolhe **H — HOSPEDAR**. O convidado informa o IPv4 do host e a mesma porta e escolhe **J — CONECTAR**. A sessão sincroniza posição, rotação e vida de um jogador remoto e foi mantida deliberadamente pequena para preservar o desempenho do jogo.

Em uma mesma rede, use o IPv4 privado do host. Entre redes diferentes, o host precisa permitir a porta TCP no firewall e, normalmente, configurar encaminhamento de porta no roteador; o convidado deve informar o IPv4 público do host. O sistema atual aceita um convidado por host e não oferece matchmaking, relay ou autenticação externa.

## Salvamento

O save binário passou para a versão 6. A primeira abertura de um save da 0.1.4 migra automaticamente o prefixo antigo e cria o pet padrão. As preferências de shader e qualidade são salvas em `eldoria_settings.cfg` junto do diretório de execução, ou no armazenamento privado do Android.

## Áudio e encerramento

Os arquivos opcionais ficam em `assets/audio/`. A build inclui efeitos de ataque, impacto, coleta, crafting, portal, dano e vitória, além da música e da fala da sequência final. Se o dispositivo de áudio não estiver disponível, o jogo continua funcionando sem som. Após derrotar o Arconte, Eldoria mostra uma celebração, reproduz a fala final, apresenta os créditos e desbloqueia o **Modo Prime**.

## Estrutura

`include/eldoria/types.h` contém os dados compartilhados; `include/eldoria/net.h` e `src/net.c` implementam o transporte TCP; `src/main.c` contém a implementação jogável; `android/app/src/main/java/com/raycastcorp/eldoria/EldoriaActivity.java` cuida do seletor de arquivos Android; `assets/shaders/` contém o shader desktop; `assets/textures/` contém os novos atlases; e os CMake/Gradle definem os builds multiplataforma.

## Licença e assets

Os assets autorais de Eldoria deverão ser adicionados com licença documentada em `LICENSES/`. Nenhum asset de Hytale é usado. A raylib possui sua própria licença e deve ser distribuída conforme os termos da versão utilizada.
