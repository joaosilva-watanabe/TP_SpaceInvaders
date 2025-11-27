#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

// --- Configs gerais ---
#define SCREEN_W 800
#define SCREEN_H 600
#define FPS 60.0

// cores
#define COLOR_PLAYER al_map_rgb(50, 255, 50)
#define COLOR_ENEMY  al_map_rgb(255, 50, 50)
#define COLOR_BULLET al_map_rgb(255, 255, 0)
#define COLOR_BG     al_map_rgb(0, 0, 0)

// player
#define PLAYER_W 64
#define PLAYER_H 64
#define PLAYER_SPEED 5.0

// tiros
#define BULLET_W 4
#define BULLET_H 10
#define BULLET_SPEED 10.0
#define MAX_BULLETS 3

// inimigos
#define ENEMY_W 40
#define ENEMY_H 40
#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define ENEMY_START_SPEED 3.0
#define ENEMY_DROP_SPEED 10.0
#define SPEED_INCREMENT 0.5 

// animacao
#define EXPLOSION_ANIMATION_SPEED 8
#define NUM_EXPLOSION_FRAMES 7 
#define MAX_EXPLOSIONS 10 
#define ANIMATION_SPEED 20 
#define NUM_FRAMES 2 

// Configuracao de High Scores
#define MAX_HIGHSCORES 5
#define SCORE_FILENAME "records.bin" 
#define MAX_NAME_LEN 10

// Estados do Jogo
typedef enum {
    STATE_MENU, // janela de menu 
    STATE_INPUT_NAME, // janela do input do nome do player 
    STATE_PLAYING, // jogo operando 
    STATE_GAME_OVER, // fim de jogo 
    STATE_HIGHSCORES // janela de scores 
} GameState;

typedef struct {
    char name[MAX_NAME_LEN + 1];
    int score;
} Record;

// structs do jogo
typedef struct {
    float x, y;
    int w, h;
    bool active;
} Bullet;

typedef struct {
    float x, y;
    int w, h;
    bool alive;
} Enemy;

typedef struct {
    float x, y;
    int w, h;
    int lives;
    int score;
} Player;

typedef struct {
    ALLEGRO_BITMAP* spritesheet;
    ALLEGRO_BITMAP* frames[NUM_FRAMES];
    int current_frame;
    int frame_counter;
} SpriteManager;

typedef struct {
    float x, y;
    int current_frame;
    int frame_counter;
    bool active;
} Explosion;

typedef struct {
    ALLEGRO_BITMAP* spritesheet;
    ALLEGRO_BITMAP* frames[NUM_EXPLOSION_FRAMES];
} ExplosionSpriteManager;

// definicoes globais
Player player;
Bullet bullets[MAX_BULLETS];
Enemy enemies[ENEMY_ROWS][ENEMY_COLS];
SpriteManager enemy_sprites;
SpriteManager player_sprites;
ExplosionSpriteManager explosion_sprites;
Explosion explosions[MAX_EXPLOSIONS];
ALLEGRO_BITMAP* background = NULL;
float enemy_dx = ENEMY_START_SPEED;
int enemies_remaining;
int level = 1; 

// Variaveis de controle de estado e recordes
GameState state = STATE_MENU;
Record high_scores[MAX_HIGHSCORES]; // array de structs -> nome do player e pontuacao
int menu_option = 0; 

// Variaveis para captura de nome
char input_name[MAX_NAME_LEN + 1] = "";
int input_pos = 0;

// funcoes do arquivo binario: 
void load_scores() {
    FILE *file = fopen(SCORE_FILENAME, "rb");
    if (file) {
        fread(high_scores, sizeof(Record), MAX_HIGHSCORES, file);
        fclose(file);
    } else {
        // Se nao existir nomes, zera tudo e coloca nomes padrao
        for(int i=0; i<MAX_HIGHSCORES; i++) {
            high_scores[i].score = 0;
            sprintf(high_scores[i].name, "Vazio");
        }
    }
}

void save_scores() {
    FILE *file = fopen(SCORE_FILENAME, "wb");
    if (file) {
        fwrite(high_scores, sizeof(Record), MAX_HIGHSCORES, file);
        fclose(file);
    }
}

