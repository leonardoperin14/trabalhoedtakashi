#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cJSON.h"

#define TAMANHO_TABELA 6000

typedef struct {
    int codigo_ibge;
    char nome[200];
    float latitude;
    float longitude;
    int capital;
    int codigo_uf;
    int siafi_id;
    int ddd;
    char fuso_horario[40];
} Cidade;

typedef struct {
    char nome[200];
    int codigo_ibge;
} ItemHash;


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

//----------------------

typedef struct {
    ItemHash *itens[TAMANHO_TABELA];
} TabelaHash;

int hashString(const char *str) {
    int hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % TAMANHO_TABELA;
}

void inicializarTabela(TabelaHash *tabela) {
    for (int i = 0; i < TAMANHO_TABELA; i++) {
        tabela->itens[i] = NULL;
    }
}

void inserirItem(TabelaHash *tabela, const char *nome, int codigo_ibge) {
    int hashValor = hashString(nome);

    ItemHash *item = (ItemHash *)malloc(sizeof(ItemHash));
    strncpy(item->nome, nome, sizeof(item->nome));
    item->codigo_ibge = codigo_ibge;

    while (tabela->itens[hashValor] != NULL) {
        hashValor = (hashValor + 1) % TAMANHO_TABELA;
    }

    tabela->itens[hashValor] = item;
}

ItemHash *buscarItem(TabelaHash *tabela, const char *nome) {
    int hashValor = hashString(nome);

    while (tabela->itens[hashValor] != NULL) {
        if (strcmp(tabela->itens[hashValor]->nome, nome) == 0) {
            return tabela->itens[hashValor];
        }
        hashValor = (hashValor + 1) % TAMANHO_TABELA;
    }

    return NULL;
}

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

    FILE *file = fopen("cidades.json", "r");
    if (!file) {
        printf("Erro ao abrir o arquivo.\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileContent = (char *)malloc(fileSize + 1);
    fread(fileContent, 1, fileSize, file);
    fclose(file);
    fileContent[fileSize] = '\0';

    cJSON *root = cJSON_Parse(fileContent);
    if (!root) {
        printf("Erro ao analisar o JSON.\n");
        return 1;
    }

    cJSON *cidades = cJSON_Parse(fileContent);
    if (!cidades) {
        printf("Erro ao obter as cidades do JSON.\n");
        cJSON_Delete(root);
        return 1;
    }

    TabelaHash tabela;
    inicializarTabela(&tabela);

    cJSON *cidadeItem;
    cJSON_ArrayForEach(cidadeItem, cidades) {
        Cidade cidade;
        cidade.codigo_ibge = cJSON_GetObjectItem(cidadeItem, "codigo_ibge")->valueint;
        strcpy(cidade.nome, cJSON_GetObjectItem(cidadeItem, "nome")->valuestring);
        cidade.latitude = cJSON_GetObjectItem(cidadeItem, "latitude")->valuedouble;
        cidade.longitude = cJSON_GetObjectItem(cidadeItem, "longitude")->valuedouble;
        cidade.capital = cJSON_GetObjectItem(cidadeItem, "capital")->valueint;
        cidade.codigo_uf = cJSON_GetObjectItem(cidadeItem, "codigo_uf")->valueint;
        cidade.siafi_id = cJSON_GetObjectItem(cidadeItem, "siafi_id")->valueint;
        cidade.ddd = cJSON_GetObjectItem(cidadeItem, "ddd")->valueint;
        strcpy(cidade.fuso_horario, cJSON_GetObjectItem(cidadeItem, "fuso_horario")->valuestring);

        inserirItem(&tabela, cidade.nome, cidade.codigo_ibge);
        
    }

    cJSON_Delete(root);
    free(fileContent);
    
    // JSON READ

    FILE *file1 = fopen("cidadestree.json", "r");
    if (!file1) {
        fprintf(stderr, "Erro ao abrir o arquivo JSON.\n");
        return 1;
    }

    fseek(file1, 0, SEEK_END);
    long file_size = ftell(file1);
    fseek(file1, 0, SEEK_SET);
    char *json_content = (char *)malloc(file_size + 1);
    fread(json_content, 1, file_size, file1);
    json_content[file_size] = '\0';
    fclose(file1);

    cJSON *json = cJSON_Parse(json_content);
    free(json_content);
    if (!json) {
        fprintf(stderr, "Erro ao fazer o parse do JSON.\n");
        return 1;
    }

    kdtree tree = { NULL };

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
    

    char nomeBusca[200];
    printf("Nome da cidade que deseja buscar: ");
    fgets(nomeBusca, sizeof(nomeBusca), stdin);
    nomeBusca[strcspn(nomeBusca, "\n")] = '\0';

    ItemHash *resultado = buscarItem(&tabela, nomeBusca);


    if (resultado != NULL) {
        printf("Cidade encontrada:\n");
        printf("Nome: %s\n", resultado->nome);
        printf("Codigo IBGE: %d\n", resultado->codigo_ibge);
    } else {
        printf("Cidade com nome %s não encontrada.\n", nomeBusca);
    }
    
    int codigo_ibge_alvo = resultado->codigo_ibge;

    // Buscando o nó correspondente na KDTree
    cidade *alvo = NULL;
    tnode *atual = tree.raiz;
    while (atual != NULL) {
        if (atual->cidade->codigo_ibge == codigo_ibge_alvo) {
            alvo = atual->cidade;
            break;
        } else if (codigo_ibge_alvo < atual->cidade->latitude) {
            atual = atual->esq;
        } else {
            atual = atual->dir;
        }
    }
    
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
