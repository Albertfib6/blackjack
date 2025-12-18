#include <libc.h>

/* ==========================================================
   DEFINICIONS I ESTRUCTURES
   ========================================================== */

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT * 2)

// Colors
#define C_BLACK 0x00
#define C_BLUE 0x01
#define C_GREEN 0x02
#define C_CYAN 0x03
#define C_RED 0x04
#define C_MAGENTA 0x05
#define C_BROWN 0x06
#define C_LGRAY 0x07
#define C_DGRAY 0x08
#define C_LBLUE 0x09
#define C_LGREEN 0x0A
#define C_LCYAN 0x0B
#define C_LRED 0x0C
#define C_LMAGENTA 0x0D
#define C_YELLOW 0x0E
#define C_WHITE 0x0F

// Combinacions de colors 
#define COL_TABLE     (C_GREEN << 4 | C_BLACK)      // Taula verda
#define COL_CARD      (C_WHITE << 4 | C_BLACK)      // Carta blanca
#define COL_CARD_RED  (C_WHITE << 4 | C_RED)        // Carta vermella 
#define COL_TEXT      (C_GREEN << 4 | C_YELLOW)     // Text normal
#define COL_HIGHLIGHT (C_BLACK << 4 | C_WHITE)      // Text ressaltat
#define COL_TITLE     (C_BLACK << 4 | C_YELLOW)     // Títol
#define COL_ERROR     (C_RED << 4 | C_WHITE)        // Error

// Caracters per marcs dobles
#define CH_TL 201 // ╔
#define CH_TR 187 // ╗
#define CH_BL 200 // ╚
#define CH_BR 188 // ╝
#define CH_HOR 205 // ═
#define CH_VER 186 // ║

// Estats del joc
enum State {
    ST_BETTING,
    ST_PLAYER_TURN,
    ST_DEALER_TURN,
    ST_GAME_OVER
};

typedef struct {
    int rango; // Rang: 1-13 (1=A, 11=J, 12=Q, 13=K)
    int palo;  // Pal: 0-3 (Piques, Cors, Trèvols, Diamants)
} Carta;

typedef struct {
    Carta cartas[10];
    int num_cartas;
    int valor;      // Valor total
    int soft;       // 1 si té un As comptat com 11
} Mano;

typedef struct {
    Carta cartas[52];
    int top;
} Mazo;

typedef struct {
    int money;
    int current_bet;
    Mazo deck;
    Mano player_hand;
    Mano dealer_hand;
    enum State state;
    char message[64];
    char last_key_pressed;
    int anim_played;
    int is_all_in;
} Game;

/* ==========================================================
   VARIABLES GLOBALS
   ========================================================== */

volatile char ultima_tecla = 0;
volatile int hi_ha_tecla = 0;
unsigned int next_rand = 1;
char global_buffer[SCREEN_SIZE]; 

/* ==========================================================
   FUNCIONS AUXILIARS
   ========================================================== */

void sleep_ticks(int ticks) {
    while (ticks-- > 0) WaitForTick(); // Milestone 4
}

int my_rand(void) {
    next_rand = next_rand * 1103515245 + 12345;
    return (unsigned int)(next_rand / 65536) % 32768;
}

void my_srand(unsigned int seed) {
    next_rand = seed;
}

void my_strcpy(char *dest, char *src) {
    while ((*dest++ = *src++));
}

/* ==========================================================
   LÒGICA DEL JOC
   ========================================================== */

void initMazo(Mazo *m) {
    int i = 0;
    for (int p = 0; p < 4; p++) {
        for (int r = 1; r <= 13; r++) {
            m->cartas[i].rango = r;
            m->cartas[i].palo = p;
            i++;
        }
    }
    m->top = 0;
    // Barrejar
    for (int k = 0; k < 1000; k++) {
        int p1 = my_rand() % 52;
        int p2 = my_rand() % 52;
        Carta temp = m->cartas[p1];
        m->cartas[p1] = m->cartas[p2];
        m->cartas[p2] = temp;
    }
}

int valorCarta(int r) {
    if (r >= 10) return 10;
    if (r == 1) return 11; // Per defecte 11
    return r;
}

void calcularMano(Mano *m) {
    m->valor = 0;
    int ases = 0;
    for (int i = 0; i < m->num_cartas; i++) {
        int v = valorCarta(m->cartas[i].rango);
        m->valor += v;
        if (m->cartas[i].rango == 1) ases++;
    }
    while (m->valor > 21 && ases > 0) {
        m->valor -= 10;
        ases--;
    }
}