// Compara scores para o qsort (decrescente)
int compare_scores(const void *a, const void *b) {
    Record *recA = (Record *)a;
    Record *recB = (Record *)b;
    return (recB->score - recA->score);
}

// Adiciona score e nome
void add_score(int score, const char* player_name) {
    // Verifica se entra no ranking 
    if (score > high_scores[MAX_HIGHSCORES - 1].score) {
        high_scores[MAX_HIGHSCORES - 1].score = score;
        
        // Copia o nome
        strncpy(high_scores[MAX_HIGHSCORES - 1].name, player_name, MAX_NAME_LEN);
        high_scores[MAX_HIGHSCORES - 1].name[MAX_NAME_LEN] = '\0'; // terminador 

        // Ordenacao
        qsort(high_scores, MAX_HIGHSCORES, sizeof(Record), compare_scores);
        save_scores();
    }
}

// Funcoes de Inicializacao 
bool init_sprites(const char* filename, int frame_width, int frame_height) {
    enemy_sprites.spritesheet = al_load_bitmap(filename);
    if (!enemy_sprites.spritesheet) return false;

    for (int i = 0; i < NUM_FRAMES; i++) {
        enemy_sprites.frames[i] = al_create_sub_bitmap(
            enemy_sprites.spritesheet, 0, i * frame_height, frame_width, frame_height
        );
    }
    enemy_sprites.current_frame = 0;
    enemy_sprites.frame_counter = 0;
    return true;
}

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

bool init_explosion_sprites(const char* filename, int frame_width, int frame_height) {
    explosion_sprites.spritesheet = al_load_bitmap(filename);
    if (!explosion_sprites.spritesheet) return false;

    for (int i = 0; i < NUM_EXPLOSION_FRAMES; i++) {
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

void reset_level() {
    player.x = SCREEN_W / 2 - player.w / 2;
    player.y = SCREEN_H - 60;

    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;

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
    
    // <--- ALTERADO: Velocidade agora depende do nível
    enemy_dx = ENEMY_START_SPEED + ((level - 1) * SPEED_INCREMENT);
}

void start_new_game() {
    player.w = PLAYER_W;
    player.h = PLAYER_H;
    player.lives = 3;
    player.score = 0;
    level = 1; // Reseta para 1 quando começar novo jogo
    reset_level();
    state = STATE_PLAYING;
}

//Prepara o estado de input
void start_input_name() {
    state = STATE_INPUT_NAME;
    memset(input_name, 0, sizeof(input_name));
    input_pos = 0;
}

// Logica do Jogo 

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

void update_explosions() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            explosions[i].frame_counter++;
            if (explosions[i].frame_counter >= EXPLOSION_ANIMATION_SPEED) {
                explosions[i].frame_counter = 0;
                explosions[i].current_frame++;
                if (explosions[i].current_frame >= NUM_EXPLOSION_FRAMES) {
                    explosions[i].active = false;
                }
            }
        }
    }
}

void update_animation() {
    enemy_sprites.frame_counter++;
    if (enemy_sprites.frame_counter >= ANIMATION_SPEED) {
        enemy_sprites.frame_counter = 0;
        enemy_sprites.current_frame = (enemy_sprites.current_frame + 1) % NUM_FRAMES;
    }
    player_sprites.frame_counter++;
    if (player_sprites.frame_counter >= ANIMATION_SPEED) {
        player_sprites.frame_counter = 0;
        player_sprites.current_frame = (player_sprites.current_frame + 1) % NUM_FRAMES;
    }
}

void fire_bullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = player.x + (player.w / 2) - (BULLET_W / 2);
            bullets[i].y = player.y;
            bullets[i].w = BULLET_W;
            bullets[i].h = BULLET_H;
            bullets[i].active = true;
            break; 
        }
    }
}

bool colisao(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

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

void draw_explosions() {
    if (!explosion_sprites.spritesheet) return;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            al_draw_bitmap(explosion_sprites.frames[explosions[i].current_frame], explosions[i].x, explosions[i].y, 0);
        }
    }
}

