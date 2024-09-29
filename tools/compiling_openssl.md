# Compilando OpenSSL com compiling_openssl.py

Este repositório fornece um script em Python, `compiling_openssl.py`, que automatiza a compilação do OpenSSL para várias plataformas e arquiteturas. O script suporta Linux, Windows, macOS, Android, iOS e HTML5 usando Emscripten.

## Sumário

- [Requisitos](#requisitos)
- [Instalação](#instalação)
- [Configuração](#configuração)
- [Uso](#uso)
  - [Exemplos](#exemplos)
- [Plataformas e Arquiteturas Suportadas](#plataformas-e-arquiteturas-suportadas)
- [Dependências Específicas](#dependências-específicas)
- [Parâmetros do Script](#parâmetros-do-script)
- [Configurando compiling_openssl.cfg](#configurando-compiling_opensslcfg)
- [Informações Adicionais](#informações-adicionais)
- [Observações](#observações)
- [Licença](#licença)

## Requisitos

- **Python 3.x**
- **OpenSSL** versão 3.x baixado e extraído
- **Compiladores e ferramentas de construção** apropriados para as plataformas alvo
- **Dependências específicas** para cada plataforma (detalhadas abaixo)

## Instalação

Clone este repositório ou copie os arquivos `compiling_openssl.py` e `compiling_openssl.cfg` para o seu diretório de trabalho.

```bash
git clone https://github.com/seu_usuario/seu_repositorio.git
```

## Configuração

Edite o arquivo compiling_openssl.cfg para configurar os caminhos necessários:

```ini
[Paths]
ndk_dir = /caminho/para/android/ndk
sdk_dir = /caminho/para/xcode/sdk
source_dir = /caminho/para/openssl/fonte
install_dir = /caminho/para/instalacao/openssl
```

- **ndk_dir:** Caminho para o Android NDK (necessário para compilação para Android)
- **sdk_dir:** Caminho para o Xcode SDK (necessário para compilação para iOS)
- **source_dir:** Caminho para o diretório de origem do OpenSSL
- **install_dir:** Caminho onde os arquivos compilados serão instalados

## Uso

```bash
python compiling_openssl.py [plataforma] [arquitetura] [opções]
```

## Exemplos

- **Compilar para Linux x86_64 com 4 threads:**

```bash
python compiling_openssl.py linux x86_64 --threads 4
```

- **Listar plataformas disponíveis:**

```bash
python compiling_openssl.py --list_platform
```

- **Listar arquiteturas disponíveis para Android:**

```bash
python compiling_openssl.py --list_arch android
```

- **Verificar dependências para iOS:**

```bash
python compiling_openssl.py --check ios
```

- **Exibir informações de dependências gerais:**

```bash
python compiling_openssl.py --info
```

- **Exibir informações de dependências para Windows:**

```bash
python compiling_openssl.py --info windows
```

- **Obter ajuda do script:**

```bash
python compiling_openssl.py --help
```

## Plataformas e Arquiteturas Suportadas

- **Plataformas**

  | Linux | windows | macos | android | IOS | html5 |
  | ----- | ------- | ----- | ------- | --- | ----- |
- **Arquiteturas**

  - |       Linux       |        |      |       |         |         |           |           |
    | :---------------: | :-----: | :---: | ----- | ------- | ------- | --------- | --------- |
    |      x86_32      | x86_64 | armv7 | armv8 | aarch64 | riscv64 | powerpc32 | powerpc64 |
    | **Windows** |        |      |       |         |         |           |           |
    |      x86_32      | x86_64 |      |       |         |         |           |           |
    |  **macOS**  |        |      |       |         |         |           |           |
    |      x86_64      | arm64v8 |      |       |         |         |           |           |
    | **Android** |        |      |       |         |         |           |           |
    |        x86        | x86_64 | armv7 | armv8 | aarch64 |         |           |           |
    |   **iOS**   |        |      |       |         |         |           |           |
    |       armv7       | arm64v8 |      |       |         |         |           |           |
    |  **HTML5**  |        |      |       |         |         |           |           |
    |       wasm       |  asmjs  |      |       |         |         |           |           |

## Dependências Específicas

* **Linux**

  - GCC ou Clang
  - Make
  - Perl
* **Windows**

  - Visual Studio com ferramentas de linha de comando
  - NASM assembler
  - Perl
  - Make
* **macOS**

  - Xcode com ferramentas de linha de comando
  - Make
  - Perl
* **Android**

  - Android NDK
  - Make
  - Perl
* **iOS**

  - Xcode com ferramentas de linha de comando
  - SDK do iOS
  - Make
  - Perl
* **HTML5 (Emscripten)**

  - Emscripten SDK
  - Make
  - Perl

Parâmetros do Script

* `plataforma`: Plataforma alvo para compilação.
* `arquitetura`: Arquitetura alvo para compilação.
* `--help`: Exibe a ajuda do script.
* `--list_platform`: Lista as plataformas disponíveis.
* `--list_arch PLATFORM`: Lista as arquiteturas disponíveis para a plataforma especificada.
* `--check TARGET`: Verifica as dependências para a plataforma ou arquitetura especificada.
* `--info [TARGET]`: Exibe informações de dependências. Se `TARGET` for especificado, exibe informações específicas para ele.
* `--threads N`: Define o número de threads a serem usadas durante a compilação (padrão: 1).

```ini
[Paths]
ndk_dir = /home/usuario/Android/Sdk/ndk-bundle
sdk_dir = /Applications/Xcode.app/Contents/Developer
source_dir = /home/usuario/Downloads/openssl-3.x
install_dir = /home/usuario/openssl_builds
```

* Certifique-se de que os caminhos apontam para os diretórios corretos em seu sistema.
* Para plataformas que não exigem `ndk_dir` ou `sdk_dir`, esses campos podem ser deixados em branco ou ignorados.

## Informações Adicionais

* O script compila o OpenSSL com as seguintes flags: `no-ssl2`, `no-shared`, `no-weak-ssl-ciphers`, `no-legacy`, `no-tests`, `no-dso`, `no-engine`.
* Apenas bibliotecas estáticas serão geradas.
* As bibliotecas compiladas serão instaladas em subdiretórios dentro de `install_dir`, separados por plataforma e arquitetura.
* **Compilação paralela:** Utilize a opção `--threads` para acelerar o processo de compilação utilizando múltiplas threads. Por exemplo, `--threads 4` usará 4 threads durante a compilação.

## Observações

* **Verificação de Dependências:** Antes de iniciar a compilação, é altamente recomendado verificar se todas as dependências estão instaladas usando o parâmetro `--check`. Por exemplo:

```bash
python compiling_openssl.py --check android
```

* **Compilação Cruzada:** A compilação para plataformas como Android e iOS pode requerer configurações adicionais específicas do compilador ou do ambiente. Certifique-se de que o NDK e o SDK do Xcode estão corretamente configurados nos caminhos especificados.
* **Permissões:** Em sistemas Unix, você pode precisar de permissões adequadas para executar scripts e instalar software. Utilize `chmod +x compiling_openssl.py` para tornar o script executável, se necessário.

## Licença

Este projeto está licenciado sob os termos da licença MIT.

---

## Detalhes das Implementações

### Verificação de Dependências Específicas

A função `check_dependencies(target)` agora verifica a presença de dependências gerais e específicas para cada plataforma. Ela verifica:

- **Dependências Gerais:**

  - `gcc` ou `clang`
  - `make`
  - `perl `
- **Dependências Específicas:**

  - **Windows:** `nasm`
  - **Android:** Verifica se o `ndk_dir` está configurado corretamente
  - **iOS:** Verifica se o `sdk_dir` está configurado corretamente
  - **HTML5:** `emcc` (Emscripten)

Se alguma dependência estiver faltando, o script listará as dependências faltantes e encerrará a execução.

### Lógica de Compilação Específica

A função `compile_openssl(platform_name, arch, threads)` agora inclui lógica para configurar e compilar o OpenSSL com base na plataforma e arquitetura especificadas. Exemplos de configurações específicas:

- **Linux:** Utiliza `linux-arch` como destino.
- **Windows:** Utiliza `mingw-arch` como destino.
- **macOS:** Utiliza `darwin64-arch-cc` como destino.
- **Android:** Configura com base na arquitetura Android específica.
- **iOS:** Configura com base na arquitetura iOS específica.
- **HTML5:** Utiliza `asm` como destino.

Além disso, a função define o número de threads a serem usadas durante a compilação com a opção `-j` no comando `make`.

### Inclusão de `clang` e `gcc`

A função `set_environment(platform, arch)` define as variáveis de ambiente `CC` e `CXX` para utilizar `gcc` ou `clang`, dependendo de qual está disponível no sistema.

### Opção para Definir o Número de Threads

O parâmetro `--threads N` permite ao usuário especificar o número de threads a serem usadas durante a compilação. O valor padrão é `1` se o parâmetro não for fornecido.

### Atualizações no README.md

O arquivo `README.md` foi atualizado para incluir:

- **Descrição das Novas Funcionalidades:**

  - Verificação de dependências específicas
  - Lógica de compilação para cada plataforma e arquitetura
  - Suporte para `clang` e `gcc`
  - Opção para definir o número de threads durante a compilação
- **Instruções de Uso Atualizadas:**

  - Exemplos de como usar a nova opção `--threads`
  - Informações detalhadas sobre dependências específicas para cada plataforma
- **Detalhes Adicionais:**

  - Explicações sobre como o script lida com a compilação cruzada
  - Recomendações para verificar dependências antes de compilar

Se tiver alguma dúvida ou precisar de mais assistência, sinta-se à vontade para perguntar!
