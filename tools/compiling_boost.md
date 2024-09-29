# Compilação do Boost Multiplataforma

Este repositório contém um script em Python (`compiling_boost.py`) que facilita a compilação das bibliotecas Boost para diversas plataformas e arquiteturas. Além disso, inclui um arquivo de configuração (`compiling_boost.cfg`) e um guia completo de utilização neste `README.md`.

## Índice

- [Pré-requisitos](#pré-requisitos)
- [Configuração](#configuração)
- [Uso do Script](#uso-do-script)
  - [Sintaxe Básica](#sintaxe-básica)
  - [Argumentos](#argumentos)
  - [Exemplos](#exemplos)
- [Parâmetros do Arquivo de Configuração](#parâmetros-do-arquivo-de-configuração)
- [Compiladores Suportados](#compiladores-suportados)
- [Dependências por Plataforma e Arquitetura](#dependências-por-plataforma-e-arquitetura)
- [Compilação Cruzada](#compilação-cruzada)
- [Notas Adicionais](#notas-adicionais)

## Pré-requisitos

Antes de utilizar o script, certifique-se de que os seguintes compiladores e ferramentas estão instalados no seu sistema host:

- **GCC**: Compilador padrão para muitas distribuições Linux.
- **Clang**: Alternativa ao GCC, com suporte avançado para várias arquiteturas.
- **Boost.Build (`b2`)**: Ferramenta de construção do Boost.
- **Python 3**: Necessário para executar o script.
- **Dependências específicas de cada plataforma**: Veja a seção [Dependências por Plataforma e Arquitetura](#dependências-por-plataforma-e-arquitetura).

## Configuração

1. **Clone o repositório ou faça o download dos arquivos**:

```bash
	git clone https://github.com/seu-usuario/seu-repositorio.git
	cd seu-repositorio
```

```ini
[DEFAULT]
ndk_dir = /usr/local/android-ndk
sdk_dir = /Applications/Xcode.app/Contents/Developer
source_dir = /home/usuario/boost_1_80_0
build_dir: Caminho onde o Boost será compilado. (Deve ser um diretório onde você tem permissões de escrita)
install_dir = /home/usuario/boost_install
compiler = clang
```