void donarCarta(Mazo *m, Mano *h) {
    if (m->top >= 52) initMazo(m); // Reiniciar si s'acaba
    h->cartas[h->num_cartas++] = m->cartas[m->top++];
    calcularMano(h);
}

void resetRound(Game *g) {
    g->player_hand.num_cartas = 0;
    g->dealer_hand.num_cartas = 0;
    g->current_bet = 0;
    g->anim_played = 0;
    g->is_all_in = 0;
    
    if (g->money < 10) {
        g->state = ST_GAME_OVER;
        my_strcpy(g->message, "RUINA TOTAL! PREM [R] PER REINICIAR JOC.");
    } else {
        g->state = ST_BETTING;
        my_strcpy(g->message, "FES LA TEVA APOSTA: [1] 10$ [2] 50$ [3] 100$ [4] ALL IN");
    }
}

/* ==========================================================
   GRÀFICS
   ========================================================== */

void set_pixel(char *b, int x, int y, char c, char color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    int p = (y * SCREEN_WIDTH + x) * 2;
    b[p] = c;
    b[p + 1] = color;
}

void draw_rect(char *b, int x, int y, int w, int h, char color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            set_pixel(b, x + j, y + i, ' ', color);
        }
    }
}

void print_str(char *b, int x, int y, char *s, char color) {
    while (*s) {
        set_pixel(b, x++, y, *s++, color);
    }
}

void draw_card(char *b, int x, int y, Carta c, int hidden) {
    // Fons de la carta
    draw_rect(b, x, y, 5, 4, COL_CARD);
    
    // Marc de la carta
    char color = (c.palo == 1 || c.palo == 3) ? COL_CARD_RED : COL_CARD;
    if (hidden) color = COL_CARD; // Carta oculta sempre negra/blanca

    set_pixel(b, x, y, CH_TL, color);
    set_pixel(b, x+4, y, CH_TR, color);
    set_pixel(b, x, y+3, CH_BL, color);
    set_pixel(b, x+4, y+3, CH_BR, color);
    
    for(int i=1; i<4; i++) {
        set_pixel(b, x+i, y, CH_HOR, color);
        set_pixel(b, x+i, y+3, CH_HOR, color);
    }
    set_pixel(b, x, y+1, CH_VER, color);
    set_pixel(b, x, y+2, CH_VER, color);
    set_pixel(b, x+4, y+1, CH_VER, color);
    set_pixel(b, x+4, y+2, CH_VER, color);

    if (hidden) {
        print_str(b, x + 1, y + 1, "???", COL_CARD);
        return;
    }

    char rank[3];
    if (c.rango == 1) my_strcpy(rank, "A");
    else if (c.rango == 11) my_strcpy(rank, "J");
    else if (c.rango == 12) my_strcpy(rank, "Q");
    else if (c.rango == 13) my_strcpy(rank, "K");
    else itoa(c.rango, rank);

    char suit;
    if (c.palo == 0) suit = 6;
    else if (c.palo == 1) suit = 3;
    else if (c.palo == 2) suit = 5; 
    else suit = 4;

    // Dibuixar contingut
    set_pixel(b, x+1, y+1, rank[0], color);
    if (rank[1]) set_pixel(b, x+2, y+1, rank[1], color);
    
    set_pixel(b, x+3, y+2, suit, color);
}

