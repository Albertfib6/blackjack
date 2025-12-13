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

// Combinacions de colors (Fons << 4 | Text)
#define COL_TABLE     (C_GREEN << 4 | C_BLACK)      // Taula verda
#define COL_CARD      (C_WHITE << 4 | C_BLACK)      // Carta blanca
#define COL_CARD_RED  (C_WHITE << 4 | C_RED)        // Carta vermella (Cors/Diamants - opcional)
#define COL_TEXT      (C_GREEN << 4 | C_YELLOW)     // Text normal
#define COL_HIGHLIGHT (C_BLACK << 4 | C_WHITE)      // Text ressaltat
#define COL_TITLE     (C_BLACK << 4 | C_YELLOW)     // Títol
#define COL_ERROR     (C_RED << 4 | C_WHITE)        // Error

// Estats del joc
enum State {
    ST_BETTING,
    ST_PLAYER_TURN,
    ST_DEALER_TURN,
    ST_GAME_OVER
};

typedef struct {
    int rango; // 1-13 (1=A, 11=J, 12=Q, 13=K)
    int palo;  // 0-3 (Piques, Cors, Trèvols, Diamants)
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
} Game;

/* ==========================================================
   VARIABLES GLOBALS
   ========================================================== */

volatile char ultima_tecla = 0;
volatile int hi_ha_tecla = 0;
unsigned int next_rand = 1;

/* ==========================================================
   FUNCIONS AUXILIARS
   ========================================================== */

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

void my_itoa(int n, char *buf) {
    int i = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int temp = n;
    while (temp > 0) { temp /= 10; i++; }
    buf[i] = '\0';
    while (n > 0) { buf[--i] = (n % 10) + '0'; n /= 10; }
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
    
    if (hidden) {
        print_str(b, x + 1, y + 1, "???", COL_CARD);
        return;
    }

    char rank[3];
    if (c.rango == 1) my_strcpy(rank, "A");
    else if (c.rango == 11) my_strcpy(rank, "J");
    else if (c.rango == 12) my_strcpy(rank, "Q");
    else if (c.rango == 13) my_strcpy(rank, "K");
    else my_itoa(c.rango, rank);

    char suit;
    // Piques, Cors, Trèvols, Diamants (ASCII estesos si suportats, sino lletres)
    // Utilitzarem lletres per compatibilitat: S, H, C, D
    if (c.palo == 0) suit = 'S'; // Spades
    else if (c.palo == 1) suit = 'H'; // Hearts
    else if (c.palo == 2) suit = 'C'; // Clubs
    else suit = 'D'; // Diamonds

    char color = (c.palo == 1 || c.palo == 3) ? COL_CARD_RED : COL_CARD;

    // Dibuixar cantonades
    set_pixel(b, x, y, rank[0], color);
    if (rank[1]) set_pixel(b, x+1, y, rank[1], color);
    
    set_pixel(b, x+2, y+2, suit, color);
}

void render(char *b, Game *g) {
    // 1. Fons
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            set_pixel(b, j, i, 176, COL_TABLE); // 176 és un caràcter de trama
        }
    }

    // 2. Header
    draw_rect(b, 0, 0, SCREEN_WIDTH, 3, COL_TITLE);
    print_str(b, 30, 1, "CASINO BAKA BAKA", COL_TITLE);
    
    char buf[32];
    my_strcpy(buf, "DINERS: $");
    char num[10];
    my_itoa(g->money, num);
    int len = 0; while(buf[len]) len++;
    int k=0; while(num[k]) buf[len++] = num[k++];
    buf[len] = 0;
    print_str(b, 2, 1, buf, COL_TITLE);

    if (g->current_bet > 0) {
        my_strcpy(buf, "APOSTA: $");
        my_itoa(g->current_bet, num);
        len = 0; while(buf[len]) len++;
        k=0; while(num[k]) buf[len++] = num[k++];
        buf[len] = 0;
        print_str(b, 60, 1, buf, COL_TITLE);
    }

    // 3. Dealer Area
    // Centrar text "BAKA BAKA: XX"
    int dealer_cx = 40;
    print_str(b, dealer_cx - 8, 5, "BAKA BAKA: ", COL_TEXT);
    
    if (g->state != ST_BETTING) {
        char val[10];
        if (g->state == ST_PLAYER_TURN) print_str(b, dealer_cx + 8, 5, "(?)", COL_TEXT);
        else {
            my_itoa(g->dealer_hand.valor, val);
            print_str(b, dealer_cx + 8, 5, val, COL_TEXT);
        }

        // Centrar cartes
        int num_c = g->dealer_hand.num_cartas;
        int total_w = num_c * 6; // 5 width + 1 space
        int start_x = 40 - (total_w / 2);
        
        for (int i = 0; i < num_c; i++) {
            int hidden = (i == 0 && g->state == ST_PLAYER_TURN);
            draw_card(b, start_x + (i * 6), 7, g->dealer_hand.cartas[i], hidden);
        }
    }

    // 4. Player Area
    // Centrar text "ALUMNE SOA: XX"
    int player_cx = 40;
    print_str(b, player_cx - 8, 14, "ALUMNE SOA: ", COL_TEXT);
    
    if (g->state != ST_BETTING) {
        char val[10];
        my_itoa(g->player_hand.valor, val);
        print_str(b, player_cx + 8, 14, val, COL_TEXT);

        // Centrar cartes
        int num_c = g->player_hand.num_cartas;
        int total_w = num_c * 6;
        int start_x = 40 - (total_w / 2);

        for (int i = 0; i < num_c; i++) {
            draw_card(b, start_x + (i * 6), 16, g->player_hand.cartas[i], 0);
        }
    }

    // 5. Message Bar
    draw_rect(b, 0, 22, SCREEN_WIDTH, 3, COL_HIGHLIGHT);
    print_str(b, 2, 23, g->message, COL_HIGHLIGHT);

    // 6. Debug / Last Key
    char keymsg[20] = "LAST KEY: ";
    keymsg[10] = g->last_key_pressed ? g->last_key_pressed : '-';
    keymsg[11] = 0;
    print_str(b, 65, 23, keymsg, COL_HIGHLIGHT);
}

