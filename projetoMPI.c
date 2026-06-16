#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define INF 9999999

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0) printf("Uso: %s <arquivo_entrada>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int N;
    int *matrix_base = NULL;
    int *matrix_final = NULL;

    // ---------------------------------------------------------
    // 1. LEITURA DE DADOS (Apenas Rank 0) - Não conta no tempo
    // ---------------------------------------------------------
    if (rank == 0) {
        FILE *arquivo_entrada = fopen(argv[1], "r");
        if (!arquivo_entrada) {
            printf("Erro ao abrir %s\n", argv[1]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fscanf(arquivo_entrada, "%d", &N);

        size_t size_bytes = (size_t)N * N * sizeof(int);
        matrix_base = (int*)malloc(size_bytes);
        matrix_final = (int*)malloc(size_bytes);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                int val;
                fscanf(arquivo_entrada, "%d", &val);
                matrix_base[i * N + j] = (val == 0 && i != j) ? INF : val;
            }
        }
        fclose(arquivo_entrada);
    }

    // Compartilha o tamanho N com todos os processos
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // ---------------------------------------------------------
    // 2. CÁLCULO DE DIVISÃO DE CARGA (Fatiamento de Linhas)
    // ---------------------------------------------------------
    int base_rows = N / size;
    int remainder = N % size;
    
    int local_rows = base_rows + (rank < remainder ? 1 : 0);
    int start_row = rank * base_rows + (rank < remainder ? rank : remainder);

    // Variáveis para Scatterv e Gatherv (apenas no Rank 0)
    int *sendcounts = NULL;
    int *displs = NULL;
    if (rank == 0) {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));
        int disp = 0;
        for (int i = 0; i < size; i++) {
            int rows = base_rows + (i < remainder ? 1 : 0);
            sendcounts[i] = rows * N;
            displs[i] = disp;
            disp += sendcounts[i];
        }
    }

    // Alocação da memória local para cada processo
    int *local_matrix = (int*)malloc((size_t)local_rows * N * sizeof(int));
    int *row_k_buffer = (int*)malloc(N * sizeof(int)); // Buffer para receber o broadcast da linha k

    if (rank == 0) {
        printf("\n======================================================\n");
        printf("   BENCHMARK FLOYD-WARSHALL MPI | GRAFO N = %d\n", N);
        printf("   Processos alocados: %d\n", size);
        printf("======================================================\n");
    }

    double total_time = 0.0;

    // ---------------------------------------------------------
    // 3. EXECUÇÃO DA SUÍTE DE BENCHMARK (5 Repetições)
    // ---------------------------------------------------------
    for (int rep = 0; rep < 5; rep++) {
        // Envia a matriz limpa e original para todos os processos antes de cada teste
        MPI_Scatterv(matrix_base, sendcounts, displs, MPI_INT,
                     local_matrix, local_rows * N, MPI_INT, 0, MPI_COMM_WORLD);

        // Sincroniza todos os processos e inicia o cronômetro [cite: 64]
        MPI_Barrier(MPI_COMM_WORLD);
        double start_time = MPI_Wtime();

        for (int k = 0; k < N; k++) {
            // Descobre qual processo é o dono da linha k
            int owner;
            if (k < remainder * (base_rows + 1)) {
                owner = k / (base_rows + 1);
            } else {
                owner = remainder + (k - remainder * (base_rows + 1)) / base_rows;
            }

            // O dono copia sua linha k para o buffer
            if (rank == owner) {
                int local_k = k - start_row;
                memcpy(row_k_buffer, &local_matrix[local_k * N], N * sizeof(int));
            }

            // O dono transmite (Broadcast) a linha k para todos os outros processos na rede
            MPI_Bcast(row_k_buffer, N, MPI_INT, owner, MPI_COMM_WORLD);

            // Cada processo atualiza o seu pedaço local da matriz
            for (int i = 0; i < local_rows; i++) {
                for (int j = 0; j < N; j++) {
                    int ik = local_matrix[i * N + k];
                    int kj = row_k_buffer[j];

                    if (ik != INF && kj != INF) {
                        int sum = ik + kj;
                        if (local_matrix[i * N + j] > sum) {
                            local_matrix[i * N + j] = sum;
                        }
                    }
                }
            }
        }

        // Sincroniza todos e para o cronômetro
        MPI_Barrier(MPI_COMM_WORLD);
        double end_time = MPI_Wtime();
        double elapsed = end_time - start_time;

        if (rank == 0) {
            printf("  Repeticao %d: %f segundos\n", rep + 1, elapsed);
            total_time += elapsed;
        }
    }

    // ---------------------------------------------------------
    // 4. COLETA DOS RESULTADOS E GRAVAÇÃO
    // ---------------------------------------------------------
    if (rank == 0) {
        printf("  -> TEMPO MEDIO: %f segundos\n", total_time / 5.0); // Valores médios solicitados [cite: 74]
        printf("======================================================\n\n");
    }

    // Junta as fatias de todos os processos de volta no matrix_final do Rank 0
    MPI_Gatherv(local_matrix, local_rows * N, MPI_INT,
                matrix_final, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // Rank 0 grava no arquivo [cite: 69]
    if (rank == 0) {
        char arquivo_saida_nome[100];
        sprintf(arquivo_saida_nome, "resultadoGrafo_%d_mpi.txt", N);
        FILE *arquivo_saida = fopen(arquivo_saida_nome, "w");
        if (arquivo_saida) {
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    fprintf(arquivo_saida, "%d ", matrix_final[i * N + j]);
                }
                fprintf(arquivo_saida, "\n");
            }
            fclose(arquivo_saida);
        }
    }

    // Liberação de Memória
    free(local_matrix);
    free(row_k_buffer);
    if (rank == 0) {
        free(matrix_base);
        free(matrix_final);
        free(sendcounts);
        free(displs);
    }

    MPI_Finalize();
    return 0;
}