void logica() {
    update_animation();
    update_explosions();

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y -= BULLET_SPEED;
            if (bullets[i].y < 0) bullets[i].active = false;

            for (int r = 0; r < ENEMY_ROWS; r++) {
                for (int c = 0; c < ENEMY_COLS; c++) {
                    if (enemies[r][c].alive && bullets[i].active) {
                        if (colisao(bullets[i].x, bullets[i].y, bullets[i].w, bullets[i].h,
                                    enemies[r][c].x, enemies[r][c].y, enemies[r][c].w, enemies[r][c].h)) {
                            create_explosion(enemies[r][c].x, enemies[r][c].y);
                            enemies[r][c].alive = false;
                            bullets[i].active = false;
                            player.score += 10;
                            enemies_remaining--;
                        }
                    }
                }
            }
        }
    }

    bool touch_edge = false;
    bool game_over_trigger = false;

    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (enemies[r][c].alive) {
                enemies[r][c].x += enemy_dx;
                if (enemies[r][c].x <= 0 || enemies[r][c].x + ENEMY_W >= SCREEN_W) {
                    touch_edge = true;
                }
                if (enemies[r][c].y + ENEMY_H >= player.y) {
                    game_over_trigger = true;
                }
            }
        }
    }

    if (touch_edge) {
        enemy_dx = -enemy_dx;
        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                enemies[r][c].y += ENEMY_DROP_SPEED;
            }
        }
    }

    if (game_over_trigger) {
        add_score(player.score, input_name);
        state = STATE_GAME_OVER;
    }

    if (enemies_remaining == 0) {
        level++; // Aumenta o nivel quando termina a fase
        reset_level();
    }
}

void graficos(ALLEGRO_FONT* font) {
    al_clear_to_color(COLOR_BG);

    if (background) {
        al_draw_scaled_bitmap(background, 
            0, 0, al_get_bitmap_width(background), al_get_bitmap_height(background),
            0, 0, SCREEN_W, SCREEN_H, 0);
    }

    if (state == STATE_MENU) {
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W/2, 150, ALLEGRO_ALIGN_CENTER, "SPACE INVADERS");
        al_draw_text(font, (menu_option == 0) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), SCREEN_W/2, 300, ALLEGRO_ALIGN_CENTER, "JOGAR");
        al_draw_text(font, (menu_option == 1) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), SCREEN_W/2, 350, ALLEGRO_ALIGN_CENTER, "RECORDES");
        al_draw_text(font, (menu_option == 2) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255), SCREEN_W/2, 400, ALLEGRO_ALIGN_CENTER, "SAIR");
        al_draw_text(font, al_map_rgb(150, 150, 150), SCREEN_W/2, 500, ALLEGRO_ALIGN_CENTER, "Use SETAS e ENTER");

    } else if (state == STATE_INPUT_NAME) {
        // Desenha tela de input
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W/2, 200, ALLEGRO_ALIGN_CENTER, "DIGITE SEU NOME:");
        
        // Desenha caixa de texto
        al_draw_rectangle(SCREEN_W/2 - 100, 240, SCREEN_W/2 + 100, 280, al_map_rgb(0, 255, 0), 2);
        
        // Desenha o nome que está sendo digitado e um cursor piscante 
        al_draw_textf(font, al_map_rgb(255, 255, 0), SCREEN_W/2, 250, ALLEGRO_ALIGN_CENTER, "%s_", input_name);

        al_draw_text(font, al_map_rgb(150, 150, 150), SCREEN_W/2, 350, ALLEGRO_ALIGN_CENTER, "Pressione ENTER para iniciar");
        al_draw_text(font, al_map_rgb(150, 150, 150), SCREEN_W/2, 370, ALLEGRO_ALIGN_CENTER, "Pressione ESC para voltar");

    } else if (state == STATE_HIGHSCORES) {
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W/2, 100, ALLEGRO_ALIGN_CENTER, "TOP 5 RECORDES");
        
        for(int i=0; i<MAX_HIGHSCORES; i++) {
            // Exibe Nome e Score
            al_draw_textf(font, al_map_rgb(50, 255, 50), SCREEN_W/2, 200 + (i * 40), ALLEGRO_ALIGN_CENTER, 
                          "%d. %-10s ..... %05d", i+1, high_scores[i].name, high_scores[i].score);
        }
        al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W/2, 500, ALLEGRO_ALIGN_CENTER, "Pressione ESC ou ENTER para voltar");

    } else if (state == STATE_PLAYING) {
        al_draw_bitmap(player_sprites.frames[player_sprites.current_frame], player.x, player.y, 0);

        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (enemies[r][c].alive) {
                    if (enemy_sprites.spritesheet) {
                        al_draw_bitmap(enemy_sprites.frames[enemy_sprites.current_frame], enemies[r][c].x, enemies[r][c].y, 0);
                    } else {
                        al_draw_filled_rectangle(enemies[r][c].x, enemies[r][c].y,
                            enemies[r][c].x + enemies[r][c].w, enemies[r][c].y + enemies[r][c].h, COLOR_ENEMY);
                    }
                }
            }
        }
        draw_explosions();
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                al_draw_filled_rectangle(bullets[i].x, bullets[i].y, bullets[i].x + bullets[i].w, bullets[i].y + bullets[i].h, COLOR_BULLET);
            }
        }
        
        // Exibe o nome, pontuacao e nivel 
        al_draw_multiline_textf(
        font, al_map_rgb(255, 255, 255), 10, 10, 800, al_get_font_line_height(font), 0, "Jogador: %s\n \nPontos: %d\n \nNível: %d", input_name, player.score, level
    );
    } else if (state == STATE_GAME_OVER) {
        al_draw_text(font, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2 - 20, ALLEGRO_ALIGN_CENTER, "GAME OVER");
        al_draw_textf(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 20, ALLEGRO_ALIGN_CENTER, "%s fez %d pontos (Nível %d)", input_name, player.score, level); // <--- Mostra o nível no game over também
        al_draw_text(font, al_map_rgb(200, 200, 200), SCREEN_W / 2, SCREEN_H / 2 + 60, ALLEGRO_ALIGN_CENTER, "Pressione R --> Menu");
    }

    al_flip_display();
}

