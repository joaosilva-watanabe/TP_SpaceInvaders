# README
<br>

## Criadores:
**Arthur Damasceno Dalvino, 
Inácio Guimarães Oliveira, 
João Pedro Rodrigues da Silva**

<br>

## 1. Visão Geral do Projeto  

Este projeto consiste no desenvolvimento de uma releitura moderna do clássico _Space Invaders_, construída integralmente na linguagem **C** com o suporte da biblioteca multimídia **Allegro 5**. O software foi arquitetado sobre uma Máquina de Estados Finita, garantindo uma transição fluida entre menus, jogabilidade e sistemas de pontuação.

Embora o objetivo central permaneça fiel ao original — pilotar uma nave e sobreviver a ondas de ataques alienígenas —, o projeto expande a experiência clássica com mecânicas contemporâneas. Foram implementados um **sistema de Power-ups** estratégicos (como tiros múltiplos e congelamento temporal) e um motor de áudio dinâmico que varia a frequência sonora dos disparos para evitar repetição mecânica. Além do entretenimento, a aplicação serve como demonstração prática de conceitos acadêmicos essenciais ensinados durante a disciplina.

<br>

### 1.1 Manual de Uso   


**Objetivo do Jogo:** O jogador controla uma nave espacial e deve sobreviver a ondas consecutivas de ataques alienígenas. O objetivo é destruir todos os inimigos da tela para avançar de nível, acumulando a maior pontuação possível sem deixar que os inimigos toquem a nave ou atinjam a base da tela.  


**Controles do Jogo:**

| **Ação**           | **Tecla**         | **Descrição**                                   |
| ------------------ | ----------------- | ----------------------------------------------- |
| **Mover Esquerda** | `Seta Esquerda`   | Desloca a nave para a esquerda.                 |
| **Mover Direita**  | `Seta Direita`    | Desloca a nave para a direita.                  |
| **Atirar**         | `Espaço`          | Dispara um projétil simples contra os inimigos. |
| **Pausar**         | `P`               | Interrompe o jogo momentaneamente.              |
| **Navegar Menu**   | `Setas` / `Enter` | Seleciona opções nos menus.                     |
<br>

**Habilidades Especiais (Power-ups):** O jogador possui uma barra de energia que recarrega ao destruir inimigos. Essa energia pode ser gasta para ativar poderes especiais através das teclas numéricas:

- **Tecla 1 - Multi-Shot (Custo: 20):** Dispara três projéteis simultâneos em ângulo.

- **Tecla 2 - RailGun (Custo: 50):** Dispara um laser vertical instantâneo que destrói todos os inimigos em uma coluna

- **Tecla 3 - Freeze (Custo: 100):** Congela o movimento de todos os inimigos por 3 segundos.  

<br>
<br>



## 2. Arquitetura e Estruturas de Dados

O projeto foi estruturado utilizando a linguagem C e a biblioteca Allegro 5. A base do desenvolvimento foi o [^1]paradigma procedural, onde o estado das entidades é mantido em estruturas (`structs`) e manipulado por funções específicas.

<br>

### 2.2 Entidades do Jogo

Para gerenciar os objetos em cena, definimos estruturas que agrupam coordenadas espaciais, dimensões e estados lógicos.

**Trecho de Código (Definição de Structs):**
```C
// Entidade: Inimigo
typedef struct {
    float x, y;   // Posição espacial
    int w, h;     // Dimensões para colisão (Hitbox)
    bool alive;   // Flag: false indica que deve ser ignorado no render/lógica
} Enemy;

// Gerenciador de Sprites (Otimização de memória)
typedef struct {
    ALLEGRO_BITMAP* spritesheet;    // Imagem única carregada da memória
    ALLEGRO_BITMAP* frames[NUM_FRAMES]; // Ponteiros para recortes da imagem original
    int current_frame;              // Índice atual da animação
} SpriteManager;
```

<br>
<br>


## 3. Sistema Gráfico e Animação

A renderização utiliza a técnica de _Sprite Sheets_ para economizar memória de vídeo (VRAM). Em vez de carregar múltiplos arquivos de imagem, carregamos uma única imagem e criamos "sub-bitmaps" que apontam para regiões específicas dessa textura.

<br>

### 3.1 Inicialização de Recursos

A função `init_sprites` é responsável por "fatiar" a folha de sprites.

