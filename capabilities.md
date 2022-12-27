## Capabilities:

### Connection:
  - Set/Get Properties
  - SSL/TLS connections
  - Map of properties 

### Simple text queries:
  - Query
  - Execute
  - Update
  - Transactions & Save Points

### Prepared Statement for:
  - Query
  - Execute
  - Update
  - Transactions & Save Points


### Supported sql data types:
####  TYPE         GODOT             SQL -> sql::DataType::

  NULL   =>    null      =>   NILL - res->isNull
  
  BOOL   =>    bool      =>   BIT
  
  INT32  =>  Integer 32  =>   TINYINT - SMALLINT - MEDIUMINT
  
  INT64  =>  Integer 64  =>   INTEGER - BIGINT
  
  FLOAT  =>    float     =>   REAL - DOUBLE -  DECIMAL - NUMERIC
  
  TIME   =>    Array     =>   DATE - TIME - TIMESTAMP - YEAR
  
  CHAR   =>    String    =>   ENUM - CHAR - VARCHAR - LONGVARCHAR
  
  JSON   =>  JsonString  =>   JSON/TEXT
  
  BINARY =>   ByteArray  =>   BINARY - VARBINARY - LONGVARBINARY




