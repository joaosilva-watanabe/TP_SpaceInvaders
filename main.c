#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
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
#define PLAYER_W 40
#define PLAYER_H 20
#define PLAYER_SPEED 5.0

// tiros
#define BULLET_W 4
#define BULLET_H 10
#define BULLET_SPEED 10.0
#define MAX_BULLETS 3 // tiros na tela ao mesmo tempo

// inimigos
#define ENEMY_W 30
#define ENEMY_H 20
#define ENEMY_ROWS 5
#define ENEMY_COLS 11
#define ENEMY_START_SPEED 3.0 // velocidade inicial dos inimigos, podemos aumentar isso conforme o nivel
#define ENEMY_DROP_SPEED 10.0 // quanto os inimigos descem 

// struct's
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

// def. globais
Player player;
Bullet bullets[MAX_BULLETS];
Enemy enemies[ENEMY_ROWS][ENEMY_COLS];
float enemy_dx = ENEMY_START_SPEED;
bool game_over = false;
int enemies_remaining;

// inicializar
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

    enemies_remaining = 0;
    for (int row = 0; row < ENEMY_ROWS; row++) {
        for (int col = 0; col < ENEMY_COLS; col++) {
            enemies[row][col].w = ENEMY_W;
            enemies[row][col].h = ENEMY_H;
            // Espaçamento entre inimigos
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

// inimigo encostar na nave
bool colisao(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void logica() {
    if (game_over) return;

    // Atualiza municao
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
                // inimigo na borda
                if (enemies[r][c].x <= 0 || enemies[r][c].x + ENEMY_W >= SCREEN_W) {
                    touch_edge = true;
                }
                // inimigo atinge o jogador
                if (enemies[r][c].y + ENEMY_H >= player.y) {
                    game_over = true;
                }
            }
        }
    }

    if (touch_edge) {
        enemy_dx = -enemy_dx; // mudado a direcao dos inimigos
        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                enemies[r][c].y += ENEMY_DROP_SPEED; // descendo os inimigos
            }
        }
    }

    if (enemies_remaining == 0) {
        //reset
        init_game();
        
    }
}

void graficos(ALLEGRO_FONT* font) {
    al_clear_to_color(COLOR_BG);

    if (!game_over) {
        // player
        al_draw_filled_rectangle(player.x, player.y, player.x + player.w, player.y + player.h, COLOR_PLAYER);

        // enemy
        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (enemies[r][c].alive) {
                    al_draw_filled_rectangle(enemies[r][c].x, enemies[r][c].y,
                                             enemies[r][c].x + enemies[r][c].w,
                                             enemies[r][c].y + enemies[r][c].h, COLOR_ENEMY);
                }
            }
        }

        // municao
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
        // Tela de Game Over
        al_draw_text(font, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTER, "Perdeu otario --> se quiser jogar dnv aperta R");
    }

    al_flip_display();
}

// main --> so isso deve estar em main, o restante precisa ser modularizado 
int main() {
    if (!al_init()) return -1;
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));

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
                     key[ALLEGRO_KEY_SPACE] = false; // nao permite disparos simultaneos 
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
             // segundo o Gemini, isso permite dar cliques mais rapidos na tecla de espaco
             // antes dessa alteracao os tiros estavam meio nerfados, muito lento 
            if (event.keyboard.keycode != ALLEGRO_KEY_SPACE) {
                key[event.keyboard.keycode] = false;
            }
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            graficos(font);
            redraw = false;
        }
    }

    al_destroy_font(font);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}