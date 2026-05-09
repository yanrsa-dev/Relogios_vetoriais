/**
 * Implementação de Relógios Vetoriais (MPI)
 * -------------------------------------------------------------
 * Compilação: mpicc -o rvet rvet.c
 * Execução:   mpiexec -n 3 ./rvet
 */

#include <stdio.h>
#include <string.h>
#include <mpi.h>

/* Estrutura do relógio vetorial (3 processos). */
typedef struct Clock {
    int p[3];
} Clock;

/*
 * Imprime o estado atual do relógio vetorial.
 * Mostra: qual processo realizou a atualização e o vetor resultante.
 */
void PrintClock(const char *acao, Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    printf("[Atualizado por P%d] %s -> Clock = (%d,%d,%d)\n",
           pid, acao, clock->p[0], clock->p[1], clock->p[2]);
}

/*
 * Evento interno (sem comunicação).
 * Incrementa o contador local do processo.
 */
void Event(Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    
    // Incrementa o relógio local do próprio processo
    clock->p[pid]++;
    
    // No final, imprime relógio atualizado
    PrintClock("Evento interno", clock);
}

/*
 * Envio de mensagem.
 */
void Send(int dest, Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    
    // 1. Incrementar clock local (evento de envio)
    clock->p[pid]++;
    
    // 2. Enviar clock->p via MPI_Send
    MPI_Send(clock->p, 3, MPI_INT, dest, 0, MPI_COMM_WORLD);
    
    // No final, imprime relógio atualizado:
    PrintClock("Envio de mensagem", clock);
}

/*
 * Recebimento de mensagem.
 */
void Receive(int src, Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    
    // 1. Incrementar clock local (evento de recebimento)
    clock->p[pid]++;
    
    // 2. Receber vetor remoto via MPI_Recv
    int remote_clock[3];
    MPI_Recv(remote_clock, 3, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // 3. Fazer fusão (max elemento a elemento) com vetor recebido
    for (int i = 0; i < 3; i++) {
        if (remote_clock[i] > clock->p[i]) {
            clock->p[i] = remote_clock[i];
        }
    }
    
    // No final, imprime relógio atualizado
    PrintClock("Recebimento de mensagem", clock);
}

/*
 * Sequência de eventos do Processo 0 conforme o diagrama.
 */
void process0() {
    Clock clock = {{0,0,0}};
    PrintClock("Estado inicial", &clock);

    Event(&clock);         // a: Evento interno (1,0,0)
    Send(1, &clock);       // b: Envia para P1 (2,0,0)
    Receive(1, &clock);    // c: Recebe de P1 (3,1,0)
    Send(2, &clock);       // d: Envia para P2 (4,1,0)
    Receive(2, &clock);    // e: Recebe de P2 (5,1,2)
    Send(1, &clock);       // f: Envia para P1 (6,1,2)
    Event(&clock);         // g: Evento interno (7,1,2)
}

/*
 * Sequência de eventos do Processo 1 conforme o diagrama.
 */
void process1() {
    Clock clock = {{0,0,0}};
    PrintClock("Estado inicial", &clock);

    Send(0, &clock);       // h: Envia para P0 (0,1,0)
    Receive(0, &clock);    // i: Recebe de P0 (2,2,0)
    Receive(0, &clock);    // j: Recebe de P0 (6,3,2)
}

/*
 * Sequência de eventos do Processo 2 conforme o diagrama.
 */
void process2() {
    Clock clock = {{0,0,0}};
    PrintClock("Estado inicial", &clock);

    Event(&clock);         // k: Evento interno (0,0,1)
    Send(0, &clock);       // l: Envia para P0 (0,0,2)
    Receive(0, &clock);    // m: Recebe de P0 (4,1,3)
}

int main(void) {
    int my_rank;

    // Inicialização do MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Delegação da execução de acordo com o ranking do processo
    if (my_rank == 0)
        process0();
    else if (my_rank == 1)
        process1();
    else if (my_rank == 2)
        process2();

    // Finalização do MPI
    MPI_Finalize();
    return 0;
}
