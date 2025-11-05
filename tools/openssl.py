# tools/openssl.py
# - Usa o build nativo do OpenSSL (./Configure + make install_sw)
# - Compila ESTÁTICO e instala em: thirdparty/bin/<platform>/<arch>/openssl/install/{include,lib|lib64}
# - Detecta plataforma/bits (Linux e cross Linux->Windows via MinGW)
# - Não requer paths do usuário; assume submódulo em thirdparty/openssl

import os
import subprocess
from os.path import join as PJ

def _run(cmd, cwd=None, env=None):
    print(f'[openssl] {cmd}')
    subprocess.check_call(cmd, shell=True, cwd=cwd, env=env)

def _jobs():
    try:
        n = os.cpu_count() or 1
    except Exception:
        n = 1
    return max(1, int(n))

def _target_for_configure(platform, bits):
    if platform == 'linuxbsd':
        return 'linux-x86_64' if bits == 64 else 'linux-x86'
    if platform == 'windows':
        return 'mingw64' if bits == 64 else 'mingw'
    raise RuntimeError(f'[openssl] Plataforma não suportada: {platform}/{bits} bits')

def _env_for_cross(platform, bits, prefer_mingw):
    env = os.environ.copy()
    if platform == 'windows' and prefer_mingw:
        trip = 'x86_64-w64-mingw32' if bits == 64 else 'i686-w64-mingw32'
        env.update({
            'CC':      f'{trip}-gcc',
            'CXX':     f'{trip}-g++',
            'AR':      f'{trip}-ar',
            'RANLIB':  f'{trip}-ranlib',
            'WINDRES': f'{trip}-windres',
            'CROSS_COMPILE': f'{trip}-',
        })
    return env

def _ensure_dir(p):
    os.makedirs(p, exist_ok=True)

def _has_static_libs(libdir):
    if not os.path.isdir(libdir):
        return False
    ssl_a      = os.path.join(libdir, 'libssl.a')
    crypto_a   = os.path.join(libdir, 'libcrypto.a')
    ssl_lib    = os.path.join(libdir, 'libssl.lib')
    crypto_lib = os.path.join(libdir, 'libcrypto.lib')
    return (os.path.isfile(ssl_a) and os.path.isfile(crypto_a)) or (os.path.isfile(ssl_lib) and os.path.isfile(crypto_lib))

def _has_headers(incdir):
    return os.path.isfile(os.path.join(incdir, 'openssl', 'ssl.h'))

def _find_libdir_abs(install_root_abs):
    """Retorna o diretório de libs que realmente contém as libs estáticas (lib/ ou lib64/)."""
    for cand in ('lib', 'lib64'):
        d = os.path.join(install_root_abs, cand)
        if _has_static_libs(d):
            return d
    return None

def OpenSSLStatic(env, thirdparty_base, outbin_base, platform, arch, bits, prefer_mingw=False):
    # Caminhos relativos (para retorno ao SCons)
    install_root_rel = PJ(outbin_base, 'openssl', 'install')
    install_inc_rel  = PJ(install_root_rel, 'include')
    # lib dir relativo será decidido depois (lib ou lib64)
    build_dir_rel    = PJ(outbin_base, 'openssl', 'build')
    src_dir_rel      = PJ(thirdparty_base, 'openssl')

    # Caminhos absolutos para as chamadas externas
    build_dir_abs    = os.path.abspath(build_dir_rel)
    install_root_abs = os.path.abspath(install_root_rel)
    install_inc_abs  = os.path.abspath(install_inc_rel)
    src_dir_abs      = os.path.abspath(src_dir_rel)
    src_config_abs   = os.path.join(src_dir_abs, 'Configure')

    # Já instalado? (reutiliza)
    libdir_abs = _find_libdir_abs(install_root_abs)
    if _has_headers(install_inc_abs) and libdir_abs:
        # Determina relativo libdir (lib ou lib64) a partir do sufixo
        libdir_name = os.path.basename(libdir_abs)
        install_lib_rel = PJ(install_root_rel, libdir_name)
        print(f'[openssl] usando instalação pré-existente: {install_root_rel} ({libdir_name})')
        return {'include_dirs': [install_inc_rel], 'lib_dirs': [install_lib_rel], 'libs': ['ssl', 'crypto']}

    # Submódulo de fontes do OpenSSL
    if not os.path.isfile(src_config_abs):
        raise RuntimeError(
            '[mysql module] OpenSSL não encontrado em thirdparty/openssl (submódulo ausente).\n'
            'Execute: git submodule update --init --recursive'
        )

    # Pastas de build/instalação
    _ensure_dir(build_dir_abs)
    _ensure_dir(install_inc_abs)
    # Não criamos lib/ aqui; deixe o instalador escolher (lib ou lib64)

    target   = _target_for_configure(platform, bits)
    xenv     = _env_for_cross(platform, bits, prefer_mingw)

    conf_flags = [
        target,
        'no-shared',
        'no-dso',
        'no-tests',
        'no-module',
        'no-filenames',
        f'--prefix={install_root_abs}'   # prefixo absoluto exigido pelo OpenSSL
    ]

    # -fPIC em Linux para linkar estático dentro de .so do Godot
    cflags = xenv.get('CFLAGS', '')
    if platform == 'linuxbsd' and '-fPIC' not in cflags:
        xenv['CFLAGS']   = (cflags + ' -fPIC').strip()
        xenv['CXXFLAGS'] = (xenv.get('CXXFLAGS', '') + ' -fPIC').strip()

    # Em Windows/MinGW, se não houver nasm, desabilita asm para simplificar
    if platform == 'windows':
        try:
            subprocess.check_call(['nasm', '-v'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception:
            conf_flags.append('no-asm')

    print(f'[openssl] compilando a partir de fontes: {src_dir_rel}')
    _run(f'perl "{src_config_abs}" {" ".join(conf_flags)}', cwd=src_dir_abs, env=xenv)
    _run(f'make -j{_jobs()}', cwd=src_dir_abs, env=xenv)
    _run('make install_sw',   cwd=src_dir_abs, env=xenv)

    # Após instalar, descubra se foi lib/ ou lib64/
    libdir_abs = _find_libdir_abs(install_root_abs)
    if not (_has_headers(install_inc_abs) and libdir_abs):
        raise RuntimeError('[openssl] Build finalizado, mas não encontrei headers/libs estáticos esperados.')

    libdir_name     = os.path.basename(libdir_abs)  # 'lib' ou 'lib64'
    install_lib_rel = PJ(install_root_rel, libdir_name)

    print(f'[openssl] instalado em: {install_root_rel} ({libdir_name})')
    # Retorna caminhos RELATIVOS para o SCons
    return {'include_dirs': [install_inc_rel], 'lib_dirs': [install_lib_rel], 'libs': ['ssl', 'crypto']}

def generate(env):
    env.AddMethod(OpenSSLStatic, 'OpenSSLStatic')

def exists(env):
    return True