void render(char *b, Game *g) {
    // 1. Fons de la taula
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            set_pixel(b, j, i, ' ', COL_TABLE); 
        }
    }

    // 2. Header
    draw_rect(b, 0, 0, SCREEN_WIDTH, 3, COL_TITLE);
    
    int cx = SCREEN_WIDTH / 2;
    print_str(b, cx - 8, 1, "CASINO BAKA BAKA", COL_TITLE);
    
    char buf[32];
    my_strcpy(buf, "DINERS: $");
    char num[10];
    itoa(g->money, num);
    int len = 0; while(buf[len]) len++;
    int k=0; while(num[k]) buf[len++] = num[k++];
    buf[len] = 0;
    print_str(b, 2, 1, buf, COL_TITLE);

    if (g->current_bet > 0) {
        my_strcpy(buf, "APOSTA: $");
        itoa(g->current_bet, num);
        len = 0; while(buf[len]) len++;
        k=0; while(num[k]) buf[len++] = num[k++];
        buf[len] = 0;
        print_str(b, 60, 1, buf, COL_TITLE);
    }

    // Regles del joc
    char rule_col = (C_GREEN << 4) | C_YELLOW;
    
    print_str(b, 10, 8, "BLACKJACK", rule_col);
    print_str(b, 10, 9, "PAYS 3 to 2", rule_col);
    
    print_str(b, SCREEN_WIDTH - 22, 8, "DEALER", rule_col);
    print_str(b, SCREEN_WIDTH - 22, 9, "STANDS ON 17", rule_col);

    // 3. Àrea Baka Baka
    int dealer_cx = 40;
    print_str(b, dealer_cx - 8, 5, "BAKA BAKA: ", COL_TEXT);
    
    if (g->state != ST_BETTING) {
        char val[10];
        if (g->state == ST_PLAYER_TURN) print_str(b, dealer_cx + 8, 5, "(?)", COL_TEXT);
        else {
            itoa(g->dealer_hand.valor, val);
            print_str(b, dealer_cx + 8, 5, val, COL_TEXT);
        }

        int num_c = g->dealer_hand.num_cartas;
        int total_w = num_c * 6; 
        int start_x = 40 - (total_w / 2);
        
        for (int i = 0; i < num_c; i++) {
            int hidden = (i == 0 && g->state == ST_PLAYER_TURN);
            draw_card(b, start_x + (i * 6), 7, g->dealer_hand.cartas[i], hidden);
        }
    }

    // 4. Àrea del jugador
    int player_cx = 40;
    print_str(b, player_cx - 8, 14, "ALUMNE SOA: ", COL_TEXT);
    
    if (g->state != ST_BETTING) {
        char val[10];
        itoa(g->player_hand.valor, val);
        print_str(b, player_cx + 8, 14, val, COL_TEXT);

        int num_c = g->player_hand.num_cartas;
        int total_w = num_c * 6;
        int start_x = 40 - (total_w / 2);

        for (int i = 0; i < num_c; i++) {
            draw_card(b, start_x + (i * 6), 16, g->player_hand.cartas[i], 0);
        }
    }

    // 5. Barra de missatges
    draw_rect(b, 0, 22, SCREEN_WIDTH, 3, COL_HIGHLIGHT);
    print_str(b, 2, 23, g->message, COL_HIGHLIGHT);

    char keymsg[20] = "LAST KEY: ";
    keymsg[10] = g->last_key_pressed ? g->last_key_pressed : '-';
    keymsg[11] = 0;
    print_str(b, 65, 23, keymsg, COL_HIGHLIGHT);
}

void draw_fancy_box(char *b, int x, int y, int w, int h, char color) {
    draw_rect(b, x, y, w, h, color);
    
    set_pixel(b, x, y, CH_TL, color);
    set_pixel(b, x+w-1, y, CH_TR, color);
    set_pixel(b, x, y+h-1, CH_BL, color);
    set_pixel(b, x+w-1, y+h-1, CH_BR, color);
    
    for(int i=1; i<w-1; i++) {
        set_pixel(b, x+i, y, CH_HOR, color);
        set_pixel(b, x+i, y+h-1, CH_HOR, color);
    }
    for(int i=1; i<h-1; i++) {
        set_pixel(b, x, y+i, CH_VER, color);
        set_pixel(b, x+w-1, y+i, CH_VER, color);
    }
}

void game_over_screen(char *b) {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    char color = (C_RED << 4) | C_YELLOW;
    
    draw_fancy_box(b, cx - 20, cy - 6, 40, 12, color);
    
    print_str(b, cx - 4, cy - 4, "GAME OVER", color);
    print_str(b, cx - 9, cy - 1, "EL SENYOR BAKA BAKA", color);
    print_str(b, cx - 6, cy + 1, "SEMPRE GUANYA", color);
    print_str(b, cx - 11, cy + 4, "PREM [R] PER REINICIAR", color);
}

