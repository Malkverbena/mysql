# tools/boost.py
# - Usa o sistema nativo do Boost (bootstrap + b2) para preparar/instalar headers
# - Instala em: thirdparty/bin/<platform>/<arch>/boost/install/include
# - Não requer paths do usuário; assume submódulo em thirdparty/boost (ou boost_1_xx_x)

import os
import glob
import subprocess
import shutil
from os.path import join as PJ

REQUIRED_REL_HEADER = PJ('boost', 'mysql', 'mariadb_collations.hpp')

def _run(cmd, cwd=None, env=None):
    print(f'[boost] {cmd}')
    subprocess.check_call(cmd, shell=True, cwd=cwd, env=env)

def _jobs():
    try:
        n = os.cpu_count() or 1
    except Exception:
        n = 1
    return max(1, int(n))

def _dir(p): return os.path.isdir(p)
def _file(p): return os.path.isfile(p)

def _find_boost_root(thirdparty_base):
    """
    Localiza a raiz do submódulo Boost (onde ficam bootstrap.sh/b2).
    Suporta:
      - thirdparty/boost
      - thirdparty/boost_1_XX_X
    """
    candidates = [PJ(thirdparty_base, 'boost')]
    candidates += sorted(glob.glob(PJ(thirdparty_base, 'boost_*')))

    for root in candidates:
        if _file(PJ(root, 'bootstrap.sh')) or _file(PJ(root, 'bootstrap.bat')):
            return root

    raise RuntimeError(
        "[mysql module] Não encontrei o submódulo do Boost em thirdparty/.\n"
        "Esperado algo como: thirdparty/boost (com bootstrap.sh)."
    )

def _ensure_dir(p):
    os.makedirs(p, exist_ok=True)

def _installed_headers_ok(install_inc):
    return _file(PJ(install_inc, REQUIRED_REL_HEADER))

def BoostInstall(env, thirdparty_base, outbin_base, platform, bits):
    boost_root   = _find_boost_root(thirdparty_base)
    install_root = PJ(outbin_base, 'boost', 'install')
    install_inc  = PJ(install_root, 'include')
    dst_boost    = PJ(install_inc, 'boost')

    # Já instalado?
    if _dir(dst_boost) and _installed_headers_ok(install_inc):
        print(f'[boost] usando instalação existente: {install_inc}')
        return {'include_dirs': [install_inc]}

    # 1) Bootstrap (se necessário)
    if os.name == 'nt':
        b2_path = PJ(boost_root, 'b2.exe')
        b2_cmd  = 'b2.exe'  # com cwd=boost_root
        if not _file(b2_path):
            _run('cmd /c bootstrap.bat', cwd=boost_root)
    else:
        b2_path = PJ(boost_root, 'b2')
        b2_cmd  = './b2'    # com cwd=boost_root (CORREÇÃO AQUI)
        if not _file(b2_path):
            _run('bash ./bootstrap.sh', cwd=boost_root)
        # garante executável (eventuais permissões após checkout)
        if _file(b2_path):
            try:
                os.chmod(b2_path, 0o755)
            except Exception:
                pass

    if not _file(b2_path):
        raise RuntimeError('[boost] Falha ao gerar o b2 (bootstrap).')

    # 2) Rodar b2 "headers" (não compila libs; prepara árvore de headers)
    common_args = [
        b2_cmd,                          # usar comando relativo ao cwd
        f'-j{_jobs()}',
        f'address-model={bits}',
        'threading=multi',
        'variant=release',
        'link=static',
        'runtime-link=static',
        '--ignore-site-config',
    ]
    if platform == 'windows':
        common_args.append('target-os=windows')
    elif platform == 'linuxbsd':
        common_args.append('target-os=linux')

    _run(' '.join(common_args + ['headers']), cwd=boost_root)

    # 3) Instalar headers no prefixo do módulo
    _ensure_dir(install_inc)
    print(f'[boost] instalando headers em: {install_inc}')
    # Copia somente a pasta "boost/" (headers) para o prefixo
    src_headers = PJ(boost_root, 'boost')
    if not _dir(src_headers):
        alt = PJ(boost_root, 'include', 'boost')
        if _dir(alt):
            src_headers = alt

    if not _dir(src_headers):
        raise RuntimeError('[boost] Não encontrei diretório de headers "boost/".')

    shutil.copytree(src_headers, dst_boost, dirs_exist_ok=True)

    # 4) Verificação final
    if not _installed_headers_ok(install_inc):
        raise RuntimeError(
            "[boost] Instalação de headers concluída, mas não encontrei "
            "'boost/mysql/mariadb_collations.hpp' no destino."
        )

    print(f'[boost] headers instalados em: {install_inc}')
    return {'include_dirs': [install_inc]}

def generate(env):
    env.AddMethod(BoostInstall, 'BoostInstall')

def exists(env):
    return True
