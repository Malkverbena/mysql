#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess
import platform
import configparser
import shutil

from pathlib import Path

# Plataformas e arquiteturas suportadas
SUPPORTED_PLATFORMS = {
    'linux': ['x86', 'arm', 'riscv', 'power'],
    'windows': ['x86', 'arm'],
    'macos': ['x86', 'arm'],
    'android': ['arm', 'x86', 'riscv'],
    'ios': ['arm'],
    'html5': ['emscripten']
}

SUPPORTED_ARCHITECTURES = {
    'x86': ['32', '64'],
    'arm': ['32', '64'],
    'riscv': ['32', '64'],
    'power': ['64'],
    'emscripten': ['32', '64']
}

SUPPORTED_COMPILERS = ['gcc', 'clang']

# Lista de bibliotecas Boost reconhecidas
BOOST_LIBS_AVAILABLE = [
    'atomic', 'chrono', 'container', 'context', 'coroutine', 'date_time',
    'exception', 'filesystem', 'graph', 'iostreams', 'locale', 'log',
    'math', 'mpi', 'program_options', 'random', 'regex', 'serialization',
    'signals', 'system', 'test', 'thread', 'timer', 'type_erasure',
    'wave', 'mysql'  # Adicione 'mysql' como biblioteca personalizada
]

# Mapeamento de bibliotecas Boost que requerem exceções
BOOST_LIBS_WITH_EXCEPTIONS = [
    'asio', 'beast', 'filesystem', 'regex', 'system', 'program_options', 'thread', 'chrono', 'date_time'
]

# Mapeamento de bibliotecas Boost que não requerem exceções
BOOST_LIBS_WITHOUT_EXCEPTIONS = [
    'iostreams', 'log', 'test', 'serialization'
]

# Dependências por plataforma
DEPENDENCIES = {
    'linux': ['gcc', 'clang', 'make', 'python3', 'openssl'],
    'windows': ['gcc', 'clang', 'make', 'python3', 'openssl'],
    'macos': ['gcc', 'clang', 'make', 'python3', 'openssl'],
    'android': ['gcc', 'clang', 'make', 'python3', 'android-ndk', 'openssl'],
    'ios': ['clang', 'make', 'python3', 'xcode', 'openssl'],
    'html5': ['emscripten', 'make', 'python3', 'openssl']
}

def parse_args():
    parser = argparse.ArgumentParser(
        description='Script para compilar as bibliotecas Boost para várias plataformas e arquiteturas.'
    )
    parser.add_argument('platform', nargs='?', help='Plataforma alvo (e.g., linux, windows, macos, android, ios, html5)')
    parser.add_argument('architecture', nargs='?', help='Arquitetura alvo (e.g., x86, arm, riscv, power, emscripten)')
    parser.add_argument('bits', nargs='?', help='Bitagem alvo (32 ou 64)')
    parser.add_argument('--threads', '-j', type=int, default=1, help='Número de threads a serem usadas durante a compilação')
    parser.add_argument('--list_platform', action='store_true', help='Lista as plataformas disponíveis')
    parser.add_argument('--list_arch', metavar='PLATFORM', help='Lista as arquiteturas e bitagens disponíveis para uma plataforma específica')
    parser.add_argument('--check', metavar='TARGET', help='Verifica as dependências para uma plataforma ou arquitetura específica')
    parser.add_argument('--info', nargs='?', const=True, metavar='TARGET', help='Lista as dependências necessárias. Opcionalmente, especifique uma plataforma ou arquitetura')
    parser.add_argument('--libs', action='store_true', help='Lista as bibliotecas que serão compiladas com as configurações atuais')
    parser.add_argument('--helpme', action='help', help='Exibe esta mensagem de ajuda e sai')
    return parser.parse_args()

def load_config():
    config = configparser.ConfigParser()
    config_file = Path(__file__).parent / 'compiling_boost.cfg'
    if not config_file.exists():
        print(f"Arquivo de configuração {config_file} não encontrado.")
        sys.exit(1)
    config.read(config_file)
    return config

def list_platforms():
    print("Plataformas disponíveis:")
    for platform_name in SUPPORTED_PLATFORMS.keys():
        print(f"- {platform_name}")

