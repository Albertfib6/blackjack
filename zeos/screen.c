#include <screen.h>

int write_screen(buffer, nbytes) {
    if (nbytes > TAM_SCREEN_BUFFER) nbytes = TAM_SCREEN_BUFFER;

    Word *screen = (Word *)SCREEN_MEM;
    int words = nbytes / 2;

    for (int i = 0; i < words; i++) {
        screen[i] = ((Word)buffer[i * 2]) | ((Word)buffer[i * 2 + 1] << 8);
    }

    return nbytes;
}


void generate_checkerboard_pattern(char *buffer) {
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            int pos = (y * NUM_COLUMNS + x) * 2;

            /* Row 0 is black for time/FPS display */
            if (y == 0) {
                buffer[pos] = ' ';
                buffer[pos + 1] = 0x00;
                continue;
            }

            /* Checkerboard pattern: alternate A/B both horizontally and vertically */
            int is_alternate = (x + y) % 2;
            buffer[pos] = is_alternate ? 'B' : 'A';       /* Character */
            buffer[pos + 1] = is_alternate ? 0x4F : 0x1F; /* Red on white : Blue on white */
        }
    }
}

void test_screen() {
    char screen_buffer[80*25*2];
    generate_checkerboard_pattern(screen_buffer);
    write(10, screen_buffer, sizeof(screen_buffer));
}
