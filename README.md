# TITAN-OS-Genesis
A custom 16-bit executive kernel for the Sega Genesis.

TITAN OS is an original executive kernel and terminal environment developed for the Motorola 68000 architecture. Unlike a standard game, this project focuses on system-level utilities, hardware telemetry, and an integrated multitasking state machine, all running at the original hardware speed of 7.61MHz.

---

## 🚀 Key Features
*   **H32 Optimized Display:** Custom narrow-mode scaling (256x224) to ensure UI stability across modern and CRT displays.
*   **Live Telemetry:** Real-time CPU Load Meter and RAM monitoring utilizing VDP Plane B interleaving.
*   **Integrated Task Manager:** Active process monitoring and system state tracking.
*   **Utility Suite:** 
    *   **CALC:** A full 16-bit decimal calculator with operator cycling.
    *   **HEXSCAN:** A live memory peeker for scanning Work RAM (0xFF0000).
    *   **PONG/SNAKE:** Optimized game modules with dirty-tile rendering for zero-flicker performance.
*   **Pure 68k Logic:** Optimized C code using SGDK 2.11 with no heavy Z80 audio overhead for maximum bus stability.

---

## 🎮 Controls
| Mode | Action | Input |
| :--- | :--- | :--- |
| **Terminal** | Navigate Commands | D-Pad Up/Down |
| **Terminal** | Execute Command | Button A / Start |
| **Calculator** | Edit Value A/B | Button A |
| **Calculator** | Cycle Operator | Button B |
| **Calculator** | **EXECUTE** | **Button C** |
| **Global** | Return to Terminal | Start |

---

## 🛠️ Technical Specifications
*   **CPU:** Motorola 68000 (Primary)
*   **Language:** C (SGDK 2.11 / GCC M68k)
*   **Graphics:** 2-Plane VDP Logic (Plane A: Apps, Plane B: HUD)
*   **Resolution:** 256 x 224 (Narrow H32)

---

## 📦 Building from Source
To compile the project yourself, you will need the [SGDK](https://github.com/Stephane-D/SGDK) environment installed.
1.  Place the project folder in your SGDK/projects directory.
2.  Run `Build.bat`.
3.  The compiled ROM will be located in `out/rom.bin`.

---

## 📄 License
This project is released under the MIT License. (C) Sega Corp. 2026.
