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
- [x] Player taking damage and death

### ✅ Phase 3: Progression
- [x] Level up system
- [x] Weapon types and upgrades
- [x] Audio test

### ✅ Phase 4: Polish & Features
- [x] GUI: XP bar, level, press start
- [x] Visual effects: Bloom, screenshake, blood particles, blood on floor
- [x] Multiplayer support (4 players)

### ✅ Phase 5: Enhanced Progression
- [x] Random weapon upgrades
- [x] Random new weapon acquisition

### ✅ Phase 6: Bug Fixes
- [x] Fixed projectile null pointer dereference crash
- [x] Fixed weapon initialization/cleanup issue causing crashes on upgrade
- [x] Cleaned up excessive safety checks in projectile system

### ✅ Phase 7: UI Improvements
- [x] Enhanced weapon overview to show all weapons for each player
- [ ] Enemies go to dead players again

### ✅ Phase 8: Debugging Features
- [x] Added debug button (L) to level up players for testing

## 🛠️ Implemented Features

### Weapons
- **Projectile Weapon**: Basic weapon that fires projectiles in a straight line
- **Homing Weapon**: Fires projectiles that home in on nearby enemies
- **Circular Weapon**: Fires projectiles in a circular pattern around the player

### Upgrades
- **Weapon Upgrades**: Each weapon can be upgraded to increase its effectiveness
- **New Weapons**: Players can acquire new weapon types as they level up
- **Random Selection**: When leveling up, a random weapon is upgraded (if any can be upgraded) or a new random weapon type is acquired (if not already owned)

## 🛠️ KNOWN ISSUES
~~All the geometry is black (not showing the configured color). Something with the custom code generated geometry or render pipeline is not correct.~~
~~Projectile null pointer dereference crash has been fixed.~~
~~Weapon initialization/cleanup issue causing crashes on upgrade has been fixed.~~
~~Excessive safety checks in projectile system have been cleaned up.~~
~~Weapon overview now shows all weapons for each player.~~
~~Debug button (L) added to level up players for testing.~~