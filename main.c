/*
 * ============================================================================
 * SEGA PowerShell OS v7.0.1 (POWERCORE KERNEL MASTER)
 * ----------------------------------------------------------------------------
 * System: Motorola 68000 @ 7.61MHz | SDK: SGDK 2.11
 * Graphics: H32 (256x224) | UI: Dual-Plane Interleaving
 * IO: 6-Button Arcade Pad (Mandatory)
 * ============================================================================
 */

#include <genesis.h>
#include <string.h>
#include "resources.h"

// --- KERNEL DEFINITIONS ---
#define KERNEL_VER "7.0.1"
#define SYSTEM_VERSION "SEGA POWERSHELL V7.0.1"
#define SYSTEM_COPYRIGHT "(C) SEGA CORP. RIGHTS RESERVED" 
#define TERM_Y_START 10
#define MAX_Y 24
#define MARGIN_X 2
#define MIN_SAFE_ADDR 0x000400 

// --- KERNEL GLOBAL DATA ---
u16 cursor_y = TERM_Y_START;
s16 cmd_ptr = 0;
u32 vblank_frames = 0;

// // Snake Module Data
#define SNAKE_MAX 64
Vect2D_s16 snake_body[SNAKE_MAX];
u16 s_len = 6;
s16 dx = 1, dy = 0;
Vect2D_s16 apple_pos = {15, 15}; 
u8 snake_update_timer = 0;       

// // Other Module Data
Vect2D_s16 ball_pos = {128, 112}, ball_vel = {2, 2};
s16 p1_y = 100, ai_y = 100;
fix32 opA = FIX32(0), opB = FIX32(0), res = FIX32(0);
u8 calc_slot = 0, calc_op = 0;
u32 mem_addr = 0xFF0000;
u8 hex_locked = 1; 
u16 psg_pitch = 200;

typedef enum { 
    MODE_INTRO, MODE_TERM, MODE_SNAKE, MODE_CALC, MODE_HEX, 
    MODE_PONG, MODE_TASKS, MODE_OSK, MODE_CREDITS, MODE_SOUND, 
    MODE_TETRIS, MODE_OPTIONS, MODE_JOYTEST, MODE_PALTEST, MODE_DISTEST,
    MODE_SYSINFO 
} OSState;

OSState sys_state = MODE_INTRO;
OSState prev_state = MODE_TERM; 

// --- MASTER PROTOTYPES ---
void ShowSegaIntro();
void ClearConsole();
void PrintLine(const char* msg);
void DrawPrompt();
void UpdateSystemBar();
void UpdateCalcUI();
void TickSnake();
void RunHexScan();
void RunPong();
void RunTaskMgr();
void RunSysInfo();
void RunSoundTest();
void RunTetrisGame();
void ProcessCommand();
void RunOSK();
void ParseStringCommand();
void SetupCredits();
void RunOptionsMenu();
void RunHardwareLockout();
void initialize_v701_color_matrix();
 

// Options Module Isolated Tracking Flags
u8 opt_theme_idx = 0;  // 0 = CLASSIC (White), 1 = CYAN_MOD (Cyan)
u8 opt_save_flash = 0;

// Expanded Application Registry (SYSINFO Added)
const char* cmd_list[] = {"TASKS", "PONG", "CALC", "SNAKE", "HEXSCAN", "SOUND", "TETRIS", "OPTIONS", "JOYTEST", "PALTEST", "DISTEST", "SYSINFO", "REBOOT"};
#define CMD_COUNT 13

// OSK & String Parsing
char cmd_buffer[32] = "";
u8 buf_len = 0;
u8 osk_cx = 0, osk_cy = 0, osk_layout = 0; 
const char osk_alpha[3][11] = { "QWERTYUIOP", "ASDFGHJKL ", "ZXCVBNM   " };
const char osk_num[3][11]   = { "1234567890", "!@#$%^&*()", "+-/*=.,?  " };
const char osk_hex[3][11]   = { "0123456789", "ABCDEF    ", "          " };

// Sub-Second Millisecond Lookup Array (60Hz)
const u16 vblank_to_ms[60] = {
    0,   16,  33,  50,  66,  83,  100, 116, 133, 150,
    166, 183, 200, 216, 233, 250, 266, 283, 300, 316,
    333, 350, 366, 383, 400, 416, 433, 450, 466, 483,
    500, 516, 533, 550, 566, 583, 600, 616, 633, 650,
    666, 683, 700, 716, 733, 750, 766, 783, 800, 816,
    833, 850, 866, 883, 900, 916, 933, 950, 966, 983
};

