# MySQL Module 

This module is a MySQL connector for Godot.


#### Requirements: 

C++ MySQL Connector Library. 

You can get here https://dev.mysql.com/downloads/connector/cpp/. On linux you can use sudo apt-get install libmysqlcppconn-dev.

   
#### Installation: 

Copy or clone the repository into Godot_path/modules

Rename the folder to mysql.

On linux like systems no trouble found to compile, but in Windows systems there is some issues to compile. 

Check out the SCsub file before you star the compilation to Windows. (You must to edit this file with the path of  Boost libraries.)

If you trying to compile this module to windows, make a copy of the file "mysqlcppconn.lib" and rename it on the same folder to "mysqlcppconn.windows.tools.64.lib" or "mysqlcppconn.windows.tools.32.lib", according to the target of your compilation.(Don't know if it's a bug)

Boost library are need to compile on Windows.

I also had issues to link the libraries but this gonna be different for every system. If you get trouble,  just leave a mensage. 

Should work fine on Mac

- One last thing: Feel free to change, upgrede or add more functions to this module. ;)


#### Methods: 

You can find a exemple project here:
https://github.com/Malkverbena/Mysql_module_exemple



