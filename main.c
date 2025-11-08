#include <stdio.h>
#include <allegro5/allegro.h>

int main(int argc, char **argv) {
    // Variáveis principais
    ALLEGRO_DISPLAY *display = NULL;
    ALLEGRO_EVENT_QUEUE *event_queue = NULL;

    // 1. Inicialização da biblioteca principal
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar a Allegro 5.\n");
        return -1;
    }

    // 2. Instalação do teclado (para podermos fechar com ESC ou qualquer tecla)
    if (!al_install_keyboard()) {
        fprintf(stderr, "Falha ao inicializar o teclado.\n");
        return -1;
    }

    // 3. Criação da janela (Display)
    display = al_create_display(800, 600);
    if (!display) {
        fprintf(stderr, "Falha ao criar o display.\n");
        return -1;
    }
    al_set_window_title(display, "Teste Allegro 5 - Funcionou!");

    // 4. Criação da fila de eventos
    event_queue = al_create_event_queue();
    if (!event_queue) {
        fprintf(stderr, "Falha ao criar a fila de eventos.\n");
        al_destroy_display(display);
        return -1;
    }

    // Registra as fontes de eventos (janela e teclado)
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    // 5. Loop principal
    bool running = true;
    while (running) {
        ALLEGRO_EVENT ev;
        ALLEGRO_TIMEOUT timeout;
        al_init_timeout(&timeout, 0.06); // Aguarda um pouco por eventos

        // Verifica se há eventos na fila
        bool get_event = al_wait_for_event_until(event_queue, &ev, &timeout);

        if (get_event) {
            // Se clicou no X da janela
            if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                running = false;
            }
            // Se pressionou alguma tecla
            else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                running = false;
            }
        }

        // Desenha na tela
        // Limpa a tela com uma cor (R, G, B) - Azul escuro neste exemplo
        al_clear_to_color(al_map_rgb(0, 19, 77));
        
        // Atualiza o display (flip buffers)
        al_flip_display();
    }

    // 6. Limpeza de memória
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);

    return 0;
}