// Tetris Engine Data (1989 GHOST)
#define TETRIS_W 10
#define TETRIS_H 18
u8 board[TETRIS_W][TETRIS_H];
s16 px, py, prot;
u8 ptype;
u16 t_lines = 0, t_level = 1;
u32 t_score = 0;
const u16 shapes[7][4] = {
    {0x0F00, 0x4444, 0x0F00, 0x4444}, // I
    {0x4460, 0x0E80, 0xC440, 0x2E00}, // L
    {0x44C0, 0x8E00, 0x6440, 0x0E20}, // J
    {0xCC00, 0xCC00, 0xCC00, 0xCC00}, // O
    {0x06C0, 0x8C40, 0x06C0, 0x8C40}, // S
    {0x0C60, 0x4C80, 0x0C60, 0x4C80}, // Z
    {0x0E40, 0x4C40, 0x4E00, 0x8C80}  // T
};

// Credits Data
const char* credits_text[] = {
    "==========================",
    " SEGA PowerShell OS",
    " VERSION 7.0.1 MASTER",
    "==========================",
    "",
    "LEAD ARCHITECT:",
    "-> TITAN DEV",
    "",
    "SYSTEM KERNEL:",
    "-> PowerCORE Engine",
    "-> Motorola 68000 (7.67MHz)",
    "",
    "==========================",
    " THANK YOU FOR BOOTING."
};
#define CREDITS_LINES 14
s16 scroll_y = 0; 
 

// ============================================================================
// SYSTEM KERNEL CORE
// ============================================================================

void initialize_v701_color_matrix() {
    PAL_setColor(16 + 15, RGB24_TO_VDPCOLOR(0x00FFFF)); // PAL1 (15): Cyan Header
    PAL_setColor(32 + 15, RGB24_TO_VDPCOLOR(0x00FF00)); // PAL2 (15): Success Green
    PAL_setColor(48 + 15, RGB24_TO_VDPCOLOR(0xFF0000)); // PAL3 (15): Lockout Red
}

void UpdateSystemBar() {
    VDP_setTextPlane(BG_B);
    
    VDP_setTextPalette(PAL1);
    VDP_drawText("[POWERCORE: OK]", 1, 1);
    char hex_cpu_buffer[16];
    sprintf(hex_cpu_buffer, "CPU:#%04X", (u16)(vblank_frames & 0xFFFF));
    VDP_drawText(hex_cpu_buffer, 22, 1);

    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    if (sys_state == MODE_OSK) VDP_drawText("[INPUT: OSK]   ", 1, 2);
    else if (!hex_locked) {
        VDP_setTextPalette(PAL3); 
        VDP_drawText("[WARN: UNLOCKED]", 1, 2); 
    }
    else { VDP_drawText("                ", 1, 2); }
    
    JOY_update();
    u16 real_pad_type = JOY_getJoypadType(JOY_1);

    if (real_pad_type != JOY_TYPE_PAD6) {
        if (vblank_frames & 0x20) {
            VDP_setTextPalette(PAL3); 
            VDP_drawText("![3-BTN PAD]", 20, 2); 
        } else {
            VDP_drawText("            ", 20, 2); 
        }
    } else {
        VDP_drawText("            ", 20, 2);
    }
    
    VDP_setTextPalette(PAL0); 
    VDP_setTextPlane(BG_A);
}

void ShowSegaIntro() {
    VDP_clearPlane(BG_A, TRUE);
    
    VDP_setTextPalette(PAL1);
    PAL_setColors(PAL1 * 16, palette_black, 16, DMA);
    VDP_drawText("S E G A", 12, 12);
    
    XGM_setPCM(64, wav_sega, sizeof(wav_sega));
    XGM_startPlayPCM(64, 1, SOUND_PCM_CH1);
    
    PAL_fadeIn(PAL1 * 16, (PAL1 * 16) + 15, font_pal_default.data, 30, FALSE);
    
    for(u16 i = 0; i < 30; i++) { vblank_frames++; SYS_doVBlankProcess(); }
    for(u16 i = 0; i < 60; i++) { vblank_frames++; SYS_doVBlankProcess(); }
    
    PAL_fadeOut(PAL1 * 16, (PAL1 * 16) + 15, 30, FALSE);
    
    for(u16 i = 0; i < 30; i++) { vblank_frames++; SYS_doVBlankProcess(); }
    
    VDP_clearPlane(BG_A, TRUE);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL0, font_pal_default.data, DMA);
    initialize_v701_color_matrix();
    
    sys_state = MODE_TERM;
    ClearConsole(); 
    DrawPrompt(); 
}

// ============================================================================
// TETRIS ENGINE LOGIC
// ============================================================================

