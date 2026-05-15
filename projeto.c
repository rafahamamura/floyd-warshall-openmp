#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define INF 9999999

// Definimos o tamanho máximo exigido pelo projeto (16.000)
#define MAX_VERTICES 16000

#define min(a,b) (((a)<(b))?(a):(b))

// ALOCAÇÃO ESTÁTICA GLOBAL
// Declarado fora da main para evitar Stack Overflow (Estouro de Pilha).
// Formato tradicional de matriz 2D.
int dist_matrix_base[MAX_VERTICES][MAX_VERTICES];
int dist_matrix[MAX_VERTICES][MAX_VERTICES];

double tempo_total = 0.0; // Variável para acumular o tempo total gasto em todas as execuções

/*
 * Função estritamente sequencial do Floyd-Warshall
 * A matriz global é acessada via notação 2D.
 */
void floyd_warshall_sequencial(int N) {
    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (dist_matrix[i][k] != INF && dist_matrix[k][j] != INF) {
                   dist_matrix[i][j] = min(dist_matrix[i][j], dist_matrix[i][k] + dist_matrix[k][j]);
                }
            }
        }
    }
}

/*
 * Função paralelizada do Floyd-Warshall usando OpenMP
 * A matriz global é acessada via notação 2D.
 */
void floyd_warshall_openmp(int N, int numThreads) {
    for (int k = 0; k < N; k++) {
        #pragma omp parallel for num_threads(numThreads) // Paraleliza os loops i e j usando a qtde de threads especificada
        for (int i = 0; i < N; i++) {   
            for (int j = 0; j < N; j++) {
                if (dist_matrix[i][k] != INF && dist_matrix[k][j] != INF) {
                   dist_matrix[i][j] = min(dist_matrix[i][j], dist_matrix[i][k] + dist_matrix[k][j]);
                }
            }
        }
    }
}

void fill_dist_matrix(int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            dist_matrix[i][j] = dist_matrix_base[i][j];
        }
    }
}

int execucaoSequencial(int N) {
    clock_t inicio, fim; // Variáveis para medir o tempo do processamento sequencial
            
    printf("Iniciando o processamento do Floyd-Warshall Sequencial (N = %d)...\n", N);
    
    tempo_total = 0.0; // Reinicia o tempo total para a execução sequencial
    for (int r = 0; r < 5; r++) { // Executa 5 vezes para obter uma média mais confiável
        fill_dist_matrix(N); // Restaura a matriz original antes de cada execução

        inicio = clock(); // Inicia o cronômetro
        
        floyd_warshall_sequencial(N);
        
        fim = clock(); // Para o cronômetro
        
        // Calcula o tempo em segundos
        double tempo_gasto = (double)(fim - inicio) / CLOCKS_PER_SEC;
        tempo_total += tempo_gasto;
        printf("Tempo de processamento nº %d: %f segundos.\n", r + 1, tempo_gasto);
    }
    
    return 0;
}

int execucaoOpenMP(int N, int numThreads) {
    printf("Iniciando o processamento do Floyd-Warshall OPENMP (N = %d, Threads = %d)...\n", N, numThreads);
    
    tempo_total = 0.0; // Reinicia o tempo total para esta configuração de threads
    for (int r = 0; r < 5; r++) { // Executa 5 vezes para obter uma média mais confiável
        fill_dist_matrix(N); // Restaura a matriz original antes de cada execução
        // OpenMP utiliza uma função diferente para medir o tempo omp_get_wtime(), que é a correta para o paralelismo.
        double inicio_openmp = omp_get_wtime(); // Inicia o cronômetro
        
        floyd_warshall_openmp(N, numThreads);
        
        double fim_openmp = omp_get_wtime(); // Para o cronômetro
        
        // Calcula o tempo em segundos
        double tempo_gasto = (double)(fim_openmp - inicio_openmp);
        tempo_total += tempo_gasto;
        printf("Tempo de processamento nº %d: %f segundos.\n", r + 1, tempo_gasto);
    }
    
    return 0;
}


