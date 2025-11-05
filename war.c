// missoes.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_TERR 5
#define NAME_SZ 32
#define COLOR_SZ 16

const char *PLAYER_COLOR = "Azul";

typedef struct {
    char nome[NAME_SZ];
    char cor[COLOR_SZ];
    char original_cor[COLOR_SZ]; // usado para detectar territórios conquistados recentemente
    int tropas;
} Territorio;

typedef enum {
    MISSION_DESTROY_GREEN,
    MISSION_CONQUER_3
} MissionType;

typedef struct {
    MissionType type;
    char descricao[128];
    int target; // usado para CONQUER_3: quantos conquistar
} Missao;

/* Protótipos de funções */
Territorio *alloc_map(void);
void init_territories(Territorio *mapa, size_t n);
void print_map(const Territorio *mapa, size_t n);
Missao assign_mission(void);
void print_mission(const Missao *m);
int simulate_attack(Territorio *atacante, Territorio *defensor);
int check_mission(const Missao *m, const Territorio *mapa, size_t n);
int count_new_conquests(const Territorio *mapa, size_t n, const char *player_color);
void cleanup(Territorio *mapa);

/* main */
int main(void) {
    srand((unsigned)time(NULL));

    Territorio *mapa = alloc_map();
    if (!mapa) {
        fprintf(stderr, "Erro: nao foi possivel alocar mapa\n");
        return 1;
    }

    init_territories(mapa, N_TERR);
    Missao missao = assign_mission();

    int opcao = -1;
    while (1) {
        puts("\n=== MENU PRINCIPAL ===");
        print_map(mapa, N_TERR);
        print_mission(&missao);
        puts("\nEscolha uma opcao:");
        puts("1 - Atacar");
        puts("2 - Verificar Missao");
        puts("0 - Sair");
        printf("> ");
        if (scanf("%d", &opcao) != 1) {
            // limpa entrada inválida
            int c; while ((c = getchar()) != EOF && c != '\n') {}
            puts("Entrada invalida.");
            continue;
        }

        if (opcao == 0) break;

        if (opcao == 1) {
            int a_idx = -1, d_idx = -1;
            printf("Escolha territorio atacante (1-%d): ", N_TERR);
            if (scanf("%d", &a_idx) != 1) { int c; while ((c = getchar()) != EOF && c != '\n') {} puts("Entrada invalida."); continue; }
            printf("Escolha territorio defensor (1-%d): ", N_TERR);
            if (scanf("%d", &d_idx) != 1) { int c; while ((c = getchar()) != EOF && c != '\n') {} puts("Entrada invalida."); continue; }

            if (a_idx < 1 || a_idx > N_TERR || d_idx < 1 || d_idx > N_TERR) {
                puts("Indices fora do intervalo.");
                continue;
            }
            if (a_idx == d_idx) {
                puts("Atacante e defensor devem ser territorios diferentes.");
                continue;
            }

            Territorio *at = &mapa[a_idx - 1];
            Territorio *df = &mapa[d_idx - 1];

            // só permite que o jogador ataque a partir de territórios da sua cor
            if (strcmp(at->cor, PLAYER_COLOR) != 0) {
                printf("Voce so pode atacar de territorios de cor \"%s\".\n", PLAYER_COLOR);
                continue;
            }
            if (at->tropas <= 0) {
                puts("Territorio atacante nao tem tropas suficientes.");
                continue;
            }

            int conquered = simulate_attack(at, df);
            if (conquered) {
                // opcional: ação adicional ao conquistar (já tratado dentro de simulate_attack)
                puts("Territorio conquistado com sucesso!");
            }
        } else if (opcao == 2) {
            int ok = check_mission(&missao, mapa, N_TERR);
            if (ok) {
                puts("\n***** MISSÃO CUMPRIDA! Parabens! *****");
                break;
            } else {
                puts("\nMissao ainda nao cumprida. Continue jogando.");
            }
        } else {
            puts("Opcao invalida.");
        }
    }

    cleanup(mapa);
    puts("Programa finalizado.");
    return 0;
}

/* aloca mapa dinamicamente com calloc */
Territorio *alloc_map(void) {
    Territorio *m = calloc(N_TERR, sizeof *m);
    return m;
}

/* inicializa os territórios automaticamente */
void init_territories(Territorio *mapa, size_t n) {
    // nomes e cores predefinidos (pode alterar)
    const char *names[N_TERR]  = {"Norte", "Sul", "Leste", "Oeste", "Centro"};
    const char *colors[N_TERR] = {"Azul",  "Verde", "Vermelho", "Azul", "Verde"};

    for (size_t i = 0; i < n; ++i) {
        strncpy(mapa[i].nome, names[i], NAME_SZ-1);
        mapa[i].nome[NAME_SZ-1] = '\0';
        strncpy(mapa[i].cor, colors[i], COLOR_SZ-1);
        mapa[i].cor[COLOR_SZ-1] = '\0';
        strncpy(mapa[i].original_cor, colors[i], COLOR_SZ-1);
        mapa[i].original_cor[COLOR_SZ-1] = '\0';
        // tropas aleatórias 1..6
        mapa[i].tropas = rand() % 6 + 1;
    }
}

