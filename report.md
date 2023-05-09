# PAGINADOR DE MEMÓRIA - RELATÓRIO

### 1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

### 2. Membros do grupo e alocação de esforço

  * Artur Gaspar da Silva <arthur.gaspar2011@gmail.com> 60%
  * Kaio Henrique Masse Vieira <kaiomassevieira@gmail.com> 40%

### 3. Referências bibliográficas

  Utilizamos como referência as páginas dos manuais de [temporização](https://man7.org/linux/man-pages/man7/time.7.html), [contexto](https://pubs.opengroup.org/onlinepubs/7908799/xsh/ucontext.h.html) e [sinais](https://man7.org/linux/man-pages/man7/signal.7.html).

### 4. Estruturas de dados
	
  Para armazenar as informações relacionadas à cada thread criada pela biblioteca, o struct `dcc_thread` foi criado, conforme especificação. Este tipo abstrato de dados é uma versão simplificada de um Thread Control Block (TCB), e contém as seguintes informacões:

  - `const char* name` : representa o nome da thread.
  - `ucontext_t context` : armazena o contexto em que a thread está executando, isto é, informações sobre sua stack, estado dos registradores, e contexto a ser aplicado após a thread terminar de executar.
  - `char stack[]` : armazena a stack da thread. Utilizou-se o tipo `char`, pois variáveis deste tipo ocupam um byte de memória. O tamanho máximo da stack é dado pela constante `MAX_BYTES_PER_STACK = 8192`.
  - `dccthread_t* waiting_thread` : ponteiro para a thread que está esperando a thread atual. Assume o valor `NULL` quando não há thread esperando.
  - `int completed` : flag indicando se a thread já terminou sua execução.

  Para armazenar as threads que foram criadas pela biblioteca de threads e que estão prontas para execução, uma fila de prontos foi utilizada, através da implementação de uma lista encadeada circular provida em `dlist.c`. Ao realizar esta escolha, estamos utilizando um algoritmo de _Round Robin_ para fazer o escalonamento das threads, que possui a vantagem de previnir o fenômeno de inanição.

  Em vista de realizar a administração das threads, é criada uma thread `scheduler` ao chamar-se `dccthread_init`. Além do escalonador, também é armazenado o ponteiro para a thread em execução em um dado momento na variável `executing`. As funções auxiliares `schedule()` e `execute()` realizam as rotinas de escalonamento e execução de uma nova thread, respectivamente.

  Para adicionar suporte às chamadas de `dccthread_sleep`, usamos um contador de threads que estão em estado de `sleep` e mantemos o escalonador em espera enquanto o contador for positivo. Quando uma thread retorna após uma chamada de `dccthread_sleep`, ela retorna à fila de prontos para que não possa abusar deste mecanismo e tomar controle da CPU.

  O mecanismo de sincronização envolve o uso de dois sinais de preempção: `SIGRTMIN` para o temporizador e `SIGRTMAX` para chamadas de sleep. Quando estes eventos são disparados, as funções `dccthread_sighandler` e `dccthread_sighandler_sleep` são chamadas, respectivamente. Nos eventos do temporizador, apenas chamamos `dccthread_yield`, enquanto nos eventos de `sleep` apenas colocamos a thread na fila de prontos.

  Além isso, uma decisão importante para evitar condições de corrida, possivelmente causadas por interrupções do temporizador, foi desabilitar os sinais de interrupções na entrada das funções de `dccthread`, reabilitando-os ao final da execução. Isto é feito ao chamar `sigprocmask` com os sinais `SIG_BLOCK` e `SIG_UNBLOCK`, respectivamente, armazenando o controle de sinais na variável `sigrt_both`.