u8 CheckCollision(s16 nx, s16 ny, s16 nr) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shapes[ptype][nr] & (0x8000 >> (i + j * 4))) {
                int bx = nx + i;
                int by = ny + j;
                if (bx < 0 || bx >= TETRIS_W || by >= TETRIS_H || (by >= 0 && board[bx][by])) return 1;
            }
        }
    }
    return 0;
}

void ClearLines() {
    u8 cleared = 0;
    for (int y = TETRIS_H - 1; y >= 0; y--) {
        u8 full = 1;
        for (int x = 0; x < TETRIS_W; x++) if (!board[x][y]) full = 0;
        if (full) {
            cleared++;
            for (int ty = y; ty > 0; ty--)
                for (int tx = 0; tx < TETRIS_W; tx++) board[tx][ty] = board[tx][ty - 1];
            y++; 
        }
    }
    if (cleared > 0) {
        t_lines += cleared;
        t_score += (cleared * cleared * 100) * t_level;
        t_level = (t_lines / 10) + 1;
    }
}

void RunTetrisGame() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE); 
    memset(board, 0, sizeof(board));
    ptype = random() % 7; px = 3; py = 0; prot = 0;
    t_lines = 0; t_level = 1; t_score = 0;
    u16 drop_timer = 0;
    char s_buf[32];

    while(1) {
        u16 joy = JOY_readJoypad(JOY_1);
        
        if (joy & BUTTON_START) { 
            VDP_clearPlane(BG_B, TRUE);
            VDP_clearPlane(BG_A, TRUE);
            waitMs(250); 
            break; 
        }
        
        if (joy & BUTTON_LEFT)  if (!CheckCollision(px - 1, py, prot)) px--;
        if (joy & BUTTON_RIGHT) if (!CheckCollision(px + 1, py, prot)) px++;
        if (joy & BUTTON_A) { 
            s16 next_r = (prot + 1) % 4;
            if (!CheckCollision(px, py, next_r)) prot = next_r;
            waitMs(100); 
        }
        
        drop_timer++;
        u16 max_speed = 20 - (t_level * 2);
        if (max_speed < 2) max_speed = 2; 
        
        if (drop_timer > (joy & BUTTON_DOWN ? 2 : max_speed)) {
            if (!CheckCollision(px, py + 1, prot)) { py++; }
            else {
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        if ((shapes[ptype][prot] & (0x8000 >> (i + j * 4))) && py + j >= 0) board[px + i][py + j] = 1;
                ClearLines();
                ptype = random() % 7; px = 3; py = 0; prot = 0;
                if (CheckCollision(px, py, prot)) break; 
            }
            drop_timer = 0;
        }
        
        VDP_drawText("--- 1989 GHOST ---", 6, 1);
        sprintf(s_buf, "LVL: %02u", t_level); VDP_drawText(s_buf, 2, 22);
        sprintf(s_buf, "SCR: %06lu", t_score); VDP_drawText(s_buf, 12, 22);

        for (int y = 0; y < TETRIS_H; y++) {
            VDP_drawText("|", 9, y + 3);
            for (int x = 0; x < TETRIS_W; x++) {
                u8 active = 0;
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        if ((shapes[ptype][prot] & (0x8000 >> (i + j * 4))) && (px + i == x && py + j == y)) active = 1;
                VDP_drawText(board[x][y] || active ? "[]" : " .", 10 + (x * 2), y + 3);
            }
            VDP_drawText("|", 30, y + 3);
        }
        
        vblank_frames++;
        SYS_doVBlankProcess();
    }
    ClearConsole();
    DrawPrompt();
}

// ============================================================================
// MAIN HARDWARE ENTRY & HANDLERS
// ============================================================================

void ParseStringCommand() {
    if (strcmp(cmd_buffer, "RUN-STETRIS") == 0) { ClearConsole(); sys_state = MODE_TETRIS; RunTetrisGame(); } 
    else if (strcmp(cmd_buffer, "CREDITS") == 0) { ClearConsole(); SetupCredits(); }
    else if (strcmp(cmd_buffer, "CALIB") == 0) { ClearConsole(); sys_state = MODE_DISTEST; }
    else if (strcmp(cmd_buffer, "INFO") == 0) { ClearConsole(); sys_state = MODE_SYSINFO; }
    else if (strcmp(cmd_buffer, "CLEAR") == 0) { ClearConsole(); }
    else { PrintLine("> ERR: UNKNOWN INTERNAL MACRO"); }
    
    buf_len = 0; memset(cmd_buffer, 0, sizeof(cmd_buffer));
}

