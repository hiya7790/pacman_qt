# 🍒 Pac-Man Labyrinth Saga (Qt 6 C++)

An innovative, feature-packed C++ game built with **Qt 6**, blending classic **Pac-Man** arcade mechanics with **multi-window procedurally generated labyrinths**, **atmospheric fog-of-war lantern lighting**, **1P & 2P Co-op multiplayer**, and a **Candy Crush Saga–style continuous serpentine scrolling map** across infinite candy worlds!

---

## 🌟 Key Features

### 🍭 1. Continuous Serpentine Saga Map (Candy Crush Style)
- **Unified Infinite Snake Road:** All stages and worlds are laid out along one continuous, smoothly undulating S-curve mountain road that scrolls endlessly without disruptive page switches.
- **20 Stages Per World:** Every world features 20 handcrafted stages with progressive difficulty:
  - **World 1:** *Sweet Peppermint Plains* (Twilight plum & raspberry ambiance)
  - **World 2:** *Caramel Lava Caverns* (Molten amber & golden caramel glow)
  - **World 3:** *Gummy Bear Galaxy* (Cosmic indigo & violet nebula)
  - **World 4:** *Cotton Candy Clouds* (Pastel strawberry frost & lavender)
  - **World 5:** *Chocolate Chip Chasm* (Dark mocha & hazelnut bronze)
  - *Infinite procedural worlds continuing beyond World 5!*
- **Dynamic Real-Time Sky Shifting:** The sky gradient dynamically interpolates and morphs in real time as you scroll up and down through the world zones.
- **World Gateway Archways:** Majestic themed archways bridge each world boundary (Stages 20→21, 40→41, etc.), displaying glowing milestone unlocks or shimmering forcefields with golden padlocks for locked worlds.
- **Milestone Rewards:** Jeweled rings on Stages 5, 10, 15, and the golden **Summit Crown Trophy** on Stage 20.
- **Quick Jump & Controls:**
  - **`[ 📍 MY HERO (L%1) ]`:** Click anytime to spring-glide the camera directly to your active hero.
  - **`[ 🏆 TOP: LVL %1 ]`:** Real-time badge tracking your personal record.
  - Right-margin mountain elevation scrollbar, mouse drag, mouse wheel, trackpad, and keyboard scroll support.

---

### 🗺️ 2. Expanding Multi-Window Labyrinth Grid
- **Dynamic Grid Growth:** Instead of a single static maze, the labyrinth is composed of interconnected window sectors that expand as you advance:
  - **Stage 1:** 2×1 grid (2 windows to explore)
  - **Stage 2:** 2×2 grid (4 windows)
  - **Stages 5–6:** 3×3 grid (9 windows)
  - **Stages 11–12:** 4×4 grid (16 windows)
  - **Stages 17–18:** 5×5 grid (25 windows)
  - **Stage 20 (Summit):** Massive **6×6 grid (36 interconnected window sectors)**!
- **Seamless Sector Transitions:** Venture through glowing wall gateways with smooth 240ms camera slide transitions into uncharted rooms.
- **Compass Guidance & Mission Objectives:** The in-game HUD displays dynamic directional cues and mission status guiding you toward unexplored sectors and the green exit portal.

---

### 🕯️ 3. Fog of War & Lantern Vision
- **Atmospheric Darkness:** Unexplored sectors are cloaked in mystery until illuminated by your hero's lantern.
- **Dynamic Vision Radius:** Your lantern radius subtly narrows on higher stages, intensifying the thrill of exploration.
- **Breadcrumb Pathfinding:** Dots eaten along corridors illuminate paths, leaving breadcrumbs to help you navigate back.
- **Fog Peek Ability (`[F]`):** Need a quick glimpse of the labyrinth? Press `[F]` to temporarily unveil the maze for **1.0 second**. Each level provides **3 peek charges**; eating bonus fruits grants additional charges!

---