int main(int argc, char* argv[]) {
    // Verifica se o usuário passou o arquivo de entrada como argumento
    if (argc != 2) {
        printf("Uso correto: %s <arquivo_de_entrada> \n", argv[0]);
        return 1;
    }

    int N;
    
    // ---------------------------------------------------------
    // LEITURA DE DADOS (Não contabilizada no tempo)
    // ---------------------------------------------------------
    // Abre o arquivo passado como argumento (argv[1])
    FILE *arquivo_entrada = fopen(argv[1], "r");
    if (arquivo_entrada == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", argv[1]);
        return 1;
    }

    // Lê o tamanho do grafo (N)
    if (fscanf(arquivo_entrada, "%d", &N) != 1) {
        printf("Erro ao ler o tamanho do grafo!\n");
        fclose(arquivo_entrada);
        return 1;
    }

    // Proteção para não estourar o limite estático configurado
    if (N > MAX_VERTICES) {
        printf("Tamanho do grafo (%d) excede o limite estático de %d!\n", N, MAX_VERTICES);
        fclose(arquivo_entrada);
        return 1;
    }

    // Preenche a matriz global usando notação 2D [i][j]
    int transitional;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(arquivo_entrada, "%d", &transitional);
            // Se o valor lido for 0 e não for a diagonal, significa que não há caminho entre i e j
            // Aqui deve ser explicitamente tratado para evitar confusão com o caso de caminho de um vértice para ele mesmo (diagonal)
            if (transitional == 0 && i != j) {
                dist_matrix_base[i][j] = INF; // Sem caminho, exceto para a diagonal
            } else {
            dist_matrix_base[i][j] = transitional;
            }
        }
    }
    fclose(arquivo_entrada);

    double tempo_gasto; // Variável para armazenar o tempo gasto em segundos
    
    
    // ---------------------------------------------------------
    // EXECUÇÃO E MEDIÇÃO DE TEMPO SEQUENCIAL (Apenas processamento)
    // ---------------------------------------------------------    
    execucaoSequencial(N);
    
    // GRAVAÇÃO DE RESULTADOS (Não contabilizada no tempo)
    // Monta o nome do arquivo dinamicamente usando sprintf: resultadoGrafo_N_sequencial.txt
    char nome_arquivo_saida[100];
    sprintf(nome_arquivo_saida, "resultadoGrafo_%d_sequencial.txt", N);

    FILE *arquivo_saida_seq = fopen(nome_arquivo_saida, "w");
    if (arquivo_saida_seq == NULL) {
        printf("Erro ao criar o arquivo %s!\n", nome_arquivo_saida);
        return 1;
    }

    // Grava a matriz final
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(arquivo_saida_seq, "%d ", dist_matrix[i][j]);
        }
        fprintf(arquivo_saida_seq, "\n");
    }
    
    printf("Tempo médio gasto em todas as execuções sequenciais: %f segundos.\n", tempo_total / 5); // Supondo 5 execuções
    fclose(arquivo_saida_seq);
    printf("Resultados gravados em %s com sucesso.\n", nome_arquivo_saida);

    // ---------------------------------------------------------
    // EXECUÇÃO E MEDIÇÃO DE TEMPO OPENMP (Apenas processamento)
    // ---------------------------------------------------------
    for (int numThreads = 2; numThreads <= 33; numThreads *= 2) { // Testa com 2, 4, 8, 16 e 32 threads
        execucaoOpenMP(N, numThreads);
        printf("Tempo médio gasto em todas as execuções OpenMP: %f segundos.\n", tempo_total / 5); // Supondo 5 execuções
    }

    // GRAVAÇÃO DE RESULTADOS OpenMP(Não contabilizada no tempo)
    // Monta o nome do arquivo dinamicamente usando sprintf: resultadoGrafo_N_openmp.txt
    char nome_arquivo_saida_openmp[100];
    sprintf(nome_arquivo_saida_openmp, "resultadoGrafo_%d_openmp.txt", N);

    FILE *arquivo_saida_openmp = fopen(nome_arquivo_saida_openmp, "w");
    if (arquivo_saida_openmp == NULL) {
        printf("Erro ao criar o arquivo %s!\n", nome_arquivo_saida_openmp);
        return 1;
    }

    // Grava a matriz final
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(arquivo_saida_openmp, "%d ", dist_matrix[i][j]);
        }
        fprintf(arquivo_saida_openmp, "\n");
    }
    
    fclose(arquivo_saida_openmp);        
    printf("Resultados gravados em %s com sucesso.\n", nome_arquivo_saida_openmp);

    return 0;
}