void ProcessCommand() {
    cursor_y++;
    switch(cmd_ptr) {
        case 0: ClearConsole(); sys_state = MODE_TASKS; break;
        case 1: ClearConsole(); sys_state = MODE_PONG; break;
        case 2: ClearConsole(); sys_state = MODE_CALC; UpdateCalcUI(); break;
        case 3: ClearConsole(); s_len = 6; dx = 1; dy = 0; 
                for(int i=0; i<s_len; i++) { snake_body[i].x = 10-i; snake_body[i].y = 15; }
                sys_state = MODE_SNAKE; break;
        case 4: ClearConsole(); sys_state = MODE_HEX; break;
        case 5: ClearConsole(); sys_state = MODE_SOUND; break;
        case 6: ClearConsole(); sys_state = MODE_TETRIS; RunTetrisGame(); break;
        case 7: ClearConsole(); sys_state = MODE_OPTIONS; break;
        case 8: ClearConsole(); sys_state = MODE_JOYTEST; break;
        case 9: ClearConsole(); sys_state = MODE_PALTEST; break;
        case 10: ClearConsole(); sys_state = MODE_DISTEST; break;
        case 11: ClearConsole(); sys_state = MODE_SYSINFO; break;
        case 12: SYS_hardReset(); break;
    }
    if (sys_state == MODE_TERM) DrawPrompt();
}

