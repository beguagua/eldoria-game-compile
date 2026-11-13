# Eldoria — Áudio, encerramento e Modo Prime

## Áudio e conclusão da jornada

A versão atual adiciona efeitos sonoros originais para ataque, impacto, coleta, crafting, portais, dano e vitória. Também inclui música e fala em português brasileiro para a conclusão da jornada. O Arconte agora inicia uma sequência de celebração, créditos com um poema original e uma mensagem de alegria para o jogador.

Ao terminar os créditos, o **Modo Prime** é desbloqueado e salvo na partida. Ele concede vida infinita, stamina infinita, dano máximo e grandes quantidades de todos os itens, permitindo explorar e construir sem limites. O carregamento dos arquivos de áudio é opcional: se o dispositivo de áudio ou algum arquivo estiver indisponível, o jogo continua executando sem som.

## Correção de controles e renderização

A entrada em primeira pessoa foi separada da simulação de movimento. O teclado agora usa **W para avançar na direção da mira** e **S para recuar**; o mouse gira a visão na direção correspondente, com captura de cursor controlada por estado e descarte do primeiro delta ao entrar no jogo. A tela também é limpa antes de cada quadro 3D para evitar a reutilização de profundidade e artefatos gráficos durante o deslocamento.

Esta atualização substitui o acompanhamento instável por uma **câmera em primeira pessoa**. A câmera fica na altura dos olhos, aponta para uma direção calculada a partir de yaw e pitch e acompanha diretamente a posição do jogador. O mouse controla a visão e WASD desloca o personagem em relação a essa orientação.

O jogo agora abre com uma tela de apresentação da **Raycast Corp** e, em seguida, exibe um menu inicial. Use as setas ou W/S para escolher uma opção e Enter ou Espaço para confirmar. O menu oferece uma nova jornada, continuação do save, resumo dos controles e saída.

No Windows, o executável é gerado sem janela de console e recebe o ícone `Eldoria.ico` como recurso embutido. Caso o Explorer continue exibindo um ícone antigo depois da atualização, feche todas as janelas do Explorer e use o novo executável do ZIP atualizado; o cache de ícones do Windows pode manter a miniatura anterior temporariamente.

## Versão 0.1.5 — Companheiros e Conexões

- Adicionados quatro pets alternáveis: Raposa de Aurora, Coruja de Musgo, Salamandra Brasa e Lebre de Cobalto. O pet acompanha o jogador e aparece no HUD.
- Adicionado menu **Opções**, com perfil de qualidade compatível/alto e shader realista opcional.
- Adicionado fluxo de importação de pacote de shader: desktop usa seletor `zenity` quando disponível; Android abre o gerenciador de arquivos do sistema, copia o fragment shader para o armazenamento privado e recarrega o shader sem alterar o APK.
- Incluído shader desktop com vertex shader próprio e variante OpenGL ES para Android. O renderer mantém fallback compatível quando o shader não existe ou falha.
- Adicionados atlases de pets, personagens e materiais; o atlas de faces substitui o atlas anterior e o atlas de materiais melhora os quatro reinos.
- Adicionado multiplayer direto TCP para uma sessão com host e um convidado. A tela aceita endereço IPv4 e porta; o host mantém o socket e os jogadores sincronizam posição, rotação e vida.
- Incluída permissão `INTERNET` e uma `EldoriaActivity` Android para o seletor de arquivos.
- Save atualizado para a versão 6, com migração automática do prefixo de save da 0.1.4 e persistência das opções/pet.
- Build Android atualizada para `versionCode 15` e `versionName 0.1.5`.