def list_architectures(platform_name):
    platform_name = platform_name.lower()
    if platform_name not in SUPPORTED_PLATFORMS:
        print(f"Plataforma '{platform_name}' não é suportada.")
        return
    print(f"Arquiteturas para {platform_name}:")
    for arch in SUPPORTED_PLATFORMS[platform_name]:
        bits = SUPPORTED_ARCHITECTURES.get(arch, [])
        bits_str = ', '.join(bits)
        print(f"- {arch} ({bits_str} bits)")

def check_dependencies(target=None):
    missing = []
    config = load_config()
    if target:
        target = target.lower()
        if target in SUPPORTED_PLATFORMS:
            deps = DEPENDENCIES.get(target, [])
        elif target in SUPPORTED_ARCHITECTURES:
            deps = ['gcc', 'clang', 'make', 'python3', 'openssl']
        else:
            print(f"Alvo desconhecido '{target}' para verificação de dependências.")
            return
    else:
        # Verificar dependências para a plataforma host
        host_platform = platform.system().lower()
        deps = DEPENDENCIES.get(host_platform, [])
    
    for dep in deps:
        if dep == 'android-ndk':
            ndk_dir = config.get('PATHS', 'ndk_dir', fallback='')
            if not os.path.isdir(ndk_dir):
                missing.append('Android NDK não encontrado. Por favor, defina ndk_dir em compiling_boost.cfg')
        elif dep == 'xcode':
            if platform.system() != 'Darwin' or not shutil.which('xcodebuild'):
                missing.append('Xcode não encontrado. Por favor, instale o Xcode e defina sdk_dir em compiling_boost.cfg')
        elif dep == 'emscripten':
            if not shutil.which('emcc'):
                missing.append('Emscripten não encontrado. Por favor, instale o Emscripten e adicione ao PATH.')
        elif dep == 'openssl':
            openssl_dir = config.get('PATHS', 'openssl_dir', fallback='')
            if not os.path.isdir(openssl_dir):
                missing.append('OpenSSL não encontrado. Por favor, defina openssl_dir em compiling_boost.cfg')
        else:
            if not shutil.which(dep):
                missing.append(dep)
    
    if missing:
        print("Dependências faltando:")
        for dep in missing:
            print(f"- {dep}")
    else:
        print("Todas as dependências estão satisfeitas.")

def show_info(target=None):
    if target:
        target = target.lower()
        if target in SUPPORTED_PLATFORMS:
            deps = DEPENDENCIES.get(target, [])
            print(f"Dependências para a plataforma '{target}':")
            for dep in deps:
                print(f"- {dep}")
        elif target in SUPPORTED_ARCHITECTURES:
            print(f"Dependências para a arquitetura '{target}':")
            print("- gcc ou clang")
            print("- make")
            print("- python3")
            print("- openssl")
        else:
            print(f"Alvo desconhecido '{target}'.")
    else:
        print("Dependências gerais para compilar o Boost:")
        all_deps = set(dep for deps in DEPENDENCIES.values() for dep in deps)
        for dep in all_deps:
            print(f"- {dep}")

def list_libs(config, exceptions_enabled):
    libs = config.get('PATHS', 'libs', fallback='')
    libs = [lib.strip() for lib in libs.split(',') if lib.strip()]
    print("Bibliotecas que serão compiladas:")
    for lib in libs:
        if lib == 'all':
            print("- all (Compilará todas as bibliotecas reconhecidas)")
            continue
        if lib in BOOST_LIBS_WITH_EXCEPTIONS:
            status = "Requer exceções" if exceptions_enabled else "Não será compilada (Requer exceções)"
        elif lib in BOOST_LIBS_WITHOUT_EXCEPTIONS:
            status = "Não requer exceções"
        else:
            status = "Status desconhecido quanto a exceções"
        print(f"- {lib} ({status})")

def validate_libs(libs):
    # 'all' é uma opção especial que inclui todas as bibliotecas disponíveis
    if 'all' in libs:
        return BOOST_LIBS_AVAILABLE
    invalid_libs = [lib for lib in libs if lib not in BOOST_LIBS_AVAILABLE]
    if invalid_libs:
        print("As seguintes bibliotecas não são reconhecidas pelo Boost.Build e serão removidas:")
        for lib in invalid_libs:
            print(f"- {lib}")
    return [lib for lib in libs if lib in BOOST_LIBS_AVAILABLE]

