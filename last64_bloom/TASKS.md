- Shield with alpha / transparency / Dithering
- Rework shield to hit multiple targets, no downtime just cooldown
- bosses are very similar, fast boss?
- Improved Spawning: Similar to vampire with fill screen instead of basic frequency
- More passive upgrades
    - Player movespeed
    - More enemy spawn
    - Faster projectile speed
    - More xp per enemy
- Different enemy types
    - Shooter
    - etc...
- Fix title image behind debug start menu
- Show a little icon left of the upgrade in the upgrade menu
- Intro with camera movement
- Our different levels should feature custom enemy wave setups
- Switch the Weapon info order. Instead of p1 and 2 underneath, put the p1 and 3 underneath in the top left cronerand p2 and 4 underneath in the top right corner
- Settings menu: Music, marble background, etc...
- Colored debug text (Should work with the font styles)
- Player Light (Currently the shapes are not affected by light)
- Quick test whether texture on enemies would work, like xp (failed)
- Main menu in main.cpp instead of sceneLast64.cpp
- Spiral Weapon upgrades break old projectiles, stay in place detached from player
- Split up code more. main and scene are pretty big

# DONE- XP Bar flash in the actual size of gain xp, not fixed size?

- Unlock weapons
- Render arena boder only if round is running
- Background in different color depending on level and main menu
- More levels, unlocked when completing previous level


## Phase 0: Files and Compile
- Create the needed files, incl. a makefile in order to compile
- Render a single box
## Phase 1: Core Setup
- Init tiny3d with bloom pipeline
- Basic scene with background
- Camera setup (static camera implemented)
- Render player (basic player actor created)
- Input to move player

## Phase 2: Combat System
- Auto-fire projectiles
- Enemy spawning and AI
- Collision and damage system
- Player taking damage and death

## Phase 3: Progression
- Level up system
- Weapon types and upgrades
- Audio test

## Phase 4: Polish & Features
- GUI: XP bar, level, press start
- Visual effects: Bloom, screenshake, blood particles, blood on floor
- Multiplayer support (4 players)

## Phase 5: Enhanced Progression
- Random weapon upgrades
- Random new weapon acquisition

## Phase 6: Bug Fixes
- Fixed projectile null pointer dereference crash
- Fixed weapon initialization/cleanup issue causing crashes on upgrade
- Cleaned up excessive safety checks in projectile system

## Phase 7: UI Improvements
- Enhanced weapon overview to show all weapons for each player
- Enemies go to dead players again

## Phase 8: Debugging Features
- Added debug button (L) to level up players for testing

## Phase 9: Enemy Spawning & Boss
- Implemented rudimentary spawn manager
- Enemies spawn in waves
- Added first boss enemy
- Fixed bug: Enemies targeting dead players

## Phase 10: Refinement
- Player with white vertex color to better identify the direction
- Color test strip
- Color Conversion tool from hex to hdr 8bRGBA8it
- Test ambient light, see how color strip changes -> Changed to neutral color
- New Weapon defensive shield
- New Weapon mechanic: Shape (stationary, damage on frequency, per-enemy tracking)
- HDR/Bloom effect when level up or boss kill
- Spawn death vfx, rotate, scale, color
- Player drawn below projectiles (especially shield weapon)
- Shield with reduced color
- Projectiles in different sizes
- Projectiles in player color
- HUD: Weapon Icons
- Music and audio system out of scene so the music can play when the scene restarts
- Last save off value screen
- Linear movement waves: Comet Style

## Phase 11: Upgrades Overhaul
- a/b, slowdown, next to player
- Weapon choice is wrong way around a / b buttons left / right
- SAVING test
- SAVING useful data: best time, completion flags, total level ups
- SAVING music setting
- Main Menu, stats menu, upgrade menu
- Convert total level ups to new currency in main menu: CREDITS, show in top right corner
- Passive upgrades (paid with CREDITS and saved)
- Option to return to main menu, from Start menu
- Remove purge and save game from start menu (now in stats menu)
- Profiling, what are the performance eaters
- Collision performance optimization
- Background marble effect performance
- Larger font for main menu
- Use full screen for arena, no black bars
- Reduce height of xp bar
- XP Bar effects
- XP transparency
- Main menu listen to stick input and all controllers
- Title Image in main menu
- Proper level complete state

## Phase 12: MORE
- XP as collectible
- Comet style wave, player needs to dodge
- Debug button that spawns xp shards all over the screen
- Xp pickup sound higher pitch the closer we are to level up
- Marble background (toggle in debug menu)
- Projectiles off screen are immediately depleted with no grace zone
- Boss now correctly only spawns once
- Wave counter is not cleared after all players have died
- Fixed github auth

## Phase 13: Passive Upgrades System
- Pickup Range upgrade (+10% per level, max 10 levels, 10 credits each)
- Damage upgrade (+5% per level, max 20 levels, 20 credits each)
- Projectile Count upgrade (+1 per level, max 5 levels, 50 credits each)
- Applied damage multiplier to all weapon types
- Applied projectile count bonus to multi-projectile weapons
- Reset all upgrades option with full refund
- Xp pickup sound higher pitch the closer we are to level up
- Marble background (toggle in debug menu)
- Projectiles off screen are immediately depleted with no grace zone

- Weapon overview now shows all weapons for each player
- Debug button (L) added to level up players for testing
- Enemies now correctly target dead players

# Known Issues Fixed
- Projectile null pointer dereference crash
- Weapon initialization/cleanup issue causing crashes on upgrade
- Excessive safety checks in projectile system
- All options are the same when one weapon is fully upgraded
- Cache problem: RSP reading from DMEM address 0x230