void play_game_over_animation(char *b, Game *g) {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    char *l1 = "EL SENYOR BAKA BAKA";
    char *l2 = "SEMPRE GUANYA";

    for (int i = 0; i < 450; i++) {
        // 1. Renderitza el joc base
        render(b, g);

        if (i < 350 && (i / 5) % 2 == 0) {
            for(int y=0; y<SCREEN_HEIGHT; y+=2) {
                for(int x=(y%2); x<SCREEN_WIDTH; x+=2) {
                    set_pixel(b, x, y, ' ', (C_RED << 4) | C_RED);
                }
            }
        }

        // 3. Caixa de text 
        int shake_x = 0, shake_y = 0;
        if (i < 350) {
            shake_x = (my_rand() % 3) - 1;
            shake_y = (my_rand() % 3) - 1;
        }
        
        int bx = cx - 20 + shake_x;
        int by = cy - 6 + shake_y;

        char color = (C_RED << 4) | C_YELLOW;
        if (i < 350 && i % 10 < 5) color = (C_BLACK << 4) | C_RED; // Inverteix els colors

        draw_fancy_box(b, bx, by, 40, 12, color);

        print_str(b, bx + 16, by + 2, "GAME OVER", color);
        print_str(b, bx + 11, by + 5, l1, color);
        print_str(b, bx + 14, by + 7, l2, color);
        
           // Mostra el text de reinici suaument
        if (i > 350) {
             print_str(b, bx + 9, by + 10, "PREM [R] PER REINICIAR", color);
        }

        write(10, b, SCREEN_SIZE);
        sleep_ticks(1);
    }
}

void victory_animation(char *b, Game *g) {
    char *msg = "VICTORY!";
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    
    // Sistema de partícules confeti 
    #define MAX_PARTICLES 60
    int px[MAX_PARTICLES];
    int py[MAX_PARTICLES];
    int pc[MAX_PARTICLES]; // Color
    int ps[MAX_PARTICLES]; // Velocitat
    
    // Inicialitza les partícules
    for(int k=0; k<MAX_PARTICLES; k++) {
        px[k] = my_rand() % SCREEN_WIDTH;
        py[k] = -(my_rand() % SCREEN_HEIGHT); // Comença per sobre de la pantalla
        pc[k] = (my_rand() % 15) + 1;
        ps[k] = (my_rand() % 2) + 1; 
    }

    // Bucle d'animació 
    for (int i = 0; i < 600; i++) {
        // 1. Neteja l'escena
        render(b, g);

        // 2. Actualitza i dibuixa les partícules
        for (int k = 0; k < MAX_PARTICLES; k++) {
            py[k] += ps[k];
            if (py[k] >= SCREEN_HEIGHT) {
                py[k] = -2; // Reinicia a dalt
                px[k] = my_rand() % SCREEN_WIDTH;
            }
            
            if (py[k] >= 0) {
                char symbol = (k % 2 == 0) ? '$' : '*';
                set_pixel(b, px[k], py[k], symbol, pc[k]);
            }
        }
        
        // 3. Flaix a la capsa del missatge
        char bg_col, txt_col;
        if ((i / 10) % 2 == 0) {
            bg_col = C_YELLOW;
            txt_col = C_RED;
        } else {
            bg_col = C_RED;
            txt_col = C_YELLOW;
        }
        char color = (bg_col << 4) | txt_col;
        
        // Efecte de rebot
        int offset = (i % 20) < 10 ? 0 : 1;
        
        // Dibuixa una capsa 
        int bx = cx - 10;
        int by = cy - 3 + offset;
        draw_rect(b, bx, by, 20, 7, color);
        
        // Dibuixa la vora de la capsa
        set_pixel(b, bx, by, CH_TL, color);
        set_pixel(b, bx+19, by, CH_TR, color);
        set_pixel(b, bx, by+6, CH_BL, color);
        set_pixel(b, bx+19, by+6, CH_BR, color);
        for(int z=1; z<19; z++) {
            set_pixel(b, bx+z, by, CH_HOR, color);
            set_pixel(b, bx+z, by+6, CH_HOR, color);
        }
        for(int z=1; z<6; z++) {
            set_pixel(b, bx, by+z, CH_VER, color);
            set_pixel(b, bx+19, by+z, CH_VER, color);
        }

        print_str(b, cx - 4, cy + offset, msg, color);
        
        // 4. Mostra el fotograma
        write(10, b, SCREEN_SIZE);
        
        // 5. Retard
        sleep_ticks(1); 
    }
}

void simple_victory_animation(char *b, Game *g) {
    // Animació de victòria quan no es all in
    for (int i = 0; i < 400; i++) {
        render(b, g);

        char bg_col, txt_col;
        if ((i / 10) % 2 == 0) {
            bg_col = C_GREEN;
            txt_col = C_WHITE;
        } else {
            bg_col = C_WHITE;
            txt_col = C_GREEN;
        }
        char color = (bg_col << 4) | txt_col;

        // Dibuixa columnes de $ als laterals
        for (int y = 4; y < SCREEN_HEIGHT - 4; y++) {
            // Bloc esquerre (amplada 3)
            set_pixel(b, 2, y, '$', color);
            set_pixel(b, 3, y, '$', color);
            set_pixel(b, 4, y, '$', color);
            
            // Bloc dret (amplada 3)
            set_pixel(b, SCREEN_WIDTH - 5, y, '$', color);
            set_pixel(b, SCREEN_WIDTH - 4, y, '$', color);
            set_pixel(b, SCREEN_WIDTH - 3, y, '$', color);
        }
        
        write(10, b, SCREEN_SIZE);
        sleep_ticks(2); 
    }
}

