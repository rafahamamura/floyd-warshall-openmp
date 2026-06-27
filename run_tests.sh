#!/bin/bash

# Nome do executável
EXEC="floyd_mpi"
CODIGO="floyd_mpi.c"
LOG_FILE="resultados_mpi.log"

# Define os tamanhos dos grafos e as quantidades de processos exigidas
GRAFOS=("entr1000.dat" "entr2000.dat" "entr4000.dat" "entr8000.dat" "entr16000.dat")
PROCESSOS=(2 4 8 16 32)

echo "Limpando logs antigos..."
> $LOG_FILE

echo "Compilando o programa com otimizacao máxima (-O3 -march=native)..."
mpicc $CODIGO -o $EXEC -O3 -march=native
if [ $? -ne 0 ]; then
    echo "Erro na compilação! Abortando os testes."
    exit 1
fi
echo "Compilação concluída com sucesso."
echo ""

echo "======================================================" | tee -a $LOG_FILE
echo "       INICIANDO BATERIA DE TESTES AUTOMATIZADA       " | tee -a $LOG_FILE
echo "======================================================" | tee -a $LOG_FILE
echo "" | tee -a $LOG_FILE

# Laço principal iterando sobre cada arquivo de grafo
for grafo in "${GRAFOS[@]}"; do
    
    # Verificando se o arquivo do grafo realmente existe na pasta
    if [ ! -f "$grafo" ]; then
        echo "AVISO: O arquivo $grafo nao foi encontrado. Pulando..." | tee -a $LOG_FILE
        continue
    fi

    # Laço interno iterando sobre o número de processos
    for p in "${PROCESSOS[@]}"; do
        
        # Como N=16000 com poucos processos levará horas, permitimos a interrupção do teste para evitar longos tempos de execução
        echo "--- Executando $grafo com $p processos ---" | tee -a $LOG_FILE
        
        # Executa o mpirun permitindo oversubscribe e joga a saída na tela e no log
        mpirun --oversubscribe -np $p ./$EXEC $grafo | tee -a $LOG_FILE
        
        echo "" | tee -a $LOG_FILE
    done
done

echo "======================================================" | tee -a $LOG_FILE
echo "   BATERIA FINALIZADA! Log salvo em: $LOG_FILE        " | tee -a $LOG_FILE
echo "======================================================" | tee -a $LOG_FILE