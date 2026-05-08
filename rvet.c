/**
 * Relógios Vetoriais com MPI (3 processos)
 *
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

/* Imprime o estado atual do relógio vetorial */
void PrintClock(const char *acao, Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    printf("[Atualizado por P%d] %s -> Clock = (%d,%d,%d)\n",
           pid, acao, clock->p[0], clock->p[1], clock->p[2]);
}

/* Evento interno */
void Event(Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    clock->p[pid]++;

    PrintClock("Evento interno", clock);
}

/* Envio de mensagem */
void Send(int dest, Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    // Evento de envio
    clock->p[pid]++;

    // Envia vetor
    MPI_Send(clock->p, 3, MPI_INT, dest, 0, MPI_COMM_WORLD);

    PrintClock("Envio de mensagem", clock);
}

/* Recebimento de mensagem */
void Receive(int src, Clock *clock) {
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    int received[3];

    // Recebe vetor remoto
    MPI_Recv(received, 3, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Fusão (máximo elemento a elemento)
    for (int i = 0; i < 3; i++) {
        if (received[i] > clock->p[i]) {
            clock->p[i] = received[i];
        }
    }

    // Evento de recebimento
    clock->p[pid]++;

    PrintClock("Recebimento de mensagem", clock);
}

/* Processo 0 */
void process0() {
    Clock clock = {{0,0,0}};
    PrintClock("Estado inicial", &clock);

    Event(&clock);        // e1
    Send(1, &clock);      // envia para P1
    Event(&clock);        // e2
    Receive(2, &clock);   // recebe de P2
    Event(&clock);        // e3
}

/* Processo 1 */
void process1() {
    Clock clock = {{0,0,0}};
    PrintClock("Estado inicial", &clock);

    Receive(0, &clock);   // recebe de P0
    Event(&clock);        // e1
    Send(2, &clock);      // envia para P2
    Event(&clock);        // e2
}

/* Processo 2 */
void process2() {
    Clock clock = {{0,0,0}};
    PrintClock("Estado inicial", &clock);

    Event(&clock);        // e1
    Receive(1, &clock);   // recebe de P1
    Send(0, &clock);      // envia para P0
    Event(&clock);        // e2
}

int main(void) {
    int my_rank;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (my_rank == 0)
        process0();
    else if (my_rank == 1)
        process1();
    else if (my_rank == 2)
        process2();

    MPI_Finalize();
    return 0;
}
