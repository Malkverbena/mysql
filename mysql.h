/* mysql.h */

#ifndef MYSQL_H
#define MYSQL_H


#include "core/object/ref_counted.h"
#include "core/core_bind.h"


class MySQL : public RefCounted{
	GDCLASS(MySQL, RefCounted);



private:


protected:
	static void _bind_methods();


public:

	MySQL();
	~MySQL();

};



//VARIANT_ENUM_CAST(mysql::ENUM);


#endif  // MYSQL_H



