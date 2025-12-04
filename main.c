// ==========================================
// CARREGAMENTO DAS BIBLIOTECAS 
// ==========================================
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

// ==========================================
// CONFIGURAÇÕES GERAIS E CONSTANTES
// ==========================================

// Dimensões da tela e taxa de atualização
#define SCREEN_W 800
#define SCREEN_H 600
#define FPS 60.0

// Definições de Cores (Mapeamento RGB)
#define COLOR_PLAYER al_map_rgb(50, 255, 50)
#define COLOR_ENEMY  al_map_rgb(255, 50, 50)
#define COLOR_BULLET al_map_rgb(255, 255, 0)
#define COLOR_BG     al_map_rgb(0, 0, 0)

// Cores específicas para Power Ups
#define COLOR_ENERGY_BAR al_map_rgb(0, 200, 255)
#define COLOR_FREEZE     al_map_rgb(100, 100, 255) 
#define COLOR_RAILGUN    al_map_rgb(0, 255, 255)   

// Atributos do Jogador (Player)
#define PLAYER_W 64
#define PLAYER_H 64
#define PLAYER_SPEED 5.0

// Atributos dos Projéteis (bullets)
#define BULLET_W 4
#define BULLET_H 10
#define BULLET_SPEED 10.0
#define MAX_BULLETS 20 // Máximo de disparos por tela 

// Configuração da Matriz de Inimigos
#define ENEMY_W 40
#define ENEMY_H 40
#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define ENEMY_START_SPEED 3.0
#define ENEMY_DROP_SPEED 10.0 // Quanto descem ao tocar na borda
#define SPEED_INCREMENT 0.5   // Aumento de velocidade por nível

// Configurações de Animação e Sprites
#define EXPLOSION_ANIMATION_SPEED 8     // Delay entre frames da explosão
#define NUM_EXPLOSION_FRAMES 7          // Total de quadros na imagem de explosão
#define MAX_EXPLOSIONS 10               // Limite de explosões simultâneas
#define ANIMATION_SPEED 20              // Velocidade de troca de sprites (player/enemy)
#define NUM_FRAMES 2                    // Quadros de animação base (movimento)

// Sistema de Recordes (High Score)
#define MAX_HIGHSCORES 5
#define SCORE_FILENAME "records.bin"    // Arquivo binário para o armazenamento de scores 
#define MAX_NAME_LEN 10

// Balanceamento dos Power Ups (Habilidades)
#define MAX_ENERGY 100
#define ENERGY_PER_KILL 5     
#define COST_MULTISHOT 20
#define COST_RAILGUN 50     
#define COST_FREEZE 100

#define FREEZE_DURATION 180   // Duração em frames (180 frames / 60 FPS = 3 segundos)
#define RAILGUN_VISUAL_TIME 15 

// Sistema de Áudio
#define NUM_LEVEL_SOUNDS 15             // Quantidade de sons variados para níveis

// ==========================================
// ESTRUTURAS DE DADOS (STRUCTS)
// ==========================================

// Máquina de Estados do Jogo (Game State Machine)
typedef enum {
    STATE_MENU, 
    STATE_INPUT_NAME, 
    STATE_PLAYING, 
    STATE_PAUSE,        
    STATE_GAME_OVER, 
    STATE_HIGHSCORES 
} GameState;

// Estrutura para salvar o recorde no arquivo binário
typedef struct {
    char name[MAX_NAME_LEN + 1];
    int score;
    int level; 
} Record;

// Entidade: Projétil
typedef struct {
    float x, y;
    float dx, dy; // Vetores de direção
    int w, h;
    bool active;  // Flag para saber se deve ser desenhado/calculado
} Bullet;

// Entidade: Inimigo
typedef struct {
    float x, y;
    int w, h;
    bool alive;   // Se false, o inimigo foi destruído
} Enemy;

// Entidade: Jogador
typedef struct {
    float x, y;
    int w, h;
    int score;
    int energy;   // Barra de "mana" para usar poderes
} Player;

// Gerenciador de Sprites (animação simples de 2 quadros)
typedef struct {
    ALLEGRO_BITMAP* spritesheet;     // Imagem original carregada
    ALLEGRO_BITMAP* frames[NUM_FRAMES]; // Sub-bitmaps recortados
    int current_frame;               // Índice do frame atual
    int frame_counter;               // Timer interno para troca de frame
} SpriteManager; 

// Entidade: Explosão (Efeito Visual)
typedef struct {
    float x, y;
    int current_frame;
    int frame_counter;
    bool active;            
} Explosion;

// Gerenciador específico para sprites de explosão (mais quadros)
typedef struct {
    ALLEGRO_BITMAP* spritesheet;                    
    ALLEGRO_BITMAP* frames[NUM_EXPLOSION_FRAMES];   
} ExplosionSpriteManager;