/* ==========================================================
   MAIN
   ========================================================== */

void teclat_handler(char tecla, int pressed) {
    if (pressed) {
        ultima_tecla = tecla;
        hi_ha_tecla = 1;
    }
}

void blackjack_game(void *arg) {
    struct stats st;
    get_stats(getpid(), &st);
    my_srand(st.elapsed_total_ticks);

    char buffer[SCREEN_SIZE];
    Game game;
    game.money = 1000;
    game.last_key_pressed = 0;
    initMazo(&game.deck);
    resetRound(&game);

    int dirty = 1;
    while (1) {
        // Renderitzat
        if (dirty) {
            render(buffer, &game);
            if (game.state == ST_GAME_OVER && game.money < 10) {
                 if (!game.anim_played) {
                     play_game_over_animation(buffer, &game);
                     game.anim_played = 1;
                 }
                 game_over_screen(buffer);
            }
            write(10, buffer, SCREEN_SIZE);
            dirty = 0;
        }

        // Entrada
        if (!hi_ha_tecla) {
            yield(); 
            continue;
        }
        
        dirty = 1;
        char c = ultima_tecla;
        hi_ha_tecla = 0;
        game.last_key_pressed = c;

        switch (game.state) {
            case ST_BETTING:
                if (c == '1' && game.money >= 10) {
                    game.current_bet = 10;
                    game.money -= 10;
                    game.state = ST_PLAYER_TURN;
                } else if (c == '2' && game.money >= 50) {
                    game.current_bet = 50;
                    game.money -= 50;
                    game.state = ST_PLAYER_TURN;
                } else if (c == '3' && game.money >= 100) {
                    game.current_bet = 100;
                    game.money -= 100;
                    game.state = ST_PLAYER_TURN;
                } else if (c == '4' && game.money > 0) {
                    game.current_bet = game.money;
                    game.money = 0;
                    game.is_all_in = 1;
                    game.state = ST_PLAYER_TURN;
                }

                if (game.state == ST_PLAYER_TURN) {
                    donarCarta(&game.deck, &game.player_hand);
                    donarCarta(&game.deck, &game.player_hand);
                    donarCarta(&game.deck, &game.dealer_hand);
                    donarCarta(&game.deck, &game.dealer_hand);
                    my_strcpy(game.message, "JUGAR: [P] DEMANAR  [S] PLANTAR-SE");
                    
                    if (game.player_hand.valor == 21) {
                        game.state = ST_DEALER_TURN;
                    }
                }
                break;

            case ST_PLAYER_TURN:
                if (c == 'p' || c == 'P') {
                    donarCarta(&game.deck, &game.player_hand);
                    if (game.player_hand.valor > 21) {
                        if (game.money < 10) {
                            my_strcpy(game.message, "RUINA TOTAL! PREM [R] PER REINICIAR.");
                        } else {
                            my_strcpy(game.message, "T'HAS PASSAT! (BUST). PREM [C] PER CONTINUAR.");
                        }
                        game.state = ST_GAME_OVER;
                    }
                } else if (c == 's' || c == 'S') {
                    game.state = ST_DEALER_TURN;
                }
                break;

            case ST_GAME_OVER:
                if (game.money < 10) {
                    if (c == 'r' || c == 'R') {
                        game.money = 1000;
                        resetRound(&game);
                    }
                } else if (c == 'c' || c == 'C') {
                    resetRound(&game);
                }
                break;
                
            default: break;
        }

        // Lògica del BAKA BAKA
        if (game.state == ST_DEALER_TURN) {
            // Renderitzar una vegada per ensenyar la carta oculta
            render(buffer, &game);
            write(10, buffer, SCREEN_SIZE);
            
            // Petit delay
            sleep_ticks(10); 

            while (game.dealer_hand.valor < 17) {
                donarCarta(&game.deck, &game.dealer_hand);
                render(buffer, &game);
                write(10, buffer, SCREEN_SIZE);
                sleep_ticks(10);
            }

            // Avaluar guanyador
            int pv = game.player_hand.valor;
            int dv = game.dealer_hand.valor;
            int won = 0;
            
            // Comprovar blackjack natural (2 cartes sumant 21)
            int player_bj = (pv == 21 && game.player_hand.num_cartas == 2);
            int dealer_bj = (dv == 21 && game.dealer_hand.num_cartas == 2);

            if (pv > 21) {
                // Ja tractat abans, però per seguretat
                my_strcpy(game.message, "HAS PERDUT. PREM [C].");
            } else if (dv > 21) {
                my_strcpy(game.message, "BAKA BAKA ES PASSA! GUANYES! PREM [C].");
                game.money += game.current_bet * 2;
                won = 1;
            } else if (player_bj && !dealer_bj) {
                my_strcpy(game.message, "BLACKJACK! PAGO 3:2! PREM [C].");
                // Paga 3:2 (aposta + 1,5 * aposta) = 2,5 * aposta
                // Com que ja s'havia restat, hi sumem 2,5 * aposta
                game.money += game.current_bet + (game.current_bet * 3) / 2;
                won = 1;
            } else if (pv > dv) {
                my_strcpy(game.message, "GUANYES! PREM [C].");
                game.money += game.current_bet * 2;
                won = 1;
            } else if (pv < dv) {
                my_strcpy(game.message, "GUANYA EL SENYOR BAKA BAKA. PREM [C].");
            } else {
                // Empat (inclou que tots dos tinguin blackjack)
                my_strcpy(game.message, "EMPAT (PUSH). PREM [C].");
                game.money += game.current_bet;
            }
            
            if (game.money < 10) {
                my_strcpy(game.message, "RUINA TOTAL! PREM [R] PER REINICIAR.");
            }
            
            game.state = ST_GAME_OVER;

            // Actualitzar pantalla amb el resultat final
            render(buffer, &game);
            if (won) {
                if (game.is_all_in) {
                    victory_animation(buffer, &game);
                } else {
                    simple_victory_animation(buffer, &game);
                }
            }
            write(10, buffer, SCREEN_SIZE);
        }
    }
}