void game_over_screen(char *b) {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    
    // Big red box
    draw_rect(b, cx - 20, cy - 6, 40, 12, (C_RED << 4) | C_YELLOW);
    
    // Text
    print_str(b, cx - 4, cy - 3, "GAME OVER", (C_RED << 4) | C_YELLOW);
    print_str(b, cx - 5, cy, "RUINA TOTAL", (C_RED << 4) | C_YELLOW);
    print_str(b, cx - 11, cy + 3, "PREM [R] PER REINICIAR", (C_RED << 4) | C_YELLOW);
}

void victory_animation(char *b) {
    char *msg = "VICTORY!";
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    
    for (int i = 0; i < 120; i++) {
        // 1. Draw random confetti
        for (int k = 0; k < 120; k++) {
            int x = my_rand() % SCREEN_WIDTH;
            int y = my_rand() % SCREEN_HEIGHT;
            char color = (my_rand() % 15) + 1;
            set_pixel(b, x, y, '*', color);
        }
        
        // 2. Flash the message box
        char bg_col = (i % 2) ? C_YELLOW : C_RED;
        char txt_col = (i % 2) ? C_RED : C_YELLOW;
        char color = (bg_col << 4) | txt_col;
        
        draw_rect(b, cx - 10, cy - 3, 20, 7, color);
        print_str(b, cx - 4, cy, msg, color);
        
        // 3. Show frame
        write(10, b, SCREEN_SIZE);
        
        // 4. Delay
        for(int d=0; d<1000000; d++);
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

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    if (KeyboardEvent(teclat_handler) < 0) {
        write(1, "Error init teclat\n", 18);
        return 1;
    }

    /* Random seed generation based on user input timing */
    write(1, "Press any key to start...\n", 26);
    while (!hi_ha_tecla);
    hi_ha_tecla = 0;
    
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
        // Render
        if (dirty) {
            render(buffer, &game);
            if (game.state == ST_GAME_OVER && game.money < 10) {
                 game_over_screen(buffer);
            }
            write(10, buffer, SCREEN_SIZE);
            dirty = 0;
        }

        // Input
        if (!hi_ha_tecla) continue;
        
        dirty = 1;
        char c = ultima_tecla;
        hi_ha_tecla = 0;
        game.last_key_pressed = c;

        if (c == 'q') break;

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
                    game.state = ST_PLAYER_TURN;
                }

                if (game.state == ST_PLAYER_TURN) {
                    donarCarta(&game.deck, &game.player_hand);
                    donarCarta(&game.deck, &game.player_hand);
                    donarCarta(&game.deck, &game.dealer_hand);
                    donarCarta(&game.deck, &game.dealer_hand);
                    my_strcpy(game.message, "JUGAR: [P] DEMANAR  [S] PLANTAR-SE");
                    
                    // Blackjack natural?
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

        // Dealer Logic (Automàtic quan canvia l'estat)
        if (game.state == ST_DEALER_TURN) {
            // Renderitzar una vegada per ensenyar la carta oculta
            render(buffer, &game);
            write(10, buffer, SCREEN_SIZE);
            
            // Petit delay
            for(int d=0; d<500000; d++);

            while (game.dealer_hand.valor < 17) {
                donarCarta(&game.deck, &game.dealer_hand);
                render(buffer, &game);
                write(10, buffer, SCREEN_SIZE);
                for(int d=0; d<500000; d++);
            }

            // Avaluar guanyador
            int pv = game.player_hand.valor;
            int dv = game.dealer_hand.valor;
            int won = 0;

            if (pv > 21) {
                // Ja tractat abans, però per seguretat
                my_strcpy(game.message, "HAS PERDUT. PREM [C].");
            } else if (dv > 21) {
                my_strcpy(game.message, "BAKA BAKA ES PASSA! GUANYES! PREM [C].");
                game.money += game.current_bet * 2;
                won = 1;
            } else if (pv > dv) {
                my_strcpy(game.message, "GUANYES! PREM [C].");
                game.money += game.current_bet * 2;
                won = 1;
            } else if (pv < dv) {
                my_strcpy(game.message, "GUANYA EL SENYOR BAKA BAKA. PREM [C].");
            } else {
                my_strcpy(game.message, "EMPAT (PUSH). PREM [C].");
                game.money += game.current_bet;
            }
            
            if (game.money < 10) {
                my_strcpy(game.message, "RUINA TOTAL! PREM [R] PER REINICIAR.");
            }
            
            game.state = ST_GAME_OVER;

            // Actualitzar pantalla amb el resultat final
            render(buffer, &game);
            if (won) victory_animation(buffer);
            write(10, buffer, SCREEN_SIZE);
        }
    }

    return 0;
}