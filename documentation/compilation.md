# Compilação

Este módulo **não baixa nem compila** o Boost e o OpenSSL. Você precisa
compilá-los manualmente antes de compilar o Godot com o módulo. Este guia
traz o passo a passo e as flags usadas.

## Requisitos

- Um compilador com suporte a C++17: GCC, Clang (Linux/macOS) ou Visual
  C++ (Windows).
- [**NASM**](https://www.nasm.us/pub/nasm/releasebuilds/) — só no Windows,
  necessário para o OpenSSL.
- Git.
- Todos os requisitos para compilar o Godot.

## Layout esperado

O módulo espera, por padrão, que Boost e OpenSSL já compilados estejam em
pastas **irmãs** do módulo:

```
seu_workspace/
├── godot/
├── mysql/              <- este módulo
└── thirdparty/
    ├── boost/          <- clone do Boost, compilado (passo abaixo)
    └── openssl/        <- clone do OpenSSL, compilado (passo abaixo)
```

## Configuração (`config.cfg`)

Os caminhos do Boost e do OpenSSL e as opções de compilação do módulo ficam
em `mysql/config.cfg`, lido pelo `SCsub` a cada build:

```ini
[paths]
boost_path = ../thirdparty/boost
openssl_path = ../thirdparty/openssl

[build]
boost_mysql_mode = separate
```

| Opção | Valores | Descrição |
|---|---|---|
| `boost_path` | pasta | Boost compilado (`boost/` com os headers e `stage/lib/`). Caminhos relativos partem da pasta do `config.cfg`. |
| `openssl_path` | pasta | OpenSSL compilado (`include/` e `lib64/`). Mesma regra de caminhos relativos. |
| `boost_mysql_mode` | `separate` (padrão) ou `header-only` | `separate`: define `BOOST_MYSQL_SEPARATE_COMPILATION` e o Boost.MySQL é compilado uma única vez, em `register_types.cpp`. `header-only`: o Boost.MySQL é instanciado em cada unidade de compilação (build mais lento). |

Se Boost/OpenSSL estiverem em outro local, edite `boost_path` e
`openssl_path` no `config.cfg` — não há mais opções `boost_path=`/
`openssl_path=` na linha do scons.

## Padrões de linguagem

- **C++17** (`-std=c++17`, ou `/std:c++17` no MSVC), o mesmo padrão do Godot 4.
- **Sem exceções (`no_exception`)**: o módulo não adiciona `-fexceptions` e
  segue o padrão do Godot (`disable_exceptions=yes`). Os erros do
  Boost.MySQL são tratados pelas sobrecargas com `error_code`/`diagnostics`,
  e as operações assíncronas usam callbacks (`void(error_code)`), não
  corrotinas C++20 nem `use_future`.

### Arquivos do módulo ligados a `no_exception`

- `scr/throw_exception.cpp`: com `-fno-exceptions` o Boost declara
  `boost::throw_exception()` mas não a define. O módulo a define: registra a
  mensagem no Godot e aborta (`CRASH_NOW_MSG`). Não há como recuperar sem
  exceções, então chegar ali é sempre fatal.
- `scr/boost_mysql_src.cpp`: no modo `separate` instancia o Boost.MySQL. Faz o
  papel de `<boost/mysql/src.hpp>`, mas sem `impl/connection_pool.ipp`, cujo
  `try/catch(...)` não compila com `-fno-exceptions`. O módulo não usa o pool.
  Ao atualizar o Boost, compare a lista de `.ipp` com a de `boost/mysql/src.hpp`.

## Bibliotecas linkadas

Segundo a documentação do Boost.MySQL ("Integrating Boost.MySQL"), os requisitos
de link são:

| Biblioteca | Motivo |
|---|---|
| `libboost_charconv` | Única dependência do Boost.MySQL com parte compilada (Boost >= 1.85). |
| `libssl`, `libcrypto` | OpenSSL: TLS e autenticação `caching_sha2_password`. |
| Threads (`pthread`) | Já é linkado pelo Godot. |

**`libquadmath`:** por padrão o `b2` detecta o `__float128` do GCC e o
`libboost_charconv` passa a depender de `libquadmath` (`quadmath_snprintf`,
`strtoflt128`, `isnanq`, `isinfq`). O `BOOST_CHARCONV_NO_QUADMATH` só vale no
CMake, não no `b2`. Compilando com `cxxstd=17 cxxstd-dialect=iso` (comando acima)
essa dependência some e o módulo não linka `libquadmath`.

**Ordem no link:** `libssl` vem antes de `libcrypto` (a `libssl.a` depende da
`libcrypto.a`). O `SCsub` já faz isso.

`libboost_thread` **não** é necessária. `Boost.Context` só seria necessária
com `asio::spawn`/`yield_context`, que o módulo não usa.

## 1. Compilando o Boost

O Boost.MySQL faz parte do Boost desde a versão 1.82; um clone completo do
monorepo `boostorg/boost` já traz tudo que é preciso.

```bash
git clone --recurse-submodules https://github.com/boostorg/boost.git thirdparty/boost
cd thirdparty/boost

# Linux/macOS
./bootstrap.sh --prefix="$(pwd)" --libdir="$(pwd)/stage/lib" --includedir="$(pwd)/include"
./b2 headers
./b2 -j"$(nproc)" \
    link=static \
    threading=multi \
    runtime-link=static \
    variant=release \
    --stagedir="$(pwd)/stage" \
    toolset=gcc \
    address-model=64 \
    architecture=x86 \
    target-os=linux \
    cxxstd=17 \
    cxxstd-dialect=iso
```

No Windows, troque `bootstrap.sh` por `bootstrap.bat` e `./b2` por `b2.exe`;
ajuste `toolset` para `msvc` (ou `gcc-mingw`/`clang-mingw`, se estiver
cruzando com MinGW), `target-os=windows` e `architecture`/`address-model`
conforme o alvo.

### Por que essas flags

| Flag | Motivo |
|---|---|
| `link=static`, `runtime-link=static` | O módulo embarca o Boost estaticamente — quem joga o jogo não precisa ter `.so`/`.dll` do Boost instalado. |
| `threading=multi` | Boost.MySQL usa Boost.Asio, que exige suporte a múltiplas threads. |
| `variant=release` | Build de produção (sem símbolos de debug do Boost). |
| `cxxstd=17`, `cxxstd-dialect=iso` | Mesmo padrão de linguagem do módulo (`-std=c++17`, sem extensões GNU). O dialeto `iso` também impede o `b2` de detectar o `__float128`, então o `libboost_charconv` não depende de `libquadmath` (ver "Bibliotecas linkadas"). |
| `toolset` |
| `toolset` | Precisa casar com o compilador usado para compilar o Godot — um Boost compilado com `gcc` não linka de forma confiável contra um Godot compilado com `clang`, e vice-versa. |
| `--stagedir` | Onde as libs compiladas ficam (`stage/lib/`) — é o caminho `stage/lib` dentro do `boost_path` do `config.cfg`. |

**Resultado:** headers em `thirdparty/boost/boost/` (gerados por `./b2 headers`;
o `boost_path` do `config.cfg` aponta para `thirdparty/boost`, não para essa
subpasta) e bibliotecas estáticas `libboost_*.a` em `thirdparty/boost/stage/lib/`.

A compilação leva alguns minutos, pois o `b2` compila todas as bibliotecas do
Boost. O módulo só linka `libboost_charconv` (ver "Bibliotecas linkadas").
Para conferir: `ls thirdparty/boost/stage/lib/libboost_charconv.a` e
`ls thirdparty/boost/boost/mysql.hpp`.

## 2. Compilando o OpenSSL

```bash
git clone https://github.com/openssl/openssl.git thirdparty/openssl
cd thirdparty/openssl

# Linux x86_64 (troque o target abaixo para outra plataforma — ver tabela)
./Configure linux-x86_64 \
    no-ssl3 \
    no-weak-ssl-ciphers \
    no-legacy \
    no-shared \
    no-tests \
    no-docs \
    --prefix="$(pwd)" \
    --openssldir="$(pwd)"

make depend
make -j"$(nproc)"
make install
```

No Windows (com NASM instalado e um "VS toolset" no PATH), troque
`./Configure` por `perl Configure` e use `nmake`/`nmake install` no lugar
de `make`/`make install`; target `VC-WIN64A` (64 bits) ou `VC-WIN32`.

### Targets comuns

| Plataforma / arquitetura | Target |
|---|---|
| Linux x86_64 (gcc) | `linux-x86_64` |
| Linux x86_64 (clang) | `linux-x86_64-clang` |
| Linux arm64 | `linux-aarch64` |
| Windows x86_64 (MSVC) | `VC-WIN64A` |
| macOS x86_64 | `darwin64-x86_64` |
| macOS arm64 | `darwin64-arm64` |

Compilação cruzada (Android, iOS, riscv, powerpc...) não está coberta por
este guia — consulte a documentação oficial do
[Boost.Build](https://www.boost.org/build/tutorial.html) e do
[OpenSSL](https://wiki.openssl.org/index.php/Compilation_and_Installation)
para as opções específicas de cada alvo.

### Por que essas flags

| Flag | Motivo |
|---|---|
| `no-shared` | Gera `libssl.a`/`libcrypto.a` estáticas, mesmo raciocínio do `link=static` do Boost. |
| `no-ssl3`, `no-weak-ssl-ciphers`, `no-legacy` | Remove protocolos e algoritmos obsoletos/inseguros que este módulo não usa — reduz superfície de ataque. |
| `no-tests`, `no-docs` | Só encurta o tempo de build; não afeta o resultado final. |

**Resultado:** headers em `thirdparty/openssl/include/openssl/`, bibliotecas
`libssl.a` e `libcrypto.a` em `thirdparty/openssl/lib64/` (algumas versões instalam em `lib/` — confira
depois do `make install` e ajuste `openssl_path` no `config.cfg` e o caminho da lib no `SCsub` se for o caso).

## 3. Compilando o módulo junto com o Godot

```bash
git clone https://github.com/Malkverbena/mysql.git
# (ou coloque este módulo dentro de godot/modules/, ou use custom_modules
# apontando para fora da árvore do Godot, como no exemplo abaixo)

cd godot
scons platform=linuxbsd arch=x86_64 target=editor \
    custom_modules=../mysql \
    precision=double \
    -j"$(nproc)"
```

É altamente recomendado compilar com `precision=double`.

Se Boost/OpenSSL não estiverem no layout de pastas irmãs padrão, ajuste
`boost_path` e `openssl_path` em `mysql/config.cfg` (ver "Configuração").

### Nota

No momento só é possível compilar este módulo para Linux e Windows.
Suporte a macOS ainda está em desenvolvimento. É perfeitamente possível
compilar para outras plataformas como Android e iOS, mas esse suporte
ainda não foi adicionado — ajuda é bem-vinda.