/* imprime o mapa de forma organizada */
void print_map(const Territorio *mapa, size_t n) {
    puts("\n--- Estado Atual do Mapa ---");
    for (size_t i = 0; i < n; ++i) {
        printf("%zu) %s | Cor: %s | Tropas: %d\n",
               i + 1, mapa[i].nome, mapa[i].cor, mapa[i].tropas);
    }
}

/* atribui aleatoriamente uma das duas missões */
Missao assign_mission(void) {
    Missao m;
    if (rand() % 2 == 0) {
        m.type = MISSION_DESTROY_GREEN;
        strncpy(m.descricao, "Destruir o exército Verde (todas as tropas Verde devem ser eliminadas).", sizeof m.descricao-1);
        m.descricao[sizeof m.descricao - 1] = '\0';
        m.target = 0;
    } else {
        m.type = MISSION_CONQUER_3;
        m.target = 3;
        strncpy(m.descricao, "Conquistar 3 territórios (novos territorios colocados sob sua cor).", sizeof m.descricao-1);
        m.descricao[sizeof m.descricao - 1] = '\0';
    }
    return m;
}

void print_mission(const Missao *m) {
    puts("\n--- Missao atual ---");
    printf("%s\n", m->descricao);
    if (m->type == MISSION_CONQUER_3) {
        printf("Objetivo: %d territorios.\n", m->target);
    }
}

/* Simula um ataque entre atacante e defensor.
   Retorna 1 se o defensor foi conquistado como resultado deste ataque, caso contrário 0.
   Regras:
     - cada lado rola 1d6
     - empate favorece o atacante
     - se atacante vence (>=) -> defensor perde 1 tropa
         - se defensor.tropas <= 0 -> território conquistado (def.cor vira atk.cor, tropas = 1)
     - caso contrário -> atacante perde 1 tropa
*/
int simulate_attack(Territorio *atacante, Territorio *defensor) {
    int dadoA = rand() % 6 + 1;
    int dadoD = rand() % 6 + 1;

    printf("\n--- BATALHA ---\n");
    printf("%s (cor: %s, tropas: %d) tirou %d\n", atacante->nome, atacante->cor, atacante->tropas, dadoA);
    printf("%s (cor: %s, tropas: %d) tirou %d\n", defensor->nome, defensor->cor, defensor->tropas, dadoD);

    if (dadoA >= dadoD) { // atacante vence em empate
        defensor->tropas -= 1;
        printf("Resultado: atacante vence a rodada. %s perde 1 tropa (restam %d).\n", defensor->nome, defensor->tropas);
        if (defensor->tropas <= 0) {
            // conquistado
            printf(">>> %s foi conquistado pelo exército %s!\n", defensor->nome, atacante->cor);
            // muda a cor para a cor do atacante
            strncpy(defensor->cor, atacante->cor, COLOR_SZ-1);
            defensor->cor[COLOR_SZ-1] = '\0';
            defensor->tropas = 1; // tropa mínima de ocupação
            return 1;
        }
    } else {
        atacante->tropas -= 1;
        printf("Resultado: defensor vence a rodada. %s perde 1 tropa (restam %d).\n", atacante->nome, atacante->tropas);
    }
    return 0;
}

/* Avalia se a missão foi cumprida com base no mapa atual. */
int check_mission(const Missao *m, const Territorio *mapa, size_t n) {
    if (m->type == MISSION_DESTROY_GREEN) {
        for (size_t i = 0; i < n; ++i) {
            if (strcmp(mapa[i].cor, "Verde") == 0 && mapa[i].tropas > 0) {
                return 0; // ainda existem tropas verdes
            }
        }
        return 1; // não há mais tropas verdes
    } else if (m->type == MISSION_CONQUER_3) {
        int conquered = count_new_conquests(mapa, n, PLAYER_COLOR);
        printf("Territorios conquistados (novos) atualmente: %d / %d\n", conquered, m->target);
        return conquered >= m->target;
    }
    return 0;
}

/* Conta territórios cuja cor atual é player_color mas a cor original era diferente.
   Isso representa conquistas novas pelo jogador.
*/
int count_new_conquests(const Territorio *mapa, size_t n, const char *player_color) {
    int cnt = 0;
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(mapa[i].cor, player_color) == 0 && strcmp(mapa[i].original_cor, player_color) != 0) {
            ++cnt;
        }
    }
    return cnt;
}

void cleanup(Territorio *mapa) {
    free(mapa);
}
