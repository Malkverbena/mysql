A classe Mysql vai abrigar coneções

As coneções vão ser objetos em C++ não expostos. As coneções serão separadas da classe MySql para diminuir o tamanho da classe quando instaciada.

* As conexões pode ser mantidas vivas.
* Toda interação com as conexões serão feitas pela instancia unica do MySql
* As coneções podem ser sincronas ou assincronas
* ConeçÕes podem ser do tipo UNIX/TCP e com ou sem SLL
* 

ssl

* set_certificate_authority  - CA -  (boost::asio::buffer(CA_PEM))



set_certificate(const String p_certificate, String host_name = "mysql") - Define um CA  you need to call "add_certificate_authority"
