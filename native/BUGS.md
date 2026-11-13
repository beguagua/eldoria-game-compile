# Auditoria de bugs e portabilidade — Eldoria nativo 0.1.5

## Corrigidos nesta integração

- O `Makefile` compilava apenas `src/main.c`; `src/net.c` ficava fora do executável. Isso quebrava o link ou removia o multiplayer dependendo do método de build.
- O script de macOS também compilava apenas `src/main.c`. A camada TCP agora é incluída.
- A verificação de socket inválido usava `socketValue < 0`, incorreta para `SOCKET` unsigned no Windows. O código agora compara com `INVALID_SOCKET` nessa plataforma.
- `MSG_NOSIGNAL` não é uma constante disponível em todos os sistemas Unix, especialmente no macOS. O envio agora usa a flag somente quando ela existe.
- `popen`/`pclose` têm os nomes `_popen`/`_pclose` no runtime Microsoft; foram adicionados aliases para permitir a compilação Windows.

## Pendências importantes

1. **Save binário não é portável entre arquiteturas.** `SaveGame` grava `EldoriaGame` inteiro com `fwrite`, incluindo padding, `bool`, enums e estruturas dependentes do compilador. Um save criado em 32 bits, 64 bits, Intel ou ARM não deve ser considerado intercambiável. A solução definitiva é um formato serializado com campos de largura fixa e endianness definido.
2. **O multiplayer TCP ainda não trata mensagens parciais.** `recv` pode retornar metade de um pacote ou vários pacotes juntos; o código atual interpreta cada leitura como uma mensagem completa. É necessário um buffer por conexão e processamento linha a linha.
3. **A conexão TCP não possui autenticação, criptografia ou validação de limites.** Não exponha a porta diretamente à Internet sem uma camada de relay/segurança.
4. **O seletor de shaders usa `zenity` no desktop.** Sem `zenity`, a importação externa não funciona, embora o jogo continue usando o shader padrão/fallback.
5. **A build macOS 10.6.5 não pode ser validada em GitHub Actions.** Ela exige SDK/toolchain antigos e runner Mac próprio. Apple Silicon não executa macOS 10.6.5.
6. **Windows 7, Windows 32 bits e Windows ARM precisam de validação em máquinas reais ou runners self-hosted.** O workflow fornece cross-builds quando o toolchain está disponível, mas não substitui testes de execução.
7. **Os avisos `-Wconversion` restantes são de qualidade de código.** Não impediram o build Linux, mas devem ser tratados antes de habilitar `-Werror`.

## Resultado local

O build CMake Linux foi concluído com sucesso usando raylib 5.5 vendorizado. Os avisos restantes vêm principalmente de conversões numéricas do jogo e do código interno da raylib.
