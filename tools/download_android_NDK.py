import os
import sys
import platform
import requests
from bs4 import BeautifulSoup
import zipfile
import tarfile
import shutil

def get_os_arch():
    system = platform.system()
    machine = platform.machine().lower()
    
    if system == "Windows":
        os_name = "windows"
        extension = "zip"
    elif system == "Darwin":
        os_name = "darwin"
        extension = "zip"
    elif system == "Linux":
        os_name = "linux"
        extension = "zip"  # Alguns pacotes podem ser tar.xz
    else:
        raise Exception("Sistema operacional não suportado.")
    
    # Determina a arquitetura
    if "64" in machine:
        arch = "x86_64"
    elif "arm" in machine or "aarch64" in machine:
        arch = "aarch64"
    else:
        arch = "x86"  # Padrão
    
    return os_name, arch, extension

def get_latest_ndk_version():
    url = "https://developer.android.com/ndk/downloads"
    response = requests.get(url)
    if response.status_code != 200:
        raise Exception("Não foi possível acessar a página de downloads do NDK.")
    
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # Encontrar todas as versões listadas
    download_links = soup.find_all('a', href=True)
    versions = []
    for link in download_links:
        href = link['href']
        if 'android-ndk-r' in href and (href.endswith('.zip') or href.endswith('.tar.gz')):
            # Extrai a versão, por exemplo, r25b
            start = href.find('android-ndk-r') + len('android-ndk-r')
            end = href.find('-', start)
            if end == -1:
                end = href.find('.', start)
            version = href[start:end]
            versions.append(version)
    
    if not versions:
        raise Exception("Não foi possível encontrar versões do NDK na página de downloads.")
    
    # Supondo que as versões estão ordenadas, a última é a mais recente
    latest_version = sorted(versions, key=lambda s: [int(part) if part.isdigit() else part for part in s.replace('b','').split('.')], reverse=True)[0]
    return latest_version

def construct_download_url(version, os_name, arch, extension):
    base_url = "https://dl.google.com/android/repository/"
    
    if os_name == "windows":
        file_name = f"android-ndk-r{version}-{os_name}-{arch}.{extension}"
    elif os_name == "darwin":
        file_name = f"android-ndk-r{version}-{os_name}-{arch}.{extension}"
    elif os_name == "linux":
        file_name = f"android-ndk-r{version}-{os_name}-{arch}.{extension}"
    else:
        raise Exception("Sistema operacional não suportado para construção da URL.")
    
    download_url = base_url + file_name
    return download_url, file_name

def download_ndk(download_url, file_name, download_dir):
    local_path = os.path.join(download_dir, file_name)
    if os.path.exists(local_path):
        print(f"O arquivo {file_name} já foi baixado anteriormente.")
        return local_path
    
    print(f"Baixando NDK de {download_url}...")
    response = requests.get(download_url, stream=True)
    if response.status_code != 200:
        raise Exception(f"Falha ao baixar o NDK: status code {response.status_code}")
    
    with open(local_path, 'wb') as f:
        shutil.copyfileobj(response.raw, f)
    
    print(f"Download concluído: {local_path}")
    return local_path

def extract_ndk(file_path, extract_dir):
    print(f"Extraindo {file_path} para {extract_dir}...")
    if file_path.endswith('.zip'):
        with zipfile.ZipFile(file_path, 'r') as zip_ref:
            zip_ref.extractall(extract_dir)
    elif file_path.endswith('.tar.gz') or file_path.endswith('.tgz'):
        with tarfile.open(file_path, 'r:gz') as tar_ref:
            tar_ref.extractall(extract_dir)
    else:
        raise Exception("Formato de arquivo não suportado para extração.")
    print("Extração concluída.")

def get_installed_ndk_version(ndk_path):
    # Assume que o nome da pasta do NDK contém a versão, por exemplo, android-ndk-r25b
    if not os.path.exists(ndk_path):
        return None
    folders = [f for f in os.listdir(ndk_path) if os.path.isdir(os.path.join(ndk_path, f)) and 'android-ndk-r' in f]
    if not folders:
        return None
    # Pega a primeira pasta encontrada
    installed_version = folders[0].replace('android-ndk-r', '')
    return installed_version

def set_environment_variable(ndk_path):
    # Esta função configura a variável de ambiente ANDROID_NDK_HOME
    # Nota: Alterar variáveis de ambiente do sistema geralmente requer permissões administrativas
    # e métodos diferentes dependendo do SO. Como alternativa, pode-se informar ao usuário.

    print("\nPara configurar a variável de ambiente ANDROID_NDK_HOME, você pode adicionar o seguinte ao seu arquivo de configuração de shell (como .bashrc ou .zshrc):\n")
    print(f"export ANDROID_NDK_HOME={ndk_path}\n")
    print("Após adicionar, execute 'source ~/.bashrc' ou reinicie o terminal.")

def main():
    try:
        # Diretório onde o NDK será instalado
        home_dir = os.path.expanduser("~")
        ndk_dir = os.path.join(home_dir, "Android", "ndk")
        download_dir = os.path.join(home_dir, "Android", "ndk", "downloads")
        os.makedirs(download_dir, exist_ok=True)
        
        os_name, arch, extension = get_os_arch()
        print(f"Sistema: {os_name}, Arquitetura: {arch}, Extensão: {extension}")
        
        latest_version = get_latest_ndk_version()
        print(f"Última versão do NDK: r{latest_version}")
        
        installed_version = get_installed_ndk_version(ndk_dir)
        if installed_version:
            print(f"Versão instalada atualmente: r{installed_version}")
            if installed_version == latest_version:
                print("O NDK já está atualizado.")
                return
            else:
                print("Uma versão mais recente do NDK está disponível. Atualizando...")
        else:
            print("Nenhuma versão do NDK encontrada. Instalando a versão mais recente...")
        
        download_url, file_name = construct_download_url(latest_version, os_name, arch, extension)
        file_path = download_ndk(download_url, file_name, download_dir)
        
        extract_ndk(file_path, ndk_dir)
        
        # Opcional: Remover arquivos de download após extração
        os.remove(file_path)
        print(f"Arquivo de download {file_name} removido.")
        
        print(f"NDK r{latest_version} está instalado em {ndk_dir}")
        set_environment_variable(os.path.join(ndk_dir, f"android-ndk-r{latest_version}"))
        
    except Exception as e:
        print(f"Erro: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

