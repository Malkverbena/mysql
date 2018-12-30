# MySQL Module 

### This module is a MySQL connector for Godot.
### If you use this module, let me know it. Leave a star. 


#### Requirements: 

C++ MySQL Connector Library. 

You can get here https://dev.mysql.com/downloads/connector/cpp/. 
On linux you can use sudo apt-get install libmysqlcppconn-dev.

   
#### Installation: 

Copy or clone the repository into Godot_path/modules

Rename the folder to mysql.

On linux like systems there are no trouble found to compile, but in Windows systems there is some issues to compile. 

Edit the SCsub file before you start the compilation to Windows. (You must edit this file with the path of Boost libraries.)

#### Compilling on windows:

Install Boost library. You get it on:
https://dl.bintray.com/boostorg/master/


Install the MySQL C++ Connector. You can get it here:
https://www.mysql.com/products/connector/


- Find the file "mysqlcppconn.lib" on connector's folder.
- Make a copy of this file and rename the copy to "mysqlcppconn.windows.tools.64.lib" or "mysqlcppconn.windows.tools.32.lib",   according to the target of your compilation system. 64 to 64bits systems and 32 to 32bits system.
- Copy this file to the folder of the mysql module and the folder "bin", inside godot's folder. 


######        YOU MUST TO EDIT THE SCsub FILE!!
- Open the SCsub file on MySQL module folder.
- On this file you must to set the paths to Boost libraly and the connector files. (on SCsub file is there an exemple).



#### Should work fine on Mac

- One last thing: Feel free to change, upgrede or add more functions to this module. ;)


#### Methods: 

You can find the usage exemple on file mysql_exemple.gd