int main() {
    VDP_setScreenWidth256();
    PAL_setPalette(PAL1, font_pal_default.data, DMA);
    PSG_reset();
    ShowSegaIntro();
    UpdateSystemBar();
    
    while(1) {
        u16 joy = JOY_readJoypad(JOY_1);
        vblank_frames++; 
        
        if (joy & BUTTON_X && sys_state != MODE_OSK && sys_state != MODE_CREDITS && sys_state != MODE_TETRIS) {
            prev_state = sys_state; sys_state = MODE_OSK; waitMs(200);
        }

        if (joy & BUTTON_Z) {
            XGM_stopPlay();
            for(int i=0; i<4; i++) PSG_setEnvelope(i, 0);
            VDP_drawText("[AUDIO MUTED]", 1, 2);
            waitMs(200);
        }
        
        switch(sys_state) {
            case MODE_INTRO: break;
            
            case MODE_TERM:
                if (joy & BUTTON_UP) { cmd_ptr = (cmd_ptr > 0) ? cmd_ptr - 1 : CMD_COUNT - 1; DrawPrompt(); waitMs(150); }
                if (joy & BUTTON_DOWN) { cmd_ptr = (cmd_ptr + 1) % CMD_COUNT; DrawPrompt(); waitMs(150); }
                if (joy & BUTTON_A || joy & BUTTON_START) { 
                    waitMs(250); 
                    ProcessCommand(); 
                }
                break;
                
            case MODE_OSK:
                RunOSK();
                if (joy & BUTTON_Y) { osk_layout = (osk_layout + 1) % 3; waitMs(200); }
                if (joy & BUTTON_UP && osk_cy > 0) { osk_cy--; waitMs(150); }
                if (joy & BUTTON_DOWN && osk_cy < 2) { osk_cy++; waitMs(150); }
                if (joy & BUTTON_LEFT && osk_cx > 0) { osk_cx--; waitMs(150); }
                if (joy & BUTTON_RIGHT && osk_cx < 9) { osk_cx++; waitMs(150); }
                
                if (joy & BUTTON_A) {
                    const char (*active_grid)[11] = (osk_layout == 0) ? osk_alpha : (osk_layout == 1 ? osk_num : osk_hex);
                    if (buf_len < 30 && active_grid[osk_cy][osk_cx] != ' ') {
                        cmd_buffer[buf_len] = active_grid[osk_cy][osk_cx]; buf_len++; cmd_buffer[buf_len] = '\0'; waitMs(200);
                    }
                }
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); ParseStringCommand(); if(sys_state == MODE_TERM) DrawPrompt(); waitMs(300); }
                break;
                
            case MODE_CREDITS:
                VDP_setVerticalScroll(BG_A, scroll_y);
                scroll_y += 1; 
                if (scroll_y > (CREDITS_LINES * 16) + 224) scroll_y = -224; 
                
                if (joy & BUTTON_START) { 
                    VDP_setVerticalScroll(BG_A, 0); // Reset hardware scroll registers
                    sys_state = MODE_TERM; 
                    ClearConsole(); // CRITICAL: This now sweeps columns 0-32 to drop the credits tiles!
                    DrawPrompt(); 
                    waitMs(300); 
                }
                waitMs(16); 
                break;

            case MODE_HEX:
                // 1. Force the hardware register status to refresh
                JOY_update();
                
                // 2. Check the real-time pad type definition
                if (JOY_getJoypadType(JOY_1) != JOY_TYPE_PAD6) {
                    RunHardwareLockout();
                    
                    // AUTO-PASS OVERRIDE: If the controller initializes while on this screen,
                    // automatically force a console clear and drop straight into the tool!
                    if (JOY_getJoypadType(JOY_1) == JOY_TYPE_PAD6) {
                        ClearConsole();
                        waitMs(100);
                    }

                    // Read raw active inputs directly inside the block to handle screen exits manually
                    u16 lockout_joy = JOY_readJoypad(JOY_1);
                    if (lockout_joy & BUTTON_START) { 
                        sys_state = MODE_TERM; 
                        ClearConsole(); 
                        DrawPrompt(); 
                        waitMs(300); // Input debounce
                    }
                } else {
                    // 3. Safe Zone: 6-Button Pad fully authenticated.
                    RunHexScan();
                    if (joy & BUTTON_UP) { mem_addr -= 4; if(mem_addr < MIN_SAFE_ADDR) mem_addr = MIN_SAFE_ADDR; }
                    if (joy & BUTTON_DOWN) mem_addr += 4;
                    if ((joy & BUTTON_START) && (joy & BUTTON_C)) { hex_locked = !hex_locked; waitMs(300); } 
                    else if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                    waitMs(60);
                }
                break;
                
            case MODE_CALC:
                if (joy & BUTTON_A) { calc_slot = !calc_slot; UpdateCalcUI(); waitMs(200); }
                if (joy & BUTTON_B) { calc_op = (calc_op + 1) % 4; UpdateCalcUI(); waitMs(200); }
                if (joy & BUTTON_UP) { if(calc_slot==0) opA += FIX32(1); else opB += FIX32(1); UpdateCalcUI(); }
                if (joy & BUTTON_DOWN) { if(calc_slot==0) opA -= FIX32(1); else opB -= FIX32(1); UpdateCalcUI(); }
                if (joy & BUTTON_C) {
                    if(calc_op == 0) res = opA + opB;
                    else if(calc_op == 1) res = opA - opB;
                    else if(calc_op == 2) res = F32_mul(opA, opB); 
                    else if(calc_op == 3) { 
                        if (opB == 0) { 
                            VDP_setTextPalette(PAL3);
                            VDP_drawText("ERR: DIV BY 0!", MARGIN_X+4, 20); 
                            VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
                            waitMs(1000); 
                        } else { 
                            res = F32_div(opA, opB); 
                        } 
                    }
                    UpdateCalcUI(); waitMs(250);
                }
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_OPTIONS:
                RunOptionsMenu();
                
                if (joy & BUTTON_UP || joy & BUTTON_DOWN) { 
                    calc_slot = !calc_slot; 
                    waitMs(150); 
                }
                if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT) || (joy & BUTTON_A)) {
                    if (calc_slot == 0) { calc_op = !calc_op; } 
                    else { opt_theme_idx = !opt_theme_idx; } 
                    waitMs(200);
                }
                if (joy & BUTTON_START) { 
                    opt_save_flash = 30; 
                    waitMs(200);
                }
                if (joy & BUTTON_B) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_PONG:
                RunPong();
                if (joy & BUTTON_UP) p1_y -= 4; 
                if (joy & BUTTON_DOWN) p1_y += 4;
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_SNAKE:
                // 1. Poll input at a blistering 60Hz (no waitMs freezing!)
                if (joy & BUTTON_UP    && dy == 0) { dx = 0;  dy = -1; }
                if (joy & BUTTON_DOWN  && dy == 0) { dx = 0;  dy = 1;  }
                if (joy & BUTTON_LEFT  && dx == 0) { dx = -1; dy = 0;  }
                if (joy & BUTTON_RIGHT && dx == 0) { dx = 1;  dy = 0;  }
                
                if (joy & BUTTON_START) { 
                    sys_state = MODE_TERM; 
                    ClearConsole(); 
                    DrawPrompt(); 
                    waitMs(300); 
                }

                // 2. Control game speed cleanly using the 60Hz system clock ticks
                snake_update_timer++;
                if (snake_update_timer >= 5) { // Moves the snake every 5 frames (~12 steps a second)
                    TickSnake();
                    snake_update_timer = 0;
                }
                break;

            case MODE_TASKS:
                RunTaskMgr();
                if (joy & BUTTON_START) { VDP_clearPlane(BG_B, TRUE); sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_SYSINFO:
                RunSysInfo();
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;
                
            case MODE_SOUND:
                RunSoundTest();
                if (joy & BUTTON_UP) { psg_pitch += 5; waitMs(50); }
                if (joy & BUTTON_DOWN && psg_pitch > 5) { psg_pitch -= 5; waitMs(50); }
                if (joy & BUTTON_A) { PSG_setEnvelope(0, 15); PSG_setTone(0, psg_pitch); }
                if (joy & BUTTON_B) { PSG_setEnvelope(0, 0); }
                
                if (joy & BUTTON_START) { 
                    PSG_setEnvelope(0, 0); 
                    PSG_reset(); 
                    sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); 
                }
                break;
                
            case MODE_PALTEST:
                VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
                VDP_drawText("--- CRAM PALETTE TEST ---", MARGIN_X, 10);
                VDP_setTextPalette(PAL1); VDP_drawText("PAL1: LIGHT BLUE (INFO)", MARGIN_X, 12);
                VDP_setTextPalette(PAL2); VDP_drawText("PAL2: GREEN (SUCCESS)", MARGIN_X, 13);
                VDP_setTextPalette(PAL3); VDP_drawText("PAL3: RED (WARNING)", MARGIN_X, 14);
                
                VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
                VDP_drawText("PRESS [START] TO EXIT", MARGIN_X, 17);
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_DISTEST:
                VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
                VDP_drawText("X00====================H32", 0, 8);
                VDP_drawText("| MARGIN BOUNDS ALIGN  |", 0, 12);
                VDP_drawText("X32=======================", 0, 16);
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_JOYTEST:
                VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
                VDP_drawText("--- I/O BUS PAD TEST ---", MARGIN_X, 10);
                char joy_buf[32];
                sprintf(joy_buf, "RAW I/O: %04X", joy);
                VDP_drawText(joy_buf, MARGIN_X, 12);
                
                if (JOY_getJoypadType(JOY_1) == JOY_TYPE_PAD6) {
                    VDP_setTextPalette(PAL2); 
                    VDP_drawText("DETECTED: 6-BUTTON OK", MARGIN_X, 14);
                } else {
                    VDP_setTextPalette(PAL3); 
                    VDP_drawText("DETECTED: 3-BTN LEGACY", MARGIN_X, 14);
                }
                VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_TETRIS:
                break;
        }
        if (sys_state != MODE_CREDITS && sys_state != MODE_TETRIS) UpdateSystemBar(); 
        SYS_doVBlankProcess(); 
    }
    return 0;
}

