#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cJSON.h"
#include <string.h>

//STRUCT DE CIDADE

typedef struct {
    int codigo_ibge;
    char nome_cidade[100];
    double latitude;
    double longitude;
} cidade;

//NÓ DA KDTree

typedef struct tnode {
    cidade *cidade;
    struct tnode *esq;
    struct tnode *dir;
} tnode;


typedef struct {
    tnode *raiz;
} kdtree;

//CALCULO DE DISTANCIA LAT/LONG

double distancia_euclidiana(cidade *cidade1, cidade *cidade2) {
    double lat_diff = cidade1->latitude - cidade2->latitude;
    double lon_diff = cidade1->longitude - cidade2->longitude;
    return sqrt(lat_diff * lat_diff + lon_diff * lon_diff);
}

//INSERE CIDADE NA KDTREE

void insere_cidade(kdtree *tree, cidade *nova_cidade) {
    tnode *novo_no = (tnode *)malloc(sizeof(tnode));
    novo_no->cidade = nova_cidade;
    novo_no->esq = NULL;
    novo_no->dir = NULL;

    if (tree->raiz == NULL) {
        tree->raiz = novo_no;
        return;
    }

    tnode *atual = tree->raiz;
    int dim = 0;
    
//ALTERNANDO LAT E LONG

    while (1) {
        if (dim == 0) {
            if (nova_cidade->latitude < atual->cidade->latitude) {
                if (atual->esq == NULL) {
                    atual->esq = novo_no;
                    return;
                } else {
                    atual = atual->esq;
                }
            } else {
                if (atual->dir == NULL) {
                    atual->dir = novo_no;
                    return;
                } else {
                    atual = atual->dir;
                }
            }
        } else { 
            if (nova_cidade->longitude < atual->cidade->longitude) {
                if (atual->esq == NULL) {
                    atual->esq = novo_no;
                    return;
                } else {
                    atual = atual->esq;
                }
            } else {
                if (atual->dir == NULL) {
                    atual->dir = novo_no;
                    return;
                } else {
                    atual = atual->dir;
                }
            }
        }
        dim = (dim + 1) % 2;
    }
}

//FUNCAO AUX PARA BUSCAR N VIZINHOS MAIS PROXIMOS

void busca_vizinhos_rec(tnode *node, cidade *alvo, cidade **vizinhos, int *index, int N, double *menores_distancias) {
    if (node == NULL) return;

    double dist = distancia_euclidiana(alvo, node->cidade);

    for (int i = 0; i < N; i++) {
        if (dist < menores_distancias[i]) {
            for (int j = N - 1; j > i; j--) {
                vizinhos[j] = vizinhos[j - 1];
                menores_distancias[j] = menores_distancias[j - 1];
            }
            vizinhos[i] = node->cidade;
            menores_distancias[i] = dist;
            break;
        }
    }

//VERIFICAÇO DE QUAL NÓ EXPLORAR ESQ/DIR ALTERNANDO LAT/LONG
    int dim = *index % 2; 
    if (dim == 0) {
        if (alvo->latitude < node->cidade->latitude) {
            busca_vizinhos_rec(node->esq, alvo, vizinhos, index, N, menores_distancias);
            (*index)++;
            busca_vizinhos_rec(node->dir, alvo, vizinhos, index, N, menores_distancias);
        } else {
            busca_vizinhos_rec(node->dir, alvo, vizinhos, index, N, menores_distancias);
            (*index)++;
            busca_vizinhos_rec(node->esq, alvo, vizinhos, index, N, menores_distancias);
        }
    } else {
        if (alvo->longitude < node->cidade->longitude) {
            busca_vizinhos_rec(node->esq, alvo, vizinhos, index, N, menores_distancias);
            (*index)++;
            busca_vizinhos_rec(node->dir, alvo, vizinhos, index, N, menores_distancias);
        } else {
            busca_vizinhos_rec(node->dir, alvo, vizinhos, index, N, menores_distancias);
            (*index)++;
            busca_vizinhos_rec(node->esq, alvo, vizinhos, index, N, menores_distancias);
        }
    }
}

//FUNCAO PRINCIPAL PARA BUSCAR N VIZINHOS MAIS PROXIMOS

cidade **busca_vizinhos(kdtree *tree, cidade *alvo, int N) {
    cidade **vizinhos = (cidade **)malloc(N * sizeof(cidade *));
    double *menores_distancias = (double *)malloc(N * sizeof(double));
    for (int i = 0; i < N; i++) {
        vizinhos[i] = NULL;
        menores_distancias[i] = INFINITY;
    }

    int index = 0;
    busca_vizinhos_rec(tree->raiz, alvo, vizinhos, &index, N, menores_distancias);

    free(menores_distancias);
    return vizinhos;
}

int main() {

    kdtree tree = { NULL };

// JSON READ

    FILE *file = fopen("municipios.json", "r");
    if (!file) {
        fprintf(stderr, "Erro ao abrir o arquivo JSON.\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *json_content = (char *)malloc(file_size + 1);
    fread(json_content, 1, file_size, file);
    json_content[file_size] = '\0';
    fclose(file);


    cJSON *json = cJSON_Parse(json_content);
    free(json_content);
    if (!json) {
        fprintf(stderr, "Erro ao fazer o parse do JSON.\n");
        return 1;
    }


    cJSON *cidade_item = NULL;
    cJSON_ArrayForEach(cidade_item, json) {
        cidade *nova_cidade = (cidade *)malloc(sizeof(cidade));
        nova_cidade->codigo_ibge = cJSON_GetObjectItem(cidade_item, "codigo_ibge")->valueint;
        strcpy(nova_cidade->nome_cidade, cJSON_GetObjectItem(cidade_item, "nome")->valuestring);
        nova_cidade->latitude = cJSON_GetObjectItem(cidade_item, "latitude")->valuedouble;
        nova_cidade->longitude = cJSON_GetObjectItem(cidade_item, "longitude")->valuedouble;

        insere_cidade(&tree, nova_cidade);
    }

    cJSON_Delete(json);
    
//FIM DA JSON READ
    
    int codigo_ibge_alvo = 0;
    printf("Digite o codigo ibge alvo desejado: ");
    scanf("%d", &codigo_ibge_alvo);
    
//BUSCA DA CIDADE ALVO

    cidade *alvo = NULL;
    tnode *atual = tree.raiz;
    while (atual != NULL) {
        if (atual->cidade->codigo_ibge == codigo_ibge_alvo) {
            alvo = atual->cidade;
            break;
        } else if (atual->cidade->codigo_ibge < codigo_ibge_alvo) {
            atual = atual->dir;
        } else {
            atual = atual->esq;
        }
    }
    
//---------------------------

    if (alvo == NULL) {
        printf("Cidade com código IBGE %d não encontrada na KDTree.\n", codigo_ibge_alvo);
        return 1;
    }
    
    int N = 0;
    printf("Digite o número de vizinhos desejados: ");
    scanf("%d", &N);
    
    cidade **vizinhos = busca_vizinhos(&tree, alvo, N+1);

    printf("Os %d vizinhos mais próximos de %s (Codigo IBGE %d):\n", N, alvo->nome_cidade, alvo->codigo_ibge);
    for (int i = 0; i < N; i++) {
        if (vizinhos[i] != NULL) {
            printf("Codigo IBGE %d\n", vizinhos[i+1]->codigo_ibge);
        }
    }

    free(vizinhos);
    return 0;
}