// ==========================================
// VARIÁVEIS GLOBAIS
// ==========================================

Player player;
Bullet bullets[MAX_BULLETS];            // Pool de tiros
Enemy enemies[ENEMY_ROWS][ENEMY_COLS];  // Grid de inimigos

// Sprites e Imagens
SpriteManager enemy_sprites; 
SpriteManager player_sprites;
ExplosionSpriteManager explosion_sprites;
Explosion explosions[MAX_EXPLOSIONS];
ALLEGRO_BITMAP* background = NULL;
ALLEGRO_BITMAP* logo = NULL;  

// Controle de Nível e Inimigos
float enemy_dx = ENEMY_START_SPEED; // Velocidade atual dos inimigos
int enemies_remaining;
int level = 1; 

// Áudio
ALLEGRO_SAMPLE *som_tiro = NULL;    // Efeitos curtos carregados na RAM
ALLEGRO_SAMPLE *som_power1 = NULL;
ALLEGRO_SAMPLE *som_power2 = NULL;
ALLEGRO_SAMPLE *sons_level[NUM_LEVEL_SOUNDS];
ALLEGRO_AUDIO_STREAM *musica = NULL; // Stream para arquivos longos (música de fundo)
ALLEGRO_SAMPLE *som_menu = NULL;
ALLEGRO_SAMPLE *som_menu2 = NULL;
ALLEGRO_SAMPLE *acabou = NULL;

// Controle de Power Ups
int freeze_timer = 0;        
int railgun_timer = 0;       
float railgun_x_pos = 0;     

// Controle de Interface e Estado
GameState state = STATE_MENU;
Record high_scores[MAX_HIGHSCORES]; 
int menu_option = 0;
int pause_option = 0; 
char input_name[MAX_NAME_LEN + 1] = "";
int input_pos = 0;

// ==========================================
// FUNÇÕES DE PERSISTÊNCIA (ARQUIVO)
// ==========================================

// Carrega os recordes do arquivo binário. Se não existir, zera a lista.
void load_scores() {
    FILE *file = fopen(SCORE_FILENAME, "rb");
    if (file) {
        fread(high_scores, sizeof(Record), MAX_HIGHSCORES, file);
        fclose(file);
    } else {
        // Inicializa com valores padrão caso seja a primeira execução
        for(int i=0; i<MAX_HIGHSCORES; i++) {
            high_scores[i].score = 0;
            high_scores[i].level = 0;
            sprintf(high_scores[i].name, "Vazio");
        }
    }
}

// Salva o estado atual do vetor de recordes no disco
void save_scores() {
    FILE *file = fopen(SCORE_FILENAME, "wb"); // "wb" -> escrita binária
    if (file) {
        fwrite(high_scores, sizeof(Record), MAX_HIGHSCORES, file);
        fclose(file);
    }
}

// Função comparadora para o qsort (ordenação decrescente de pontuação)
int compare_scores(const void *a, const void *b) {
    Record *recA = (Record *)a;
    Record *recB = (Record *)b;
    return (recB->score - recA->score);
}

// Verifica se a pontuação entra no Top 5 e atualiza a lista
void add_score(int score, int level_reached, const char* player_name) {
    // Se a pontuação for maior que a do último colocado
    if (score > high_scores[MAX_HIGHSCORES - 1].score) {
        high_scores[MAX_HIGHSCORES - 1].score = score;
        high_scores[MAX_HIGHSCORES - 1].level = level_reached;
        strncpy(high_scores[MAX_HIGHSCORES - 1].name, player_name, MAX_NAME_LEN);
        high_scores[MAX_HIGHSCORES - 1].name[MAX_NAME_LEN] = '\0'; // Garante terminador nulo
        
        // Reordena e salva
        qsort(high_scores, MAX_HIGHSCORES, sizeof(Record), compare_scores);
        save_scores();
    }
}

// ==========================================
// FUNÇÕES DE INICIALIZAÇÃO E SETUP
// ==========================================

// Carrega sprite sheet e divide em sub-bitmaps (frames) para Inimigos
bool init_sprites(const char* filename, int frame_width, int frame_height) {  
    enemy_sprites.spritesheet = al_load_bitmap(filename);
    if (!enemy_sprites.spritesheet) return false;

    for (int i = 0; i < NUM_FRAMES; i++) {
        // Cria um recorte da imagem original
        enemy_sprites.frames[i] = al_create_sub_bitmap(
            enemy_sprites.spritesheet, 0, i * frame_height, frame_width, frame_height
        );
    }
    enemy_sprites.current_frame = 0;
    enemy_sprites.frame_counter = 0; 
    return true; 
}

