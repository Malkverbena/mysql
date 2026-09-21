# Testes

Testes locais de integração do módulo, versionados no repositório (Fase 6 do roadmap
de reescrita). Não são testes unitários isolados — cada um roda contra um
MySQL/MariaDB de verdade, de ponta a ponta.

## Pré-requisitos

- O módulo já compilado junto com o Godot (ver
  [`../documentation/compilation.md`](../documentation/compilation.md)).
- Um servidor MySQL ou MariaDB acessível, com um usuário e um schema **dedicados a
  teste** — os testes criam e derrubam suas próprias tabelas nesse schema, mas não
  tocam em mais nada do banco. Não use um schema de produção.

## `smoke_test.gd`

Cobre o caminho ponta a ponta de cada classe exposta pelo módulo: conexão e validação
de TLS, os três jeitos de executar SQL (`execute_text`/`execute_formatted`/
`execute_prepared`), tipos de dado (incluindo os casos difíceis — `BIGINT UNSIGNED`
acima de `INT64_MAX`, `TIME` acima de 24h com microssegundos, `json_result_mode`),
transações (`commit`/`rollback`/rollback automático), streaming, assíncrono
(`async_execute_text` + `await`), pool de conexões e `execute_script`.

**Nunca coloque credenciais neste arquivo nem em nenhum outro arquivo versionado** —
elas vêm só de variáveis de ambiente, lidas em tempo de execução:

| Variável | Obrigatória | Padrão |
|---|---|---|
| `MYSQL_TEST_HOST` | Não | `127.0.0.1` |
| `MYSQL_TEST_PORT` | Não | `3306` |
| `MYSQL_TEST_USER` | Sim | — |
| `MYSQL_TEST_PASSWORD` | Sim | — |
| `MYSQL_TEST_DATABASE` | Sim | — |

### Rodando

A partir da árvore do Godot já compilada com `custom_modules=../mysql`:

```bash
MYSQL_TEST_HOST=127.0.0.1 \
MYSQL_TEST_PORT=3306 \
MYSQL_TEST_USER=usuario_de_teste \
MYSQL_TEST_PASSWORD='sua_senha_aqui' \
MYSQL_TEST_DATABASE=schema_de_teste \
./bin/godot.<seu_binário> --headless --script ../mysql/tests/smoke_test.gd
```

Sai com código `0` se todas as checagens passarem, `1` se alguma falhar (ou se as
variáveis de ambiente obrigatórias não estiverem definidas). Imprime `OK`/`FAIL` por
checagem e um total no final.

O teste é seguro de rodar mais de uma vez: a tabela (`t_mysql_module_smoke_test`) é
derrubada no início (se já existir) e no fim.

### Se o servidor usa TLS com certificado autoassinado

Isso é esperado e faz parte do teste: a seção 0 confirma que `TCP_TLS_REQUIRED` rejeita
um certificado não confiável (em vez de aceitar em silêncio). O resto do teste conecta
com `TCP_TLS_DISABLED` de propósito, assumindo uma rede local/de teste confiável — não
é o padrão recomendado para produção (ver [`../documentation/capabilities.md`](../documentation/capabilities.md)).
