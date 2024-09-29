
# compiling_openssl.py


import sys
import os
import argparse
import configparser
import subprocess
import platform
import shutil

# Função para converter caminhos para absolutos
def to_absolute_path(path):
    if not path:
        return ''
    return os.path.abspath(os.path.expanduser(path))

# Carregar configurações do arquivo compiling_openssl.cfg
config = configparser.ConfigParser()
config.read('compiling_openssl.cfg')

ndk_dir = to_absolute_path(config.get('Paths', 'ndk_dir', fallback=''))
sdk_dir = to_absolute_path(config.get('Paths', 'sdk_dir', fallback=''))
source_dir = to_absolute_path(config.get('Paths', 'source_dir', fallback=''))
install_dir = to_absolute_path(config.get('Paths', 'install_dir', fallback=''))

# Listas de plataformas e arquiteturas suportadas
platforms = ['linux', 'windows', 'macos', 'android', 'ios', 'html5']
architectures = {
    'linux': ['x86_32', 'x86_64', 'armv7', 'armv8', 'aarch64', 'riscv64', 'powerpc32', 'powerpc64'],
    'windows': ['x86_32', 'x86_64'],
    'macos': ['x86_64', 'arm64v8'],
    'android': ['x86', 'x86_64', 'armv7', 'armv8', 'aarch64'],
    'ios': ['armv7', 'arm64v8'],
    'html5': ['asmjs', 'wasm']
}

def parse_args():
    parser = argparse.ArgumentParser(
        description='Script para compilar o OpenSSL para várias plataformas e arquiteturas.',
        add_help=False
    )
    parser.add_argument('platform', nargs='?', help='Plataforma alvo.')
    parser.add_argument('arch', nargs='?', help='Arquitetura alvo.')
    parser.add_argument('--help', action='store_true', help='Mostra esta mensagem de ajuda e sai.')
    parser.add_argument('--list_platform', action='store_true', help='Lista as plataformas disponíveis.')
    parser.add_argument('--list_arch', help='Lista as arquiteturas disponíveis para a plataforma especificada.')
    parser.add_argument('--check', help='Verifica as dependências para a plataforma ou arquitetura especificada.')
    parser.add_argument('--info', nargs='?', const=True, help='Exibe informações de dependências.')
    parser.add_argument('--threads', type=int, default=1, help='Define o número de threads a serem usadas durante a compilação.')
    parser.add_argument('--j', type=int, help='Define o número de threads a serem usadas durante a compilação (alias para --threads).')
    return parser.parse_args()

def list_platforms():
    print('Plataformas disponíveis:')
    for p in platforms:
        print(f'- {p}')

def list_architectures(platform_name):
    if platform_name in architectures:
        print(f'Arquiteturas disponíveis para {platform_name}:')
        for arch in architectures[platform_name]:
            print(f'- {arch}')
    else:
        print(f'Plataforma desconhecida: {platform_name}')

def check_command_exists(command):
    return shutil.which(command) is not None

def check_dependencies(target):
    print(f'Verificando dependências para {target}...')
    missing = []

    # Dependências gerais
    if check_command_exists('gcc') or check_command_exists('clang'):
        pass
    else:
        missing.append('gcc ou clang')

    required_commands = ['make', 'perl']
    for cmd in required_commands:
        if not check_command_exists(cmd):
            missing.append(cmd)

    # Dependências específicas por plataforma
    if target == 'windows':
        if not check_command_exists('nasm'):
            missing.append('nasm')

    if target == 'android':
        if not ndk_dir or not os.path.isdir(ndk_dir):
            missing.append('Android NDK (verifique ndk_dir no compiling_openssl.cfg)')

    if target == 'ios':
        if not sdk_dir or not os.path.isdir(sdk_dir):
            missing.append('Xcode SDK (verifique sdk_dir no compiling_openssl.cfg)')

    if target == 'html5':
        if not check_command_exists('emcc'):
            missing.append('Emscripten SDK (emcc)')

    if missing:
        print('Dependências faltando:')
        for dep in missing:
            print(f'- {dep}')
        sys.exit(1)
    else:
        print('Todas as dependências estão instaladas.')

def show_info(target=None):
    if target is True:
        print('Dependências gerais para compilar o OpenSSL:')
        print('- Compilador C/C++ adequado (gcc ou clang)')
        print('- Make')
        print('- Perl')
    else:
        print(f'Dependências para {target}:')
        if target == 'linux':
            print('- GCC ou Clang')
            print('- Make')
            print('- Perl')
        elif target == 'windows':
            print('- Visual Studio com ferramentas de linha de comando')
            print('- NASM assembler')
            print('- Perl')
            print('- Make')
        elif target == 'macos':
            print('- Xcode com ferramentas de linha de comando')
            print('- Make')
            print('- Perl')
        elif target == 'android':
            print('- Android NDK')
            print('- Make')
            print('- Perl')
        elif target == 'ios':
            print('- Xcode com ferramentas de linha de comando')
            print('- SDK do iOS')
            print('- Make')
            print('- Perl')
        elif target == 'html5':
            print('- Emscripten SDK')
            print('- Make')
            print('- Perl')
        else:
            print('Plataforma ou arquitetura desconhecida.')

