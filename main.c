/*
 * ============================================================================
 * SEGA PowerShell OS v7.0.0 (THE NUCLEAR UPDATE)
 * ----------------------------------------------------------------------------
 * System: Motorola 68000 @ 7.61MHz | SDK: SGDK 2.11
 * Graphics: H32 (256x224) | UI: Dual-Plane Interleaving
 * ----------------------------------------------------------------------------
 * FEATURES: Hardware Fades, Tetris Levels, Dynamic RAM Tasker, PSG Sound Lab
 * ============================================================================
 */

#include <genesis.h>
#include <string.h>
#include "resources.h"

// --- KERNEL DEFINITIONS ---
#define KERNEL_VER "7.0.0"
#define TERM_Y_START 10
#define MAX_Y 24
#define MARGIN_X 2
#define MIN_SAFE_ADDR 0x000400 

typedef enum { MODE_INTRO, MODE_TERM, MODE_SNAKE, MODE_CALC, MODE_HEX, MODE_PONG, MODE_TASKS, MODE_OSK, MODE_CREDITS, MODE_SOUND, MODE_TETRIS } OSState;
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
void RunSoundTest();
void RunTetrisGame();
void ProcessCommand();
void RunOSK();
void ParseStringCommand();
void SetupCredits();

// --- KERNEL GLOBAL DATA ---
u16 cursor_y = TERM_Y_START;
s16 cmd_ptr = 0;
const char* cmd_list[] = {"TASKS", "PONG", "CALC", "SNAKE", "HEXSCAN", "SOUND", "TETRIS", "SYSINFO", "REBOOT"};
#define CMD_COUNT 9

// OSK & String Parsing
char cmd_buffer[32] = "";
u8 buf_len = 0;
u8 osk_cx = 0, osk_cy = 0, osk_layout = 0; 
const char osk_alpha[3][11] = { "QWERTYUIOP", "ASDFGHJKL ", "ZXCVBNM   " };
const char osk_num[3][11]   = { "1234567890", "!@#$%^&*()", "+-/*=.,?  " };
const char osk_hex[3][11]   = { "0123456789", "ABCDEF    ", "          " };

// Tetris Engine Data
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
    " VERSION 7.0.0 MASTER",
    "==========================",
    "",
    "LEAD ARCHITECT:",
    "-> TITAN DEV",
    "",
    "SYSTEM KERNEL:",
    "-> Motorola 68000",
    "-> SGDK 2.11 Framework",
    "",
    "SECRET MODULE:",
    "-> 1989 GHOST (TETRIS)",
    "",
    "==========================",
    " THANK YOU FOR BOOTING."
};
#define CREDITS_LINES 17
s16 scroll_y = 0; 

// Other Module Data
#define SNAKE_MAX 64
Vect2D_s16 snake_body[SNAKE_MAX];
u16 s_len = 6;
s16 dx = 1, dy = 0;
Vect2D_s16 ball_pos = {128, 112}, ball_vel = {2, 2};
s16 p1_y = 100, ai_y = 100;
fix32 opA = FIX32(0), opB = FIX32(0), res = FIX32(0);
u8 calc_slot = 0, calc_op = 0;
u32 mem_addr = 0xFF0000;
u8 hex_locked = 1; 
u16 psg_pitch = 200; // Starting pitch divider for Sound Lab

// ============================================================================
// SYSTEM KERNEL CORE
// ============================================================================

void UpdateSystemBar() {
    VDP_setTextPlane(BG_B); 
    VDP_drawText("[TITAN-CORE: OK]", MARGIN_X, 2);
    if (sys_state == MODE_OSK) VDP_drawText("[INPUT: OSK]    ", 2, 3);
    else if (!hex_locked)      VDP_drawText("[WARN: UNLOCKED]", 2, 3);
    else                       VDP_drawText("                ", 2, 3);
    u16 load = SYS_getCPULoad(); 
    VDP_drawText("CPU:", 21, 2);
    if (load > 50)      VDP_drawText("####", 25, 2);
    else if (load > 20) VDP_drawText("##..", 25, 2);
    else                VDP_drawText("#...", 25, 2);
    VDP_setTextPlane(BG_A);
}