**Trecho de Código (`init_sprites`):**
```C
bool init_sprites(const char* filename, int frame_width, int frame_height) {  
    enemy_sprites.spritesheet = al_load_bitmap(filename);
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        // Cria um recorte virtual da imagem original
        // Não aloca nova memória de pixel, apenas cria referência
        enemy_sprites.frames[i] = al_create_sub_bitmap(
            enemy_sprites.spritesheet, 0, i * frame_height, frame_width, frame_height
        );
    }
    return true; 
}
```

<br>

### 3.2 Lógica de Atualização de Quadros

A animação ocorre dentro do _Game Loop_. Utilizamos aritmética modular para criar um ciclo infinito entre os quadros disponíveis (ex: 0→1→0→1...).

**Trecho de Código (`update_animation`):**
```C
// Incrementa o contador interno
enemy_sprites.frame_counter++;

// Verifica se atingiu o tempo de troca (baseado em FPS)
if (enemy_sprites.frame_counter >= ANIMATION_SPEED) {
    enemy_sprites.frame_counter = 0;
    
    // O operador módulo (%) garante o loop cíclico da animação
    enemy_sprites.current_frame = (enemy_sprites.current_frame + 1) % NUM_FRAMES;
}
```

<br>
<br>


## 4. Física e Mecânica de Jogo

<br>

### 4.1 Detecção de Colisão 

Para verificar interações entre projéteis e inimigos, implementamos o algoritmo AABB (_Axis-Aligned Bounding Box_). Esta abordagem verifica se há sobreposição entre dois retângulos sem rotação.

**Trecho de Código (`colisao`):**
```C
// Retorna true apenas se houver intersecção em ambos os eixos
bool colisao(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 &&      // Aresta esquerda de 1 < Aresta direita de 2
            x1 + w1 > x2 &&      // Aresta direita de 1 > Aresta esquerda de 2
            y1 < y2 + h2 &&      // Aresta topo de 1 < Aresta base de 2
            y1 + h1 > y2);       // Aresta base de 1 > Aresta topo de 2
}
```

<br>

### 4.2 Inteligência Artificial dos Inimigos

Os inimigos não se movem aleatoriamente. Eles seguem um padrão determinístico: movem-se horizontalmente até tocarem a borda da tela, momento em que invertem a direção e descem uma linha.

**Trecho de Código (`logica`):**
```C
bool touch_edge = false;

// 1. Move horizontalmente
enemies[r][c].x += enemy_dx;

// 2. Verifica limites da tela
if (enemies[r][c].x <= 0 || enemies[r][c].x + ENEMY_W >= SCREEN_W) {
    touch_edge = true;
}

// 3. Se tocou na borda, aplica a lógica de descida e inversão
if (touch_edge) {
    enemy_dx = -enemy_dx; // Inverte direção (Esquerda <-> Direita)
    
    // Todos os inimigos descem uma posição fixa
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            enemies[r][c].y += ENEMY_DROP_SPEED;
        }
    }
}
```

<br>
<br>


## 5. Sistema de Power-ups (Habilidades Especiais)

Foi implementado um sistema de gerenciamento de recursos (_Mana_) onde o jogador acumula energia na variável `player.energy` ao destruir inimigos. Cada habilidade possui um custo e uma mecânica de implementação distinta, manipulando diretamente as estruturas de dados do jogo.

<br>

### 5.1 Multi-Shot (Disparo Triplo)

Esta habilidade permite cobrir uma área maior da tela disparando três projéteis simultaneamente. A implementação não cria um novo tipo de objeto, mas sim instancia três entidades `Bullet` reutilizando a _pool_ de memória existente, alterando apenas seus vetores de direção.

**Lógica de Implementação:** Ao invés de apenas um vetor vertical (dx = 0, dy = −10), calculamos vetores diagonais para os tiros laterais (dx = ±2.0), criando uma trajetória em ângulo sem uso de funções trigonométricas complexas (sen/cos), o que otimiza o processamento.

**Trecho de Código (`activate_powerup`):**
```C
if (type == 1) { // Multi-Shot
    if (player.energy >= COST_MULTISHOT) {
        player.energy -= COST_MULTISHOT;
        
        // Calcula o centro da nave para origem dos disparos
        float cx = player.x + (player.w / 2) - (BULLET_W / 2);
        
        // Instancia 3 balas com vetores de direção (dx) diferentes
        spawn_bullet(cx, player.y,  0.0, -BULLET_SPEED); // Central (Reto)
        spawn_bullet(cx, player.y, -2.0, -BULLET_SPEED); // Diagonal Esquerda
        spawn_bullet(cx, player.y,  2.0, -BULLET_SPEED); // Diagonal Direita
    }
}
```

