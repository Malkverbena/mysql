# MySQL Module 

## This module is an wrapper of the Mysql C++ connector for Godot. 

### If you use this module, let me know it. Leave a star. 

#### Supported operating systems:
MacOS

Linux

Windows

#### Requirements: 

C++ MySQL Connector Library. 

You can get here https://dev.mysql.com/downloads/connector/cpp/. 

On linux you can use the command: sudo apt-get install libmysqlcppconn-dev.

This wrapper works online and is also compatible with MariaDB.
   
#### Installation: 

Copy or clone the repository into Godot_path/modules

Rename the folder to MySQL.

On linux like systems there are no trouble found to compile, but in Windows systems there is some issues to compile. 

Edit the SCsub file before you start the compilation on Windows. (You must edit this file with the path of Boost libraries.)

#### Compilling on windows:

Install Boost library. You get it on:
https://dl.bintray.com/boostorg/master/


Install the MySQL C++ Connector. You can get it here:
https://www.mysql.com/products/connector/

There is a video in Portuguese teaching how to compile on the Windows platform.
English subtitle can be activated anyway.
https://www.youtube.com/watch?v=ohGP-q_Na9g - A special thanks to Andrius Salgado for!! (github.com/OnelinkGames) 


- Find the file "mysqlcppconn.lib" on connector's folder.
- Make a copy of this file and rename the copy to "mysqlcppconn.windows.tools.64.lib" or "mysqlcppconn.windows.tools.32.lib",   according to the target of your compilation system. 64 to 64bits systems and 32 to 32bits system.
- Copy this file to the folder of the mysql module and the folder "bin", inside godot's folder. 
- Copy all .dll files lib64 or lib32(according with your system) inside the folder "bin" or add the directory of files to       PATH environment variable in Windows.


######        YOU MUST EDIT THE SCsub FILE!!
- Open the SCsub file on MySQL module folder.
- On this file you must to set the paths to Boost libraly and the connector files. (on SCsub file is there an exemple).
- You must comment this line below to enable support for Godot 3 or leave it the way it is to be able to compile for Godot 4:

- mysql_env.Append(CPPDEFINES=["GODOT4"])

#### Should work fine on Desktop systems like macOS, Linux, Windows and Solaris. This module is a wrapper, as until the time I wrote these lines there are no connector for Android and IOS available, DO NOT expect this to work on mobile systems. 

- One last thing: Feel free to change, upgrede or add more functions to this module. ;)

####  Documentation
Here are the instructions to add it to the engine.
https://docs.godotengine.org/en/3.1/development/cpp/custom_modules_in_cpp.html#writing-custom-documentation


#### Methods: 

You can find the usage exemple on file mysql_exemple.gd

# Disclaimer

> THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


