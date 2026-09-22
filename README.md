# MySQL Module for Godot 4

A custom C++ module that wraps [Boost.MySQL](https://github.com/boostorg/mysql) — a
MySQL/MariaDB client built on Boost.Asio — so Godot 4 projects can talk to a
MySQL/MariaDB server directly, without a separate backend process.

Written in C++17 and, like Godot's own default, built without C++ exceptions
(`no_exception`); errors are reported through explicit return values, never thrown.

> **This module is for a headless Godot server or an internal tool, never for a game
> shipped to players.** It opens a real network connection to a MySQL/MariaDB server;
> anyone who can reach an exported game can also reach whatever that connection can
> reach. See "Intended use" in [documentation/features.md](documentation/features.md)
> before using it in anything a player runs.

> **Being fully rewritten on this branch (`4.x`).** An audit found critical memory, TLS
> and concurrency problems in the previous code. Instead of patching them in place, the
> module is being rewritten from scratch, phase by phase.
> [documentation/features.md](documentation/features.md) describes the target design;
> check the module's commit history for the exact state of the implementation at any
> point.

Minimum supported Godot version: **4.6**. Supported platforms: Linux, Windows, macOS,
Android (see [documentation/features.md](documentation/features.md) for the status of
each). iOS is not on the list for now — not a technical decision, just deferred until
Apple hardware is available to build and test it.

## Where to go next

| Document | Content |
|---|---|
| [documentation/features.md](documentation/features.md) | Everything the module does: connection, methods, limits, error model, data types, platform status, with diagrams of the module's structure. |
| [documentation/instructions.md](documentation/instructions.md) | How to configure, compile and test the module together with Godot, per platform. |
| [documentation/usage.md](documentation/usage.md) | How to use the module from GDScript: class overview and worked examples. |
| [doc_classes/](doc_classes/) | The reference used by Godot's own built-in help (`F1` in the editor), one XML file per class. |

## Old version note

Version 1.0 used the C++ MySQL Connector Library from
[Oracle](https://dev.mysql.com/doc/connector-cpp/8.3/en/):
[Godot MySQL 2.0](https://github.com/Malkverbena/mysql/releases/tag/V2.0). There are no
plans to back-port this rewrite to Godot 3.x, but help from anyone who wants to port it
is welcome.

## Contributing / feedback

If you use this module, a star is appreciated. Suggestions, shared experiences and bug
reports are all welcome as issues.

## License

MIT — see [LICENSE](LICENSE).

> THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
