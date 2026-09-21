# Notas de design — módulo MySQL

Documento vivo para reunir decisões e ideias de design da reescrita, à medida que forem
surgindo em conversa. Complementa o `roadmap.md`: o roadmap é o plano de fases, aqui é
onde os detalhes de API e comportamento vão sendo fechados antes de virar código.

Cada entrada tem data, status e a fase do `roadmap.md` a que se aplica.

Status possíveis: **Decidido**, **Em aberto** (falta sua resposta), **Ideia a avaliar**
(ainda não é decisão, só está registrada para não se perder).

## 2026-09-21

### Flags de execução perigosa: `allow_sql_script_execution` e `allow_multi_queries`

Status: Decidido. Fase 4.

Ambas `false` por padrão. São independentes uma da outra:

- `allow_sql_script_execution` controla a API manual de execução de scripts SQL
  (conteúdo com múltiplos comandos, passado pelo usuário do módulo).
- `allow_multi_queries` controla a capacidade multi-statement negociada com o servidor
  na conexão (equivalente a `CLIENT_MULTI_STATEMENTS`).

Relaciona: S7, S8 (superfície de multi-query), R-17 do `sugestoes.docx`.

### Modos de transporte: `transport_mode`

Status: Decidido. Fase 2.

Valores:

- `TCP_TLS_DISABLED`
- `TCP_TLS_PREFERRED`
- `TCP_TLS_REQUIRED` (padrão)
- `UNIX_SOCKET`

Sem combinação UNIX+TLS: socket UNIX nunca usa TLS (é local por natureza), então essa
combinação não é oferecida — não é uma capacidade perdida, nunca fez sentido. Corrige a
alegação incorreta que existe hoje em `capabilities.md`.

Relaciona: S5, A3.

### Modo de resultado JSON: `json_result_mode`

Status: Decidido. Fase 3.

Usa a classe `JSON` do Godot para converter, com três modos:

- `RAW_STRING`: devolve o texto exatamente como veio do servidor. O mais fiel.
- `PARSED_VARIANT`: converte imediatamente para `Dictionary`/`Array`/`Variant`.
- `LAZY_PARSED_VARIANT` (**padrão**): guarda a string e só converte quando o usuário
  pedir; o resultado convertido pode ficar em cache para não reconverter à toa.

Relaciona: item "JSON configurável" da Fase 3 do roadmap.

### Warning em qualquer configuração insegura

Status: Decidido. Transversal — vale a partir da Fase 2.

Toda configuração que reduz segurança emite warning no Godot **no momento em que é
definida**, não só quando é usada. Exemplos: `transport_mode = TCP_TLS_DISABLED`,
`allow_multi_queries = true`, `allow_sql_script_execution = true`, certificado TLS não
verificado.

Relaciona: S5, S14, R-11.

### Documentação em Markdown + Mermaid

Status: Decidido. Fase 6.

A documentação do módulo (guias, não só o roadmap) pode usar diagramas Mermaid dentro do
Markdown, como já é feito em `roadmap.md`.

### `BIGINT UNSIGNED` acima de `INT64_MAX`: devolve como `String`

Status: Decidido. Fase 3. Resolve D3 (parte de precisão de 64 bits).

`Variant::INT` do Godot é `int64_t` assinado — não cabe um `BIGINT UNSIGNED` acima de
`9223372036854775807`. Decisão: valores nessa faixa vêm como `String` (valor exato,
decimal); valores dentro da faixa de `int64` continuam como `int`, normalmente.

⚠️ **Isso precisa ficar bem destacado na documentação final (Fase 6), não só citado de
passagem** — é uma pegadinha real: a mesma coluna `BIGINT UNSIGNED` pode devolver `int`
na maioria das linhas e `String` só na(s) linha(s) com valor grande, então todo script
que lê essa coluna e faz conta com o valor precisa checar o tipo primeiro (ex.:
`typeof(valor) == TYPE_STRING`) antes de assumir `int`. Colocar isso como aviso/callout
separado na tabela de tipos do `capabilities.md` reescrito, não só como uma linha a mais
na tabela.

### Formato de distribuição: módulo customizado, com GDExtension como direção futura

Status: Decidido. Observação geral — não é de nenhuma fase específica do roadmap ainda.

