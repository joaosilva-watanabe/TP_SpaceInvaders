#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>
#include <stdio.h>

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

// animação da explosão
#define EXPLOSION_ANIMATION_SPEED 8 // (menor = mais rápido)
#define NUM_EXPLOSION_FRAMES 7 
#define MAX_EXPLOSIONS 10 

// animação
#define ANIMATION_SPEED 20 // quanto maior, mais lenta a animação
#define NUM_FRAMES 2 // número de frames na spritesheet

// structs
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

// Estrutura para gerenciar sprites
typedef struct {
    ALLEGRO_BITMAP* spritesheet;
    ALLEGRO_BITMAP* frames[NUM_FRAMES];
    int current_frame;
    int frame_counter;
} SpriteManager;

// Estrutura para gerenciar explosões individuais
typedef struct {
    float x, y;
    int current_frame;
    int frame_counter;
    bool active;
} Explosion;

// Estrutura para gerenciar sprites de explosão
typedef struct {
    ALLEGRO_BITMAP* spritesheet;
    ALLEGRO_BITMAP* frames[NUM_EXPLOSION_FRAMES];
} ExplosionSpriteManager;

// definições globais
Player player;
Bullet bullets[MAX_BULLETS];
Enemy enemies[ENEMY_ROWS][ENEMY_COLS];
SpriteManager enemy_sprites;
SpriteManager player_sprites;
ExplosionSpriteManager explosion_sprites;
Explosion explosions[MAX_EXPLOSIONS]; // Array de explosões ativas
ALLEGRO_BITMAP* background = NULL;
float enemy_dx = ENEMY_START_SPEED;
bool game_over = false;
int enemies_remaining;

// Inicializar sprites
bool init_sprites(const char* filename, int frame_width, int frame_height) {
    enemy_sprites.spritesheet = al_load_bitmap(filename);
    
    if (!enemy_sprites.spritesheet) {
        printf("Erro: não foi possível carregar '%s'\n", filename);
        printf("Usando retângulos coloridos como fallback.\n");
        return false;
    }

    for (int i = 0; i < NUM_FRAMES; i++) {
        enemy_sprites.frames[i] = al_create_sub_bitmap(
            enemy_sprites.spritesheet,
            0, i * frame_height,
            frame_width, frame_height
        );
    }

    enemy_sprites.current_frame = 0;
    enemy_sprites.frame_counter = 0;
    
    return true;
}

bool init_spritess(const char* filename, int frame_width, int frame_height) {
    player_sprites.spritesheet = al_load_bitmap(filename);
    
    if (!player_sprites.spritesheet) {
        printf("Erro: não foi possível carregar '%s'\n", filename);
        printf("Usando retângulos coloridos como fallback.\n");
        return false;
    }

    for (int i = 0; i < NUM_FRAMES; i++) {
        player_sprites.frames[i] = al_create_sub_bitmap(
            player_sprites.spritesheet,
            0, i * frame_height,
            frame_width, frame_height
        );
    }

    player_sprites.current_frame = 0;
    player_sprites.frame_counter = 0;
    
    return true;
}

bool init_explosion_sprites(const char* filename, int frame_width, int frame_height) {
    explosion_sprites.spritesheet = al_load_bitmap(filename);
    
    if (!explosion_sprites.spritesheet) {
        printf("Erro: não foi possível carregar '%s'\n", filename);
        return false;
    }

    for (int i = 0; i < NUM_EXPLOSION_FRAMES; i++) {
        explosion_sprites.frames[i] = al_create_sub_bitmap(
            explosion_sprites.spritesheet,
            i * frame_width,  // Mudou: agora é i * frame_width (horizontal)
            0,                 // Mudou: agora é 0 (não muda a altura)
            frame_width, 
            frame_height
        );
    }
    
    return true;
}

// Inicializar background
bool init_background(const char* filename) {
    background = al_load_bitmap(filename);
    
    if (!background) {
        printf("Aviso: não foi possível carregar o background '%s'\n", filename);
        printf("Usando cor sólida como fallback.\n");
        return false;
    }
    
    return true;
}

// Criar uma nova explosão
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

// Atualizar explosões
void update_explosions() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            explosions[i].frame_counter++;
            
            if (explosions[i].frame_counter >= EXPLOSION_ANIMATION_SPEED) {
                explosions[i].frame_counter = 0;
                explosions[i].current_frame++;
                
                // Se chegou ao último frame, desativa a explosão
                if (explosions[i].current_frame >= NUM_EXPLOSION_FRAMES) {
                    explosions[i].active = false;
                }
            }
        }
    }
}

// Renderizar explosões
void draw_explosions() {
    if (!explosion_sprites.spritesheet) return;
    
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosions[i].active) {
            al_draw_bitmap(
                explosion_sprites.frames[explosions[i].current_frame],
                explosions[i].x,
                explosions[i].y,
                0
            );
        }
    }
}

// Atualizar animação
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

// Destruir sprites
void destroy_sprites() {
    if (enemy_sprites.spritesheet) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            if (enemy_sprites.frames[i]) {
                al_destroy_bitmap(enemy_sprites.frames[i]);
            }
        }
        al_destroy_bitmap(enemy_sprites.spritesheet);
    }
    if (player_sprites.spritesheet) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            if (player_sprites.frames[i]) {
                al_destroy_bitmap(player_sprites.frames[i]);
            }
        }
        al_destroy_bitmap(player_sprites.spritesheet);
    }

    if (explosion_sprites.spritesheet) {
        for (int i = 0; i < NUM_EXPLOSION_FRAMES; i++) {
            if (explosion_sprites.frames[i]) {
                al_destroy_bitmap(explosion_sprites.frames[i]);
            }
        }
        al_destroy_bitmap(explosion_sprites.spritesheet);
    }
}