<br>

### 5.2 RailGun (Feixe de Dano Instantâneo)

Diferente dos tiros comuns, o _RailGun_ realiza uma "varredura instantânea". Ele não cria um projétil físico que viaja frame a frame; ele calcula o dano imediatamente em toda a coluna vertical alinhada ao jogador.

**Lógica de Implementação:** O algoritmo percorre a matriz de inimigos (`enemies[ROWS][COLS]`) e verifica se a coordenada X de qualquer inimigo vivo intercepta a área do feixe (definida por `beam_x` ± `beam_w`). A eliminação e a pontuação são processadas no mesmo frame da ativação.

**Trecho de Código (`activate_powerup`):**
```C
else if (type == 2) { // Rail Gun
    if (player.energy >= COST_RAILGUN) {
        player.energy -= COST_RAILGUN;
        
        // Define a largura da área de colisão do laser (Hitbox vertical)
        float beam_x = player.x + (player.w / 2);
        float beam_w = 10; 

        // Itera sobre toda a matriz para verificar colisão na coluna atual
        for (int r = 0; r < ENEMY_ROWS; r++) {
            for (int c = 0; c < ENEMY_COLS; c++) {
                if (enemies[r][c].alive) {
                    // Verifica intersecção no eixo X
                    if (enemies[r][c].x < beam_x + beam_w && 
                        enemies[r][c].x + enemies[r][c].w > beam_x - beam_w) {
                        
                        enemies[r][c].alive = false; // Destruição imediata
                        create_explosion(enemies[r][c].x, enemies[r][c].y);
                    }
                }
            }
        }
    }
}
```

<br>

### 5.3 Time Freeze (Congelamento Temporal)

Esta habilidade suspende a lógica de atualização dos inimigos por um período determinado, oferecendo uma vantagem estratégica ao jogador.

**Lógica de Implementação:** A mecânica funciona através de uma variável de controle (`freeze_timer`). Quando ativada, o _timer_ recebe 180 frames (3 segundos a 60 FPS). No _Game Loop_ principal, condicionais (`if`) verificam esse valor: se for maior que zero, o código de movimentação (`x += dx`) e de animação de sprites é ignorado. Visualmente, a função de desenho altera a coloração do sprite para azul (`COLOR_FREEZE`) para fornecer uma indicação ao usuário.

**Trecho de Código (Lógica e Renderização):**
```C
// 1. Ativação (activate_powerup)
else if (type == 3) { 
    if (player.energy >= COST_FREEZE) {
        player.energy -= COST_FREEZE;
        freeze_timer = FREEZE_DURATION; // Define 180 frames de duração
    }
}

// 2. Efeito na Movimentação (logica)
// O bloco de código de movimento só executa se o timer for zero
if (freeze_timer == 0) {
    for (int r = 0; r < ENEMY_ROWS; r++) {
         enemies[r][c].x += enemy_dx; // Atualiza posição física
         // ... verificação de bordas ...
    }
}

// 3. Efeito Visual (draw_game_elements)
if (freeze_timer > 0) {
    // Desenha o sprite com coloração azulada se congelado
    al_draw_tinted_bitmap(enemy_sprites.frames[frame], COLOR_FREEZE, x, y, 0);
} else {
    // Desenha normal
    al_draw_bitmap(enemy_sprites.frames[frame], x, y, 0);
}
```

<br>
<br>


## 6. Sistema de Áudio Dinâmico

Para evitar a fadiga auditiva causada pela repetição de sons idênticos, foi implementada uma variação de frequência sonora em tempo real.

**Trecho de Código (`spawn_bullet`):**
```C
if (som_tiro) {
    // Gera um fator aleatório entre 0.9 (mais grave) e 1.1 (mais agudo)
    float pitch = 0.9f + ((float)rand() / RAND_MAX) * 0.2f;  
    
    // Toca o sample com a velocidade/frequência alterada
    al_play_sample(som_tiro, 0.5, 0.0, pitch, ALLEGRO_PLAYMODE_ONCE, NULL);      
}
```

<br>
<br>


## 7. Persistência de Dados

O sistema de _High Scores_ utiliza manipulação de arquivos binários (`wb`/`rb`) para garantir que os dados não sejam facilmente editáveis externamente como em arquivos de texto.