void draw_title_screen(char *b, int frame) {
    // Omple el fons - negre
    for (int i = 0; i < SCREEN_SIZE; i += 2) {
        b[i] = ' ';
        b[i+1] = (C_BLACK << 4) | C_LGRAY;
    }

    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    // Dibuixa la vora
    char border_col = (C_BLACK << 4) | C_GREEN;
    draw_rect(b, 2, 1, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 2, border_col);
    
    // Dibuixa la capsa del títol
    int bx = cx - 15;
    int by = cy - 6;
    char title_col = (C_BLACK << 4) | C_YELLOW;
    draw_fancy_box(b, bx, by, 30, 8, title_col);

    // Text del títol
    print_str(b, cx - 8, by + 2, "CASINO BAKA BAKA", title_col);
    print_str(b, cx - 4, by + 4, "BLACKJACK", title_col);

    // Decoració dels pals
    set_pixel(b, bx + 2, by + 2, 3, (C_BLACK << 4) | C_RED); // Cor
    set_pixel(b, bx + 27, by + 2, 4, (C_BLACK << 4) | C_RED); // Diamant
    set_pixel(b, bx + 2, by + 5, 5, (C_BLACK << 4) | C_WHITE); // Trèvol
    set_pixel(b, bx + 27, by + 5, 6, (C_BLACK << 4) | C_WHITE); // Pica

    // Parpelleig de "Prem una tecla"
    if ((frame / 20) % 2 == 0) {
        char blink_col = (C_BLACK << 4) | C_WHITE;
        print_str(b, cx - 11, cy + 6, "PRESS ANY KEY TO START", blink_col);
    }
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{   
     // Milestone 2: teclat
    if (KeyboardEvent(teclat_handler) < 0) {
        write(1, "Error init teclat\n", 18);
        return 1;
    }

    int frame = 0;
    while (!hi_ha_tecla) {
        // Actualitza la pantalla periòdicament
        if (frame % 50 == 0) {
             draw_title_screen(global_buffer, frame / 50);
             write(10, global_buffer, SCREEN_SIZE);
        }
        frame++;
        
        // Petit bucle de retard 
        for(int k=0; k<1000; k++); 
    }
    hi_ha_tecla = 0;

    // Milestone 1: crear un thread pel joc
    if (ThreadCreate(blackjack_game, 0) < 0) {
        write(1, "Error creating game thread\n", 27);
        return 1;
    }

    while(1);
}
