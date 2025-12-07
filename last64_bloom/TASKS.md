## 🛠️ Implemented Features

### Weapons
- **Projectile Weapon**: Basic weapon that fires projectiles in a straight line
- **Homing Weapon**: Fires projectiles that home in on nearby enemies
- **Circular Weapon**: Fires projectiles in a circular pattern around the player
- **Spiral Weapon**: Spawns projectiles tha circle around the player

### Upgrades
- **Weapon Upgrades**: Each weapon can be upgraded to increase its effectiveness
- **New Weapons**: Players can acquire new weapon types as they level up
- **Random Selection**: When leveling up, a random weapon is upgraded (if any can be upgraded) or a new random weapon type is acquired (if not already owned)

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
- [x] Enemies go to dead players again

### ✅ Phase 8: Debugging Features
- [x] Added debug button (L) to level up players for testing

### ✅ Phase 9: Enemy Spawning & Boss
- [x] Implemented rudimentary spawn manager
- [x] Enemies spawn in waves
- [x] Added first boss enemy
- [x] Fixed bug: Enemies targeting dead players

### ✅ Phase 10: Refinment
- [x] Player with white vertex color to better identify the direction?
- [x] Color test strip
- [x] Color Conversion tool from hex to hdr 8bRGBA8it
- [x] Test ambient light, see how color strip changes? -> Changed to netural color
- [x] New Weapon defensive shield
- [x] New Weapon mechanic: Shape. Instead of a projectile the shape is not moving or attached to the player character. It has not speed on its own. Deals damage with an attack frequency until the lifetime is over. The attacks are immediate, so an enemy entering the shape take damage right away, then the attack frequency timer runs down before that enemy can take damage from that shape again. This is tracked per enemy. Weapons using it have a cooldown to respawn it. 
- [x] HDR/Bloom effect when level up or boss kill
- [x] Spawn death vfx, rotate, scale, color
- [x] Player drawn below projectiles (especially shield weapon)
- [x] Shield with reduced color
- [x] projectiles in different sizes
- [x] Proectiles in player color
- [x] HUD: Weapon Icons
- [x] Music and audio system out of scene so the music can play when the scene restarts
- [x] last save off value screen
- [x] Linear movement waves: Comet Style

### ✅ Phase 11: Upgrades overhaul
- [x] a/b, slowdown, next to player
- [x] weapon choice is wron way around a / b buttons left / right
- [x] SAVING test
- [x] SAVING useful data: best time, completion flags, total level ups
- [x] SAVING music setting
- [ ] Main Menu
- [ ] Convert level ups to new currency: CREDITS
- [ ] Passive upgrades (payed with CREDITS and saved)
- [x] proper level complete state

### ✅ Phase 12: MORE
- [x] XP as collectible 
- [ ] Shield with alpha / transparency / Dithering
- [ ] Rework shield to hit multiple targets, no downtime just cooldown
- [ ] bosses are very similar, fast boss?
- [x] Comet style wave, player needs to dodge
- [ ] Improved Spawning: Similar to vampire with fill screen instead of basic frequency
- [x] Debug button that spawns xp shards all over the screen.
- [x] Xp pickup sound higher pitch the closer we are to level up
- [x] Marble background (toggle in debug menu)
- [x] projectiles off screen are immediately depleted with no grace zone

###  FAILED
- [ ] Intro with camera movement (FAILED)
- [ ] Colored debug text? (failed)
- [ ] Player Light (Failed, there is not shading, depth, etc)

## 🛠️ KNOWN ISSUES
- [x] All the geometry is black (not showing the configured color). Something with the custom code nerated geometry or render pipeline is not correct.
- [x] Projectile null pointer dereference crash has been fixed.
- [x] Weapon initialization/cleanup issue causing crashes on upgrade has been fixed.
- [x] Excessive safety checks in projectile system have been cleaned up.
- [x] Weapon overview now shows all weapons for each player.
- [x] Debug button (L) added to level up players for testing.
- [x] Enemies now correctly target dead players again.
- [x] Bug: Wave counter is not cleared after all players have died
- [x] Enemies now correctly target dead players again.
- [x] Fix bug: Wave counter is not cleared after all players have died Deconstructor missing!
- [x] Fix github auth
- [x] Fix: Boss now correctly only spawns once.
- Spiral Weapon upgrades break old projectiles, stay in place detached from player
- [x] Fix: All options are the same when one weapon is fully upgraded.

- Cache problem:
[unusual] RSP reading from DMEM address 0x230 which contains a value which is not cache coherent
        Current RSP PC: 0xc70
        The value read was previously written by RSP DMA from RDRAM address 0x002358f0
        RSP DMA started at RSP PC: 0x03c
        The relative CPU cacheline was dirty (missing cache writeback?)

~~After a while the optins are all the smame~~ Its just when one weapon is fully upgraded. Needs more upgrades
~~Player outline for better readaibility? (failed)~~