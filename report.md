# PAGINADOR DE MEMÓRIA - RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

  * Artur Gaspar da Silva <email@domain> XX%
  * Kaio Henrique Masse Vieira <kaiomassevieira@gmail.com> XX%

3. Referências bibliográficas

4. Estruturas de dados
	
  Para armazenar as informações relacionadas à cada thread criada pela biblioteca, o struct `dcc_thread` foi criado, conforme especificação. Este tipo abstrato de dados é uma versão simplificada de um Thread Control Block (TCB), e contém as seguintes informacões:

  - `const char* name` : contém o nome da thread.
  - `ucontext_t context` : armazena o contexto em que a thread está executando, isto é, informações sobre sua stack, estado dos registradores, e contexto a ser aplicado após o retorno da thread.
  - `char stack[]` : armazena a stack da thread. Utilizou-se o tipo `char`, pois variáveis deste tipo ocupam um byte de memória. O tamanho máximo da stack é dado pela constante `MAX_BYTES_PER_STACK = 8192`.
  - `dccthread_t* waiting_thread` : ponteiro para a thread que está esperando a thread atual. Assume o valor `NULL` quando não há thread esperando.
  - `int completed` : flag indicando se a thread já terminou sua execução.

  Para armazenar as threads que foram criadas pela biblioteca de threads e que estão prontas para execução, uma fila de prontos foi utilizada, através da implementação de uma lista encadeada circular provida em `dlist.c`.

  Para realizar a administração das threads, é criada uma thread `scheduler` ao chamar-se `dccthread_init`. Além do escalonador, também é armazenado o ponteiro para a thread em execução em um dado momento na variável `executing`. As funções auxiliares `schedule()` e `execute()` realizam as rotinas de escalonamento e execução de uma nova thread, respectivamente.
  

  1. Descreva e justifique as estruturas de dados utilizadas para
     gerência das threads de espaço do usuário (partes 1, 2 e 5).

  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread\_yield e disparos do temporizador (parte 4).
