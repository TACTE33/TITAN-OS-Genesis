# SEGA PowerShell 

**Version:** 7.0.0 (The Nuclear Update)  
**Platform:** Sega Genesis / Mega Drive  
**Architecture:** Motorola 68000 & Zilog Z80
**Framework:** SGDK 2.11

## Overview
SEGA PowerShell is a custom 16-bit executive kernel and multi-tasking simulated environment built for original Sega Genesis hardware. It utilizes a highly optimized switch-case state machine, allowing seamless, zero-reset transitions between the command terminal and real-time utility modules.

## Key Features

* **Dual-Plane Interleaved UI:** System telemetry and the status bar are mapped to VDP Plane B, ensuring real-time CPU load tracking remains visible while applications render to VDP Plane A.
* **Cinematic Boot Sequence:** Features a DMA-driven hardware palette fade synchronized with a 16kHz Mono PCM boot chant utilizing the XGM audio driver.
* **Dynamic Memory Tracking:** The Task Manager polls SGDK's internal `MEM_getFree()` and `MEM_getAllocated()` functions to display Work RAM usage in real-time.
* **Global Hardware Mute:** A system-wide audio panic switch (Button Z) designed to kill XGM and PSG channels instantly to prevent hardware lockups.

## Integrated Modules

* **1989 GHOST (Tetris):** A fully functional arcade engine featuring bitmask-based collision detection, line-clear combo scoring, and dynamic gravity leveling.
* **Z80 Sound Lab:** A direct hardware interface for the SN76489 PSG chip. Allows users to manually tune pitch dividers and trigger test frequencies.
* **HexScanner:** A real-time memory viewer with an unlockable Write Access mode (START + C) protected by safety boundaries.
* **SNAKE & PONG:** Interactive modules demonstrating real-time controller polling and physics.

## Hardware Requirements
* **Console:** Sega Genesis Model 1/2/3 or accurate clones.
* **Controller:** **6-Button Arcade Pad is MANDATORY**.
* **ROM Size:** ~128 KB.

## Master Control Scheme

| Input | Global Action |
| :--- | :--- |
| **D-Pad** | Navigate Menus / Adjust Values / Movement |
| **Button A** | Execute Command / OSK Confirm |
| **Button X** | Toggle On-Screen Keyboard (OSK) |
| **Button Y** | Cycle OSK Layout (Alpha / Numeric / Hex) |
| **Button Z** | **GLOBAL AUDIO PANIC (Mute all channels)** |
| **Button C** | Execute / Unlock HexScanner Write Mode |
| **START** | Exit Active Module / Return to Terminal |

SEGA PowerShell for the Sega Mega Drive. (SGDk 2.11 is needed to use the files. NOT needed for the release file.)

(DO NOT PRESS START TO ENTER TETRIS!!! Instead, press A.)
