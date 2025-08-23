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
