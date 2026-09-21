// SPDX-License-Identifier: MIT
/* mysql_config.h */
#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/type_info.h"

#include <string>

// MySQLConfig — configuração de conexão: credenciais, transport_mode, tinyint1_mode,
// json_result_mode, allow_sql_script_execution, allow_multi_queries. Pensada pra ser
// imutável depois do primeiro connect() (convenção de uso, não travada em C++ — quem
// monta a conexão, MySQLSession/MySQLPool na Fase 2+, não deve reconfigurar um
// MySQLConfig já em uso). Compartilhada por referência entre todas as MySQLConnection
// nascidas dela, inclusive dentro de um MySQLPool.
//
// Sem get_password(): a senha nunca é devolvida ao GDScript (resolve S6 da auditoria —
// a versão anterior do módulo expunha a senha em texto puro). Fica só em
// get_password_std(), de uso interno (Fase 2), nunca vinculada via ClassDB.
//
// Ver documentation/design-notes.md e documentation/roadmap.md (Fases 2-5).
class MySQLConfig : public RefCounted {
	GDCLASS(MySQLConfig, RefCounted);

public:
	enum TransportMode {
		TCP_TLS_DISABLED,
		TCP_TLS_PREFERRED,
		TCP_TLS_REQUIRED,
		UNIX_SOCKET,
	};

	enum JsonResultMode {
		RAW_STRING,
		PARSED_VARIANT,
		LAZY_PARSED_VARIANT,
	};

private:
	String host = "127.0.0.1";
	int port = 3306;
	String unix_socket_path;
	String user;
	std::string password; // Nunca exposta de volta ao GDScript — ver S6 da auditoria.
	String database;

	TransportMode transport_mode = TCP_TLS_REQUIRED;
	bool tinyint1_mode = false;
	JsonResultMode json_result_mode = LAZY_PARSED_VARIANT;
	bool allow_sql_script_execution = false;
	bool allow_multi_queries = false;

	// Timeout de operações assíncronas (Fase 5, resolve parte do S9). 0 = sem timeout.
	int async_timeout_ms = 30000;
	// Limite de tamanho de pacote/linha (S9); o Boost.MySQL já vem com um padrão de
	// 64MB, aqui só o expomos como opção em vez de ficar embutido sem controle.
	int max_buffer_size = 0x4000000;

	void _wipe_password();

protected:
	static void _bind_methods();

public:
	void set_host(const String &p_host);
	String get_host() const;

	void set_port(int p_port);
	int get_port() const;

	void set_unix_socket_path(const String &p_path);
	String get_unix_socket_path() const;

	void set_user(const String &p_user);
	String get_user() const;

	void set_password(const String &p_password);
	// Uso interno (MySQLConnection, Fase 2) — não é bind_method, não vaza pro GDScript.
	const std::string &get_password_std() const;

	void set_database(const String &p_database);
	String get_database() const;

	void set_transport_mode(TransportMode p_mode);
	TransportMode get_transport_mode() const;

	void set_tinyint1_mode(bool p_enabled);
	bool get_tinyint1_mode() const;

	void set_json_result_mode(JsonResultMode p_mode);
	JsonResultMode get_json_result_mode() const;

	void set_allow_sql_script_execution(bool p_allowed);
	bool get_allow_sql_script_execution() const;

	void set_allow_multi_queries(bool p_allowed);
	bool get_allow_multi_queries() const;

	void set_async_timeout_ms(int p_timeout_ms);
	int get_async_timeout_ms() const { return async_timeout_ms; }

	void set_max_buffer_size(int p_size);
	int get_max_buffer_size() const { return max_buffer_size; }

	MySQLConfig() = default;
	~MySQLConfig();
};

VARIANT_ENUM_CAST(MySQLConfig::TransportMode);
VARIANT_ENUM_CAST(MySQLConfig::JsonResultMode);
