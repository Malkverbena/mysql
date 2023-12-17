
# MySQL Module to Godot 4

### This module was built using Boost.MySQL.
**Check out the Boost repository:** [**Boost.MySQL**](https://github.com/boostorg/mysql?tab=readme-ov-file)

### This module works only with Godot 4.
### If you use this module, let me know it. Leave a star. 


## Features:
- Supports TCP and UNIX SOCKET connections.
- Supports TLS
- Supports asynchronous queries.
- Supports multi queries.


## Supported platforms: (Under development).
* MacOS - soon (need help).
* X11/Unix - dev.
* Windows - dev.
* javascript - need help.
* O̶S̶X̶ - possibly (need help).
* A̶n̶d̶r̶o̶i̶d̶ - possibly.


## Requirements: 
- A C++20 capable compiler as GCC, Clang (x11) , Visual C++ (Windows) or Apple Clang (Apple).
- Git
- All requirements to compile Godot.


## Installation: 

Clone this repository with the following command to checkout all the dependencies: Boost and OpenSSL.

```
$ git clone --recurse-submodules git@github.com:Malkverbena/mysql.git 
```

If you already checked out the branch use the following commands to update the dependencies:

```
$ git submodule update --init --recursive
```


## Compilation:
For now, it is necessary to compile the entire mechanism.

In the future there will be a version for GDExtension.

It is highly recommended to compile the engine with the "precision=double" option.

Clone this repository inside the godot modules folder and compile the engine normally.


## Usage:
**See the docs here** [**Docs**](https://github.com/Malkverbena/mysql/tree/Docs-%26-Exemples-3.0)


# Disclaimer

> THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

