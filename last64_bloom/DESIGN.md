# 🕹️ Last64 (Abstract Style)
A minimalist reimagining of *Vampire Survivors* for the Nintendo 64 using `libdragon` and `tiny3d`. All visuals are abstract — inspired by *REZ* and *Geometry Wars* — relying on bloom and simple shapes for aesthetic effect. With up to 4 players that can play locally on the actual Nintendo 64 console.

## 🎨 Visual Style
- No textures or sprites
- Use triangles, circles, and quads for all entities
- Color-coded shapes with bloom blending
- Rely on additive glow and density for "intensity"
- Stylish, clean, abstract — looks good with low effort

## 🧩 Core Concepts
| System            | Description |
|-------------------|-------------|
| **Player**        | Triangle or diamond shape, glows with color |
| **Enemies**       | Triangles or circles with color per type |
| **Projectiles**   | Glowing lines or thin quads |
| **XP Orbs**       | Floating dots that glow and pulse |
| **GUI**           | Line-based HUD (health, XP, level) |
| **Multiplayer**   | Shared-screen co-op for up to 4 players |

## 🛠️ Development Milestones

### ✅Phase 0: Files and Compile
- [x] Create the needed files, incl. a makefile in order to compile
- [x] Render a single box

### ✅ Phase 1: Core Setup
- [x] Init tiny3d with bloom pipeline
- [x] Basic scene with background
- [x] Camera setup (static camera implemented)
- [x] Render player (basic player actor created)
- [x] Input to move player

### ✅ Phase 2: Combat System
- [x] Auto-fire projectiles
- [x] Enemy spawning and AI
- [x] Collision and damage system
- [ ] Player taking damage and death

### ✅ Phase 3: Progression
- [ ] Level up system
- [ ] Weapon types and upgrades
- [x] Audio test

### ✅ Phase 4: Polish & Features
- [x] GUI: XP bar, level, press start
- [ ] Visual effects: Bloom, screenshake, blood particles, blood on floor
- [x] Multiplayer support (4 players)

## 🛠️ KNOWN ISSUES
~~All the geometry is black (not showing the configured color). Something with the custom code generated geometry or render pipeline is not correct.~~

## 📂 Project Structure (Suggested)
n64-project/
├── assets/
├── src/
│ ├── main.c
│ ├── actors
│ ├── camera
│ ├── scene
│ └── etc...
├── Makefile
└── DESIGN.md

## 📌 Notes
- Use object pools for projectiles and enemies
- Optimize with spatial partitioning if needed
- The scene: sceneLast64 is the current default scene for the project
