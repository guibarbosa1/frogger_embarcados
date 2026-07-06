#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Necessário para rand()
#include "xparameters.h"
#include "xaxivdma.h"
#include "xil_cache.h"
#include "xgpio.h"
#include "sleep.h"


// --- CONFIGURAÇÕES DE VÍDEO ---
#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480
#define PIXEL_BYTES  4
#define FRAME_SIZE   (FRAME_WIDTH * FRAME_HEIGHT * PIXEL_BYTES)
#define BUFFER_0_ADDR 0x10000000
#define BUFFER_1_ADDR (BUFFER_0_ADDR + FRAME_SIZE)

// Cores (0x00RRGGBB)
#define COLOR_TRANS  0xFF00FF
#define COLOR_WATER  0x000000AA
#define COLOR_ROAD   0x00222222
#define COLOR_SAFE   0x00228822
#define COLOR_BLACK  0x00000000
#define COLOR_WHITE  0x00FFFFFF
#define COLOR_YELLOW 0x00FFFF00
#define COLOR_RED    0x00FF0000
#define COLOR_GREEN  0x0000FF00

// Cores para Biomas e Veículos
#define COLOR_GRASS  0x0000AA00
#define COLOR_SAND   0x00DDDD00
#define COLOR_SNOW   0x00CCCCCC
#define COLOR_ORANGE 0x00FF9900
#define COLOR_GRAY   0x00888888
#define COLOR_PURPLE 0x009933CC

// --- HARDWARE ---
XAxiVdma Vdma;
XGpio Gpio;
int current_buffer = 0;
u32 *draw_buffer = (u32 *)BUFFER_1_ADDR;

// --- FONTE 3x5 (A-Z, 0-9) ---
const u8 FONT[36][5] = {
    {7,5,5,5,7}, {2,2,2,2,2}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1}, 
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7}, 
    {7,5,7,5,5}, {6,5,6,5,6}, {7,4,4,4,7}, {6,5,5,5,6}, {7,4,7,4,7}, 
    {7,4,7,4,4}, {7,4,5,5,7}, {5,5,7,5,5}, {7,2,2,2,7}, {1,1,1,5,7}, 
    {5,6,4,6,5}, {4,4,4,4,7}, {5,7,5,5,5}, {6,5,5,5,5}, {7,5,5,5,7}, 
    {7,5,7,4,4}, {7,5,7,1,6}, {7,5,7,6,5}, {7,4,7,1,7}, {7,2,2,2,2}, 
    {5,5,5,5,7}, {5,5,5,5,2}, {5,5,5,7,5}, {5,5,2,5,5}, {5,5,7,2,2}, 
    {7,1,2,4,7}
};

// --- SPRITES COMPACTADOS ---
#define _ COLOR_TRANS
#define B COLOR_BLACK
#define W COLOR_WHITE
#define G 0x0000FF00
#define R 0x00FF0000
#define U 0x000055FF
#define V 0x0000FFFF
#define D 0x00663300
#define L 0x00885522
#define K 0x00DD9900
#define C 0x00009900
#define H 0x0000FF00
#define Y COLOR_YELLOW
#define O COLOR_ORANGE
#define M COLOR_GRAY
#define P COLOR_PURPLE

