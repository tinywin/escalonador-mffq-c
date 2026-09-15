#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_FILAS 4
#define MAX_PROCESSOS 64
#define MAX_SEGMENTOS 4096

const int QUANTUM_POR_FILA[NUM_FILAS] = {2, 4, 6, 8};

typedef enum { NOVO, PRONTO, EXECUTANDO, FINALIZADO } Estado;

typedef struct PCB {
    int pid;
    char nome[32];
    int prioridade_inicial;
    int prioridade;
    int chegada;
    int burst;
    int restante;
    int inicio;
    int termino;
    int despachos;
    Estado estado;
    struct PCB *prox;
} PCB;

typedef struct {
    PCB *atual;
    int tamanho;
    int quantum;
    int prioridade;
} Fila;

typedef struct {
    int pid;
    int inicio;
    int fim;
} Segmento;

typedef struct {
    PCB *processos[MAX_PROCESSOS];
    int n;
    Fila filas[NUM_FILAS];
    int tempo;
    int finalizados;
    int ticks_ociosos;
    int trocas_contexto;
    int ultimo_pid;
    Segmento linha[MAX_SEGMENTOS];
    int n_segmentos;
} Sistema;

void fila_iniciar(Fila *f, int prioridade, int quantum)
{
    f->atual = NULL;
    f->tamanho = 0;
    f->quantum = quantum;
    f->prioridade = prioridade;
}

void fila_inserir(Fila *f, PCB *p)
{
    if (f->atual == NULL) {
        p->prox = p;
        f->atual = p;
    } else {
        PCB *cauda = f->atual;
        while (cauda->prox != f->atual) cauda = cauda->prox;
        cauda->prox = p;
        p->prox = f->atual;
    }
    f->tamanho++;
    p->prioridade = f->prioridade;
    p->estado = PRONTO;
}

void fila_avancar(Fila *f)
{
    if (f->atual != NULL) f->atual = f->atual->prox;
}

PCB *fila_remover_atual(Fila *f)
{
    PCB *alvo;
    if (f->atual == NULL) return NULL;

    alvo = f->atual;
    if (alvo->prox == alvo) {
        f->atual = NULL;
    } else {
        PCB *pred = alvo;
        while (pred->prox != alvo) pred = pred->prox;
        pred->prox = alvo->prox;
        f->atual = alvo->prox;
    }
    alvo->prox = NULL;
    f->tamanho--;
    return alvo;
}

void fila_imprimir(const Fila *f)
{
    PCB *cur;
    printf("  Fila %d (quantum %d): ", f->prioridade, f->quantum);
    if (f->atual == NULL) {
        printf("vazia\n");
        return;
    }
    cur = f->atual;
    do {
        printf("P%d(%d) ", cur->pid, cur->restante);
        cur = cur->prox;
    } while (cur != f->atual);
    printf("\n");
}

void adicionar_processo(Sistema *s, const char *nome, int prioridade, int chegada, int burst)
{
    PCB *p;
    if (s->n >= MAX_PROCESSOS) return;
    if (prioridade < 0) prioridade = 0;
    if (prioridade >= NUM_FILAS) prioridade = NUM_FILAS - 1;
    if (burst < 1) burst = 1;
    if (chegada < 0) chegada = 0;

    p = malloc(sizeof(PCB));
    if (p == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente.\n");
        exit(1);
    }
    memset(p, 0, sizeof(PCB));

    p->pid = s->n + 1;
    strncpy(p->nome, nome, sizeof(p->nome) - 1);
    p->prioridade_inicial = prioridade;
    p->prioridade = prioridade;
    p->chegada = chegada;
    p->burst = burst;
    p->restante = burst;
    p->inicio = -1;
    p->termino = -1;
    p->estado = NOVO;
    p->prox = NULL;

    s->processos[s->n] = p;
    s->n++;
}

void sistema_iniciar(Sistema *s)
{
    int i;
    s->tempo = s->finalizados = s->ticks_ociosos = 0;
    s->n_segmentos = s->trocas_contexto = 0;
    s->ultimo_pid = -1;
    for (i = 0; i < NUM_FILAS; i++)
        fila_iniciar(&s->filas[i], i, QUANTUM_POR_FILA[i]);
}

void admitir_processos(Sistema *s)
{
    int i;
    for (i = 0; i < s->n; i++) {
        PCB *p = s->processos[i];
        if (p->estado == NOVO && p->chegada <= s->tempo) {
            fila_inserir(&s->filas[p->prioridade], p);
            printf("Tempo %d: processo %s (PID %d) chegou na fila %d\n", s->tempo, p->nome, p->pid, p->prioridade);
        }
    }
}

int fila_prioritaria(const Sistema *s)
{
    int i;
    for (i = 0; i < NUM_FILAS; i++)
        if (s->filas[i].atual != NULL) return i;
    return -1;
}

void registrar_segmento(Sistema *s, int pid, int inicio, int fim)
{
    if (s->n_segmentos >= MAX_SEGMENTOS) return;
    s->linha[s->n_segmentos].pid = pid;
    s->linha[s->n_segmentos].inicio = inicio;
    s->linha[s->n_segmentos].fim = fim;
    s->n_segmentos++;
}

