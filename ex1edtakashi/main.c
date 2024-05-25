#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cJSON.h"

#define TAMANHO_TABELA 12000

//STRUCT DA CIDADE DEFINIDA NO JSON

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

//STRUCT DA HASH
typedef struct {
    int codigo_ibge;
    Cidade cidade;
} ItemHash;

//STRUCT DA TABELA TAMANHO_TABELA = 6000 DEVIDO A 5.565 municípios, segundo o Instituto Brasileiro de Geografia e Estatística (IBGE)

typedef struct {
    ItemHash *itens[TAMANHO_TABELA];
} TabelaHash;

int hash1(int chave) {
    return chave % TAMANHO_TABELA;
}

int hash2(int chave) {
    return 1 + (chave % (TAMANHO_TABELA - 1));
}

void inicializarTabela(TabelaHash *tabela) {
    for (int i = 0; i < TAMANHO_TABELA; i++) {
        tabela->itens[i] = NULL;
    }
}

//INSERIR ITEM NA HASH

void inserirItem(TabelaHash *tabela, int codigo_ibge, Cidade cidade) {
    ItemHash *item = (ItemHash *)malloc(sizeof(ItemHash));
    item->codigo_ibge = codigo_ibge;
    item->cidade = cidade;
    int hashValor = hash1(codigo_ibge);
    int hashValor2= hash2(codigo_ibge);

    while (tabela->itens[hashValor] != NULL) {
        hashValor += hashValor2;
        hashValor %= TAMANHO_TABELA;
    }

    tabela->itens[hashValor] = item;
}

//BUSCAR ITEM NA HASH DUPLA

ItemHash *buscarItem(TabelaHash *tabela, int codigo_ibge)
{
    int hashValor = hash1(codigo_ibge);
    int hashValor2 = hash2(codigo_ibge);

    while (tabela->itens[hashValor] != NULL)
    {
        if (tabela->itens[hashValor]->codigo_ibge == codigo_ibge)
        {
            return tabela->itens[hashValor];
        }
        hashValor += hashValor2;
        hashValor %= TAMANHO_TABELA;
    }

    return NULL;
}

int main() {
    
//JSON READ
    
    FILE *file = fopen("municipios.json", "r");
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

        inserirItem(&tabela, cidade.codigo_ibge, cidade);
    }

    cJSON_Delete(root);
    free(fileContent);
    
//FIM DA JSONREAD
    
    int codigoBusca;
    printf("Codigo IBGE da cidade que deseja buscar: ");
    scanf("%d", &codigoBusca);
    
    ItemHash *resultado = buscarItem(&tabela, codigoBusca);

    if (resultado != NULL) {
        printf("Cidade encontrada:\n");
        printf("Codigo IBGE: %d\n", resultado->cidade.codigo_ibge);
        printf("Nome: %s\n", resultado->cidade.nome);
        printf("Latitude: %.4f\n", resultado->cidade.latitude);
        printf("Longitude: %.4f\n", resultado->cidade.longitude);
        printf("Capital: %d\n", resultado->cidade.capital);
        printf("Codigo UF: %d\n", resultado->cidade.codigo_uf);
        printf("SIAFI ID: %d\n", resultado->cidade.siafi_id);
        printf("DDD: %d\n", resultado->cidade.ddd);
        printf("Fuso Horario: %s\n", resultado->cidade.fuso_horario);
    } else {
        printf("Cidade com codigo IBGE %d não encontrada.\n", codigoBusca);
    }

    return 0;
}
