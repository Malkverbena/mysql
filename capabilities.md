## Capabilities:

### Connection:

* TCP and UNIX socket connections.
* Encrypted connections (TLS). For both, TCP and UNIX socket connections.
* Authentication methods: mysql_native_password and caching_sha2_password.

### **Methods:**

* Supports asynchronous methods using C++20 coroutines.
* Supports Multi-function operations.
* Stored procedures. It can be used within Multi-function operations.
* Text querie:  MySQL refers to this as the "text protocol", as all information is passed using text (as opposed to prepared statements).
* Prepared statements: MySQL refers to this as the "binary protocol", as the result of executing a prepared statement is sent in binary format rather than in text.

### **EQUIVALENT DATA TYPES:**

| DATA TYPE |  GODOT DATA TYPE  |                                                              C++ DATA TYPE                                                              |                                  MYSQL DATA TYPE                                  |
| :-------: | :---------------: | :--------------------------------------------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------: |
|   NULL   |       null       |                                                              std::nullptr_t                                                              |                                        NILL                                        |
|   BOOL   |       bool       |                                                                   bool                                                                   |                                       TINYINT                                       |
|   INT32   |    integer 32    |                                                 signed char, short, int, long, long long                                                 |                  SIGNED TINYINT, SMALLINT, MEDIUMINT, INT, BIGINT                  |
|   INT64   |    integer 64    |                           unsigned char, unsigned short, unsigned,<br />int, unsigned long, unsigned long long                           | UNSIGNED BIGINT, UNSIGNED TINYINT, SMALLINT,<br />MEDIUMINT, INT, BIGINT, YEAR, BIT |
|   FLOAT   |       float       |                                                                  float                                                                  |                                        FLOAT                                        |
|  DOUBLE  |       float       |                                                                  double                                                                  |                                       DOUBLE                                       |
|  BINARY  |  PackedByteArray  |                                               std::basic_vector<unsigned char, Allocator>                                               |                    BINARY, VARBINARY, BLOB (all sizes), GEOMETRY                    |
|   CHAR   |      String      | std::basic_string<char, std::char_traits `<char>`, Allocator> (including std::string),<br />string_view, std::string_view, const char* |         CHAR, VARCHAR, TEXT(all sizes),<br />ENUM, JSON,  DECIMAL, NUMERIC         |
|   DATE   |    Dictionary    |                          boost::mysql::date AKA std::chrono::time_point<br /><std::chrono::system_clock, days>                          |                                        DATE                                        |
|   TIME   |    Dictionary    |                                            boost::mysql::time  AKA std::chrono::microseconds                                            |                                        TIME                                        |
| DATETIME |    Dictionary    |   boost::mysql::datetime AKA std::chrono::time_point<br /><std::chrono::system_clock, std::chrono::duration<std::int64_t, std::micro>>   |                                 DATETIME, TIMESTAMP                                 |
|   CHAR   | PackedStringArray |      std::basic_string<char, std::char_traits, Allocator> (including std::string),<br />string_view, std::string_view, const char*      |                                         SET                                         |
