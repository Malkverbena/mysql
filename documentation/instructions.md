# Instructions

Configuration and compilation of this module, together with Godot, on every supported
platform. See [tests.md](tests.md) for how to run the tests. Building **Godot itself**
(its own requirements, SCons options, how a custom module folder is picked up with
`custom_modules=`) is covered by
[Godot's own compiling documentation](https://docs.godotengine.org/en/stable/engine_details/development/compiling/index.html) —
this page only covers what is specific to this module: its dependencies (Boost, OpenSSL)
and the extra `scons` options it reads.

> **Current state:** Linux x86_64 is validated at every step; Windows (cross-compiled
> from Linux) and Android are also fully verified (see below). macOS has not been tried
> yet.

**Minimum versions: Boost 1.85, OpenSSL 3.0.** Boost.MySQL has required a compiled
Boost.Charconv since Boost 1.85; the `SCsub` checks both minimums, and the headers, before
building, and stops with an explicit error naming the problem if either is older.

**Versions tested:** Boost `boost-1.92.0`, OpenSSL `openssl-4.0.2` (stable tags, no
development submodules). Newer versions probably work; testing so far covered only the
versions above.

This module **does not download or build** Boost and OpenSSL. You must build them
manually before building Godot with the module. This guide gives the steps and the flags
used, per platform.

## Requirements

- Godot **4.6** or newer, and everything required to build Godot itself for your target
  platform (see Godot's compiling documentation, linked above).
- A compiler with C++17 support: GCC or Clang (Linux/macOS/Android NDK), or Visual C++
  (Windows).
- [**NASM**](https://www.nasm.us/pub/nasm/releasebuilds/), on Windows only, needed by
  OpenSSL.
- Git.

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

By default the module expects the already built Boost and OpenSSL in **sibling** folders
of the module (`../thirdparty/boost`, `../thirdparty/openssl`). If they are somewhere
else, edit `boost_path`/`openssl_path` — there are no equivalent options on the `scons`
command line.

To build for another platform with different paths, create a `config.<platform>.cfg`
(for example `config.windows.cfg`, already in the repository). The `SCsub` reads it
instead of `config.cfg` when the build uses that `platform=`.

**Android is a special case**: it builds one `.so` per architecture
(`arch=arm64/arm32/x86_64/x86_32`), and each needs its own compiled Boost/OpenSSL.
`config.android.cfg` points at the **root** of a per-arch tree
(`thirdparty/boost-android`, `thirdparty/openssl-android`); the `SCsub` appends the
current `arch=` automatically to reach the real build
(`thirdparty/boost-android/arm64/`, etc). See the Android subsection below.

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

## Building Boost and OpenSSL, per platform

A full clone of the `boostorg/boost` monorepo already brings everything that is needed:

```bash
git clone --recurse-submodules https://github.com/boostorg/boost.git thirdparty/boost
```

### Linux

```bash
cd thirdparty/boost
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

```bash
git clone https://github.com/openssl/openssl.git thirdparty/openssl
cd thirdparty/openssl

./Configure linux-x86_64 \
    no-ssl3 no-weak-ssl-ciphers no-legacy no-shared no-tests no-docs \
    --prefix="$(pwd)" --openssldir="$(pwd)"
make depend
make -j"$(nproc)"
make install
```

For a non-x86_64 Linux target (e.g. `linux-aarch64`), change the `Configure` target; the
Boost flags stay the same except `architecture`/`address-model`.

### Windows

**Native (MSVC):** replace `bootstrap.sh` with `bootstrap.bat` and `./b2` with `b2.exe`;
set `toolset=msvc`, `target-os=windows`. For OpenSSL, replace `./Configure` with
`perl Configure` and use `nmake`/`nmake install` instead of `make`/`make install`;
target `VC-WIN64A` (64 bits) or `VC-WIN32`. NASM must be installed and on the `PATH`.
Not tested yet.

**Cross-compiling from Linux with MinGW-w64 — verified**, including the
resulting binary run and tested (the full `tests/smoke_test.gd` suite) through
[Wine](https://www.winehq.org/) against a real MariaDB.

Prerequisite: `x86_64-w64-mingw32-gcc`/`g++`/`ar`/`ranlib`/`windres` in the `PATH` (the
`mingw-w64` package on most Linux distributions).

Each platform needs its own `stage/lib` with objects in the right format (ELF for Linux,
COFF/PE for Windows). A `git worktree` avoids cloning the whole monorepo again:

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

> The `make install` step can fail in `install_dev` when `--prefix` is the source folder
> itself: the `.a` files are already generated in the root by then. Copy them manually to
> `lib64/` (`mkdir -p lib64 && cp libssl.a libcrypto.a lib64/`).

`mysql/config.windows.cfg` already exists in the repository and points at
`thirdparty/boost-windows`/`thirdparty/openssl-windows`.

### macOS

Not attempted yet — waiting on access to Apple hardware to generate the cross-compilation
SDK (`osxcross`). An earlier version of the module supported macOS (see the commit
history); this will be reassessed against the current architecture once Apple hardware is
available. Native OpenSSL `Configure` targets are `darwin64-x86_64` and `darwin64-arm64`.

### Android

Cross-compiled with the Android NDK's own Clang (side-by-side NDK,
`$ANDROID_HOME/ndk/<version>`) — **verified**: all four ABIs build and
link, and the full integration test suite (97 checks) passed on two real devices
(arm64-v8a). See `platform/android/detect.py` in the Godot source for the NDK version
and minimum API level Godot itself requires; the same values are used here.

Android builds one `.so` **per architecture** (`arch=arm64`, `arm32`, `x86_64`,
`x86_32`), so Boost and OpenSSL are built **four times**, once per arch, each in its own
folder under `thirdparty/boost-android/<arch>/` and `thirdparty/openssl-android/<arch>/`
(a `git worktree` per arch, same idea as the Windows cross-build above). Target triples
and minimum API level (24, matching Godot's own minimum):

| `arch=` | NDK target triple |
|---|---|
| `arm64` | `aarch64-linux-android` |
| `arm32` | `armv7a-linux-androideabi` |
| `x86_64` | `x86_64-linux-android` |
| `x86_32` | `i686-linux-android` |

**Boost:** built with `toolset=clang-<name>` pointing at the NDK's per-API-level Clang
wrapper (`<NDK>/toolchains/llvm/prebuilt/linux-x86_64/bin/<triple>24-clang++`) via a
`user-config.jam`, `target-os=android`. Unlike Linux/Windows, **only `--with-charconv`**
is built (not the full Boost tree the other platforms build) — the module only links
`libboost_charconv`, and skipping the rest noticeably shortens four separate builds.

```jam
using clang : android_arm64 : /path/to/ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang++ : <archiver>llvm-ar <ranlib>llvm-ranlib ;
```

```bash
./b2 -j"$(nproc)" \
    --user-config=user-config.jam \
    link=static threading=multi runtime-link=static variant=release \
    --stagedir="$(pwd)/stage" \
    toolset=clang-android_arm64 target-os=android \
    address-model=64 architecture=arm \
    cxxstd=17 cxxstd-dialect=iso \
    --with-charconv
```

(Repeat per arch with the matching toolset/address-model/architecture: `arm32` ->
`address-model=32 architecture=arm`; `x86_64` -> `address-model=64 architecture=x86`;
`x86_32` -> `address-model=32 architecture=x86`.)

**OpenSSL:** `./Configure android-arm64|android-arm|android-x86_64|android-x86
-D__ANDROID_API__=24 no-asm no-ssl3 no-weak-ssl-ciphers no-legacy no-shared no-tests
no-docs`, with `ANDROID_NDK_ROOT` set and the NDK's `toolchains/llvm/prebuilt/.../bin`
on the `PATH` (see OpenSSL's own `NOTES-ANDROID.md`).

**`no-asm` is required, not optional.** OpenSSL's hand-written ARM64 assembly for
`poly1305` (`poly1305-armv9-sve2`) addresses a `.globl` symbol from another translation
unit with `adrp`/`add :lo12:` (an absolute-page-relative reference). Android's linker
(`ld.lld`) refuses this when linking a **shared library** (`.so`) — the symbol could be
overridden at runtime — with `relocation R_AARCH64_ADR_PREL_PG_HI21 cannot be used
against symbol 'poly1305_blocks_sve2'; recompile with -fPIC`. `-fPIC` alone does not fix
it, because it is hand-written assembly, not C compiled without `-fPIC`. This never
shows up on Linux/Windows, where the module links into an **executable**, not a shared
library, so the symbol-preemption rule the linker is enforcing does not apply there.
This module uses a blanket `no-asm` instead of patching only that one routine, to avoid
the same class of bug surfacing later in an untested routine (AES, SHA, ChaCha) — it
trades some crypto
performance for a build that will not silently regress on the next OpenSSL update.

```bash
export ANDROID_NDK_ROOT=/path/to/ndk
export PATH=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

./Configure android-arm64 -D__ANDROID_API__=24 no-asm \
    no-ssl3 no-weak-ssl-ciphers no-legacy no-shared no-tests no-docs \
    --prefix="$(pwd)" --openssldir="$(pwd)"
make depend
make -j"$(nproc)"
```

> The same `install_dev`/self-copy issue as Linux/Windows can happen here (see above):
> copy `libssl.a`/`libcrypto.a` to `lib/` manually if `make install` fails there.

`mysql/config.android.cfg` already exists in the repository and points at the per-arch
roots described above.

## Building the module together with Godot

```bash
cd godot
scons platform=linuxbsd arch=x86_64 target=editor \
    custom_modules=../mysql \
    precision=double \
    -j"$(nproc)"
```

Building with `precision=double` is highly recommended. For Windows (cross-compiled),
add `d3d12=no` (avoids requiring the Direct3D 12 SDK, not part of this module) and
`platform=windows`; Godot on Linux, without `use_mingw=1`, already detects and uses
MinGW-w64 automatically when it does not find MSVC.

For Android, build once per architecture — `platform=android` only produces one `.so`
per invocation:

```bash
cd godot
scons platform=android arch=arm64 target=template_debug \
    custom_modules=../mysql \
    -j"$(nproc)"
# repeat with arch=arm32, arch=x86_64, arch=x86_32
```

## Tests

See [tests.md](tests.md): how to run the unit tests, the desktop integration test and
the Android integration test.