### 👥 4. Solo & 2-Player Co-op Multiplayer
- **Play as Yellow Pac-Man (1P):** The timeless arcade hero.
- **Play as Pink Ms. Pac-Man (1P):** Featuring custom pink cosmetics, eyelashes, and iconic red bow.
- **2-Player Co-op Mode (2P):** Team up on the same keyboard! Work together to explore sectors, distract ghosts, collect bonus fruits, and reach the exit portal.
- **Flexible Controls:** Move using **Arrow Keys** or **WASD**. In 2P mode, press `[Tab]` or click on either character to instantly swap arrow key priority!

---

### 👻 5. Smart Ghosts & Bonus Fruit Collectibles
- **Dynamic Ghost Spawning:** Red (Blinky), Pink (Pinky), Cyan (Inky), and Orange (Clyde) roam sectors, scale in speed and count, and patrol gateways.
- **Bonus Fruit Spawns:** Cherries, Strawberries, Apples, and Oranges spawn periodically for big bonus points and **+1 extra Fog Peek charge** each!
- **Checkpoint Resumes:** When you lose a round, the game resumes directly from your current stage instead of resetting to Stage 1!

---

## 🎮 Controls

### Home Map Controls
| Action | Key / Gesture |
| :--- | :--- |
| **Start / Play Selected Stage** | `Space`, `Enter`, or click **[ ▶ PLAY ]** |
| **Select Next / Previous Stage** | `Up` / `Down` or `Right` / `Left` Arrow keys |
| **Scroll Up / Down Mountain** | `Page Up` / `Page Down`, Mouse Wheel, or Mouse Drag |
| **Jump to Level 1 / Base** | `Home` |
| **Jump to Current Hero / Summit** | `End` or click **`[ 📍 MY HERO ]`** |
| **Toggle Game Mode (Yellow / Pink / Co-op)** | `Tab`, `1`, `2`, `3`, or click the Mode buttons |

### In-Game Playing Controls
| Action | Key / Mouse |
| :--- | :--- |
| **Player 1 Movement** | `Arrow Keys` (or `WASD` in single-player) |
| **Player 2 Movement (Co-op)** | `WASD` (or `Arrow Keys` when swapped) |
| **Swap Arrow Control (Co-op)** | `Tab` or click directly on Pac-Man / Ms. Pac-Man |
| **Fog Peek (1.0s glimpse)** | `F` (3 charges per level; eat fruits for +1) |
| **Return to Saga Map** | `M` key or click the top-left **[ 🗺️ SAGA MAP ]** button |
| **Retry Current Stage (on Game Over)** | `R` or `Space` |
| **Toggle Fullscreen Mode** | `F11` (or `Esc` to exit fullscreen) |

---

## 🛠️ Build & Run Instructions

### Prerequisites
- **Qt 6** (Qt 6.5 or later recommended; tested on Qt 6.11.1)
- **C++17** compatible compiler (Clang, GCC, or MSVC)
- **Qt Creator** IDE (recommended) or command-line build tools (`qmake` / `cmake`)

### Option A: Open with Qt Creator (Recommended)
1. Launch **Qt Creator**.
2. Select **Open File or Project...** and choose [`pacman_qt.pro`](file:///Users/hiya/Downloads/pacman_qt/pacman_qt.pro).
3. Select your Qt 6 Desktop kit and click **Configure Project**.
4. Press `Cmd + R` (macOS) or `Ctrl + R` (Windows/Linux) to build and run!

### Option B: Build from Terminal
```bash
# Clone the repository
git clone https://github.com/hiya7790/pacman_qt.git
cd pacman_qt

# Run qmake and compile
qmake pacman_qt.pro
make -j4

# Run the game
# On macOS:
open pacman_qt.app
# On Linux:
./pacman_qt
```

---

## 📁 Project Architecture

```
pacman_qt/
├── pacman_qt.pro        # Qt Project configuration file
├── main.cpp             # Application entry point (QApplication setup)
├── mainwindow.h         # Game state definitions, structs, and declarations
├── mainwindow.cpp       # Game engine: rendering, procedural generation, input & physics
├── mazesolver.h         # Maze pathfinding & BFS exploration utilities
└── README.md            # Project documentation & gameplay guide
```

---

## 📜 License
This project is open-source and created for educational, entertainment, and portfolio purposes.
All Pac-Man characters and Candy Crush inspirations are tributes to the classic franchises.
