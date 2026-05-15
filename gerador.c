#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INF 0 // Representação de infinito para arestas inexistentes

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <numero_de_vertices> <nome_do_arquivo>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    char *filename = argv[2];

    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Erro ao criar arquivo.\n");
        return 1;
    }

    // Opcional: Imprime o número de vértices na primeira linha
    fprintf(file, "%d\n", N); 

    srand(time(NULL));

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == j) {
                // Distância de um vértice para ele mesmo é 0
                fprintf(file, "0 ");
            } else {
                // Densidade do grafo: 80% de chance de existir uma aresta
                if (rand() % 100 < 80) {
                    int peso = (rand() % 100) + 1; // Pesos positivos entre 1 e 100
                    fprintf(file, "%d ", peso);
                } else {
                    fprintf(file, "%d ", INF);
                }
            }
        }
        fprintf(file, "\n"); // Quebra de linha ao fim de cada linha da matriz
    }

    fclose(file);
    printf("Arquivo %s gerado com sucesso (N=%d).\n", filename, N);
    return 0;
}