// Carrega sprite sheet do Jogador (mesma lógica acima)
bool init_spritess(const char* filename, int frame_width, int frame_height) {
    player_sprites.spritesheet = al_load_bitmap(filename);
    if (!player_sprites.spritesheet) return false;

    for (int i = 0; i < NUM_FRAMES; i++) {
        player_sprites.frames[i] = al_create_sub_bitmap(
            player_sprites.spritesheet, 0, i * frame_height, frame_width, frame_height
        );
    }
    player_sprites.current_frame = 0;
    player_sprites.frame_counter = 0;
    return true;
}

// Carrega sprite sheet horizontal da Explosão
bool init_explosion_sprites(const char* filename, int frame_width, int frame_height) {
    explosion_sprites.spritesheet = al_load_bitmap(filename);
    if (!explosion_sprites.spritesheet) return false;

    for (int i = 0; i < NUM_EXPLOSION_FRAMES; i++) {
        // Nota: A lógica aqui muda para 'i * frame_width' pois o spritesheet é horizontal
        explosion_sprites.frames[i] = al_create_sub_bitmap(
            explosion_sprites.spritesheet, i * frame_width, 0, frame_width, frame_height
        );
    }
    return true;
}

bool init_background(const char* filename) { 
    background = al_load_bitmap(filename);
    return (background != NULL);
}

bool init_logo(const char* filename) {
    logo = al_load_bitmap(filename);
    return (logo != NULL);
}

// Reinicia o estado da fase (posiciona inimigos, reseta balas)
void reset_level() {
    player.x = SCREEN_W / 2 - player.w / 2;
    player.y = SCREEN_H - 60;
    
    // Reseta powerups
    freeze_timer = 0;
    railgun_timer = 0;

    // Desativa projéteis e explosões
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;

    // Reposiciona a matriz de inimigos
    enemies_remaining = 0;
    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            enemies[row][col].w = ENEMY_W;
            enemies[row][col].h = ENEMY_H;
            enemies[row][col].x = 100 + col * (ENEMY_W + 15);
            enemies[row][col].y = 50 + row * (ENEMY_H + 15);
            enemies[row][col].alive = true;
            enemies_remaining++;
        }
    }
    // Aumenta a velocidade base conforme o nível
    enemy_dx = ENEMY_START_SPEED + ((level - 1) * SPEED_INCREMENT);
}

// Configurações iniciais para uma nova partida completa
void start_new_game() {
    player.w = PLAYER_W;
    player.h = PLAYER_H;
    player.score = 0;
    player.energy = 0; 
    level = 1; 
    reset_level();
    state = STATE_PLAYING;
}

// Prepara o buffer para receber o nome do jogador
void start_input_name() {
    state = STATE_INPUT_NAME;
    memset(input_name, 0, sizeof(input_name));
    input_pos = 0;
}

// Busca um slot livre no array de explosões e ativa
void create_explosion(float x, float y) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) {
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].current_frame = 0;
            explosions[i].frame_counter = 0;
            explosions[i].active = true;
            break;
        }
    }
}

// ==========================================
// LÓGICA DE TIRO E POWER UPS
// ==========================================

// Busca um slot livre no array de balas e dispara na direção informada (dx, dy)
void spawn_bullet(float x, float y, float dx, float dy) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].dx = dx;
            bullets[i].dy = dy;
            bullets[i].w = BULLET_W;
            bullets[i].h = BULLET_H;
            bullets[i].active = true;
            
            // Tocar som com variação de pitch (para não ficar repetitivo)
            if (som_tiro) {
                float pitch = 0.9f + ((float)rand() / RAND_MAX) * 0.2f;  
                al_play_sample(som_tiro, 0.5, 0.0, pitch, ALLEGRO_PLAYMODE_ONCE, NULL);      
            }
            break; 
        }
    }
}

// Disparo padrão (reto para cima)
void fire_standard_bullet() {
    spawn_bullet(player.x + (player.w / 2) - (BULLET_W / 2), player.y, 0, -BULLET_SPEED);
}