// --- HARDWARE WRAPPERS ---
void ClearConsole() { 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1); 
    VDP_clearTextArea(0, 3, 32, 23); 
    
    if (sys_state == MODE_TERM) {
        VDP_setTextPalette(PAL1); // Light Blue!
        VDP_drawText(SYSTEM_VERSION, 1, 3);
        VDP_drawText(SYSTEM_COPYRIGHT, 1, 4);
        VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1); // Back to theme layout
    }
    cursor_y = TERM_Y_START; 
}

void PrintLine(const char* msg) { 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    VDP_drawText(msg, MARGIN_X, cursor_y); 
    if (cursor_y >= MAX_Y) ClearConsole(); else cursor_y++; 
}

void DrawPrompt() { 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    VDP_drawText("PS C:\\POWER> ", MARGIN_X, cursor_y); 
    VDP_drawText(cmd_list[cmd_ptr], MARGIN_X + 13, cursor_y); 
}

void RunHardwareLockout() {
    VDP_setTextPalette(PAL3); // Force Warning Red layout index
    VDP_clearTextArea(0, 10, 32, 11); // Clean sweep width wise to clear ghosts
    
    VDP_drawText("!! HARDWARE LOCKOUT !!", MARGIN_X, 10);
    VDP_drawText("----------------------", MARGIN_X, 11);
    VDP_drawText("IO ERROR: 6-BTN NEEDED", MARGIN_X, 13);
    VDP_drawText("VRAM ACC RESTRICTED", MARGIN_X, 15);
    VDP_drawText("PRESS [START] -> RETURN", MARGIN_X, 18);
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1); // Revert back to skin profile
}

