# BOOST
git submodule deinit -f 3party/boost
git rm -f 3party/boost
git submodule add https://github.com/boostorg/boost.git thirdparty/boost
git -C thirdparty/boost submodule update --init --recursive --depth 1

# OPENSSL
git submodule deinit -f 3party/openssl
git rm -f 3party/openssl
git submodule add https://github.com/openssl/openssl.git thirdparty/openssl

# sincronizar e atualizar tudo
git submodule sync --recursive
git submodule update --init --recursive

# substituir referências no código/build
git grep -n -- '3party'
git grep -l -- '3party' | xargs sed -i 's#\b3party\b#thirdparty#g'

# limpar e recompilar
scons -c
scons platform=linuxbsd arch=x86_64 target=editor custom_modules="../modules" -j"$(nproc)"

# commit e push
git add -A
git commit -m "chore(submodules): readicionar submódulos em thirdparty/* e remover 3party/*"
git push -u origin HEAD