void ShowSegaIntro() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_setTextPalette(PAL1); 
    PAL_setColors(PAL1 * 16, palette_black, 16, DMA); 
    VDP_drawText("S E G A", 12, 12);
    XGM_setPCM(64, wav_sega, sizeof(wav_sega));
    XGM_startPlayPCM(64, 1, SOUND_PCM_CH1);
    PAL_fadeIn(PAL1 * 16, (PAL1 * 16) + 15, font_pal_default.data, 60, FALSE);
    for(u16 i=0; i<60; i++) SYS_doVBlankProcess();
    waitMs(1000); 
    PAL_fadeOut(PAL1 * 16, (PAL1 * 16) + 15, 60, FALSE);
    for(u16 i=0; i<60; i++) SYS_doVBlankProcess();
    VDP_clearPlane(BG_A, TRUE);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL0, font_pal_default.data, DMA);
    cursor_y = 4;
    PrintLine("SEGA PowerShell V7.0.0 BOOTING...");
    waitMs(400);
    PrintLine("LOADING KERNEL... OK");
    waitMs(600);
    VDP_clearPlane(BG_A, TRUE);
    sys_state = MODE_TERM;
    cursor_y = TERM_Y_START; 
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
        if (joy & BUTTON_START) break;
        if (joy & BUTTON_LEFT)  if (!CheckCollision(px - 1, py, prot)) px--;
        if (joy & BUTTON_RIGHT) if (!CheckCollision(px + 1, py, prot)) px++;
        if (joy & BUTTON_A) { 
            s16 next_r = (prot + 1) % 4;
            if (!CheckCollision(px, py, next_r)) prot = next_r;
            waitMs(100); 
        }
        
        drop_timer++;
        u16 max_speed = 20 - (t_level * 2);
        if (max_speed < 2) max_speed = 2; // Clamp maximum drop speed
        
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
        SYS_doVBlankProcess();
    }
    ClearConsole();
    DrawPrompt();
}

// ============================================================================
// MAIN HARDWARE ENTRY & HANDLERS
// ============================================================================

void ParseStringCommand() {
    if (strcmp(cmd_buffer, "RUN -S TETRIS") == 0) { ClearConsole(); sys_state = MODE_TETRIS; RunTetrisGame(); } 
    else if (strcmp(cmd_buffer, "CREDITS") == 0) { ClearConsole(); SetupCredits(); }
    else { PrintLine("> ERR: UNKNOWN COMMAND"); }
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
        case 4: ClearConsole(); sys_state = MODE_HEX; RunHexScan(); break;
        case 5: ClearConsole(); sys_state = MODE_SOUND; break;
        case 6: ClearConsole(); sys_state = MODE_TETRIS; RunTetrisGame(); break;
        case 7: PrintLine("> TITAN KERNEL STABLE"); break;
        case 8: SYS_hardReset(); break;
    }
    if (sys_state == MODE_TERM) DrawPrompt();
}