// Lógica de ativação das habilidades especiais
void activate_powerup(int type) {
    if (type == 1) { // Multi-Shot (Disparo triplo)
        if (player.energy >= COST_MULTISHOT) {
            player.energy -= COST_MULTISHOT;
            float cx = player.x + (player.w / 2) - (BULLET_W / 2);
            spawn_bullet(cx, player.y, 0, -BULLET_SPEED);      
            spawn_bullet(cx, player.y, -2.0, -BULLET_SPEED); // Diagonal esquerda
            spawn_bullet(cx, player.y, 2.0, -BULLET_SPEED);  // Diagonal direita
        }
    } 
    else if (type == 2) { // Rail Gun (Raio laser instantâneo)
        if (player.energy >= COST_RAILGUN) {
            player.energy -= COST_RAILGUN;
            railgun_timer = RAILGUN_VISUAL_TIME;
            railgun_x_pos = player.x + (player.w / 2);

            float beam_x = railgun_x_pos;
            float beam_w = 10; // Largura da área de colisão do laser
            al_play_sample(som_power1, 0.2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
            
            // Verifica colisão instantânea com todos os inimigos na coluna
            for (int r = 0; r < ENEMY_ROWS; r++) {
                for (int c = 0; c < ENEMY_COLS; c++) {
                    if (enemies[r][c].alive) {
                        if (enemies[r][c].x < beam_x + beam_w && 
                            enemies[r][c].x + enemies[r][c].w > beam_x - beam_w) {
                            
                            enemies[r][c].alive = false;
                            create_explosion(enemies[r][c].x, enemies[r][c].y);
                            player.score += 10;
                            enemies_remaining--;
                        }
                    }
                }
            }
        }
    }
    else if (type == 3) { // Time Freeze (Congela inimigos)
        if (player.energy >= COST_FREEZE) {
            player.energy -= COST_FREEZE;
            freeze_timer = FREEZE_DURATION;
            al_play_sample(som_power2, 0.2, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
        }
    }
}

// ==========================================
// ATUALIZAÇÃO E FÍSICA (GAME LOOP)
// ==========================================

// Atualiza frames da animação de explosão
void update_explosions() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            explosions[i].frame_counter++;
            // Troca o frame com base na velocidade definida
            if (explosions[i].frame_counter >= EXPLOSION_ANIMATION_SPEED) {
                explosions[i].frame_counter = 0;
                explosions[i].current_frame++;
                // Se acabou os frames, desativa a explosão
                if (explosions[i].current_frame >= NUM_EXPLOSION_FRAMES) {
                    explosions[i].active = false;
                }
            }
        }
    }
}

// Atualiza frames dos sprites (Player e Inimigos)
void update_animation() {
    // Inimigos só animam se não estiverem congelados
    if (freeze_timer == 0) {
        enemy_sprites.frame_counter++;
        if (enemy_sprites.frame_counter >= ANIMATION_SPEED) {
            enemy_sprites.frame_counter = 0;
            // Loop cíclico dos frames usando módulo (%)
            enemy_sprites.current_frame = (enemy_sprites.current_frame + 1) % NUM_FRAMES;
        }
    }
    
    player_sprites.frame_counter++;
    if (player_sprites.frame_counter >= ANIMATION_SPEED) {
        player_sprites.frame_counter = 0;
        player_sprites.current_frame = (player_sprites.current_frame + 1) % NUM_FRAMES;
    }
}

// Detecção de colisão simples (Bounding Box / Retângulo)
bool colisao(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// Libera memória de todos os bitmaps criados
void destroy_sprites() {
    if (enemy_sprites.spritesheet) {
        for (int i = 0; i < NUM_FRAMES; i++) al_destroy_bitmap(enemy_sprites.frames[i]);
        al_destroy_bitmap(enemy_sprites.spritesheet);
    }
    if (player_sprites.spritesheet) {
        for (int i = 0; i < NUM_FRAMES; i++) al_destroy_bitmap(player_sprites.frames[i]);
        al_destroy_bitmap(player_sprites.spritesheet);
    }
    if (explosion_sprites.spritesheet) {
        for (int i = 0; i < NUM_EXPLOSION_FRAMES; i++) al_destroy_bitmap(explosion_sprites.frames[i]);
        al_destroy_bitmap(explosion_sprites.spritesheet);
    }
}

// Função de desenho auxiliar para explosões
void draw_explosions() {
    if (!explosion_sprites.spritesheet) return;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            al_draw_bitmap(explosion_sprites.frames[explosions[i].current_frame], explosions[i].x, explosions[i].y, 0);
        }
    }
}

