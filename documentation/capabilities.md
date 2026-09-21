# Capacidades

> **Design alvo da reescrita em andamento.** Este documento descreve o que o módulo faz
> quando a reescrita estiver pronta, fase a fase (ver `roadmap.md`). Neste commit o
> código é um esqueleto vazio — nada aqui está implementado ainda. As decisões que
> embasam esta lista, com o raciocínio por trás de cada uma, estão em `design-notes.md`.

## Bancos suportados

MySQL e MariaDB. Recursos que só existem num dos dois (plugins de autenticação
específicos, JSON nativo vs. `LONGTEXT` com `CHECK` no MariaDB, listas de collation
diferentes) são documentados como exceção quando aparecerem, não assumidos como
universais.

## Conexão

* Transportes (`transport_mode`): `TCP_TLS_DISABLED`, `TCP_TLS_PREFERRED`,
  `TCP_TLS_REQUIRED` (padrão), `UNIX_SOCKET`. Não existe UNIX+TLS — socket UNIX é local
  por natureza e nunca usa TLS.
* TLS com validação de certificado ligada por padrão quando em uso; hostname de
  verificação derivado do endpoint real da conexão.
* Autenticação: `mysql_native_password` e `caching_sha2_password`.
* Toda configuração que reduz segurança (TLS desligado, multi-queries ligado, etc.)
  emite warning no momento em que é definida.

## Execução

* Texto bruto, texto formatado (`with_params`/`format_sql`, sem escaping próprio) e
  prepared statements, com cache LRU por sessão.
* Multi-function operations e stored procedures.
* Scripts SQL e multi-queries **desligados por padrão**, independentes entre si:
  `allow_sql_script_execution` (API manual de scripts) e `allow_multi_queries`
  (capacidade negociada com o servidor).
* **Streaming:** leitura incremental de resultados grandes (`MySQLStreamingCursor`), sem
  carregar tudo em memória de uma vez.
* Assíncrono real: não bloqueia a thread chamadora (diferente de versões anteriores do
  módulo); cada chamada `async_*` devolve um `MySQLAsyncOperation` usável com `await`.
* Transações (`MySQLTransaction`) e pool de conexões (`MySQLPool`), com suporte a
  multithread — cada thread usa sua própria `MySQLSession`, nunca uma `Connection`
  compartilhada entre threads ao mesmo tempo.

## Modelo de erro

Sem exceções em nenhuma camada (`no_exception`, como o padrão do Godot). Toda operação
fallível expõe `is_ok()` e `get_error() -> Dictionary`, com as chaves `category`,
`message`, `server_message` e `is_fatal`. Não há estado de erro global nem por instância
— cada chamada carrega o próprio resultado.

## Tipos de dados

| Tipo MySQL/MariaDB | Tipo Godot | Observação |
|---|---|---|
| `NULL` | `null` | |
| `TINYINT(1)` | `bool` | Só quando a largura de exibição da coluna é 1; outras larguras de `TINYINT` viram `int` |
| `TINYINT`, `SMALLINT`, `MEDIUMINT`, `INT`, `BIGINT` (assinados) | `int` | |
| `BIGINT UNSIGNED` até `INT64_MAX` | `int` | |
| `BIGINT UNSIGNED` acima de `INT64_MAX` | `String` | ⚠️ **Ver aviso abaixo** |
| `FLOAT`, `DOUBLE` | `float` | |
| `BINARY`, `VARBINARY`, `BLOB`, `GEOMETRY` | `PackedByteArray` | |
| `CHAR`, `VARCHAR`, `TEXT`, `ENUM`, `DECIMAL`, `NUMERIC` | `String` | Sempre `utf8mb4` |
| `JSON` | conforme `json_result_mode` | ver abaixo |
| `DATE`, `TIME`, `DATETIME`, `TIMESTAMP` | `Dictionary` | Com microssegundos e `TIME` negativo corretos |
| `SET` | `PackedStringArray` | |

> ⚠️ **`BIGINT UNSIGNED` acima de `INT64_MAX` (9223372036854775807) vem como `String`,
> não `int`.** `Variant::INT` do Godot é assinado de 64 bits e não cabe o valor exato
> nesses casos. Isso significa que **a mesma coluna pode devolver `int` na maioria das
> linhas e `String` só nas linhas com valor grande** — sempre confira o tipo
> (`typeof(valor) == TYPE_STRING`) antes de fazer conta com um campo `BIGINT UNSIGNED`.

### `json_result_mode`

* `RAW_STRING`: devolve o texto exatamente como veio do servidor.
* `PARSED_VARIANT`: converte imediatamente para `Dictionary`/`Array`/`Variant`, via
  classe `JSON` do Godot.
* `LAZY_PARSED_VARIANT` (**padrão**): guarda a string e só converte quando pedido; o
  resultado convertido pode ficar em cache.

## Plataformas

Alvo final: Linux, Windows, macOS, Android, iOS. Durante esta reescrita, desenvolvimento
e testes acontecem só em **Linux x86_64**; as demais entram na Fase 7 do `roadmap.md`,
depois do módulo funcionar por completo em Linux.

## Distribuição

Módulo customizado em C++, compilado junto com a engine (`custom_modules=`). Suporte a
GDExtension é uma direção futura — não faz parte desta reescrita.

## Godot

Versão mínima suportada: **4.6**.