const u32 SPRITE_FROG[256] = {
    _,_,_,G,G,_,_,_,_,_,_,G,G,_,_,_, _,_,G,W,W,G,_,_,_,_,G,W,W,G,_,_,
    _,_,G,W,B,G,_,_,_,_,G,W,B,G,_,_, _,G,G,G,G,G,G,G,G,G,G,G,G,G,G,_,
    _,G,G,G,G,G,G,G,G,G,G,G,G,G,G,_, G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,
    G,G,_,G,G,G,G,G,G,G,G,G,G,_,G,G, G,G,_,_,G,G,G,G,G,G,G,G,_,_,G,G,
    _,_,_,_,_,G,G,G,G,G,G,_,_,_,_,_, _,_,_,_,G,G,G,G,G,G,G,G,_,_,_,_,
    _,_,_,G,G,G,_,_,_,_,G,G,G,_,_,_, _,_,G,G,G,_,_,_,_,_,_,G,G,G,_,_,
    _,G,G,G,_,_,_,_,_,_,_,_,G,G,G,_, G,G,G,_,_,_,_,_,_,_,_,_,_,G,G,G,
    G,G,_,_,_,_,_,_,_,_,_,_,_,_,G,G, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

const u32 SPRITE_CAR_RED[256] = {
    _,_,R,R,R,R,R,R,R,R,R,R,R,R,_,_, _,R,R,R,R,R,R,R,R,R,R,R,R,R,R,_,
    R,R,W,W,W,R,R,R,R,R,W,W,W,R,R,R, R,W,W,W,W,R,R,R,R,R,W,W,W,W,R,R,
    R,W,W,W,W,R,R,R,R,R,W,W,W,W,R,R, R,W,W,W,W,R,R,R,R,R,W,W,W,W,R,R,
    R,R,W,W,W,R,R,R,R,R,W,W,W,R,R,R, R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
    R,R,R,Y,Y,R,R,R,R,R,Y,Y,R,R,R,R, R,R,R,Y,Y,R,R,R,R,R,Y,Y,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R, R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
    B,B,_,_,R,R,R,R,R,R,R,R,_,_,B,B, B,B,_,_,R,R,R,R,R,R,R,R,_,_,B,B,
    _,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

const u32 SPRITE_CAR_BLUE[256] = {
    _,_,U,U,U,U,U,U,U,U,U,U,U,U,_,_, _,U,U,U,U,U,U,U,U,U,U,U,U,U,U,_,
    U,U,W,W,W,U,U,U,U,U,W,W,W,U,U,U, U,W,W,W,W,U,U,U,U,U,W,W,W,W,U,U,
    U,W,W,W,W,U,U,U,U,U,W,W,W,W,U,U, U,W,W,W,W,U,U,U,U,U,W,W,W,W,U,U,
    U,U,W,W,W,U,U,U,U,U,W,W,W,U,U,U, U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,
    U,U,U,Y,Y,U,U,U,U,U,Y,Y,U,U,U,U, U,U,U,Y,Y,U,U,U,U,U,Y,Y,U,U,U,U,
    U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U, U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,
    B,B,_,_,U,U,U,U,U,U,U,U,_,_,B,B, B,B,_,_,U,U,U,U,U,U,U,U,_,_,B,B,
    _,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

const u32 SPRITE_CAR_YELLOW[256] = {
    _,_,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_,_, _,_,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,_,_,
    _,Y,Y,W,W,W,W,Y,Y,W,W,W,W,Y,Y,_, Y,Y,W,W,W,W,Y,Y,W,W,W,W,W,W,Y,Y,
    Y,Y,W,W,W,W,Y,Y,Y,Y,W,W,W,W,Y,Y, Y,Y,W,W,W,W,Y,Y,Y,Y,W,W,W,W,Y,Y,
    Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y, Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
    Y,Y,R,Y,Y,Y,Y,Y,Y,Y,Y,Y,R,Y,Y,Y, Y,Y,R,Y,Y,Y,Y,Y,Y,Y,Y,Y,R,Y,Y,Y,
    Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y, Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
    B,B,_,_,Y,Y,Y,Y,Y,Y,Y,Y,_,_,B,B, B,B,_,_,Y,Y,Y,Y,Y,Y,Y,Y,_,_,B,B,
    _,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

const u32 SPRITE_BULLDOZER[256] = {
    _,_,O,O,O,O,O,O,O,O,O,O,O,O,_,_, O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,_,
    _,O,O,W,W,W,W,O,O,O,O,O,O,O,O,_, O,O,W,W,W,W,O,O,O,O,O,O,O,O,O,_,
    O,O,W,W,W,W,W,W,O,O,O,O,O,O,O,O, O,O,W,W,W,W,W,W,O,O,O,O,O,O,O,O,
    O,O,W,W,W,W,W,W,O,O,O,O,O,O,O,O, O,O,W,W,W,W,W,W,O,O,O,O,O,O,O,O,
    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
    O,O,O,M,M,O,O,O,O,O,O,O,O,M,M,O, O,O,O,M,M,O,O,O,O,O,O,O,O,M,M,O,
    B,B,_,_,O,O,O,O,O,O,O,O,_,_,B,B, B,B,_,_,O,O,O,O,O,O,O,O,_,_,B,B,
    _,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_, _,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_
};

const u32 SPRITE_VAN[256] = {
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,
    P,P,W,W,W,W,P,P,P,P,P,P,P,P,P,P, P,W,W,W,W,P,P,P,P,P,P,P,P,P,P,P,
    P,W,W,W,W,P,P,P,P,P,P,P,P,P,P,P, P,P,W,W,W,W,P,P,P,P,P,P,P,P,P,P,
    P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,P, P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,
    P,P,Y,Y,P,P,P,P,P,P,Y,Y,P,P,P,P, P,P,Y,Y,P,P,P,P,P,P,Y,Y,P,P,P,P,
    P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,P, P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,P,
    B,B,_,_,P,P,P,P,P,P,P,P,_,_,B,B, B,B,_,_,P,P,P,P,P,P,P,P,_,_,B,B,
    _,_,_,_,B,B,B,B,B,B,B,B,_,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

const u32 SPRITE_TRUCK[512] = {
    _,_,K,K,K,K,K,K,_,_,_,_,_,_,_,_, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    _,K,K,K,K,K,K,_,_,_,_,_,_,_,_,_, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,K,W,W,W,K,K,_,_,_,_,_,_,_,_,_, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,W,W,W,W,K,K,_,_,_,_,_,_,_,_,_, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,W,W,W,W,K,K,_,_,_,_,_,_,_,_,_, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,K,W,W,W,K,K,_,_,_,_,_,_,_,_,_, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K, M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
    K,W,K,K,K,K,K,K,K,K,K,K,K,K,W,K, M,W,M,M,M,M,M,M,M,M,M,M,M,W,M,M,
    K,W,K,K,K,K,K,K,K,K,K,K,K,K,W,K, M,W,M,M,M,M,M,M,M,M,M,M,M,W,M,M,
    B,B,K,K,K,K,K,K,K,K,K,K,K,K,B,B, B,B,M,M,M,M,M,M,M,M,M,M,M,M,B,B,
    B,B,_,_,K,K,K,K,K,K,K,K,_,_,B,B, B,B,_,_,M,M,M,M,M,M,M,M,_,_,B,B,
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

const u32 SPRITE_LOG[256] = {
    D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D, D,L,L,L,L,L,L,L,L,L,L,L,L,L,L,D,
    D,L,D,L,L,D,L,L,L,D,L,L,D,L,L,D, D,L,L,D,L,L,L,L,D,L,L,L,L,D,L,D,
    D,L,L,L,D,L,L,L,L,L,D,L,L,L,L,D, D,L,D,L,L,D,L,D,L,L,D,L,L,D,L,D,
    D,L,L,D,L,L,L,L,D,L,L,L,L,D,L,D, D,L,L,L,D,L,L,L,L,L,D,L,L,L,L,D,
    D,L,D,L,L,D,L,L,L,D,L,L,D,L,L,D, D,L,L,D,L,L,L,L,D,L,L,L,L,D,L,D,
    D,L,L,L,D,L,D,L,L,L,D,L,L,L,L,D, D,L,D,L,L,D,L,L,L,D,L,L,D,L,L,D,
    D,L,L,D,L,L,L,L,D,L,L,L,L,D,L,D, D,L,L,L,D,L,L,L,L,L,D,L,L,L,L,D,
    D,L,L,L,L,L,L,L,L,L,L,L,L,L,L,D, D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D
};

const u32 SPRITE_CACTUS[256] = {
    _,_,_,C,C,C,C,C,C,C,C,C,C,_,_,_, _,_,C,C,H,H,C,C,C,H,H,C,C,C,_,_,
    _,C,C,H,H,C,C,H,H,C,C,H,H,C,C,_, _,C,H,H,C,C,H,H,C,C,H,H,C,C,C,_,
    C,C,H,H,C,C,H,H,C,C,H,H,C,C,H,C, C,H,H,C,C,H,H,C,C,H,H,C,C,H,H,C,
    C,H,C,C,H,H,C,C,H,H,C,C,H,H,C,C, C,C,C,H,H,C,C,H,H,C,C,H,H,C,C,C,
    C,C,H,H,C,C,H,H,C,C,H,H,C,C,H,C, C,H,H,C,C,H,H,C,C,H,H,C,C,H,H,C,
    C,H,C,C,H,H,C,C,H,H,C,C,H,H,C,C, _,C,C,H,H,C,C,H,H,C,C,H,H,C,C,_,
    _,C,C,H,H,C,C,H,H,C,C,H,H,C,C,_, _,_,C,C,H,H,C,C,C,H,H,C,C,C,_,_,
    _,_,_,C,C,C,C,C,C,C,C,C,C,_,_,_, _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_
};

// --- ESTRUTURAS DO JOGO ---
typedef enum { STATE_MENU, STATE_PLAYING, STATE_DYING, STATE_GAMEOVER } GameState;
GameState game_state = STATE_MENU;

typedef struct {
    int x, y, w, h;
    int src_w, src_h;
    int speed;
    const u32 *sprite;
    int is_water;
    int is_leaf;
    int sink_timer;
    int is_sinking;
    int respawn_timer;
    int v_type;   // 0 = sprite, 1 = bus, 2 = big_truck
    u32 c1, c2;   // Cores para veículos procedurais
} Entity;

typedef struct {
    int x, y;
    int active;
} Coin;

#define MAX_COINS 5
Coin coins[MAX_COINS];
int coin_count = 0;

typedef struct {
    u32 bg_color;
    int is_water;
    int is_safe;
    int show_grass;
} Lane;

#define MAX_ENTITIES 40
Entity entities[MAX_ENTITIES];
Lane lanes[12];
int entity_count = 0;

Entity frog;
int btn_cooldown = 0;
int frame_count = 0;
int score = 0;
int max_y_reached = 440;
int level = 1;

u32 prev_buttons = 0;

typedef enum { THEME_DAY, THEME_NIGHT, THEME_DESERT, THEME_AURORA, THEME_COUNT } Theme;
Theme current_theme = THEME_DAY;

#define MAX_STARS 24
typedef struct { int x, y; } Star;
Star stars[MAX_STARS];
int star_count = 0;

#define DEATH_ANIM_FRAMES 50
#define DEATH_CAUSE_HIT   0
#define DEATH_CAUSE_DROWN 1
int death_timer = 0;
int death_cause = DEATH_CAUSE_HIT;
int death_x = 0, death_y = 0;

// --- INICIALIZAÇÃO ---
void init_hardware() {
    XAxiVdma_Config *Config = XAxiVdma_LookupConfig(XPAR_AXIVDMA_0_DEVICE_ID);
    XAxiVdma_CfgInitialize(&Vdma, Config, Config->BaseAddress);

    XAxiVdma_DmaSetup ReadCfg;
    memset(&ReadCfg, 0, sizeof(ReadCfg));

    ReadCfg.VertSizeInput = FRAME_HEIGHT;
    ReadCfg.HoriSizeInput = FRAME_WIDTH * PIXEL_BYTES;
    ReadCfg.Stride = FRAME_WIDTH * PIXEL_BYTES;
    ReadCfg.FrameDelay = 0;
    ReadCfg.EnableCircularBuf = 1;
    ReadCfg.EnableSync = 1; // <-- ISSO CORRIGE O RASGO DE TELA (TEARING) NO HDMI
    ReadCfg.PointNum = 0;
    ReadCfg.EnableFrameCounter = 0;
    ReadCfg.FixedFrameStoreAddr = 0;
    XAxiVdma_DmaConfig(&Vdma, XAXIVDMA_READ, &ReadCfg);
    XAxiVdma_DmaSetBufferAddr(&Vdma, XAXIVDMA_READ, (UINTPTR[]){BUFFER_0_ADDR, BUFFER_1_ADDR});

    memset((void*)BUFFER_0_ADDR, 0, FRAME_SIZE);
    memset((void*)BUFFER_1_ADDR, 0, FRAME_SIZE);
    Xil_DCacheFlushRange(BUFFER_0_ADDR, FRAME_SIZE);
    Xil_DCacheFlushRange(BUFFER_1_ADDR, FRAME_SIZE);

    XAxiVdma_DmaStart(&Vdma, XAXIVDMA_READ);
    XAxiVdma_StartParking(&Vdma, 0, XAXIVDMA_READ);

    XGpio_Initialize(&Gpio, XPAR_AXI_GPIO_0_DEVICE_ID);
}

void swap_buffers() {
    Xil_DCacheFlushRange((UINTPTR)draw_buffer, FRAME_SIZE);
    current_buffer = 1 - current_buffer;
    draw_buffer = (u32 *)(current_buffer == 0 ? BUFFER_1_ADDR : BUFFER_0_ADDR);
    XAxiVdma_StartParking(&Vdma, current_buffer, XAXIVDMA_READ);
}

// --- FUNÇÕES GRÁFICAS ---
void clear_screen(u32 color) {
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
        draw_buffer[i] = color;
    }
}

void draw_rect(int x, int y, int w, int h, u32 color) {
    int start_x = (x < 0) ? 0 : x;
    int start_y = (y < 0) ? 0 : y;
    int end_x = (x + w > FRAME_WIDTH) ? FRAME_WIDTH : x + w;
    int end_y = (y + h > FRAME_HEIGHT) ? FRAME_HEIGHT : y + h;

    for (int i = start_y; i < end_y; i++) {
        for (int j = start_x; j < end_x; j++) {
            draw_buffer[i * FRAME_WIDTH + j] = color;
        }
    }
}

void draw_circle(int cx, int cy, int r, u32 color) {
    for (int y = -r; y <= r; y++) {
        int span = (int)((r * r) - (y * y));
        if (span < 0) continue;
        int rx = 0;
        while ((rx + 1) * (rx + 1) <= span) rx++;
        draw_rect(cx - rx, cy + y, 2 * rx + 1, 1, color);
    }
}

void draw_char(int x, int y, char c, u32 color, int scale) {
    int idx = -1;
    if (c >= '0' && c <= '9') idx = c - '0';
    else if (c >= 'A' && c <= 'Z') idx = c - 'A' + 10;
    if (idx == -1) return;

    for (int row = 0; row < 5; row++) {
        u8 bits = FONT[idx][row];
        for (int col = 0; col < 3; col++) {
            if (bits & (1 << (2 - col))) {
                draw_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void draw_string(int x, int y, const char* str, u32 color, int scale) {
    int cx = x;
    for (int i = 0; str[i] != '\0'; i++) {
        draw_char(cx, y, str[i], color, scale);
        cx += 4 * scale;
    }
}

void draw_sprite(int x, int y, int logical_w, int logical_h, int src_w, int src_h, const u32 *sprite_data) {
    int scale_x = logical_w / src_w;
    int scale_y = logical_h / src_h;

    for (int i = 0; i < src_h; i++) {
        for (int j = 0; j < src_w; j++) {
            u32 color = sprite_data[i * src_w + j];
            if (color != COLOR_TRANS) {
                for (int dy = 0; dy < scale_y; dy++) {
                    for (int dx = 0; dx < scale_x; dx++) {
                        int px = x + (j * scale_x) + dx;
                        int py = y + (i * scale_y) + dy;
                        if (px >= 0 && px < FRAME_WIDTH && py >= 0 && py < FRAME_HEIGHT) {
                            draw_buffer[py * FRAME_WIDTH + px] = color;
                        }
                    }
                }
            }
        }
    }
}

// --- TEXTURAS PROCEDURAIS (CHÃO, ESTRADA, ÔNIBUS, CAMINHÃO) ---
void draw_pavement(int y, u32 base_color) {
    draw_rect(0, y, FRAME_WIDTH, 40, base_color);
    u32 dark = (base_color >> 1) & 0x007F7F7F;
    u32 light = (base_color + 0x00151515) & 0x00FFFFFF;
    
    // Padrão Xadrez de Pedras
    for (int x = 0; x < FRAME_WIDTH; x += 32) {
        for (int row = 0; row < 40; row += 20) {
            u32 c = (((x/32) + (row/20)) % 2 == 0) ? light : dark;
            draw_rect(x+1, y+row+1, 30, 18, c);
        }
    }
    // Juntas (linhas escuras)
    for (int x = 0; x < FRAME_WIDTH; x += 32) draw_rect(x, y, 1, 40, 0x00444444);
    draw_rect(0, y+20, FRAME_WIDTH, 1, 0x00444444);
    draw_rect(0, y, FRAME_WIDTH, 1, 0x00333333);
    draw_rect(0, y+39, FRAME_WIDTH, 1, 0x00333333);

    // Bancos e Postes a cada 2 faixas (apenas se não for a faixa do meio)
    if ((y % 80 == 0) && y > 0 && y < 440) {
        draw_rect(FRAME_WIDTH - 60, y + 24, 24, 4, 0x00663300); // Banco madeira
        draw_rect(FRAME_WIDTH - 60, y + 28, 4, 8, 0x00663300);  // Perna banco
        draw_rect(FRAME_WIDTH - 40, y + 28, 4, 8, 0x00663300);  // Perna banco
        draw_rect(40, y, 4, 40, 0x00555555);                    // Poste
        draw_rect(20, y, 24, 4, 0x00555555);                    // Braço poste
        draw_rect(20, y, 4, 8, 0x00FFFF00);                     // Lâmpada
    }
}

void draw_road_lane(int y, u32 road_color) {
    draw_rect(0, y, FRAME_WIDTH, 40, road_color);
    u32 dot_color = (road_color + 0x00151515) & 0x00FFFFFF;
    u32 dash_color = 0x00EEEE00;
    
    // Granulação do asfalto
    for(int x=0; x<FRAME_WIDTH; x+=4) {
        for(int yy=0; yy<40; yy+=4) {
            if(((x/4)+(yy/4))%3==0) draw_rect(x, y+yy, 1, 1, dot_color);
        }
    }
    // Faixa tracejada
    for(int x=0; x<FRAME_WIDTH; x+=40) {
        draw_rect(x+10, y+18, 20, 4, dash_color);
    }
}

void draw_water_texture(int y, u32 base_color) {
    u32 light = (base_color + 0x00202020) & 0x00FFFFFF;
    for(int x=0; x<FRAME_WIDTH; x+=8) {
        for(int yy=0; yy<40; yy+=8) {
            if(((x/8)+(yy/8))%2==0) draw_rect(x+2, y+yy+2, 4, 1, light);
        }
    }
}

void draw_bus(int x, int y) {
    // Ônibus 96x32
    draw_rect(x, y+4, 96, 24, 0x00FFFF00); // Corpo
    draw_rect(x+4, y, 88, 8, 0x00DDDD00);  // Telhado
    // Janelas
    for(int i=0; i<4; i++) draw_rect(x+10 + i*18, y+8, 14, 12, 0x0088FFFF);
    // Porta
    draw_rect(x+82, y+8, 10, 20, 0x00FFFFFF);
    // Faróis e Rodas
    draw_rect(x, y+24, 4, 4, 0x00FFFF00);
    draw_rect(x+92, y+24, 4, 4, 0x00FFFF00);
    draw_rect(x+8, y+28, 12, 4, 0x00000000);
    draw_rect(x+76, y+28, 12, 4, 0x00000000);
}

void draw_big_truck(int x, int y, u32 cab_color, u32 trailer_color) {
    // Caminhão 96x32
    // Baú
    draw_rect(x+32, y+2, 64, 28, trailer_color);
    draw_rect(x+32, y+2, 64, 2, 0x00555555); 
    draw_rect(x+32, y+28, 64, 2, 0x00555555);
    for(int i=0; i<4; i++) draw_rect(x+32 + i*16, y+2, 1, 28, 0x00333333);
    // Cabine
    draw_rect(x, y+4, 32, 26, cab_color);
    draw_rect(x+4, y, 24, 8, cab_color); 
    draw_rect(x+8, y+8, 16, 8, 0x0088FFFF); // Para-brisa
    draw_rect(x, y+24, 4, 4, 0x00FFFF00);   // Farol esq
    draw_rect(x+28, y+24, 4, 4, 0x00FFFF00);// Farol dir
    // Rodas
    draw_rect(x+4, y+28, 8, 4, 0x00000000);
    draw_rect(x+20, y+28, 8, 4, 0x00000000);
    draw_rect(x+40, y+28, 8, 4, 0x00000000);
    draw_rect(x+56, y+28, 8, 4, 0x00000000);
    draw_rect(x+72, y+28, 8, 4, 0x00000000);
}

// --- FÍSICA E LÓGICA ---
void reset_frog() {
    frog.x = FRAME_WIDTH / 2 - 16;
    frog.y = 440;
    max_y_reached = 440;
}

void start_death(int cause) {
    game_state = STATE_DYING;
    death_timer = DEATH_ANIM_FRAMES;
    death_cause = cause;
    death_x = frog.x;
    death_y = frog.y;
}

void generate_stars() {
    star_count = 6 + (rand() % (MAX_STARS - 6));
    for (int i = 0; i < star_count; i++) {
        stars[i].x = rand() % FRAME_WIDTH;
        stars[i].y = 4 + (rand() % 30);
    }
}

void generate_level(int lvl) {
    entity_count = 0;
    coin_count = 0;

    if (lvl <= 1) current_theme = THEME_DAY;
    else current_theme = 1 + (rand() % (THEME_COUNT - 1));
    
    if (current_theme == THEME_NIGHT) generate_stars();

    u32 safe_color, road_color, water_color;
    switch (current_theme) {
        case THEME_NIGHT:  safe_color = 0x00103018; road_color = 0x00101018; water_color = 0x0000003A; break;
        case THEME_DESERT: safe_color = COLOR_SAND; road_color = 0x00443322; water_color = 0x000077AA; break;
        case THEME_AURORA: safe_color = 0x00114433; road_color = COLOR_ROAD;  water_color = 0x00220066; break;
        default:           safe_color = COLOR_SAFE; road_color = COLOR_ROAD;  water_color = COLOR_WATER; break;
    }

    lanes[0].bg_color = safe_color; lanes[0].is_water = 0; lanes[0].is_safe = 1; lanes[0].show_grass = (current_theme != THEME_DESERT);
    lanes[11].bg_color = safe_color; lanes[11].is_water = 0; lanes[11].is_safe = 1; lanes[11].show_grass = (current_theme != THEME_DESERT);

    int safe_mid_lane = 3 + (rand() % 5);
    int speed_mult = 1 + (lvl / 2);
    if (speed_mult > 5) speed_mult = 5;

    for (int i = 1; i < 11; i++) {
        if (i == safe_mid_lane) {
            lanes[i].is_water = 0; lanes[i].is_safe = 1;
            int biome = rand() % 3;
            if (biome == 0) { lanes[i].bg_color = COLOR_GRASS; lanes[i].show_grass = 1; }
            else if (biome == 1) { lanes[i].bg_color = COLOR_SAND; lanes[i].show_grass = 0; }
            else { lanes[i].bg_color = COLOR_SNOW; lanes[i].show_grass = 0; }
        } else {
            lanes[i].is_safe = 0; lanes[i].show_grass = 0;
            int type = rand() % 2;
            lanes[i].is_water = type;
            if (type == 1) lanes[i].bg_color = water_color;
            else lanes[i].bg_color = road_color;

            int direction = (rand() % 2) ? 1 : -1;
            int speed = (1 + rand() % 2 + speed_mult) * direction;
            int y = i * 40;
            int num_entities = 2 + rand() % 3;

            int gap = FRAME_WIDTH / num_entities;
            for (int j = 0; j < num_entities; j++) {
                if (entity_count >= MAX_ENTITIES) break;
                Entity *e = &entities[entity_count++];
                e->y = y;
                e->x = j * gap + (rand() % 50);
                e->speed = speed;
                e->sink_timer = 0; e->is_sinking = 0; e->respawn_timer = 0;
                e->v_type = 0; // Default sprite

                if (type == 1) {
                    e->is_water = 1;
                    if (rand() % 2) { e->sprite = SPRITE_LOG; e->is_leaf = 0; e->w = 64; e->h = 32; e->src_w = 16; e->src_h = 16; }
                    else { e->sprite = SPRITE_CACTUS; e->is_leaf = 1; e->speed = 0; e->w = 32; e->h = 32; e->src_w = 16; e->src_h = 16; }
                } else {
                    e->is_water = 0; e->is_leaf = 0;
                    int v_type = rand() % 8; // Mais variedade
                    if (v_type == 0) { e->sprite = SPRITE_CAR_RED; e->w = 32; e->h = 32; e->src_w = 16; e->src_h = 16; }
                    else if (v_type == 1) { e->sprite = SPRITE_CAR_BLUE; e->w = 32; e->h = 32; e->src_w = 16; e->src_h = 16; }
                    else if (v_type == 2) { e->sprite = SPRITE_TRUCK; e->w = 64; e->h = 32; e->src_w = 32; e->src_h = 16; }
                    else if (v_type == 3) { e->sprite = SPRITE_CAR_YELLOW; e->w = 32; e->h = 32; e->src_w = 16; e->src_h = 16; }
                    else if (v_type == 4) { e->sprite = SPRITE_BULLDOZER; e->w = 32; e->h = 32; e->src_w = 16; e->src_h = 16; }
                    else if (v_type == 5) { e->sprite = SPRITE_VAN; e->w = 32; e->h = 32; e->src_w = 16; e->src_h = 16; }
                    else if (v_type == 6) { e->v_type = 1; e->w = 96; e->h = 32; } // Ônibus Procedural
                    else { e->v_type = 2; e->w = 96; e->h = 32; e->c1 = 0x00FF0000; e->c2 = 0x00CCCCCC; } // Caminhão Grande Procedural
                }
            }
            if (type == 0 && coin_count < MAX_COINS && (rand() % 2)) {
                coins[coin_count].x = (rand() % 18) * 32 + 16;
                coins[coin_count].y = y + 20;
                coins[coin_count].active = 1;
                coin_count++;
            }
        }
    }
    if (coin_count < MAX_COINS) {
        coins[coin_count].x = (rand() % 18) * 32 + 16;
        coins[coin_count].y = safe_mid_lane * 40 + 20;
        coins[coin_count].active = 1;
        coin_count++;
    }
}

void init_game() {
    frog.w = 32; frog.h = 32; frog.src_w = 16; frog.src_h = 16; frog.sprite = SPRITE_FROG;
    reset_frog();
    score = 0; level = 1;
    generate_level(level);
}

int check_collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    int shrink = 6;
    return (x1 + shrink < x2 + w2 - shrink) && (x1 + w1 - shrink > x2 + shrink) &&
           (y1 + shrink < y2 + h2 - shrink) && (y1 + h1 - shrink > y2 + shrink);
}

// Hitbox expandida e sem encolhimento. Garante coleta em tempo real.
int check_collision_coin(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return (x1 < x2 + w2) && (x1 + w1 > x2) &&
           (y1 < y2 + h2) && (y1 + h1 > y2);
}

void update_game() {
    u32 buttons = XGpio_DiscreteRead(&Gpio, 1);
    u32 pressed_now = buttons & (~prev_buttons);

    if (game_state == STATE_MENU) {
        if ((pressed_now & 0x8) && btn_cooldown == 0) { init_game(); game_state = STATE_PLAYING; btn_cooldown = 15; }
        else if (btn_cooldown > 0) btn_cooldown--;
    }
    else if (game_state == STATE_GAMEOVER) {
        if ((pressed_now & 0x8) && btn_cooldown == 0) { game_state = STATE_MENU; btn_cooldown = 15; }
        else if (btn_cooldown > 0) btn_cooldown--;
    }
    else if (game_state == STATE_DYING) {
        death_timer--;
        if (death_timer <= 0) { game_state = STATE_GAMEOVER; btn_cooldown = 30; }
    }
    else {
        if (btn_cooldown == 0) {
            int moved = 0;
            if (buttons & 0x1) { frog.x += 32; moved = 1; }
            if (buttons & 0x2) { frog.x -= 32; moved = 1; }
            if (buttons & 0x4) { frog.y += 40; moved = 1; }
            if (buttons & 0x8) { frog.y -= 40; moved = 1; }

            if (moved) {
                btn_cooldown = 10;
                if (frog.y < max_y_reached) { score += 10; max_y_reached = frog.y; }
            }
        } else btn_cooldown--;

        if (frog.x < 0) frog.x = 0;
        if (frog.x > FRAME_WIDTH - 32) frog.x = FRAME_WIDTH - 32;
        if (frog.y > 440) frog.y = 440;
        if (frog.y < 0) frog.y = 0;

        // Colisão de moeda atualizada (Hitbox estendida para 32x40, cobrindo todo o "slot" da grade)
        for (int i = 0; i < coin_count; i++) {
            if (coins[i].active) {
                if (check_collision_coin(frog.x, frog.y, frog.w, frog.h,
                                          coins[i].x - 16, coins[i].y - 20, 32, 40)) {
                    score += 50;
                    coins[i].active = 0;
                }
            }
        }

        if (frog.y <= 0) {
            score += 100; level++; generate_level(level); reset_frog(); btn_cooldown = 15;
        }

        for (int i = 0; i < entity_count; i++) {
            Entity *e = &entities[i];
            if (e->is_sinking) {
                e->respawn_timer--;
                if (e->respawn_timer <= 0) { e->is_sinking = 0; e->sink_timer = 0; }
                continue;
            }
            e->x += e->speed;
            if (e->speed > 0 && e->x > FRAME_WIDTH) e->x = -e->w;
            if (e->speed < 0 && e->x < -e->w) e->x = FRAME_WIDTH;
        }

        int frog_lane = frog.y / 40;
        if (frog_lane < 0) frog_lane = 0;
        if (frog_lane > 11) frog_lane = 11;

        if (lanes[frog_lane].is_water) {
            int on_log = 0;
            for (int i = 0; i < entity_count; i++) {
                Entity *e = &entities[i];
                if (e->is_water && e->y == frog.y) {
                    if (check_collision(frog.x, frog.y, frog.w, frog.h, e->x, e->y, e->w, e->h)) {
                        if (!e->is_sinking) {
                            on_log = 1; frog.x += e->speed;
                            if (frog.x < 0 || frog.x > FRAME_WIDTH - 32) start_death(DEATH_CAUSE_DROWN);
                            if (e->is_leaf) {
                                e->sink_timer++;
                                if (e->sink_timer > 120) { e->is_sinking = 1; e->respawn_timer = 60; on_log = 0; }
                            }
                        }
                    } else e->sink_timer = 0;
                }
            }
            if (!on_log && game_state == STATE_PLAYING) start_death(DEATH_CAUSE_DROWN);
        }
        else if (!lanes[frog_lane].is_safe) {
            for (int i = 0; i < entity_count; i++) {
                Entity *e = &entities[i];
                if (!e->is_water && e->y == frog.y) {
                    if (check_collision(frog.x, frog.y, frog.w, frog.h, e->x, e->y, e->w, e->h)) {
                        start_death(DEATH_CAUSE_HIT);
                    }
                }
            }
        }
    }
    prev_buttons = buttons;
}

void draw_theme_decorations() {
    if (current_theme == THEME_NIGHT) {
        draw_circle(560, 22, 16, COLOR_WHITE);
        for (int i = 0; i < star_count; i++) draw_rect(stars[i].x, stars[i].y, 2, 2, COLOR_WHITE);
    } else if (current_theme == THEME_DESERT) {
        draw_circle(60, 24, 20, COLOR_YELLOW);
    } else if (current_theme == THEME_AURORA) {
        for (int i = 0; i < 4; i++) {
            u32 c = (i % 2 == 0) ? 0x0000FF99 : 0x00FF0099;
            draw_rect(0, 2 + i * 5, FRAME_WIDTH, 3, c);
        }
    }
}

void draw_grass_decoration(int lane_y) {
    for (int x = 4; x < FRAME_WIDTH; x += 24) {
        int seed = (x * 7 + lane_y * 13) % 100;
        int gy = lane_y + 28 + (seed % 8);
        draw_rect(x, gy, 2, 6, 0x00006600);
        draw_rect(x + 4, gy - 2, 2, 8, 0x00007700);
        draw_rect(x + 8, gy, 2, 6, 0x00006600);
        if (seed % 5 == 0) {
            u32 petal = (seed % 3 == 0) ? COLOR_WHITE : COLOR_YELLOW;
            draw_rect(x + 12, gy - 4, 3, 3, petal);
            draw_rect(x + 15, gy - 4, 3, 3, petal);
            draw_rect(x + 13, gy - 6, 3, 3, petal);
            draw_rect(x + 13, gy - 2, 2, 2, 0x0000AA00);
        }
    }
}

void draw_playfield() {
    for (int i = 0; i < 12; i++) {
        if (lanes[i].is_safe) {
            draw_pavement(i * 40, lanes[i].bg_color); // Textura rica em calçadas
        } else if (lanes[i].is_water) {
            draw_rect(0, i * 40, FRAME_WIDTH, 40, lanes[i].bg_color);
            draw_water_texture(i * 40, lanes[i].bg_color);
        } else {
            draw_road_lane(i * 40, lanes[i].bg_color); // Asfalto com faixa tracejada
        }
        if (lanes[i].show_grass) draw_grass_decoration(i * 40);
    }

    draw_theme_decorations();

    for (int i = 0; i < coin_count; i++) {
        if (coins[i].active) {
            draw_circle(coins[i].x, coins[i].y, 6, COLOR_YELLOW);
            draw_circle(coins[i].x, coins[i].y, 3, COLOR_WHITE);
        }
    }

    for (int i = 0; i < entity_count; i++) {
        Entity *e = &entities[i];
        if (!e->is_sinking) {
            if (e->v_type == 1) draw_bus(e->x, e->y);
            else if (e->v_type == 2) draw_big_truck(e->x, e->y, e->c1, e->c2);
            else draw_sprite(e->x, e->y, e->w, e->h, e->src_w, e->src_h, e->sprite);
        }
    }
}

void draw_death_animation() {
    int elapsed = DEATH_ANIM_FRAMES - death_timer;
    if (death_cause == DEATH_CAUSE_HIT) {
        if (elapsed < 10) {
            if ((elapsed / 2) % 2 == 0) draw_sprite(death_x, death_y, frog.w, frog.h, frog.src_w, frog.src_h, frog.sprite);
        } else {
            int squash = elapsed - 10;
            if (squash > 24) squash = 24;
            int h = frog.h - squash;
            if (h < 8) h = 8;
            int y_off = frog.h - h;
            int w = frog.w + squash / 2;
            int rx = death_x - (w - frog.w) / 2;

            draw_rect(rx, death_y + y_off, w, h, COLOR_GREEN);
            draw_rect(rx, death_y + y_off + h - 2, w, 2, 0x00005500);

            int eye_y = death_y + y_off + h / 4;
            int eye1_x = rx + w / 3 - 4;
            int eye2_x = rx + 2 * w / 3 - 4;
            draw_rect(eye1_x, eye_y, 8, 8, COLOR_WHITE);
            draw_rect(eye2_x, eye_y, 8, 8, COLOR_WHITE);
            for (int k = 0; k < 8; k++) {
                draw_rect(eye1_x + k, eye_y + k, 1, 1, COLOR_RED);
                draw_rect(eye1_x + (7 - k), eye_y + k, 1, 1, COLOR_RED);
                draw_rect(eye2_x + k, eye_y + k, 1, 1, COLOR_RED);
                draw_rect(eye2_x + (7 - k), eye_y + k, 1, 1, COLOR_RED);
            }
        }
    } else {
        int radius = 4 + elapsed;
        if (radius > 36) radius = 36;
        draw_circle(death_x + frog.w / 2, death_y + frog.h / 2, radius, 0x000099FF);
        if (radius > 14) draw_circle(death_x + frog.w / 2, death_y + frog.h / 2, radius - 14, 0x0000003A);

        int shrink = elapsed * 2;
        int sw = frog.w - shrink;
        int sh = frog.h - shrink;
        if (sw < 2) sw = 2;
        if (sh < 2) sh = 2;
        draw_sprite(death_x + (frog.w - sw) / 2, death_y + (frog.h - sh) / 2, sw, sh, frog.src_w, frog.src_h, frog.sprite);
    }
}

void render_game() {
    clear_screen(COLOR_BLACK);

    if (game_state == STATE_MENU) {
        draw_string(180, 70, "FROGGER", COLOR_GREEN, 10);
        draw_string(196, 160, "CONTROLS", COLOR_WHITE, 3);
        draw_string(150, 195, "BTN0 RIGHT", COLOR_YELLOW, 2);
        draw_string(150, 215, "BTN1 LEFT", COLOR_YELLOW, 2);
        draw_string(150, 235, "BTN2 DOWN", COLOR_YELLOW, 2);
        draw_string(150, 255, "BTN3 UP START", COLOR_YELLOW, 2);
        draw_string(150, 280, "COLLECT COINS +50", COLOR_YELLOW, 2);
        if ((frame_count / 30) % 2 == 0) draw_string(176, 330, "PRESS BTN3 TO PLAY", COLOR_WHITE, 4);
    }
    else if (game_state == STATE_PLAYING) {
        draw_playfield();
        draw_sprite(frog.x, frog.y, frog.w, frog.h, frog.src_w, frog.src_h, frog.sprite);
        char score_str[30];
        sprintf(score_str, "SCORE %04d LVL %d", score, level);
        draw_string(10, 10, score_str, COLOR_YELLOW, 2);+
    }
    else if (game_state == STATE_DYING) {
        draw_playfield();
        draw_death_animation();
        char score_str[30];
        sprintf(score_str, "SCORE %04d LVL %d", score, level);
        draw_string(10, 10, score_str, COLOR_YELLOW, 2);
    }
    else if (game_state == STATE_GAMEOVER) {
        draw_string(176, 150, "GAME OVER", COLOR_RED, 8);
        char score_str[20];
        sprintf(score_str, "SCORE %d", score);
        draw_string(240, 260, score_str, COLOR_YELLOW, 4);
        if ((frame_count / 30) % 2 == 0) draw_string(168, 350, "PRESS BTN3 FOR MENU", COLOR_WHITE, 4);
    }

    swap_buffers();
}

int main() {
    init_hardware();

    for(int i = 0; i < 12; i++) {
        lanes[i].bg_color = COLOR_BLACK;
        lanes[i].is_water = 0; lanes[i].is_safe = 1; lanes[i].show_grass = 0;
    }
    entity_count = 0;
    coin_count = 0;

    while (1) {
        frame_count++;
        update_game();
        render_game();
        usleep(16000);
    }
    return 0;
}