void executar(Sistema *s)
{
    printf("\n== Execucao ==\n\n");

    while (s->finalizados < s->n) {
        int idx, usados = 0, inicio_fatia, preemptado = 0;
        Fila *f;
        PCB *p;

        admitir_processos(s);
        idx = fila_prioritaria(s);

        if (idx < 0) {
            s->tempo++;
            s->ticks_ociosos++;
            continue;
        }

        f = &s->filas[idx];
        p = f->atual;

        if (p->inicio < 0) p->inicio = s->tempo;
        p->estado = EXECUTANDO;
        p->despachos++;
        inicio_fatia = s->tempo;

        if (s->ultimo_pid != p->pid) {
            s->trocas_contexto++;
            s->ultimo_pid = p->pid;
        }

        printf("Tempo %d: despacho de %s (PID %d), fila %d, quantum %d, restante %d\n",
               s->tempo, p->nome, p->pid, idx, f->quantum, p->restante);

        while (usados < f->quantum && p->restante > 0) {
            s->tempo++;
            p->restante--;
            usados++;

            admitir_processos(s);

            if (p->restante == 0) break;
            if (fila_prioritaria(s) < idx) { preemptado = 1; break; }
        }

        registrar_segmento(s, p->pid, inicio_fatia, s->tempo);

        if (p->restante == 0) {
            p->estado = FINALIZADO;
            p->termino = s->tempo;
            fila_remover_atual(f);
            s->finalizados++;
            printf("Tempo %d: %s (PID %d) finalizado\n", s->tempo, p->nome, p->pid);
        } else {
            p->estado = PRONTO;
            if (!preemptado && usados == f->quantum && idx < NUM_FILAS - 1) {
                fila_remover_atual(f);
                fila_inserir(&s->filas[idx + 1], p);
                printf("Tempo %d: %s (PID %d) estourou o quantum, rebaixado da fila %d para fila %d\n",
                       s->tempo, p->nome, p->pid, idx, idx + 1);
            } else {
                fila_avancar(f);
                printf("Tempo %d: %s (PID %d) volta para a fila %d, restante %d\n",
                       s->tempo, p->nome, p->pid, idx, p->restante);
            }
        }
    }
}

void imprimir_gantt(const Sistema *s)
{
    int i;
    printf("\n== Linha do tempo ==\n\n");
    for (i = 0; i < s->n_segmentos; i++) {
        printf("P%d [%d-%d]  ", s->linha[i].pid, s->linha[i].inicio, s->linha[i].fim);
        if ((i + 1) % 6 == 0) printf("\n");
    }
    printf("\n");
}

void imprimir_relatorio(const Sistema *s)
{
    int i;
    double soma_turnaround = 0, soma_espera = 0, soma_resposta = 0;

    printf("\n== Relatorio final ==\n\n");
    printf("PID\tNome\tChegada\tBurst\tInicio\tFim\tTurnaround\tEspera\tResposta\n");

    for (i = 0; i < s->n; i++) {
        const PCB *p = s->processos[i];
        int turnaround = p->termino - p->chegada;
        int espera = turnaround - p->burst;
        int resposta = p->inicio - p->chegada;

        soma_turnaround += turnaround;
        soma_espera += espera;
        soma_resposta += resposta;

        printf("%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               p->pid, p->nome, p->chegada, p->burst, p->inicio, p->termino, turnaround, espera, resposta);
    }

    printf("\nTempo total de simulacao: %d\n", s->tempo);
    printf("Ticks ociosos: %d\n", s->ticks_ociosos);
    printf("Trocas de contexto: %d\n", s->trocas_contexto);
    printf("Turnaround medio: %.2f\n", soma_turnaround / s->n);
    printf("Espera media: %.2f\n", soma_espera / s->n);
    printf("Resposta media: %.2f\n", soma_resposta / s->n);
}

int main(void)
{
    static Sistema s;
    int i;

    memset(&s, 0, sizeof(Sistema));

    adicionar_processo(&s, "navegador", 0, 0, 6);
    adicionar_processo(&s, "compilador", 1, 1, 8);
    adicionar_processo(&s, "backup", 3, 2, 12);
    adicionar_processo(&s, "player", 2, 3, 5);
    adicionar_processo(&s, "antivirus", 1, 6, 7);
    adicionar_processo(&s, "editor", 0, 10, 4);
    adicionar_processo(&s, "indexador", 3, 12, 9);
    adicionar_processo(&s, "terminal", 2, 15, 3);

    sistema_iniciar(&s);

    printf("Filas de prioridade: %d\n", NUM_FILAS);
    for (i = 0; i < NUM_FILAS; i++)
        printf("  fila %d -> quantum %d\n", i, QUANTUM_POR_FILA[i]);
    printf("Processos carregados: %d\n\n", s.n);

    executar(&s);
    imprimir_gantt(&s);
    imprimir_relatorio(&s);

    printf("\nEstado final das filas:\n");
    for (i = 0; i < NUM_FILAS; i++) fila_imprimir(&s.filas[i]);

    for (i = 0; i < NUM_FILAS; i++)
        while (s.filas[i].atual != NULL) fila_remover_atual(&s.filas[i]);
    for (i = 0; i < s.n; i++) free(s.processos[i]);

    return 0;
}