O módulo continua sendo um módulo customizado em C++, compilado junto com a engine
(dentro de `custom_modules=`) — isso não muda. Além disso, deve **poder** ser compilado
como GDExtension no futuro. Por enquanto, só compilamos junto com a engine; suporte a
GDExtension não entra nas Fases 0–7 deste roadmap.

Implicação prática desde já: evitar acoplamento desnecessário a APIs internas do Godot
que não existem via GDExtension (ex.: headers de `core/` usados hoje em
`scr/throw_exception.cpp`, como `core/error/error_macros.h`), sempre que isso não custar
retrabalho agora. Não é tarefa de uma fase — é um cuidado a ter durante as Fases 0–6 para
não fechar a porta do GDExtension. A migração de fato fica para quando for decidido
priorizar.

### Confirmações gerais de padrões e políticas

Status: Decidido/reafirmado. Reunindo numa entrada só porque a maioria já valia — isto
é um checkpoint, não decisões novas isoladas.

- **`no_exception`, sempre `error_code`/`diagnostics`, nunca `try`/`catch`.** Já era a
  base desde a Fase 0/1 e do modelo de erro (`is_ok()`/`get_error()` em `Dictionary`).
  Sem mudança, só reafirmado.
- **`constexpr` e `nullptr` são permitidos/encorajados.** Novo, mas sem conflito com
  nada — complementa a regra já existente de C++17 moderno na camada interna (STL só aí,
  ver abaixo).
- **Documentação em Markdown com suporte a Mermaid.** Já decidido antes nesta lista.
- **Licença MIT.** Já decidido (auditoria, seção 3) e já no plano da Fase 0 (`LICENSE` +
  cabeçalho SPDX).
- **Godot mínimo: 4.6.** Já decidido nesta sessão (ver `roadmap.md`, seção 0).
- **STL só na camada interna que conversa com o Boost; API pública usa tipos do Godot.**
  Já era a recomendação D-6 do `sugestoes.docx`; agora confirmado como decisão, não só
  recomendação.
- **Nunca alterar arquivos da engine (`godot/`); só os do módulo (`mysql/`).** Já era
  regra desde a resposta a `questions.txt` (mesma regra vale pro `Harness/`). Reafirmado
  aqui especificamente para o trabalho de reescrita.
- **Sem automatizar a compilação do Boost/OpenSSL; sem submódulos; usuário clona e
  compila por conta própria, com instruções na documentação.** ⚠️ Isto **já está feito**
  desde o commit `9a12f412` ("Remove submodules") — confirmei agora e não há
  `.gitmodules` nem pasta `thirdparty/` rastreada dentro de `mysql/`. Fica reafirmado
  como política permanente daqui pra frente: não reintroduzir automação de build de
  dependência (é também por isso que `fix.sh`, que automatiza isso e mais, está marcado
  pra remoção na Fase 0 — R-8).

Duas confirmações que valem um destaque à parte, porque têm consequência prática:

**MySQL e MariaDB como bancos suportados.** Isso já aparecia de raspão como item de
teste da Fase 6 (`R-31`, "MySQL e MariaDB reais"), mas agora é um alvo explícito, não só
um item de qualificação. Implicação: evitar depender de recurso que só existe num dos
dois sem documentar a exceção — exemplos conhecidos a verificar durante a Fase 3/4: JSON
nativo (MariaDB trata `JSON` como alias de `LONGTEXT` com `CHECK`, não é um tipo à
parte), lista de collations (não são idênticas entre os dois), plugins de autenticação
(`caching_sha2_password` é do MySQL; MariaDB tem os seus próprios, como `ed25519`).

**Little-endian para formato binário próprio do módulo.** Importante marcar o escopo:
isso vale só pra qualquer formato binário que o **módulo venha a definir por conta
própria** (cache em disco, arquivo de configuração binário, etc. — nada concreto ainda).
**Não** se aplica ao protocolo de fio do MySQL/MariaDB em si, cuja ordem de bytes é
definida pelo protocolo e já é tratada inteiramente pelo Boost.MySQL — não é algo que o
módulo controla ou precisa replicar.

## Pendências abertas nesta lista

Nenhuma até agora — todas as entradas acima já vieram com decisão seguindo a conversa.
Decisões ainda em aberto de fora desta lista continuam registradas no `roadmap.md`
(seção "Assunções adotadas"), por exemplo a versão do OpenSSL a fixar.