def set_environment(platform, arch):
    env = os.environ.copy()
    compiler = 'gcc' if check_command_exists('gcc') else 'clang'
    env['CC'] = compiler
    env['CXX'] = compiler.replace('gcc', 'g++').replace('clang', 'clang++')
    return env

def compile_openssl(platform_name, arch, threads):
    print(f'Compilando OpenSSL para {platform_name} - {arch} com {threads} threads...')

    if not source_dir or not os.path.isdir(source_dir):
        print(f'Diretório de origem não encontrado ou inválido: {source_dir}')
        sys.exit(1)

    os.makedirs(install_dir, exist_ok=True)

    env = set_environment(platform_name, arch)

    configure_cmd = ['./Configure', 'no-shared', 'no-weak-ssl-ciphers', 'no-legacy', 'no-tests', 'no-dso', 'no-engine']

    # Definir destino e configuração específica
    if platform_name == 'linux':
        configure_cmd += [f'linux-{arch}']
    elif platform_name == 'windows':
        configure_cmd += [f'mingw-{arch}']
    elif platform_name == 'macos':
        configure_cmd += [f'darwin64-{arch}-cc']
    elif platform_name == 'android':
        # Configuração simplificada para Android
        if arch in ['armv8', 'aarch64']:
            target = f'android-{arch}'
            configure_cmd += [target]
        elif arch == 'armv7':
            target = 'android-arm'
            configure_cmd += [target]
        else:
            configure_cmd += [f'android-{arch}']
    elif platform_name == 'ios':
        configure_cmd += [f'ios-{arch}']
    elif platform_name == 'html5':
        configure_cmd += ['asm']
    else:
        print('Plataforma não suportada para compilação.')
        sys.exit(1)

    # Definir diretório de instalação específico
    install_path = os.path.join(install_dir, platform_name, arch)
    install_path = to_absolute_path(install_path)
    os.makedirs(install_path, exist_ok=True)
    configure_cmd += [f'--prefix={install_path}']

    try:
        # Rodar o configure
        print('Configurando o OpenSSL...')
        subprocess.check_call(configure_cmd, cwd=source_dir, env=env)

        # Compilar
        print('Iniciando a compilação...')
        make_cmd = ['make', f'-j{threads}']
        subprocess.check_call(make_cmd, cwd=source_dir, env=env)

        # Instalar
        print('Instalando o OpenSSL...')
        make_install_cmd = ['make', 'install_sw']
        subprocess.check_call(make_install_cmd, cwd=source_dir, env=env)

        print(f'Compilação concluída para {platform_name} - {arch}.')
    except subprocess.CalledProcessError as e:
        print(f'Erro durante a compilação: {e}')
        sys.exit(1)

def main():
    args = parse_args()

    # Priorizar --j se fornecido
    threads = args.threads
    if args.j is not None:
        threads = args.j

    if args.help:
        print('Script para compilar o OpenSSL para várias plataformas e arquiteturas.')
        print('Uso: compiling_openssl.py [plataforma] [arquitetura] [opções]')
        print('Argumentos opcionais:')
        print('  --list_platform        Lista as plataformas disponíveis.')
        print('  --list_arch PLATFORM   Lista as arquiteturas disponíveis para PLATFORM.')
        print('  --check TARGET         Verifica as dependências para TARGET.')
        print('  --info [TARGET]        Exibe informações de dependências para TARGET.')
        print('  --threads N            Define o número de threads para compilação (padrão: 1).')
        print('  --j N                  Define o número de threads para compilação (alias para --threads).')
        sys.exit()

    if args.list_platform:
        list_platforms()
        sys.exit()

    if args.list_arch:
        list_architectures(args.list_arch)
        sys.exit()

    if args.check:
        check_dependencies(args.check)
        sys.exit()

    if args.info:
        show_info(args.info)
        sys.exit()

    if args.platform and args.arch:
        if args.platform in platforms and args.arch in architectures.get(args.platform, []):
            check_dependencies(args.platform)
            compile_openssl(args.platform, args.arch, threads)
        else:
            print('Plataforma ou arquitetura inválida.')
            sys.exit(1)
    else:
        print('Plataforma e arquitetura devem ser especificadas.')
        print('Use --help para mais informações.')
        sys.exit(1)

if __name__ == '__main__':
    main()

