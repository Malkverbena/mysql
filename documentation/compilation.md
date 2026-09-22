# Compilation

> **Current state:** the module is being fully rewritten. During the rewrite only the
> Linux x86_64 build is validated at every step, plus a Windows cross-build from Linux
> (section 4). The macOS and Android instructions have not been tested yet.

**Versions tested in this rewrite:** Boost `boost-1.92.0`, OpenSSL `openssl-4.0.2`
(stable tags, no development submodules). The `SCsub` checks the minimum versions
(Boost >= 1.85, OpenSSL >= 3.0) and the headers before building. Newer versions probably
work, but only the ones above were actually tested.

This module **does not download or build** Boost and OpenSSL. You must build them
manually before building Godot with the module. This guide gives the steps and the flags
used.

## Requirements

- Godot **4.6** or newer.
- A compiler with C++17 support: GCC or Clang (Linux/macOS), or Visual C++ (Windows).
- [**NASM**](https://www.nasm.us/pub/nasm/releasebuilds/), on Windows only, needed by
  OpenSSL.
- Git.
- Everything required to build Godot itself.

## Expected layout

By default the module expects the already built Boost and OpenSSL to be in **sibling**
folders of the module:

```
your_workspace/
├── godot/
├── mysql/              <- this module
└── thirdparty/
    ├── boost/          <- Boost clone, built (step below)
    └── openssl/        <- OpenSSL clone, built (step below)
```

## Configuration (`config.cfg`)

The Boost and OpenSSL paths and the module build options live in `mysql/config.cfg`,
which the `SCsub` reads on every build:

```ini
[paths]
boost_path = ../thirdparty/boost
openssl_path = ../thirdparty/openssl

[build]
boost_mysql_mode = separate
```

| Option | Values | Description |
|---|---|---|
| `boost_path` | folder | Built Boost (`boost/` with the headers and `stage/lib/`). Relative paths start from the folder of `config.cfg`. |
| `openssl_path` | folder | Built OpenSSL (`include/` and `lib64/` or `lib/`). Same rule for relative paths. |
| `boost_mysql_mode` | `separate` (default) or `header-only` | `separate` defines `BOOST_MYSQL_SEPARATE_COMPILATION` and Boost.MySQL is compiled once, in `boost_mysql_src.cpp`. `header-only` instantiates Boost.MySQL in every translation unit (slower build). |

If Boost/OpenSSL are somewhere else, edit `boost_path` and `openssl_path` in
`config.cfg`. There are no `boost_path=`/`openssl_path=` options on the scons command
line.

To build for another platform with different paths, create a `config.<platform>.cfg`
(for example `config.windows.cfg`). The `SCsub` reads it instead of `config.cfg` when
the build uses that `platform=`.

## Language standard

- **C++17** (`-std=c++17`, or `/std:c++17` on MSVC), the same as Godot 4.
- **No exceptions (`no_exception`)**: the module does not add `-fexceptions` and follows
  the Godot default (`disable_exceptions=yes`). Boost.MySQL errors are handled through
  the `error_code`/`diagnostics` overloads, and asynchronous operations use callbacks
  (`void(error_code)`), not C++20 coroutines or `use_future`.

### Module files tied to `no_exception`

- `scr/throw_exception.cpp`: with `-fno-exceptions` Boost declares
  `boost::throw_exception()` but does not define it. The module defines it: it logs the
  message in Godot and aborts (`CRASH_NOW_MSG`). There is no way to recover without
  exceptions, so reaching it is always fatal.
- `scr/boost_mysql_src.cpp`: in `separate` mode it instantiates Boost.MySQL. It plays the
  role of `<boost/mysql/src.hpp>`, but without `impl/connection_pool.ipp`, whose
  `try/catch(...)` does not compile with `-fno-exceptions`. The module has its own pool
  (`MySQLPool`). When updating Boost, compare the list of `.ipp` files with the one in
  `boost/mysql/src.hpp`.

## Linked libraries

According to the Boost.MySQL documentation ("Integrating Boost.MySQL"), the link
requirements are:

| Library | Reason |
|---|---|
| `libboost_charconv` | The only Boost.MySQL dependency with a compiled part (Boost >= 1.85). |
| `libssl`, `libcrypto` | OpenSSL: TLS and `caching_sha2_password` authentication. |
| Threads (`pthread`) | Already linked by Godot. |

**`libquadmath`:** by default `b2` detects GCC's `__float128` and `libboost_charconv`
starts depending on `libquadmath` (`quadmath_snprintf`, `strtoflt128`, `isnanq`,
`isinfq`). `BOOST_CHARCONV_NO_QUADMATH` only works with CMake, not with `b2`. Building
with `cxxstd=17 cxxstd-dialect=iso` (command below) removes that dependency, and the
module does not link `libquadmath`.

**Link order:** `libssl` comes before `libcrypto` (`libssl.a` depends on `libcrypto.a`).
The `SCsub` already does this.

`libboost_thread` is **not** needed. `Boost.Context` would only be needed with
`asio::spawn`/`yield_context`, which the module does not use.

## 1. Building Boost

Boost.MySQL is part of Boost since version 1.82; a full clone of the `boostorg/boost`
monorepo already brings everything that is needed.

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

On Windows, replace `bootstrap.sh` with `bootstrap.bat` and `./b2` with `b2.exe`; set
`toolset` to `msvc` (or `gcc-mingw`/`clang-mingw` if cross-compiling with MinGW),
`target-os=windows`, and `architecture`/`address-model` for the target.

### Why these flags

| Flag | Reason |
|---|---|
| `link=static`, `runtime-link=static` | The module embeds Boost statically, so whoever runs the game does not need Boost `.so`/`.dll` files installed. |
| `threading=multi` | Boost.MySQL uses Boost.Asio, which requires multithreading support. |
| `variant=release` | Production build (no Boost debug symbols). |
| `cxxstd=17`, `cxxstd-dialect=iso` | The same language standard as the module (`-std=c++17`, no GNU extensions). The `iso` dialect also stops `b2` from detecting `__float128`, so `libboost_charconv` does not depend on `libquadmath` (see "Linked libraries"). |
| `toolset` | Must match the compiler used to build Godot. A Boost built with `gcc` does not reliably link against a Godot built with `clang`, and vice versa. |
| `--stagedir` | Where the built libraries go (`stage/lib/`), which is the `stage/lib` path inside the `boost_path` of `config.cfg`. |

**Result:** headers in `thirdparty/boost/boost/` (generated by `./b2 headers`; the
`boost_path` of `config.cfg` points to `thirdparty/boost`, not to that subfolder) and
static libraries `libboost_*.a` in `thirdparty/boost/stage/lib/`.

The build takes a few minutes, because `b2` builds every Boost library. The module only
links `libboost_charconv` (see "Linked libraries"). To check:
`ls thirdparty/boost/stage/lib/libboost_charconv.a` and
`ls thirdparty/boost/boost/mysql.hpp`.

## 2. Building OpenSSL

```bash
git clone https://github.com/openssl/openssl.git thirdparty/openssl
cd thirdparty/openssl

# Linux x86_64 (change the target below for another platform, see the table)
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

On Windows (with NASM installed and a "VS toolset" in the PATH), replace `./Configure`
with `perl Configure` and use `nmake`/`nmake install` instead of `make`/`make install`;
target `VC-WIN64A` (64 bits) or `VC-WIN32`.

### Common targets

| Platform / architecture | Target |
|---|---|
| Linux x86_64 (gcc) | `linux-x86_64` |
| Linux x86_64 (clang) | `linux-x86_64-clang` |
| Linux arm64 | `linux-aarch64` |
| Windows x86_64 (MSVC) | `VC-WIN64A` |
| macOS x86_64 | `darwin64-x86_64` |
| macOS arm64 | `darwin64-arm64` |

Cross-compilation (Android, iOS, riscv, powerpc...) is not covered by this guide. See
the official documentation of [Boost.Build](https://www.boost.org/build/tutorial.html)
and [OpenSSL](https://wiki.openssl.org/index.php/Compilation_and_Installation) for the
options of each target.

### Why these flags

| Flag | Reason |
|---|---|
| `no-shared` | Produces static `libssl.a`/`libcrypto.a`, the same reasoning as `link=static` in Boost. |
| `no-ssl3`, `no-weak-ssl-ciphers`, `no-legacy` | Removes obsolete or insecure protocols and algorithms that this module does not use, which reduces the attack surface. |
| `no-tests`, `no-docs` | Only shortens the build time; it does not affect the final result. |

**Result:** headers in `thirdparty/openssl/include/openssl/`, and the libraries
`libssl.a` and `libcrypto.a` in `thirdparty/openssl/lib64/`. Some versions install into
`lib/` instead; the `SCsub` looks in both.

## 3. Building the module together with Godot

```bash
git clone https://github.com/Malkverbena/mysql.git
# (or put this module inside godot/modules/, or use custom_modules
# pointing outside the Godot tree, as in the example below)

cd godot
scons platform=linuxbsd arch=x86_64 target=editor \
    custom_modules=../mysql \
    precision=double \
    -j"$(nproc)"
```

Building with `precision=double` is highly recommended.

If Boost/OpenSSL are not in the default sibling folder layout, adjust `boost_path` and
`openssl_path` in `mysql/config.cfg` (see "Configuration").

## 4. Cross-compiling for Windows from Linux (MinGW-w64)

Verified in this rewrite: Boost and OpenSSL cross-compiled with MinGW-w64, the module and
Godot built with `platform=windows`, and the resulting binary run and tested (the full
`tests/smoke_test.gd` suite) through [Wine](https://www.winehq.org/) against a real
MariaDB.

### Prerequisite

`x86_64-w64-mingw32-gcc`/`g++`/`ar`/`ranlib`/`windres` in the PATH (the `mingw-w64`
package on most Linux distributions).

### Boost

The same Boost clone used for Linux, but in a separate folder. Each platform needs its
own `stage/lib` with objects in the right format (ELF for Linux, COFF/PE for Windows). A
`git worktree` avoids cloning the whole monorepo again:

```bash
cd thirdparty/boost
git worktree add ../boost-windows boost-1.92.0   # the tag you built for Linux
cd ../boost-windows
git submodule update --init --recursive

cat > /tmp/mingw-user-config.jam <<'EOF'
using gcc : mingw : x86_64-w64-mingw32-g++ ;
EOF

./bootstrap.sh --prefix="$(pwd)" --libdir="$(pwd)/stage/lib" --includedir="$(pwd)/include"
./b2 headers
./b2 -j"$(nproc)" \
    --user-config=/tmp/mingw-user-config.jam \
    link=static \
    threading=multi \
    runtime-link=static \
    variant=release \
    --stagedir="$(pwd)/stage" \
    toolset=gcc-mingw \
    target-os=windows \
    address-model=64 \
    architecture=x86 \
    cxxstd=17 \
    cxxstd-dialect=iso
```

`toolset=gcc-mingw` alone is not enough. The `--user-config` is what teaches `b2` to find
the MinGW compiler (without it, `b2` tries to use the native `g++` and the result does not
run on Windows).

### OpenSSL

The same idea, with a separate worktree:

```bash
cd thirdparty/openssl
git worktree add ../openssl-windows openssl-4.0.2   # the tag you built for Linux
cd ../openssl-windows

./Configure mingw64 \
    no-ssl3 no-weak-ssl-ciphers no-legacy no-shared no-tests no-docs \
    --cross-compile-prefix=x86_64-w64-mingw32- \
    --prefix="$(pwd)" --openssldir="$(pwd)"
make depend
make -j"$(nproc)"
make install
```

`--cross-compile-prefix` is the piece that is missing compared with the Linux build.
Without it `Configure` uses the native `gcc` and the result is not a Windows binary.

> The same `make install` problem as in the Linux build can show up here (see section
> 2): if it fails in `install_dev` because `--prefix` is the source folder itself, the
> `.a` files were already generated in the root. Copy them manually to `lib64/`
> (`mkdir -p lib64 && cp libssl.a libcrypto.a lib64/`).

### Module config and build

Because Windows needs different paths from Linux, the `SCsub` reads `config.windows.cfg`
instead of `config.cfg` when `platform=windows` (see "Configuration"; the mechanism works
for any `config.<platform>.cfg`, not only Windows). The repository already has one, with
only the paths changed:

```ini
[paths]
boost_path = ../thirdparty/boost-windows
openssl_path = ../thirdparty/openssl-windows

[build]
boost_mysql_mode = separate
```

```bash
cd godot
scons platform=windows arch=x86_64 target=editor \
    custom_modules=../mysql \
    precision=double \
    d3d12=no \
    -j"$(nproc)"
```

`d3d12=no` avoids requiring the Direct3D 12 SDK, which is not part of this module.
Without this flag SCons stops early asking for `install_d3d12_sdk_windows.py`. Godot on
Linux, without `use_mingw=1`, already detects and uses MinGW-w64 automatically because it
does not find MSVC.

### Note

Verified: Linux and Windows (cross-compiled with MinGW). macOS is waiting for Apple
hardware to be available, iOS is deferred for the same reason, and Web is out of scope
for this module: browsers cannot open a raw TCP socket, which the MySQL protocol needs.
Android has not been tried yet in this rewrite.

## 5. Tests

The tests live in `mysql/tests/`.

**Unit tests (doctest).** Godot's own unit test runner. Every `tests/test_*.h` file is
picked up automatically when the engine is built with `tests=yes`. They need no database
server. `extra_suffix=tests` keeps this build separate from the normal one:

```bash
cd godot
scons platform=linuxbsd arch=x86_64 target=editor \
    custom_modules=../mysql \
    precision=double \
    tests=yes extra_suffix=tests \
    -j"$(nproc)"

./bin/godot.linuxbsd.editor.double.x86_64.tests --test --test-case="*MySQL*"
```

**Integration test (GDScript).** Runs against a real MySQL/MariaDB server. See
[`../tests/README.md`](../tests/README.md).

**Sanitizers.** Godot has built-in options: add `use_asan=yes use_ubsan=yes`, or
`use_tsan=yes` (TSan cannot be combined with ASan), to the `scons` line and run the
integration test with the resulting binary.
