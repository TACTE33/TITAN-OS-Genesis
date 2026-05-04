/*
 * ============================================================================
 * SEGA CMD OS v6.0.1 (FINAL MASTER RELEASE)
 * ----------------------------------------------------------------------------
 * System: Motorola 68000 @ 7.61MHz | SDK: SGDK 2.11
 * Graphics: H32 (256x224) | UI: Dual-Plane Interleaving
 * ----------------------------------------------------------------------------
 * CONTROLS (CALC): A: Swap Slot | B: Swap Op | D-Pad: Value | C: EXECUTE
 * CONTROLS (EXIT): START (Enter) returns to PowerShell Terminal.
 * ============================================================================
 */

#include <genesis.h>
#include <string.h>

// --- KERNEL DEFINITIONS ---
#define KERNEL_VER "6.0.1"
#define TERM_Y_START 10
#define MAX_Y 24
#define MARGIN_X 2

typedef enum { MODE_INTRO, MODE_TERM, MODE_SNAKE, MODE_CALC, MODE_HEX, MODE_PONG, MODE_TASKS } OSState;
OSState sys_state = MODE_INTRO;

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
void ProcessCommand();

// --- KERNEL GLOBAL DATA ---
u16 cursor_y = TERM_Y_START;
s16 cmd_ptr = 0;
const char* cmd_list[] = {"TASKS", "PONG", "CALC", "SNAKE", "HEXSCAN", "SYSINFO", "REBOOT"};

// --- SHARED APP STATES ---
#define SNAKE_MAX 64
Vect2D_s16 snake_body[SNAKE_MAX];
u16 s_len = 6;
s16 dx = 1, dy = 0;

Vect2D_s16 ball_pos = {128, 112}, ball_vel = {2, 2};
s16 p1_y = 100, ai_y = 100;

fix32 opA = FIX32(0), opB = FIX32(0), res = FIX32(0);
u8 calc_slot = 0, calc_op = 0;
u32 mem_addr = 0xFF0000;

// ============================================================================
// SYSTEM KERNEL CORE
// ============================================================================

void UpdateSystemBar() {
    VDP_setTextPlane(BG_B); 
    VDP_drawText("[TITAN-CORE: OK]", MARGIN_X, 2);
    u16 load = SYS_getCPULoad(); 
    VDP_drawText("CPU:", 21, 2);
    if (load > 50)      VDP_drawText("####", 25, 2);
    else if (load > 20) VDP_drawText("##..", 25, 2);
    else                VDP_drawText("#...", 25, 2);
    VDP_setTextPlane(BG_A);
}

void ShowSegaIntro() {
    VDP_clearPlane(BG_A, TRUE);
    waitMs(500);
    VDP_drawText("S E G A", 12, 12);
    waitMs(1500); 
    VDP_clearPlane(BG_A, TRUE);
    cursor_y = 4;
    PrintLine("TITAN KERNEL V6.0.1 MASTER...");
    waitMs(400);
    PrintLine("BOOTING MODULES... OK");
    waitMs(600);
    VDP_clearPlane(BG_A, TRUE);
    sys_state = MODE_TERM;
    cursor_y = TERM_Y_START; 
}

// ============================================================================
// CORE MODULES
// ============================================================================

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

void RunPong() {
    ball_pos.x += ball_vel.x; ball_pos.y += ball_vel.y;
    if (ball_pos.y < 70 || ball_pos.y > 200) ball_vel.y = -ball_vel.y;
    if (ai_y + 8 < ball_pos.y) ai_y += 2; if (ai_y + 8 > ball_pos.y) ai_y -= 2;
    if (ball_pos.x < 30 && ball_pos.y > p1_y && ball_pos.y < p1_y + 24) ball_vel.x = 3;
    if (ball_pos.x > 220 && ball_pos.y > ai_y && ball_pos.y < ai_y + 24) ball_vel.x = -3;
    if (ball_pos.x < 10 || ball_pos.x > 240) ball_pos.x = 128;
    
    VDP_clearTextArea(2, 8, 30, 20);
    VDP_drawText("|", 4, p1_y / 8); 
    VDP_drawText("|", 28, ai_y / 8);
    VDP_drawText("o", ball_pos.x / 8, ball_pos.y / 8);
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
        case 5: PrintLine("> TITAN KERNEL STABLE"); break;
        case 6: SYS_hardReset(); break;
    }
    if (sys_state == MODE_TERM) DrawPrompt();
}

// ============================================================================
// MAIN HARDWARE ENTRY
// ============================================================================