def select_compiler(config):
    compiler = config.get('SETTINGS', 'compiler', fallback='gcc').lower()
    if compiler not in SUPPORTED_COMPILERS:
        print(f"Compilador '{compiler}' não é suportado. Escolha entre 'gcc' ou 'clang'.")
        sys.exit(1)
    if not shutil.which(compiler):
        print(f"Compilador '{compiler}' não encontrado no PATH.")
        sys.exit(1)
    return compiler

def build_boost(config, target_platform, target_arch, target_bits, threads):
    source_dir = Path(config.get('PATHS', 'source_dir')).resolve()
    build_dir = Path(config.get('PATHS', 'build_dir')).resolve()
    install_dir_base = Path(config.get('PATHS', 'install_dir')).resolve()
    openssl_dir = Path(config.get('PATHS', 'openssl_dir')).resolve()
    
    # Verificar diretórios de origem e OpenSSL
    if not source_dir.exists():
        print(f"Diretório de origem do Boost '{source_dir}' não existe.")
        sys.exit(1)
    
    if not openssl_dir.exists():
        print(f"Diretório do OpenSSL '{openssl_dir}' não existe.")
        sys.exit(1)
    
    # Criar diretórios de build e install se não existirem
    build_dir.mkdir(parents=True, exist_ok=True)
    install_dir = install_dir_base / target_platform / target_arch / target_bits
    install_dir.mkdir(parents=True, exist_ok=True)
    
    # Selecionar o compilador
    compiler = select_compiler(config)
    toolset = 'clang' if compiler == 'clang' else 'gcc'
    
    # Definir caminhos do OpenSSL
    include_path = openssl_dir / 'include'
    lib_path = openssl_dir / 'lib'
    
    # Obter as bibliotecas a serem compiladas
    libs = config.get('PATHS', 'libs', fallback='')
    libs = [lib.strip() for lib in libs.split(',') if lib.strip()]
    
    # Verificar se exceções estão habilitadas
    exceptions = config.getboolean('SETTINGS', 'exceptions', fallback=True)
    
    # Filtrar bibliotecas com base na configuração de exceções
    libs = validate_libs(libs)
    if not libs:
        print("Nenhuma biblioteca válida para compilar.")
        sys.exit(1)
    
    if not exceptions:
        libs = [lib for lib in libs if lib not in BOOST_LIBS_WITH_EXCEPTIONS]
        if len(libs) < 1:
            print("Nenhuma biblioteca para compilar após filtrar as que requerem exceções.")
            sys.exit(1)
    
    # Exibir informações iniciais
    print("========================================")
    print("Configurações de Compilação do Boost")
    print("========================================")
    print(f"Plataforma Alvo         : {target_platform}")
    print(f"Arquitetura Alvo        : {target_arch}")
    print(f"Bitagem Alvo            : {target_bits}")
    print(f"Compilador              : {compiler}")
    print(f"Número de Threads       : {threads}")
    print(f"Diretório de Origem     : {source_dir}")
    print(f"Diretório de Build      : {build_dir}")
    print(f"Diretório de Instalação : {install_dir}")
    print(f"Diretório do OpenSSL    : {openssl_dir}")
    print("Bibliotecas a Compilar:")
    for lib in libs:
        if lib in BOOST_LIBS_WITH_EXCEPTIONS:
            print(f"- {lib} (Requer exceções)")
        elif lib in BOOST_LIBS_WITHOUT_EXCEPTIONS:
            print(f"- {lib} (Não requer exceções)")
        else:
            print(f"- {lib} (Status desconhecido quanto a exceções)")
    print("========================================\n")
    
    # Preparar o comando de bootstrap
    bootstrap_cmd = ['./bootstrap.sh', f'--prefix={install_dir}']
    if platform.system() == 'Windows':
        bootstrap_cmd = ['bootstrap.bat', f'--prefix={install_dir}']
    
    # Executar o bootstrap
    print("Executando bootstrap...")
    subprocess.run(bootstrap_cmd, cwd=source_dir, check=True)
    
    # Preparar o comando b2 com suporte a OpenSSL
    b2_cmd = [
        './b2',
        f'toolset={toolset}',
        'link=static',
        'threading=multi',
        'variant=release',
        'cxxflags="-std=c++20"',
        f'-j{threads}'
    ]
    
    # Adicionar definição de macro para desabilitar exceções, se necessário
    if not exceptions:
        b2_cmd.append('cxxflags="-DBOOST_NO_EXCEPTIONS"')
    
    # Adicionar módulos a serem compilados
    if 'all' in libs:
        # Remove 'all' e adiciona todas as bibliotecas disponíveis
        libs.remove('all')
        libs.extend([lib for lib in BOOST_LIBS_AVAILABLE if lib != 'all'])
    
    for lib in libs:
        b2_cmd.append(f'--with-{lib}')
    
    # Adicionar caminhos do OpenSSL
    b2_cmd.extend([
        f'include={include_path}',
        f'library-path={lib_path}'
    ])
    
    # Adicionar suporte ao OpenSSL
    b2_cmd.extend([
        'ssl=on',
        'openssl=on'
    ])
    
    # Adicionar links para zlib se necessário
    if 'zlib' in libs:
        b2_cmd.append('--with-zlib')
    
    if platform.system() == 'Windows':
        b2_cmd = ['b2.exe'] + b2_cmd[1:]
    
    # Definir variáveis de ambiente para OpenSSL
    env = os.environ.copy()
    env['OPENSSL_ROOT_DIR'] = str(openssl_dir)
    env['OPENSSL_INCLUDE_DIR'] = str(include_path)
    env['OPENSSL_LIB_DIR'] = str(lib_path)
    
    # Executar o b2 para compilar o Boost com suporte ao OpenSSL
    print("Compilando o Boost com suporte ao OpenSSL...")
    subprocess.run(b2_cmd, cwd=source_dir, check=True, env=env)
    
    print(f"\nO Boost foi compilado e instalado em {install_dir}")

