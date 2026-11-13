# Eldoria - RPG 3D

## Checklist de Desenvolvimento

### Core & RPG
- [x] Câmera em primeira pessoa estável.
- [x] Movimentação WASD e Sprint (Shift).
- [x] Combate e IA de inimigos.
- [x] Sistema de inventário e crafting (Teclas 1-8).
- [x] Dimensões (Vale, Grutas, Fenda, Fornalha).
- [x] Bosses e Portais.
- [x] Sistema de armaduras dimensionais e defesa.
- [x] Modo Prime (Desbloqueio pós-final).

### Áudio & Narrativa
- [x] Efeitos sonoros (ataque, hit, coleta, portal, etc).
- [x] Música de encerramento emocional.
- [x] Falas em português brasileiro sincronizadas.
- [x] Sequência final com NPCs aplaudindo e poema.

### Visual & Estilo (NOVO)
- [x] Implementar geometria de cubos com cantos arredondados (Beveled Cubes).
- [x] Criar texturas fofas, coloridas e originais (Identidade Própria).
- [x] Adicionar variedade visual aos NPCs seguindo a estética cúbica.
- [x] Melhorar atmosfera e iluminação para o novo estilo visual.

### Android & Multiplataforma
- [x] Joystick virtual e controles touch.
- [x] Projeto Gradle/NDK configurado.
- [x] Compilação local de APK/AAB funcionando.
- [x] Corrigir crash na inicialização (Tela branca -> Preta -> Fecha).
- [x] Validar novo estilo visual no Android.
- [ ] Preparar e anexar as texturas originais do Eldoria (grama, pedra, madeira, NPCs).
- [ ] Criar um painel visual (sprite sheet/preview) dos personagens e objetos arredondados.
- [ ] Tentar gravar vídeo do gameplay zerando o jogo com assets pré-carregados na RAM.

### Builds & CI/CD
- [x] Build Windows (MinGW cross-compile).
- [x] Build Linux (Nativa).
- [x] Workflow macOS (GitHub Actions via CDN).
- [x] Hospedagem do source na CDN.