void RunOptionsMenu() {
    VDP_clearTextArea(0, 10, 32, 11);
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    
    VDP_drawText("--- SYSTEM CONFIG ---", MARGIN_X, 11);
    VDP_drawText(calc_slot == 0 ? "> [AUDIO] :" : "  [AUDIO] :", MARGIN_X, 13);
    VDP_drawText(calc_op == 0 ? "XGM ENGINE " : "SN_PSG ONLY", MARGIN_X + 13, 13);
    
    VDP_drawText(calc_slot == 1 ? "> [THEME] :" : "  [THEME] :", MARGIN_X, 14);
    VDP_drawText(opt_theme_idx == 0 ? "CLASSIC   " : "CYAN_MOD  ", MARGIN_X + 13, 14);
    VDP_drawText("PRESS [START] TO COMMIT", MARGIN_X, 17);
    
    if (opt_save_flash > 0) {
        VDP_setTextPalette(PAL2); 
        VDP_drawText(">>> [ SUCCESS ] <<<", MARGIN_X+4, 19);
        VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
        opt_save_flash--;
    }
}

void RunOSK() { 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    const char (*active_grid)[11] = (osk_layout == 0) ? osk_alpha : (osk_layout == 1 ? osk_num : osk_hex); 
    for(int i=0; i<3; i++) VDP_drawText(active_grid[i], MARGIN_X + 4, 14 + (i*2)); 
    VDP_drawText("[ ]", MARGIN_X + 3 + osk_cx, 14 + (osk_cy*2)); 
    char print_buf[32];
    sprintf(print_buf, "%-26s", cmd_buffer);
    VDP_drawText(print_buf, MARGIN_X + 5, 21); 
}

void RunHexScan() { 
    char buf[32]; 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    VDP_drawText("--- VRAM VIEWER ---", MARGIN_X+4, 12);
    for(u16 i=0; i<5; i++) { 
        u32 cur = mem_addr + (i*4); 
        u32 val = *(u32*)cur; 
        sprintf(buf, "%06lX:%08lX", cur, val); 
        VDP_drawText(buf, MARGIN_X+4, 14+i); 
    } 
}

void SetupCredits() { 
    // 1. Completely wipe Plane A to clear out the previous terminal prompt
    VDP_clearPlane(BG_A, TRUE); 
    
    // 2. Force the palette to standard white so the credits are completely visible
    VDP_setTextPalette(PAL0);
    
    // 3. Reset your global scroll offset variable to baseline zero
    scroll_y = 0;
    VDP_setVerticalScroll(BG_A, 0);
    
    // 4. Paint the entire credits array onto the plane line-by-line
    // Shifting each row down by 2 vertical tile blocks (i * 2) gives a clean layout
    for (s16 i = 0; i < CREDITS_LINES; i++) {
        VDP_drawText(credits_text[i], MARGIN_X, 10 + (i * 2));
    }
    
    // 5. Flip the master kernel state over to execute the scrolling loop safely
    sys_state = MODE_CREDITS; 
}

void RunSoundTest() { 
    char buf[32];
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    VDP_drawText("--- Z80 SOUND LAB ---", MARGIN_X, 12);
    sprintf(buf, "PITCH VAL: %04u  ", psg_pitch); 
    VDP_drawText(buf, MARGIN_X, 14); 
    VDP_drawText("[UP/DN] TUNE PITCH", MARGIN_X, 16);
    VDP_drawText("[A] PLAY  [B] MUTE", MARGIN_X, 17);
}

void RunSysInfo() {
    VDP_clearTextArea(0, 10, 32, 12);
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    
    VDP_drawText("--- SYSTEM INFO ---", MARGIN_X, 10);
    VDP_drawText("CPU  : M68000 (7.61MHz)", MARGIN_X, 12);
    VDP_drawText("RAM  : 64KB WORK RAM", MARGIN_X, 13);
    VDP_drawText("VRAM : 64KB VIDEO RAM", MARGIN_X, 14);
    VDP_drawText("VDP  : H32 MODE", MARGIN_X, 15);
    VDP_drawText("AUDIO: Z80+YM2612+PSG", MARGIN_X, 16);
    VDP_drawText("OS   : POWERCORE V7.0.1", MARGIN_X, 17);
    
    VDP_drawText("PRESS [START] TO RETURN", MARGIN_X, 20);
}

void RunTaskMgr() { 
    char buf[32];
    VDP_setTextPlane(BG_B);
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    
    VDP_drawText("[SYSTEM METRICS]", MARGIN_X, 10); 
    VDP_drawText("----------------", MARGIN_X, 11);
    
    u16 current_ms = vblank_to_ms[vblank_frames % 60];
    u8 s = (vblank_frames / 60) % 60;
    u8 m = (vblank_frames / 3600) % 60;
    u8 h = (vblank_frames / 216000);
    sprintf(buf, "UP: %02d:%02d:%02d.%03d", h, m, s, current_ms);
    VDP_drawText(buf, MARGIN_X, 13); 

    sprintf(buf, "ALLOC : %06u B", MEM_getAllocated());
    VDP_drawText(buf, MARGIN_X, 14); 
    sprintf(buf, "FREE  : %06u B", MEM_getFree());
    VDP_drawText(buf, MARGIN_X, 15); 
    
    VDP_drawText("SPRITES: 14 / 80", MARGIN_X, 16);
    VDP_drawText("CPU Ld : ||||....", MARGIN_X, 17);
    VDP_drawText("[START] TO RETURN", MARGIN_X, 19);
    
    VDP_setTextPalette(PAL0);
    VDP_setTextPlane(BG_A);
}