void tocar_som_level_aleatorio() {
    int indice = rand() % NUM_LEVEL_SOUNDS;
    if (sons_level[indice]) {
        al_play_sample(sons_level[indice], 0.8, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    }
}

// ==========================================
// LÓGICA PRINCIPAL DO JOGO
// ==========================================
void logica() {
    update_animation();
    update_explosions();

    // Decrementa timers dos powerups
    if (freeze_timer > 0) freeze_timer--;
    if (railgun_timer > 0) railgun_timer--;

    // Lógica dos Projéteis
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y += bullets[i].dy;
            bullets[i].x += bullets[i].dx;

            // Remove bala se sair da tela
            if (bullets[i].y < 0 || bullets[i].x < 0 || bullets[i].x > SCREEN_W) bullets[i].active = false;

            // Colisão Bala vs Inimigo
            for (int r = 0; r < ENEMY_ROWS; r++) {
                for (int c = 0; c < ENEMY_COLS; c++) {
                    if (enemies[r][c].alive && bullets[i].active) {
                        if (colisao(bullets[i].x, bullets[i].y, bullets[i].w, bullets[i].h,
                                    enemies[r][c].x, enemies[r][c].y, enemies[r][c].w, enemies[r][c].h)) {
                            
                            create_explosion(enemies[r][c].x, enemies[r][c].y);
                            enemies[r][c].alive = false;
                            bullets[i].active = false;
                            player.score += 10;
                            
                            // Ganha energia ao destruir
                            player.energy += ENERGY_PER_KILL;
                            if (player.energy > MAX_ENERGY) player.energy = MAX_ENERGY;

                            enemies_remaining--;
                        }
                    }
                }
            }
        }
    }

    // Movimentação dos Inimigos (Se não estiver congelado)
    if (freeze_timer == 0) {
        bool touch_edge = false; // Flag para indicar que tocou na parede
        bool game_over_trigger = false;

        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (enemies[r][c].alive) {
                    enemies[r][c].x += enemy_dx;
                    
                    // Verifica limites laterais
                    if (enemies[r][c].x <= 0 || enemies[r][c].x + ENEMY_W >= SCREEN_W) {
                        touch_edge = true;
                    }
                    // Verifica se chegou na altura do jogador (Game Over)
                    if (enemies[r][c].y + ENEMY_H >= player.y) {
                        game_over_trigger = true;
                    }
                }
            }
        }

        // Lógica "Cobra": Se tocar na borda, inverte direção e desce
        if (touch_edge) {
            enemy_dx = -enemy_dx;
            for (int r = 0; r < ENEMY_ROWS; r++) {
                for (int c = 0; c < ENEMY_COLS; c++) {
                    enemies[r][c].y += ENEMY_DROP_SPEED;
                }
            }
        }

        if (game_over_trigger) {
            add_score(player.score, level, input_name);
            state = STATE_GAME_OVER;
            if (acabou) {
                al_play_sample(acabou, 0.5, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
            }
        }
    }

    // Verifica vitória de nível
    if (enemies_remaining == 0) {
        level++;
        tocar_som_level_aleatorio();
        reset_level();
    }   
}

// ==========================================
// RENDERIZAÇÃO (GRÁFICOS)
// ==========================================

// Desenha os elementos durante a partida (Player, HUD, Inimigos)
void draw_game_elements(ALLEGRO_FONT* font) {
    // Desenha Player
    al_draw_bitmap(player_sprites.frames[player_sprites.current_frame], player.x, player.y, 0);

    // Desenha Inimigos
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (enemies[r][c].alive) {
                if (enemy_sprites.spritesheet) {
                    // Se estiver congelado, desenha com tinta azulada
                    if (freeze_timer > 0) {
                        al_draw_tinted_bitmap(enemy_sprites.frames[enemy_sprites.current_frame], 
                                                COLOR_FREEZE, enemies[r][c].x, enemies[r][c].y, 0);
                    } else {
                        al_draw_bitmap(enemy_sprites.frames[enemy_sprites.current_frame], enemies[r][c].x, enemies[r][c].y, 0);
                    }
                } else {
                    // Fallback se não houver sprite: retângulo colorido
                    al_draw_filled_rectangle(enemies[r][c].x, enemies[r][c].y,
                        enemies[r][c].x + enemies[r][c].w, enemies[r][c].y + enemies[r][c].h, COLOR_ENEMY);
                }
            }
        }
    }
    
    draw_explosions();
    
    // Desenha o Laser (Railgun) se ativo
    if (railgun_timer > 0) {
        al_draw_filled_rectangle(railgun_x_pos - 5, 0, railgun_x_pos + 5, player.y, COLOR_RAILGUN);
        al_draw_rectangle(railgun_x_pos - 8, 0, railgun_x_pos + 8, player.y, al_map_rgba(255, 255, 255, 100), 2);
    }

    // Desenha Balas
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            al_draw_filled_rectangle(bullets[i].x, bullets[i].y, bullets[i].x + bullets[i].w, bullets[i].y + bullets[i].h, COLOR_BULLET);
        }
    }
    
    // Desenha HUD (Texto de Pontuação e Nome)
    al_draw_multiline_textf(font, al_map_rgb(255, 255, 255), 10, 10, 800, al_get_font_line_height(font), 0, "Jogador: %s\n \nPontos: %d\n \nNível: %d", input_name, player.score, level);

    // Desenha Menu de Power Ups na tela
    al_draw_text(font, al_map_rgb(200, 200, 200), SCREEN_W / 2, 10, ALLEGRO_ALIGN_CENTER, "[1] Multi (20)  [2] RailGun (50)  [3] Freeze (100)");

    // Desenha Barra de Energia
    float bar_w = 150;
    float bar_h = 20;
    float bar_x = SCREEN_W - bar_w - 10;
    float bar_y = 10;
    
    al_draw_rectangle(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, al_map_rgb(255, 255, 255), 2); // Contorno
    float fill_w = (float)player.energy / MAX_ENERGY * bar_w;
    al_draw_filled_rectangle(bar_x, bar_y, bar_x + fill_w, bar_y + bar_h, COLOR_ENERGY_BAR); // Preenchimento
    
    // Texto numérico sobre a barra
    int font_h = al_get_font_line_height(font);
    float text_y = bar_y + (bar_h - font_h) / 2;
    al_draw_textf(font, al_map_rgb(255, 255, 255), 
                  bar_x + bar_w/2, 
                  text_y, 
                  ALLEGRO_ALIGN_CENTER, "%d/%d", player.energy, MAX_ENERGY);
}