int main() {
    VDP_setScreenWidth256(); 
    PAL_setPalette(PAL1, font_pal_default.data, DMA); 
    ShowSegaIntro();
    UpdateSystemBar();
    DrawPrompt();

    while(1) {
        u16 joy = JOY_readJoypad(JOY_1);
        
        switch(sys_state) {
            case MODE_TERM:
                if (joy & BUTTON_UP) { cmd_ptr = (cmd_ptr > 0) ? cmd_ptr - 1 : 6; DrawPrompt(); waitMs(150); }
                if (joy & BUTTON_DOWN) { cmd_ptr = (cmd_ptr + 1) % 7; DrawPrompt(); waitMs(150); }
                if (joy & BUTTON_A || joy & BUTTON_START) { ProcessCommand(); waitMs(300); }
                break;

            case MODE_CALC:
                if (joy & BUTTON_A) { calc_slot = !calc_slot; UpdateCalcUI(); waitMs(200); }
                if (joy & BUTTON_B) { calc_op = (calc_op + 1) % 4; UpdateCalcUI(); waitMs(200); }
                if (joy & BUTTON_UP) { if(calc_slot==0) opA += FIX32(1); else opB += FIX32(1); UpdateCalcUI(); }
                if (joy & BUTTON_DOWN) { if(calc_slot==0) opA -= FIX32(1); else opB -= FIX32(1); UpdateCalcUI(); }
                
                // EXECUTE CALCULATION (C BUTTON)
                if (joy & BUTTON_C) {
                    if(calc_op == 0) res = opA + opB;
                    else if(calc_op == 1) res = opA - opB;
                    else if(calc_op == 2) res = F32_mul(opA, opB);
                    else if(calc_op == 3 && opB != 0) res = F32_div(opA, opB);
                    UpdateCalcUI(); waitMs(250);
                }
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_PONG:
                RunPong();
                if (joy & BUTTON_UP) p1_y -= 4; if (joy & BUTTON_DOWN) p1_y += 4;
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;

            case MODE_SNAKE:
                TickSnake();
                if (joy & BUTTON_UP && dy == 0) { dx = 0; dy = -1; }
                if (joy & BUTTON_DOWN && dy == 0) { dx = 0; dy = 1; }
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                waitMs(80);
                break;

            case MODE_HEX:
                RunHexScan();
                if (joy & BUTTON_UP) mem_addr -= 4; if (joy & BUTTON_DOWN) mem_addr += 4;
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                waitMs(60);
                break;

            case MODE_TASKS:
                RunTaskMgr();
                if (joy & BUTTON_START) { sys_state = MODE_TERM; ClearConsole(); DrawPrompt(); waitMs(300); }
                break;
        }

        UpdateSystemBar(); 
        SYS_doVBlankProcess(); 
    }
    return 0;
}

// --- HARDWARE WRAPPERS ---
void ClearConsole() { VDP_clearTextArea(MARGIN_X, TERM_Y_START, 28, 14); cursor_y = TERM_Y_START; }
void PrintLine(const char* msg) { VDP_drawText(msg, MARGIN_X, cursor_y); if (cursor_y >= MAX_Y) ClearConsole(); else cursor_y++; }
void DrawPrompt() { VDP_drawText("PS C:\\TITAN> ", MARGIN_X, cursor_y); VDP_drawText(cmd_list[cmd_ptr], MARGIN_X + 13, cursor_y); }
void RunHexScan() { char buf[32]; VDP_clearTextArea(MARGIN_X, 12, 28, 8); for(u16 i=0; i<5; i++) { u32 cur = mem_addr + (i*4); u32 val = *(u32*)cur; sprintf(buf, "%06lX:%08lX", cur, val); VDP_drawText(buf, MARGIN_X+4, 14+i); } }
void RunTaskMgr() { VDP_clearTextArea(MARGIN_X, 11, 28, 10); VDP_drawText("- TASK MANAGER -", MARGIN_X+4, 11); VDP_drawText("PID: 001 STATUS: OK", MARGIN_X+4, 13); VDP_drawText("RAM_ALLOC: 512B", MARGIN_X+4, 14); }
void TickSnake() { VDP_drawText(" ", snake_body[s_len-1].x, snake_body[s_len-1].y); for(u16 i=s_len-1; i>0; i--) snake_body[i] = snake_body[i-1]; snake_body[0].x += dx; snake_body[0].y += dy; if(snake_body[0].x < 1 || snake_body[0].x > 30 || snake_body[0].y < TERM_Y_START || snake_body[0].y > MAX_Y) { sys_state = MODE_TERM; PrintLine("> ERR: SEG_FAULT"); return; } VDP_drawText("O", snake_body[0].x, snake_body[0].y); }