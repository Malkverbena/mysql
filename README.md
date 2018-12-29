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
https://www.youtube.com/redirect?q=https%3A%2F%2Fdl.bintray.com%2Fboostorg%2Frelease%2F1.68.0%2Fsource%2Fboost_1_68_0.7z&event=video_description&redir_token=sqvPLY2sp-4FD8QVGWpNsyQ6bgt8MTU0NjE4ODUzOEAxNTQ2MTAyMTM4&v=dmbh1jEEnyw


Install the MySQL C++ Connector. You can get it here:
https://www.youtube.com/redirect?q=https%3A%2F%2Fwww.mysql.com%2Fproducts%2Fconnector%2F&event=video_description&redir_token=sqvPLY2sp-4FD8QVGWpNsyQ6bgt8MTU0NjE4ODUzOEAxNTQ2MTAyMTM4&v=dmbh1jEEnyw

- Find the file "mysqlcppconn.lib" on connector's folder.
- Make a copy of this file and rename it to "mysqlcppconn.windows.tools.64.lib" or "mysqlcppconn.windows.tools.32.lib",         according to the target of your compilation system. 64 to 64bits systems and 32 to 32bits system.
- Copy this file to the folder "bin", inside godot's folder. 

###### YOU MUST TO EDIT THE SCsub FILE!!
- Open the SCsub file on MySQL module folder.
- On this file you must to set the paths to Boost libraly and the connector files. (on SCsub file is there an exemple).



#### Should work fine on Mac

- One last thing: Feel free to change, upgrede or add more functions to this module. ;)


#### Methods: 

You can find the usage exemple on file mysql_exemple.gd