int main() {
    VDP_setScreenWidth256(); 
    PAL_setPalette(PAL1, font_pal_default.data, DMA); 
    PSG_reset(); // Initialize the Z80 PSG Audio Chip
    ShowSegaIntro();
    UpdateSystemBar();
    DrawPrompt();
    
    while(1) {
        u16 joy = JOY_readJoypad(JOY_1);
        
        // [X] OSK Toggle
        if (joy & BUTTON_X && sys_state != MODE_OSK && sys_state != MODE_CREDITS && sys_state != MODE_TETRIS) {
            prev_state = sys_state; sys_state = MODE_OSK; waitMs(200);
        }

        // [Z] GLOBAL AUDIO PANIC SWITCH (MUTE)
        if (joy & BUTTON_Z) {
            XGM_stopPlay(); // Stops XGM/PCM audio
            for(int i=0; i<4; i++) PSG_setEnvelope(i, 0); // Mutes all PSG channels
            VDP_drawText("[AUDIO MUTED]", 2, 3);
            waitMs(200); 

        }
        
        switch(sys_state) {
            case MODE_INTRO: break;
            
            case MODE_TERM:
                if (joy & BUTTON_UP) { cmd_ptr = (cmd_ptr > 0) ? cmd_ptr - 1 : CMD_COUNT - 1; DrawPrompt(); waitMs(150); }
                if (joy & BUTTON_DOWN) { cmd_ptr = (cmd_ptr + 1) % CMD_COUNT; DrawPrompt(); waitMs(150); }
                if (joy & BUTTON_A || joy & BUTTON_START) { ProcessCommand(); waitMs(300); }
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
                if (joy & BUTTON_START) { VDP_setVerticalScroll(BG_A, 0); sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                waitMs(16); 
                break;

            case MODE_HEX:
                RunHexScan();
                if (joy & BUTTON_UP) { mem_addr -= 4; if(mem_addr < MIN_SAFE_ADDR) mem_addr = MIN_SAFE_ADDR; }
                if (joy & BUTTON_DOWN) mem_addr += 4;
                if ((joy & BUTTON_START) && (joy & BUTTON_C)) { hex_locked = !hex_locked; waitMs(300); } 
                else if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                waitMs(60);
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
                    else if(calc_op == 3) { if (opB == 0) { VDP_drawText("ERR: DIV BY 0!", MARGIN_X+4, 20); waitMs(1000); } else res = F32_div(opA, opB); }
                    UpdateCalcUI(); waitMs(250);
                }
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_PONG:
                RunPong();
                if (joy & BUTTON_UP) p1_y -= 4; 
                if (joy & BUTTON_DOWN) p1_y += 4;
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_SNAKE:
                TickSnake();
                if (joy & BUTTON_UP && dy == 0) { dx = 0; dy = -1; }
                if (joy & BUTTON_DOWN && dy == 0) { dx = 0; dy = 1; }
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                waitMs(80);
                break;

            case MODE_TASKS:
                RunTaskMgr();
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;
                
            case MODE_SOUND:
                RunSoundTest();
                if (joy & BUTTON_UP) { psg_pitch += 5; waitMs(50); }
                if (joy & BUTTON_DOWN && psg_pitch > 5) { psg_pitch -= 5; waitMs(50); }
                if (joy & BUTTON_A) { PSG_setEnvelope(0, 15); PSG_setTone(0, psg_pitch); } // Play
                if (joy & BUTTON_B) { PSG_setEnvelope(0, 0); } // Mute
                
                if (joy & BUTTON_START) { 
                    PSG_setEnvelope(0, 0); // Ensure muted on exit
                    sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); 
                }
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
void ClearConsole() { VDP_clearTextArea(MARGIN_X, TERM_Y_START, 28, 14); cursor_y = TERM_Y_START; }
void PrintLine(const char* msg) { VDP_drawText(msg, MARGIN_X, cursor_y); if (cursor_y >= MAX_Y) ClearConsole(); else cursor_y++; }
void DrawPrompt() { VDP_drawText("PS C:\\TITAN> ", MARGIN_X, cursor_y); VDP_drawText(cmd_list[cmd_ptr], MARGIN_X + 13, cursor_y); }

void RunOSK() { 
    const char (*active_grid)[11] = (osk_layout == 0) ? osk_alpha : (osk_layout == 1 ? osk_num : osk_hex); 
    for(int i=0; i<3; i++) VDP_drawText(active_grid[i], MARGIN_X + 4, 14 + (i*2)); 
    VDP_drawText("[ ]", MARGIN_X + 3 + osk_cx, 14 + (osk_cy*2)); 
    
    // Pad the buffer print with spaces to prevent artifacting without clearing the whole screen
    char print_buf[32];
    sprintf(print_buf, "%-26s", cmd_buffer);
    VDP_drawText(print_buf, MARGIN_X + 5, 21); 
}

void RunHexScan() { 
    char buf[32]; 
    for(u16 i=0; i<5; i++) { 
        u32 cur = mem_addr + (i*4); 
        u32 val = *(u32*)cur; 
        sprintf(buf, "%06lX:%08lX", cur, val); 
        VDP_drawText(buf, MARGIN_X+4, 14+i); 
    } 
}

void SetupCredits() { 
    VDP_clearPlane(BG_A, TRUE); 
    sys_state = MODE_CREDITS; 
}

void RunSoundTest() { 
    char buf[32];
    VDP_drawText("--- Z80 SOUND LAB ---", MARGIN_X, 12); 
    VDP_drawText("SN76489 PSG TESTER", MARGIN_X, 14); 
    
    sprintf(buf, "PITCH VAL: %04u  ", psg_pitch); // Padded to overwrite cleanly
    VDP_drawText(buf, MARGIN_X, 16); 
    VDP_drawText("[UP/DOWN] TUNE PITCH", MARGIN_X, 18); 
    VDP_drawText("[A] PLAY    [B] MUTE", MARGIN_X, 19);
}

void RunTaskMgr() { 
    char buf[32];
    VDP_drawText("- TASK MANAGER -", MARGIN_X+4, 11); 
    VDP_drawText("PID: 001 [TITAN OS]", MARGIN_X+4, 13); 
    
    sprintf(buf, "RAM ALLOC: %06u B", MEM_getAllocated());
    VDP_drawText(buf, MARGIN_X+4, 15); 
    sprintf(buf, "RAM FREE : %06u B", MEM_getFree());
    VDP_drawText(buf, MARGIN_X+4, 16); 
}

void UpdateCalcUI() { 
    char buf[32]; 
    VDP_clearTextArea(MARGIN_X, 12, 28, 10); 
    fix32ToStr(opA, buf, 1); 
    VDP_drawText(calc_slot == 0 ? "> A:" : "  A:", MARGIN_X+4, 12); 
    VDP_drawText(buf, MARGIN_X+10, 12); 
    fix32ToStr(opB, buf, 1); 
    VDP_drawText(calc_slot == 1 ? "> B:" : "  B:", MARGIN_X+4, 14); 
    VDP_drawText(buf, MARGIN_X+10, 14); 
    const char* ops[] = {"ADD (+)", "SUB (-)", "MUL (*)", "DIV (/)"}; 
    VDP_drawText(ops[calc_op], MARGIN_X+10, 16); 
    VDP_drawText("------------------", MARGIN_X+4, 18); 
    fix32ToStr(res, buf, 1); 
    VDP_drawText("RESULT:", MARGIN_X+4, 20); 
    VDP_drawText(buf, MARGIN_X+13, 20); 
}

void TickSnake() { 
    VDP_drawText(" ", snake_body[s_len-1].x, snake_body[s_len-1].y); 
    for(u16 i=s_len-1; i>0; i--) snake_body[i] = snake_body[i-1]; 
    snake_body[0].x += dx; 
    snake_body[0].y += dy; 
    if(snake_body[0].x < 1 || snake_body[0].x > 30 || snake_body[0].y < TERM_Y_START || snake_body[0].y > MAX_Y) { 
        sys_state = MODE_TERM; 
        PrintLine("> ERR: SEG_FAULT"); 
        return; 
    } 
    VDP_drawText("O", snake_body[0].x, snake_body[0].y); 
}

void RunPong() { 
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
