# MySQL Module to Godot 4.

### **This module is a wrapper for Boost.MySQL.**

Boost.MySQL is a client for MySQL and MariaDB database servers, based on Boost.Asio.
Boost.MySQL is part of Boost.
This module is written in C++17 and, like Godot's default, is built without C++ exceptions (`no_exception`).
Check out the Boost repository: [Boost.MySQL](https://github.com/boostorg/mysql?tab=readme-ov-file).

> **Em reescrita completa nesta branch (`4.x`).** Uma auditoria encontrou problemas
> críticos de memória, TLS e concorrência no código anterior; em vez de corrigi-los no
> lugar, o módulo está sendo reescrito do zero. Veja o plano fase a fase em
> [roadmap.md](roadmap.md) e as decisões de design (com o porquê de cada uma) em
> [design-notes.md](design-notes.md). Neste commit específico o código é um esqueleto
> vazio — [capabilities.md](capabilities.md) descreve o design alvo, não o que já existe
> implementado.

##### This module works only with Godot 4. Minimum supported version: **4.6**.

I have no plans to back port this module to Godot 3.x, but I will accept help from anyone who wants to port it.


##### If you use this module, let me know it. Leave a star ;).

##### Do you have any suggestion? Would you like to share experiences while using the module? Please open a issue.

##### Old version note:

Version 1.0 uses C++ MySQL Connector Library from [Oracle](https://dev.mysql.com/doc/connector-cpp/8.3/en/). You can find it here: [Godot MySQL 2.0](https://github.com/Malkverbena/mysql/releases/tag/V2.0).


## Supported platforms

Final target: Linux, Windows, macOS, Android, iOS. During this rewrite, development and
testing happen only on **Linux x86_64** — the other platforms are ported afterwards
(see [roadmap.md](roadmap.md), Fase 7).

## Distribution

Custom C++ module, built together with the engine (`custom_modules=`). GDExtension
support is a future direction, out of scope for this rewrite (see
[design-notes.md](design-notes.md)).

### [See the full list of features here.](capabilities.md)

### [Compilation instructions here!](compilation.md)

### [Rewrite roadmap.](roadmap.md)

### [Design notes and decisions.](design-notes.md)

This module does not bundle nor build its dependencies. You need to
compile Boost and OpenSSL yourself before compiling the module — see
[compilation.md](compilation.md) for the exact steps and flags.


## Usage:

* **[Documentation.](https://github.com/Malkverbena/mysql/wiki)**
* **[Check out some exemples here.](https://github.com/Malkverbena/mysql/wiki)**


## License

MIT — see [LICENSE](../LICENSE).


# Disclaimer

> THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
> HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