// inicializar jogo
void init_game() {
    player.w = PLAYER_W;
    player.h = PLAYER_H;
    player.x = SCREEN_W / 2 - player.w / 2;
    player.y = SCREEN_H - 60;
    player.lives = 3;
    player.score = 0;

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    // Inicializar explosões
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        explosions[i].active = false;
    }

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
    game_over = false;
    enemy_dx = ENEMY_START_SPEED;
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

void logica() {
    if (game_over) return;

    // Atualizar animação dos inimigos
    update_animation();
    
    // Atualizar explosões
    update_explosions();

    // Atualiza munição
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y -= BULLET_SPEED;
            if (bullets[i].y < 0) bullets[i].active = false;

            // disparo no inimigo
            for (int r = 0; r < ENEMY_ROWS; r++) {
                for (int c = 0; c < ENEMY_COLS; c++) {
                    if (enemies[r][c].alive && bullets[i].active) {
                        if (colisao(bullets[i].x, bullets[i].y, bullets[i].w, bullets[i].h,
                                    enemies[r][c].x, enemies[r][c].y, enemies[r][c].w, enemies[r][c].h)) {
                            
                            // CRIAR EXPLOSÃO QUANDO O INIMIGO MORRER
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

    // atualizar os inimigos
    bool touch_edge = false;
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (enemies[r][c].alive) {
                enemies[r][c].x += enemy_dx;
                if (enemies[r][c].x <= 0 || enemies[r][c].x + ENEMY_W >= SCREEN_W) {
                    touch_edge = true;
                }
                if (enemies[r][c].y + ENEMY_H >= player.y) {
                    game_over = true;
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

    if (enemies_remaining == 0) {
        init_game();
    }
}

void graficos(ALLEGRO_FONT* font) {
    al_clear_to_color(COLOR_BG);

    // Desenhar background se disponível
    if (background) {
        al_draw_scaled_bitmap(background, 
            0, 0, al_get_bitmap_width(background), al_get_bitmap_height(background),
            0, 0, SCREEN_W, SCREEN_H, 0);
    }

    if (!game_over) {
        // player
        al_draw_bitmap(
            player_sprites.frames[player_sprites.current_frame],
            player.x,
            player.y,
            0
        );

        // inimigos - usar sprites se disponíveis, senão retângulos
        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (enemies[r][c].alive) {
                    if (enemy_sprites.spritesheet) {
                        al_draw_bitmap(
                            enemy_sprites.frames[enemy_sprites.current_frame],
                            enemies[r][c].x,
                            enemies[r][c].y,
                            0
                        );
                    } else {
                        al_draw_filled_rectangle(
                            enemies[r][c].x, enemies[r][c].y,
                            enemies[r][c].x + enemies[r][c].w,
                            enemies[r][c].y + enemies[r][c].h,
                            COLOR_ENEMY
                        );
                    }
                }
            }
        }

        // DESENHAR EXPLOSÕES
        draw_explosions();

        // munição
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                al_draw_filled_rectangle(bullets[i].x, bullets[i].y,
                                         bullets[i].x + bullets[i].w,
                                         bullets[i].y + bullets[i].h, COLOR_BULLET);
            }
        }

        // infos
        al_draw_textf(font, al_map_rgb(255, 255, 255), 10, 10, 0, "Pontuacao: %d", player.score);

    } else {
        al_draw_text(font, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2, 
                     ALLEGRO_ALIGN_CENTER, "Perdeu otario --> se quiser jogar dnv aperta R");
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

    // Carregar sprites
    init_sprites("Alien-2.png", ENEMY_W, ENEMY_H);
    init_spritess("Player.png", PLAYER_W, PLAYER_H);
    init_explosion_sprites("ExplosionSetPRE2.png", ENEMY_W - 9, ENEMY_H - 9); 
    init_background("Fundo.png"); 

    bool running = true;
    bool redraw = true;
    bool key[ALLEGRO_KEY_MAX] = { false };

    init_game();
    al_start_timer(timer);

    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER) {
            if (!game_over) {
                if (key[ALLEGRO_KEY_LEFT] && player.x > 0) player.x -= PLAYER_SPEED;
                if (key[ALLEGRO_KEY_RIGHT] && player.x < SCREEN_W - player.w) player.x += PLAYER_SPEED;
                if (key[ALLEGRO_KEY_SPACE]) {
                     fire_bullet();
                     key[ALLEGRO_KEY_SPACE] = false;
                }
                logica();
            } else {
                if (key[ALLEGRO_KEY_R]) init_game();
            }
            redraw = true;
        } else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        } else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            key[event.keyboard.keycode] = true;
        } else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            if (event.keyboard.keycode != ALLEGRO_KEY_SPACE) {
                key[event.keyboard.keycode] = false;
            }
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            graficos(font);
            redraw = false;
        }
    }

    // Limpeza
    destroy_sprites();
    if (background) {
        al_destroy_bitmap(background);
    }
    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}