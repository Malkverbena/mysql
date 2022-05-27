
# MySQL Module 

## This module is an wrapper of the Mysql C++ connector for Godot. 

### If you use this module, let me know it. Leave a star. 

#### Supported platforms:
* MacOS
* X11/Unix
* Windows
* javascript (soon)


#### Requirements: 

* **C++ MySQL Connector Library**.

    On Unix/x11 you can use the command: sudo apt install libmysqlcppconn-dev.

     https://dev.mysql.com/downloads/connector/cpp/
 

* **Boost library**.

    It is usually installed by default on Unix systems.

     https://www.boost.org/users/download/
 


#### Installation: 
**On linux like systems there are no trouble found to compile and no changes are needed. 
On Windows systems there is some issues to compile. You need edit the SCsub with the path to the libraries. 
Not tested on macOS.**


Step by step:
* Install Boost library if you haven't it.
* Install the MySQL C++ Connector.
* Copy or clone the the folder mysql into Godot/path/modules.
* On Winndows you need edit the SCsub file before you start the compilation. You have to SCsub file with the path of Boost libraries and MySQL connector. (Instructions on file).
* Compile the module with the engine.
  More info on: https://docs.godotengine.org/en/stable/development/cpp/custom_modules_in_cpp.html


#### Usage:
  There is some exemples on exemple folders.

It provides a
comprehen

# Disclaimer

> THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