// Função principal de desenho gerenciada pela State Machine
void graficos(ALLEGRO_FONT* font) {
    al_clear_to_color(COLOR_BG); // Limpa a tela

    if (background) {
        al_draw_scaled_bitmap(background, 
            0, 0, al_get_bitmap_width(background), al_get_bitmap_height(background),
            0, 0, SCREEN_W, SCREEN_H, 0);
    }

    // Desenho para o MENU
    if (state == STATE_MENU) {
        int logo_w = al_get_bitmap_width(logo);
        
        float logo_x = (SCREEN_W - logo_w) / 2;
        float logo_y = 60;  
        
        al_draw_bitmap(logo, logo_x, logo_y, 0);
  
        // Destaca a opção selecionada mudando a cor para Amarelo
        al_draw_text(font, (menu_option == 0) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), SCREEN_W/2, 300, ALLEGRO_ALIGN_CENTER, "JOGAR");
        al_draw_text(font, (menu_option == 1) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), SCREEN_W/2, 350, ALLEGRO_ALIGN_CENTER, "RECORDES");
        al_draw_text(font, (menu_option == 2) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), SCREEN_W/2, 400, ALLEGRO_ALIGN_CENTER, "SAIR");
        al_draw_text(font, al_map_rgb(150, 150, 150), SCREEN_W/2, 500, ALLEGRO_ALIGN_CENTER, "Use SETAS e ENTER");

    // Desenho para INPUT DE NOME
    } else if (state == STATE_INPUT_NAME) {
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W/2, 200, ALLEGRO_ALIGN_CENTER, "DIGITE SEU NOME:");
        al_draw_rectangle(SCREEN_W/2 - 100, 240, SCREEN_W/2 + 100, 280, al_map_rgb(0, 255, 0), 2);
        al_draw_textf(font, al_map_rgb(255, 255, 0), SCREEN_W/2, 250, ALLEGRO_ALIGN_CENTER, "%s_", input_name);
        al_draw_text(font, al_map_rgb(150, 150, 150), SCREEN_W/2, 350, ALLEGRO_ALIGN_CENTER, "Pressione ENTER para iniciar");

    // Desenho para LISTA DE RECORDES
    } else if (state == STATE_HIGHSCORES) {
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W/2, 100, ALLEGRO_ALIGN_CENTER, "TOP 5 RECORDES");
        al_draw_text(font, al_map_rgb(200, 200, 200), SCREEN_W/2, 150, ALLEGRO_ALIGN_CENTER, "Rank   Nome          Nível   Pontos");
        al_draw_text(font, al_map_rgb(200, 200, 200), SCREEN_W/2, 160, ALLEGRO_ALIGN_CENTER, "-----------------------------------");
        
        for(int i=0; i<MAX_HIGHSCORES; i++) {
            al_draw_textf(font, al_map_rgb(50, 255, 50), SCREEN_W/2, 200 + (i * 40), ALLEGRO_ALIGN_CENTER, 
                          "%d. %-10s   ...   Lvl %02d   ...   %05d", i+1, high_scores[i].name, high_scores[i].level, high_scores[i].score);
        }
        al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W/2, 500, ALLEGRO_ALIGN_CENTER, "Pressione ESC ou ENTER para voltar");

    // Desenho para JOGO EM ANDAMENTO
    } else if (state == STATE_PLAYING) {
        draw_game_elements(font);

    // Desenho para PAUSE
    } else if (state == STATE_PAUSE) {
        draw_game_elements(font); // Desenha o jogo ao fundo
        
        al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0, 0, 0, 150)); // Filtro escuro
        
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W/2, 200, ALLEGRO_ALIGN_CENTER, "- PAUSE -");
        
        al_draw_text(font, (pause_option == 0) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), 
                             SCREEN_W/2, 300, ALLEGRO_ALIGN_CENTER, "CONTINUAR");
                      
        al_draw_text(font, (pause_option == 1) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), 
                             SCREEN_W/2, 350, ALLEGRO_ALIGN_CENTER, "VOLTAR AO MENU");
                      
        al_draw_text(font, al_map_rgb(150, 150, 150), SCREEN_W/2, 500, ALLEGRO_ALIGN_CENTER, "Pressione P para voltar ao jogo");

    // Desenho para FIM DE JOGO
    } else if (state == STATE_GAME_OVER) {
        al_draw_text(font, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2 - 20, ALLEGRO_ALIGN_CENTER, "GAME OVER");
        al_draw_textf(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 20, ALLEGRO_ALIGN_CENTER, "%s fez %d pontos (Nível %d)", input_name, player.score, level);
        al_draw_text(font, al_map_rgb(200, 200, 200), SCREEN_W / 2, SCREEN_H / 2 + 60, ALLEGRO_ALIGN_CENTER, "Pressione R --> Menu"); 
    }

    al_flip_display(); // Atualiza a tela
}