int main() {
    if (!al_init()) return -1;
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon(); 
    
    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    init_sprites("Alien-2.png", ENEMY_W, ENEMY_H);
    init_spritess("Player.png", PLAYER_W, PLAYER_H);
    init_explosion_sprites("ExplosionSetPRE2.png", ENEMY_W - 9, ENEMY_H - 9); 
    init_background("Fundo.png"); 
    
    load_scores();
    
    bool running = true;
    bool redraw = true;
    bool key[ALLEGRO_KEY_MAX] = { false };

    al_start_timer(timer);

    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER) {
            if (state == STATE_PLAYING) {
                if (key[ALLEGRO_KEY_LEFT] && player.x > 0) player.x -= PLAYER_SPEED;
                if (key[ALLEGRO_KEY_RIGHT] && player.x < SCREEN_W - player.w) player.x += PLAYER_SPEED;
                if (key[ALLEGRO_KEY_SPACE]) {
                      fire_bullet();
                      key[ALLEGRO_KEY_SPACE] = false;
                }
                logica();
            } 
            redraw = true;
        } 
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        } 
        // Tratamento de digitacao de texto
        else if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (state == STATE_INPUT_NAME) {
                // Se apertar ENTER, valida o nome e começa o jogo
                if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    if (strlen(input_name) > 0) { // bloqueia caso nao tenha inserido o nome
                        start_new_game();
                    }
                }
                // Se apertar ESC, volta
                else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    state = STATE_MENU;
                }
                // Backspace (apagar)
                else if (event.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
                    if (input_pos > 0) {
                        input_pos--;
                        input_name[input_pos] = '\0';
                    }
                }
                // Caracteres imprimoveis
                else {
                    char c = event.keyboard.unichar;
                    if (input_pos < MAX_NAME_LEN && c >= 32 && c <= 126) { // ASCII imprimivel
                        input_name[input_pos] = c;
                        input_pos++;
                        input_name[input_pos] = '\0';
                    }
                }
            }
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            key[event.keyboard.keycode] = true;

            if (state == STATE_MENU) {
                if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
                    menu_option--;
                    if (menu_option < 0) menu_option = 2;
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                    menu_option++;
                    if (menu_option > 2) menu_option = 0;
                }
                else if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    if (menu_option == 0) start_input_name(); // Vai para input nome
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
                }
            }
        } 
        else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            if (event.keyboard.keycode != ALLEGRO_KEY_SPACE) {
                key[event.keyboard.keycode] = false;
            }
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            graficos(font);
            redraw = false;
        }
    }

    destroy_sprites();
    if (background) al_destroy_bitmap(background);
    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}