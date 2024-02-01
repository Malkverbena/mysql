# Instructions for compilation:


This module has two dependencies, Openssl and Boost.
The dependencies will be automatically compiled during engine compilation.

You do not need to compile dependencies every time you compile the engine, but you must recompile dependencies when updating Boost and/or Openssl or when you are compiling the module for some platform that you haven't compiled before.

You can use the "recompile_sql" option when invoking scons to compile openssl and Boost or not.

* All: It will compile BOTH Boost and Openssl.
* openssl: It will only compile Openssl.
* boost: It will only compile Boost.
* none: It will **NOT** compile either Open or Boost.


## Requirements:

- A C++20 capable compiler as GCC, Clang (x11) , Visual C++ (Windows) or Apple Clang (Apple).
- [**NASM - Only for Windows**](https://www.nasm.us/pub/nasm/releasebuilds/)
- Git
- All requirements to compile Godot.


## Compilation:

It is necessary recompile the engine together with this module.

It is highly recommended to compile the engine with the "*precision=double*" option.

Clone this repository inside the godot modules folder or an external folder with the following command to checkout all the dependencies: Boost and OpenSSL.

```
git clone --recurse-submodules git@github.com:Malkverbena/mysql.git
```

If you already checked out the branch use the following commands to update the dependencies:

```
$ git submodule update --init --recursive
```

In case you use an external folder,  you must use "*custom_modules*" when invoke Scons.

```
custom_modules=../path/to/mysql/folder
```

compile the engine normally.

You can use the command belloww to update the module the submodules at once.

```
git pull --recurse-submodules
```
