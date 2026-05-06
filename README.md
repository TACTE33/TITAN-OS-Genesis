(Note: "TITAN-CORE" name is discontinued".)

# 💻 SEGA PowerShell (SPS) — Genesis/Mega Drive Kernel

![Version](https://img.shields.io/badge/Version-v7.0.1_MASTER-00f2ff?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Sega_Genesis-111111?style=for-the-badge&logo=sega)
![Compiler](https://img.shields.io/badge/Built_With-SGDK_v2.11-success?style=for-the-badge)

**SEGA PowerShell (SPS)** is a custom 16-bit executive kernel and operating system shell built from the bare metal up for the Sega Genesis / Mega Drive. Powered by the **PowerCORE v7.0.1 Engine**, SPS bypasses standard game loops to transform the console into a fully functional, command-driven diagnostic workstation, retro-application launcher, and development environment.

---

## ⚙️ Bare-Metal Specifications
* **CPU Target:** Motorola 68000 @ 7.61 MHz
* **Graphics Subsystem:** VDP H32 Canvas Mode (256x224 Active Matrix)
* **UI Rendering:** Dual-Plane Interleaving (Plane A for Text, Plane B for Background UI)
* **Audio Engine:** Z80 + YM2612 (FM) + SN76489 (PSG) + XGM PCM Driver
* **Input Requirement:** 6-Button Arcade Pad (Strict I/O Multiplexer Validation)

---

## 🚀 What's New in v7.0.1 MASTER (Patch Notes)
The v7.0.1 update focuses on extreme hardware stability, cycle-accurate timing, and application upgrades:
* **60Hz Snake Engine Rewrite:** Completely stripped out `waitMs` blocking delays. Snake now polls inputs at a true 60Hz using a custom `snake_update_timer`, allowing instant double-turn inputs. Added VDP Palette 3 (Red) Apples (`@`) and dynamic array-based tail expansion.
* **Kega Fusion / Hardware I/O Handshake Fix:** Implemented a 5-frame VBlank delay during the cold boot sequence. This forces cycle-accurate emulators (like Kega Fusion) to properly pulse the TH multiplexer pin, resolving the 3-button false-positive lockout bug.
* **Dynamic Palette Matrices:** Overhauled the CRAM to support a global `opt_theme_idx` (Classic White vs. Cyan Mod) alongside hardcoded system palettes (PAL2 = Success Green, PAL3 = Warning Red).
* **VRAM Memory Leak Sweeps:** Exit routines across all applications (especially `CREDITS`) now trigger explicit column sweeps to drop orphaned text tiles from Plane A before returning to the terminal.

---

## 🎮 Master Control Interface

### Global System Shortcuts
No matter where you are in the terminal, these hardware interrupts are active:
* **D-PAD UP/DOWN:** Cycle through the main terminal command selector list.
* **BUTTON A / START:** Execute the currently selected terminal module.
* **BUTTON X:** Instantly pull up the On-Screen Keyboard (OSK) overlay.
* **BUTTON Z:** Hardware Audio Killswitch (Mutes all PCM and PSG envelopes instantly).

### The System Bar (Top Screen)
The background UI plane constantly monitors the hardware state:
* `[POWERCORE: OK]` - Kernel loop is stable.
* `CPU:#0000` - Live hexadecimal dump of the vertical blanking frame counter.
* `![3-BTN PAD]` - **(Flashing Red)** Warning that a 6-button controller is not synced.
* `[WARN: UNLOCKED]` - Alerts the user that memory constraints are disabled in the Hex Viewer.

---

## ⌨️ The On-Screen Keyboard (OSK) & Shell
Press **X** at the command prompt to open the OSK. The OSK intercepts raw string macros and feeds them to the background `ParseStringCommand()` interpreter.

**OSK Controls:**
* **D-Pad:** Move cursor `[ ]` across the character grid.
* **Button Y:** Cycle through 3 distinct grids: **QWERTY Alpha**, **Numeric/Symbols**, and **Hexadecimal**.
* **Button A:** Type the highlighted character into the 32-byte buffer.
* **Button START:** Submit the buffer macro to the kernel.

**Valid OSK String Macros:**
* `RUN-STETRIS` — Bypasses the shell to direct-boot the 1989 GHOST Tetris engine.
* `CREDITS` — Triggers the hardware-accelerated VDP vertical scrolling developer credits.
* `CALIB` — Jumps directly to the CRT display calibration matrix.
* `INFO` — Dumps the hardware system specs (CPU, RAM, VRAM sizes).
* `CLEAR` — Performs a clean wipe of the terminal workspace.

---

## 📂 The Application Registry (Module List)
The PowerCORE kernel comes loaded with 13 built-in bare-metal applications. Select them from the prompt `PS C:\POWER>` and press **START**:

### 1. `TASKS` (System Task Manager)
Live hardware profiler. Displays system uptime (HH:MM:SS:ms), SGDK memory heap allocation (Allocated vs. Free bytes), active VDP sprite counts, and a visual CPU load graph.

### 2. `PONG`
A standard 60fps Pong clone pitting the user (Button UP/DOWN) against a basic Y-axis tracking AI.

### 3. `CALC` (16.16 Fixed Point Math Core)
A hardware-level calculator utilizing the Genesis's CPU to process 16.16 fixed-point integers. 
* **Controls:** `A` (Swap Slot A/B), `B` (Cycle Math Operation: Add, Sub, Mul, Div), `UP/DOWN` (Change Value), `C` (Calculate). 
* Includes Division-By-Zero protection (`ERR: DIV BY 0!`).

### 4. `SNAKE` (v7.0.1 Optimized)
High-speed arcade snake. Navigate the H32 boundaries, eat red apples (`@`), and grow your tail memory array. Crashing triggers a `SEG_FAULT` terminal bailout.

### 5. `HEXSCAN` (VRAM/RAM Viewer)
A powerful memory inspection tool. Reads directly from the 68000 memory bus.
* **Controls:** `UP/DOWN` jumps addresses by 4 bytes. `START + C` toggles the memory lock state for deep system scanning.

### 6. `SOUND` (Z80 Sound Lab)
Direct interface with the SN76489 Programmable Sound Generator (PSG).
* **Controls:** `UP/DOWN` to tune pitch values by 5. Press `A` to inject the envelope tone, `B` to mute.

### 7. `TETRIS` (1989 GHOST Engine)
A fully playable Tetris clone running natively in VRAM. Features 7 hardcoded matrix shapes, wall-kick collision detection, line clearing algorithms, and dynamic level/speed scaling based on score. 

### 8. `OPTIONS` (System Configuration)
Modify the kernel state in real-time. Toggle Audio processing between XGM and PSG, or switch the global UI Theme from `CLASSIC` (White) to `CYAN_MOD` (Light Blue). Press `START` to commit changes (flashes a green success array).

### 9. `JOYTEST` (I/O Bus Diagnostic)
Prints the raw, live Hexadecimal bitmask of the controller port. Used to debug physical hardware switches and validates 3-button vs. 6-button multiplexer handshakes.

### 10. `PALTEST` (Color RAM Utility)
Injects test strings into VRAM using the specific 9-bit CRAM palette indices (Light Blue, Green, Red) to verify color bleed and TV signal clarity.

### 11. `DISTEST` (Display Margin Calibration)
Draws absolute boundary markers (`X00` to `H32`) on the far edges of the screen to help calibrate CRT television overscan limits.

### 12. `SYSINFO`
Static dump of the bare-metal environment variables. Identifies CPU clock, RAM sizes, VDP mode, Audio chips, and Kernel OS version.

### 13. `REBOOT`
Triggers a low-level `SYS_hardReset()`, clearing the M68000 registers and warm-booting the console.

---

## 🛡️ Hardware Security Lockout
SPS requires a **6-Button Arcade Pad**. If a standard 3-button controller is detected during boot or while attempting to launch advanced tools (like `HEXSCAN`), the kernel will intercept the execution and throw an `IO ERROR: VRAM ACC RESTRICTED` lockout screen. 

*(Note: v7.0.1 includes an auto-pass override. If the controller handshake finishes while on the lockout screen, the OS automatically drops the firewall and launches the tool).*

---

## 🛠️ Build & Compilation
This OS is built strictly using the **Sega Genesis Development Kit (SGDK)**.

1. Ensure [SGDK v2.11](https://github.com/Stephane-D/SGDK) is installed and configured in your system environment variables.
2. Clone this repository.
3. Run the included `Build.bat` script. This handles the `-m68000`, `-O3` optimization, and `-flto` linker plugins.
4. The compiled output (`out/rom.bin`) can be run on physical hardware via an EverDrive, or tested on cycle-accurate emulators (Kega Fusion, BlastEm).

---

## 🗺️ Roadmap (Target: v7.0.2)
* **Cartridge Bay Frontend (`LAUNCH`):** Pointer-based ROM header extraction to detect, identify, and boot external payload binaries appended to the 1MB hardware boundary (`0x00100000`).
* **Persistent SRAM:** High-score caching utilizing the cartridge's Battery-Backed RAM (`0x200001`) for Tetris and Snake.
* **Live CRAM Customizer:** Granular RGB manipulation to design custom themes directly from the terminal.

---
*SEGA and Sega Genesis are registered trademarks of Sega Corporation. This is an independent homebrew kernel architecture and is not affiliated with Sega.*