// ==========================================
// MAIN
// ==========================================
int main() {
    // Inicialização da Biblioteca Allegro e seus Addons
    if (!al_init()) return -1;
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon(); 
    
    // Criação da Janela e Eventos
    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    // Carregamento de Recursos (Assets)
    init_sprites("imagens/Alien-2.png", ENEMY_W, ENEMY_H);
    init_spritess("imagens/Player.png", PLAYER_W, PLAYER_H);
    init_explosion_sprites("imagens/ExplosionSetPRE2.png", ENEMY_W - 9, ENEMY_H - 9); 
    init_background("imagens/Fundo.png"); 
    init_logo("imagens/logo1.png"); 

    // Inicialização do Áudio
    al_install_audio(); 
    al_init_acodec_addon(); 
    al_reserve_samples(8); // Reserva canais para tocar sons simultâneos
    
    som_tiro = al_load_sample("sons/tiro.wav");
    som_power1 = al_load_sample("sons/lase.wav");
    som_power2 = al_load_sample("sons/conge.wav");

    // Carregamento dos sons de nível
    sons_level[0] = al_load_sample("sons/level1.wav");
    sons_level[1] = al_load_sample("sons/level2.wav");
    sons_level[2] = al_load_sample("sons/level3.wav");
    sons_level[3] = al_load_sample("sons/level4.wav");
    sons_level[4] = al_load_sample("sons/level5.wav");
    sons_level[5] = al_load_sample("sons/level6.wav");
    sons_level[6] = al_load_sample("sons/level7.wav");
    sons_level[7] = al_load_sample("sons/level8.wav");
    sons_level[8] = al_load_sample("sons/level9.wav");
    sons_level[9] = al_load_sample("sons/level10.wav");
    sons_level[10] = al_load_sample("sons/level11.wav");
    sons_level[11] = al_load_sample("sons/level12.wav");
    sons_level[12] = al_load_sample("sons/level13.wav");
    sons_level[13] = al_load_sample("sons/level14.wav");
    sons_level[14] = al_load_sample("sons/level15.wav");

    // Configuração da música de fundo (Audio Stream)
    musica = al_load_audio_stream("sons/musica_fundo1.wav", 4, 1024);
    
    // Sons de UI
    som_menu = al_load_sample("sons/menu.wav");
    som_menu2 = al_load_sample("sons/menu2.wav");
    acabou = al_load_sample("sons/Acabo.wav");

    // Verificação e configuração da música
    if(!musica) {
        fprintf(stderr, "Falha ao carregar música!\n");
    } else {
        al_attach_audio_stream_to_mixer(musica, al_get_default_mixer());
        al_set_audio_stream_playmode(musica, ALLEGRO_PLAYMODE_LOOP);
        al_set_audio_stream_gain(musica, 0.2);
    }
    
    load_scores(); // Carrega recordes salvos
    
    bool running = true;
    bool redraw = true;
    bool key[ALLEGRO_KEY_MAX] = { false }; // Array para controle múltiplo de teclas

    al_start_timer(timer);

    // ==========================================
    // LOOP PRINCIPAL (GAME LOOP)
    // ==========================================
    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        // Eventos de Tempo (Lógica e Física)
        if (event.type == ALLEGRO_EVENT_TIMER) {
            if (state == STATE_PLAYING) {
                // Movimento Contínuo
                if (key[ALLEGRO_KEY_LEFT] && player.x > 0) player.x -= PLAYER_SPEED;
                if (key[ALLEGRO_KEY_RIGHT] && player.x < SCREEN_W - player.w) player.x += PLAYER_SPEED;
                
                // Disparo
                if (key[ALLEGRO_KEY_SPACE]) {
                      fire_standard_bullet();
                      key[ALLEGRO_KEY_SPACE] = false; // Impede "metralhadora" se segurar
                }

                // Power Ups
                if (key[ALLEGRO_KEY_1]) {
                    activate_powerup(1);
                    key[ALLEGRO_KEY_1] = false; 
                }
                if (key[ALLEGRO_KEY_2]) {
                    activate_powerup(2);
                    key[ALLEGRO_KEY_2] = false;
                }
                if (key[ALLEGRO_KEY_3]) {
                    activate_powerup(3);
                    key[ALLEGRO_KEY_3] = false;
                }

                logica(); // Executa física do jogo
            } 
            redraw = true; // Solicita redesenho da tela
        } 
        // Evento de Fechar Janela (X)
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        } 
        // Eventos de Teclado (Pressionar Tecla - Caractere ou Controle)
        else if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            // Captura de texto para o nome do jogador
            if (state == STATE_INPUT_NAME) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    if (strlen(input_name) > 0) {
                        start_new_game();
                    }
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    state = STATE_MENU;
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
                    if (input_pos > 0) {
                        input_pos--;
                        input_name[input_pos] = '\0';
                    }
                }
                else {
                    // Captura caracteres ASCII imprimíveis
                    char c = event.keyboard.unichar;
                    if (input_pos < MAX_NAME_LEN && c >= 32 && c <= 126) {
                        input_name[input_pos] = c;
                        input_pos++;
                        input_name[input_pos] = '\0';
                    }
                }
            }
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            key[event.keyboard.keycode] = true;

            // Controles de Navegação de Menus e Estados
            if (state == STATE_PLAYING) {
                if (event.keyboard.keycode == ALLEGRO_KEY_P) {
                    state = STATE_PAUSE;
                    al_set_audio_stream_playing(musica, false);
                }
            }
            else if (state == STATE_PAUSE) {
                if (event.keyboard.keycode == ALLEGRO_KEY_P) {
                    state = STATE_PLAYING;
                    al_set_audio_stream_playing(musica, true);
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_UP || event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                    pause_option = !pause_option; 
                    if(som_menu) al_play_sample(som_menu, 0.3, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    if (pause_option == 0) { // Continuar
                        state = STATE_PLAYING;
                        al_set_audio_stream_playing(musica, true);
                    }
                    else if (pause_option == 1) { // Voltar ao Menu
                        state = STATE_MENU;
                        menu_option = 0; 
                        al_set_audio_stream_playing(musica, true); 
                        reset_level(); 
                    }
                }
            }
            else if (state == STATE_MENU) {
                if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
                    menu_option--;
                    if (menu_option < 0) menu_option = 2;
                    if (som_menu) al_play_sample(som_menu, 0.3, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                    menu_option++;
                    if (menu_option > 2) menu_option = 0;
                    if (som_menu) al_play_sample(som_menu, 0.3, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    if (som_menu) {
                        al_play_sample(som_menu, 0.5, 0.0, 1.2, ALLEGRO_PLAYMODE_ONCE, NULL);
                        al_play_sample(som_menu2, 0.5, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }
                    
                    if (menu_option == 0) start_input_name();
                    if (menu_option == 1) state = STATE_HIGHSCORES;
                    if (menu_option == 2) running = false;
                }
            }
            else if (state == STATE_HIGHSCORES) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE || 
                    event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    state = STATE_MENU;
                }
            }
            else if (state == STATE_GAME_OVER) {
                if (event.keyboard.keycode == ALLEGRO_KEY_R) {
                    state = STATE_MENU;
                    menu_option = 0;
                }
            }
        } 
        // Evento de Soltar Tecla
        else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            // Impede que teclas de ação rápida fiquem "presas" logicamente
            if (event.keyboard.keycode != ALLEGRO_KEY_SPACE && 
                event.keyboard.keycode != ALLEGRO_KEY_1 &&
                event.keyboard.keycode != ALLEGRO_KEY_2 &&
                event.keyboard.keycode != ALLEGRO_KEY_3) {
                key[event.keyboard.keycode] = false;
            }
        }

        // Momento de desenhar na tela (renderização)
        if (redraw && al_is_event_queue_empty(queue)) {
            graficos(font);
            redraw = false;
        }
    }
    
    // ==========================================
    // LIMPEZA DE MEMÓRIA (GARBAGE COLLECTION)
    // ==========================================
    destroy_sprites();
    if (background) al_destroy_bitmap(background);
    if (logo) al_destroy_bitmap(logo); 
    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_sample(som_tiro);
    al_destroy_audio_stream(musica);
    al_destroy_sample(som_power1);
    al_destroy_sample(som_power2);
    al_destroy_sample(acabou);

    if (som_menu) al_destroy_sample(som_menu);
    if (som_menu2) al_destroy_sample(som_menu2);

    for (int i = 0; i < NUM_LEVEL_SOUNDS; i++) {
        if (sons_level[i]) {
            al_destroy_sample(sons_level[i]);
        }
    }
    
    return 0;
}