**Trecho de Código (`save_scores`):**
```C
void save_scores() {
    FILE *file = fopen(SCORE_FILENAME, "wb"); // Modo de escrita binária
    if (file) {
        // Grava o array inteiro de structs diretamente na memória do disco
        fwrite(high_scores, sizeof(Record), MAX_HIGHSCORES, file);
        fclose(file);
    }
}
```

<br>
<br>


## 8. Arquitetura de Controle: Máquina de Estados

O controle de fluxo do jogo é gerenciado por uma Máquina de Estados. Este padrão de projeto foi adotado para garantir a separação lógica entre as diferentes telas e comportamentos do sistema, evitando o uso excessivo de flags booleanas complexas e aninhadas.

<br>

### 8.1 Definição dos Estados

Os estados possíveis da aplicação foram mapeados através de uma enumeração (`enum`[^2]) na linguagem C. Isso permite que o código seja legível e que a variável de controle possua apenas um valor válido por vez.

**Trecho de Código (Definição):**
```C
typedef enum {
    STATE_MENU,        // Tela inicial com opções
    STATE_INPUT_NAME,  // Tela de captura de nome do jogador
    STATE_PLAYING,     // O loop principal do jogo (gameplay)
    STATE_PAUSE,       // Interrupção temporária
    STATE_GAME_OVER,   // Tela de fim de jogo e pontuação
    STATE_HIGHSCORES   // Visualização dos recordes salvos
} GameState;

// Variável global de controle
GameState state = STATE_MENU;
```

<br>

### 8.2 Lógica de Transição e Eventos

A variável global `state` atua como o "cérebro" da aplicação. Tanto o processamento de entradas (teclado) quanto a renderização gráfica são condicionados pelo valor atual desta variável.

A arquitetura centraliza o controle no loop principal (`main`), onde eventos idênticos podem desencadear ações diferentes dependendo do estado atual:

- **Exemplo:** A tecla `ENTER` possui comportamentos distintos:
    
    - Em `STATE_MENU`: Confirma a seleção de uma opção.
        
    - Em `STATE_INPUT_NAME`: Finaliza a digitação e inicia o jogo.
        
    - Em `STATE_PAUSE`: Confirma a opção de continuar ou sair.
        
    - Em `STATE_HIGHSCORES`: Retorna ao menu principal.

<br>

### 8.3 Implementação do Controle de Fluxo

A renderização é feita utilizando estruturas condicionais que verificam o estado atual a cada quadro (frame). Isso garante que elementos do jogo (como a nave ou inimigos) jamais sejam desenhados sobre o menu principal, e vice-versa.

**Trecho de Código (Renderização Condicional):**
```C
// Função graficos()
if (state == STATE_MENU) {
    // Desenha apenas logo e textos de menu
    draw_menu_options(); 
} 
else if (state == STATE_PLAYING) {
    // Desenha todas as entidades dinâmicas
    draw_game_elements(font); 
} 
else if (state == STATE_PAUSE) {
    // Desenha o jogo congelado ao fundo e aplica um filtro escuro
    draw_game_elements(font); 
    al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0,0,0,150));
    draw_pause_text();
}
```

<br>

### 8.4 Diagrama de Transição de Estados

A visualização abaixo ilustra o fluxo completo da aplicação, detalhando os gatilhos (inputs ou eventos de jogo) que provocam a mudança entre os estados definidos.

 -  **Nota:** A transição de `STATE_PLAYING` para `STATE_GAME_OVER` é a única automática, disparada pela lógica de colisão (inimigo atinge a base), enquanto as demais dependem de interação direta do usuário. 

<br>

<img width="913" height="970" alt="diagrama" src="https://github.com/user-attachments/assets/fd80b252-64cf-449e-926a-bc2af3dba731" />


<br>
<br>


[^1]: **Paradigma Procedural:** Estilo de programação que estrutura o código em procedimentos (funções) que executam sequências de comandos para manipular dados. Caracteriza-se pela separação entre as estruturas de dados e a lógica que as processa.

[^2]: **`enum` (Enumeração):** Tipo de dado em C definido pelo usuário que consiste em um conjunto de constantes inteiras nomeadas. É utilizado para aumentar a clareza semântica do código, substituindo literais numéricos por identificadores legíveis. Internamente, o compilador trata os valores como inteiros (`int`), iniciando, por padrão, em 0.