def main():
    args = parse_args()
    config = load_config()
    
    if args.list_platform:
        list_platforms()
        sys.exit(0)
    
    if args.list_arch:
        list_architectures(args.list_arch)
        sys.exit(0)
    
    if args.check:
        check_dependencies(args.check)
        sys.exit(0)
    
    if args.info is not None:
        show_info(args.info if args.info != True else None)
        sys.exit(0)
    
    if args.libs:
        exceptions_enabled = config.getboolean('SETTINGS', 'exceptions', fallback=True)
        list_libs(config, exceptions_enabled)
        sys.exit(0)
    
    # Verificar se os argumentos necessários foram fornecidos
    if not args.platform or not args.architecture or not args.bits:
        print("Plataforma, arquitetura e bits são obrigatórios, a menos que esteja usando --list_platform, --list_arch, --check, --info ou --libs.")
        print("Use --helpme para mais informações.")
        sys.exit(1)
    
    target_platform = args.platform.lower()
    target_arch = args.architecture.lower()
    target_bits = args.bits
    
    # Validar plataforma
    if target_platform not in SUPPORTED_PLATFORMS:
        print(f"Plataforma não suportada: {target_platform}")
        list_platforms()
        sys.exit(1)
    
    # Validar arquitetura
    if target_arch not in SUPPORTED_PLATFORMS[target_platform]:
        print(f"Arquitetura '{target_arch}' não é suportada para a plataforma '{target_platform}'")
        list_architectures(target_platform)
        sys.exit(1)
    
    # Validar bitagem
    if target_bits not in SUPPORTED_ARCHITECTURES.get(target_arch, []):
        print(f"Bitagem '{target_bits}' não é suportada para a arquitetura '{target_arch}'")
        print(f"Bitagens suportadas para {target_arch}: {', '.join(SUPPORTED_ARCHITECTURES.get(target_arch, []))}")
        sys.exit(1)
    
    # Verificar dependências
    check_dependencies(target_platform)
    
    # Compilar o Boost
    try:
        build_boost(config, target_platform, target_arch, target_bits, args.threads)
    except subprocess.CalledProcessError as e:
        print(f"Ocorreu um erro durante a compilação do Boost: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