void UpdateCalcUI() { 
    char buf[32]; 
    VDP_clearTextArea(MARGIN_X, 11, 28, 12); 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    VDP_drawText("--- 16.16 CORE ---", MARGIN_X, 11);
    
    fix32ToStr(opA, buf, 4); 
    VDP_drawText(calc_slot == 0 ? "> [A]:" : "  [A]:", MARGIN_X+2, 13); 
    VDP_drawText(buf, MARGIN_X+9, 13); 
    
    fix32ToStr(opB, buf, 4); 
    VDP_drawText(calc_slot == 1 ? "> [B]:" : "  [B]:", MARGIN_X+2, 15); 
    VDP_drawText(buf, MARGIN_X+9, 15); 
    
    const char* ops[] = {"ADD (+)", "SUB (-)", "MUL (*)", "DIV (/)"}; 
    VDP_drawText(ops[calc_op], MARGIN_X+9, 17); 
    VDP_drawText("----------------", MARGIN_X+2, 19); 
    
    fix32ToStr(res, buf, 4); 
    VDP_drawText("  [RES]:", MARGIN_X+2, 21); 
    VDP_drawText(buf, MARGIN_X+9, 21); 
}

void TickSnake() { 
    // Clear out the trailing tail tile from the VDP plane canvas
    VDP_drawText(" ", snake_body[s_len-1].x, snake_body[s_len-1].y); 
    
    // Shift your body segments forward
    for(u16 i = s_len - 1; i > 0; i--) snake_body[i] = snake_body[i-1]; 
    
    // Step the snake head forward based on current velocities
    snake_body[0].x += dx; 
    snake_body[0].y += dy; 
    
    // 💀 CRASH CHECK: Handle solid grid walls boundaries
    if(snake_body[0].x < 1 || snake_body[0].x > 30 || snake_body[0].y < TERM_Y_START || snake_body[0].y > MAX_Y) {
        sys_state = MODE_TERM;
        ClearConsole();
        PrintLine("> ERR: SEG_FAULT (SNAKE_CRASH)"); 
        DrawPrompt();
        return; 
    } 
    
    // 🍎 APPLE CHECK: Did the head touch the apple?
    if (snake_body[0].x == apple_pos.x && snake_body[0].y == apple_pos.y) {
        if (s_len < SNAKE_MAX) {
            s_len++; // Grow the tail array profile
        }
        
        // Relocate the apple to a random safe spot within our display bounds
        // (Ensuring it aligns with our H32 terminal margins)
        apple_pos.x = 2 + (random() % 26);
        apple_pos.y = TERM_Y_START + (random() % 12);
    }
    
    // Render the apple onto the display canvas using custom Palette 3 (Warning Red)
    VDP_setTextPalette(PAL3);
    VDP_drawText("@", apple_pos.x, apple_pos.y);
    
    // Render the snake head onto the canvas using your selected active skin layout theme
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    VDP_drawText("O", snake_body[0].x, snake_body[0].y); 
}

void RunPong() { 
    VDP_setTextPalette(opt_theme_idx == 0 ? PAL0 : PAL1);
    ball_pos.x += ball_vel.x; 
    ball_pos.y += ball_vel.y; 
    if (ball_pos.y < 70 || ball_pos.y > 200) ball_vel.y = -ball_vel.y; 
    if (ai_y + 8 < ball_pos.y) ai_y += 2; 
    if (ai_y + 8 > ball_pos.y) ai_y -= 2; 
    if (ball_pos.x < 30 && ball_pos.y > p1_y && ball_pos.y < p1_y + 24) ball_vel.x = 3; 
    if (ball_pos.x > 220 && ball_pos.y > ai_y && ball_pos.y < ai_y + 24) ball_vel.x = -3; 
    if (ball_pos.x < 10 || ball_pos.x > 240) ball_pos.x = 128; 
    VDP_clearTextArea(2, 8, 30, 20); 
    VDP_drawText("|", 4, p1_y / 8); 
    VDP_drawText("|", 28, ai_y / 8); 
    VDP_drawText("o", ball_pos.x / 8, ball_pos.y / 8); 
}
