#include "player.h"
#include "../systems/weapon_projectile.h"
#include "../systems/weapon_homing.h"
#include "../systems/weapon_circular.h"
#include "../main.h"
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <libdragon.h>
#include <malloc.h>



namespace Actor {
    // Static members for player mesh
    T3DVertPacked* Player::sharedVertices = nullptr;
    T3DMat4FP* Player::sharedMatrix = nullptr;
    bool Player::initialized = false;
    
    void Player::initialize() {
        if (initialized) return;
        
        // Allocate vertices for the player (triangle)
        T3DVec3 normalVec = {{0.0f, 0.0f, 1.0f}};
        uint16_t norm = t3d_vert_pack_normal(&normalVec);
        sharedVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 2);
        
        // First structure: vertices 0 and 1
        sharedVertices[0] = (T3DVertPacked){};
        sharedVertices[0].posA[0] = 0; sharedVertices[0].posA[1] = 3; sharedVertices[0].posA[2] = 0; // Top vertex (smaller to match enemies)
        sharedVertices[0].normA = norm;
        sharedVertices[0].posB[0] = -3; sharedVertices[0].posB[1] = -3; sharedVertices[0].posB[2] = 0; // Bottom left
        sharedVertices[0].normB = norm;
        sharedVertices[0].rgbaA = 0xFFFF0000; // Will be set per player
        sharedVertices[0].rgbaB = 0xFFFF0000; // Will be set per player
        sharedVertices[0].stA[0] = 0; sharedVertices[0].stA[1] = 0;
        sharedVertices[0].stB[0] = 0; sharedVertices[0].stB[1] = 0;
        
        // Second structure: vertex 2 and unused
        sharedVertices[1] = (T3DVertPacked){};
        sharedVertices[1].posA[0] = 3; sharedVertices[1].posA[1] = -3; sharedVertices[1].posA[2] = 0; // Bottom right
        sharedVertices[1].normA = norm;
        sharedVertices[1].posB[0] = 0; sharedVertices[1].posB[1] = 0; sharedVertices[1].posB[2] = 0; // Unused
        sharedVertices[1].normB = norm;
        sharedVertices[1].rgbaA = 0xFFFF0000; // Will be set per player
        sharedVertices[1].rgbaB = 0xFFFF0000; // Will be set per player
        sharedVertices[1].stA[0] = 0; sharedVertices[1].stA[1] = 0;
        sharedVertices[1].stB[0] = 0; sharedVertices[1].stB[1] = 0;
        
        // Allocate matrix
        sharedMatrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
        t3d_mat4fp_identity(sharedMatrix);
        
        initialized = true;
    }
    
    void Player::cleanup() {
        if (sharedVertices) {
            free_uncached(sharedVertices);
            sharedVertices = nullptr;
        }
        
        if (sharedMatrix) {
            free_uncached(sharedMatrix);
            sharedMatrix = nullptr;
        }
        
        initialized = false;
    }
    
    Player::Player(T3DVec3 startPos, joypad_port_t port) : Base() {
        if (!initialized) {
            initialize();
        }
        
        // Allocate per-player vertices and matrix
        playerVertices = (T3DVertPacked*)malloc_uncached(sizeof(T3DVertPacked) * 2);
        playerMatrix = (T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP));
        
        // Copy shared vertices to player vertices
        playerVertices[0] = sharedVertices[0];
        playerVertices[1] = sharedVertices[1];
        
        // Initialize matrix
        t3d_mat4fp_identity(playerMatrix);
        
        position = startPos;
        velocity = {0, 0, 0};
        speed = 50.0f;
        rotation = 0.0f;
        playerPort = port;
        isDead = false;
        maxHealth = 1; // Default max health
        health = maxHealth;
        
        // Set player color based on port // RGBA8 (0xRRGGBBAA
        switch (playerPort) {
            case JOYPAD_PORT_1:
                playerColor = 0xFFFFFFFF; // White
                break;
            case JOYPAD_PORT_2:
                playerColor = 0x00FF00FF; // Green
                break;
            case JOYPAD_PORT_3:
                playerColor = 0x00FFFFFF; // Cyan
                break;
            case JOYPAD_PORT_4:
                playerColor = 0xFFFF00FF; // Yellow
                break;
        }
        
        // Initialize weapons - all weapons active simultaneously
        weapon1 = new WeaponProjectile();
        weapon1->setPlayer(this);
        
        weapon2 = new WeaponHoming();
        weapon2->setPlayer(this);
        
        weapon3 = new WeaponCircular();
        weapon3->setPlayer(this);
        
        flags &= ~FLAG_DISABLED; // Clear the disabled flag to enable the actor
    }
    
    void Player::takeDamage(int amount) {
        if (isDead) return;
        health -= amount;
        if (health <= 0) {
            kill();
        }
    }
    
    bool Player::collidesWith(Base* other) {
        // Simple circle-circle collision for now
        T3DVec3 otherPos = other->getPosition();
        float otherRadius = other->getRadius();

        float dx = position.x - otherPos.x;
        float dy = position.y - otherPos.y;
        float distance = sqrtf(dx * dx + dy * dy);

        // Player radius is assumed to be 3.0f, same as enemy
        return distance < (3.0f + otherRadius);
    }
    
    Player::~Player() {
        // Clean up per-player vertices and matrix
        if (playerVertices) {
            free_uncached(playerVertices);
            playerVertices = nullptr;
        }
        
        if (playerMatrix) {
            free_uncached(playerMatrix);
            playerMatrix = nullptr;
        }
        
        // Clean up weapons
        if (weapon1) {
            delete weapon1;
            weapon1 = nullptr;
        }
        
        if (weapon2) {
            delete weapon2;
            weapon2 = nullptr;
        }
        
        if (weapon3) {
            delete weapon3;
            weapon3 = nullptr;
        }
    }
    
    void Player::update(float deltaTime) {
        if (isDead) return; // If player is dead, do nothing

        // Handle player input
        joypad_inputs_t stick = joypad_get_inputs(playerPort); // Get analog stick inputs
        
        float moveSpeed = speed * deltaTime;
        
        // 3D movement - X and Y for horizontal/vertical movement, Z stays constant
        // Note: In the N64's coordinate system with the camera looking down the Z-axis,
        // positive Y is up, negative Y is down, positive X is right, negative X is left
        // 
        // Using analog stick for smooth movement in all directions
        float moveX = stick.stick_x / 32.0f; // Normalize analog input (-1 to 1)
        float moveY = stick.stick_y / 32.0f; // normalize
        
        // Apply deadzone to prevent drift
        static constexpr float DEADZONE = 0.25f; // 25% deadzone
        if (fabsf(moveX) < DEADZONE) moveX = 0.0f;
        if (fabsf(moveY) < DEADZONE) moveY = 0.0f;
        
        // Apply movement
        float newX = position.x + moveX * moveSpeed;
        float newY = position.y + moveY * moveSpeed;
        
        // Check boundaries
        if (newX >= SCREEN_LEFT && newX <= SCREEN_RIGHT) {
            position.x = newX;
        }
        if (newY >= SCREEN_TOP && newY <= SCREEN_BOTTOM) {
            position.y = newY;
        }
        // Z position stays constant (we're moving on the X/Y plane)
        
        // Update all weapons
        if (weapon1) {
            weapon1->update(deltaTime);
        }
        
        if (weapon2) {
            weapon2->update(deltaTime);
        }
        
        if (weapon3) {
            weapon3->update(deltaTime);
        }
        
        // Check if A button is pressed for manual firing
        joypad_buttons_t pressed = joypad_get_buttons_pressed(playerPort);
        if (pressed.a) {
            if (weapon1) {
                weapon1->fireManual();
            }
            
            if (weapon2) {
                weapon2->fireManual();
            }
            
            if (weapon3) {
                weapon3->fireManual();
            }
        }
        
        // Update rotation based on movement direction
        if (moveX != 0.0f || moveY != 0.0f) {
            rotation = atan2f(moveX, moveY); // Point in movement direction
        }
        
        // Update matrix
        if (playerMatrix) {
            t3d_mat4fp_from_srt_euler(
                playerMatrix,
                (T3DVec3){{1.0f, 1.0f, 1.0f}},  // scale
                (T3DVec3){{0.0f, 0.0f, rotation}},  // rotation around Z axis
                position                         // translation
            );
        }
    }
    
    void Player::draw3D(float deltaTime) {
    // Set up rendering state
    t3d_state_set_drawflags((enum T3DDrawFlags)(T3D_FLAG_SHADED | T3D_FLAG_DEPTH));
    
    // Draw the player using the player-specific vertices and matrix
    if (playerMatrix && playerVertices) {
        // Update vertex colors for this specific player
        playerVertices[0].rgbaA = playerColor;
        playerVertices[0].rgbaB = playerColor;
        playerVertices[1].rgbaA = playerColor;
        playerVertices[1].rgbaB = playerColor;
        
        t3d_matrix_push(playerMatrix);
        t3d_vert_load(playerVertices, 0, 4); // Load 4 vertices (2 structures)
        t3d_tri_draw(0, 1, 2); // Draw triangle with vertices 0, 1, 2
        t3d_tri_sync();
        t3d_matrix_pop(1);
    }
    
    // Draw weapon projectiles from all weapons
    if (weapon1) {
        weapon1->draw3D(deltaTime);
    }
    
    if (weapon2) {
        weapon2->draw3D(deltaTime);
    }
    
    if (weapon3) {
        weapon3->draw3D(deltaTime);
    }
}
    
    void Player::drawPTX(float deltaTime) {
        // Draw weapon particle effects from all weapons
        if (weapon1) {
            weapon1->drawPTX(deltaTime);
        }
        
        if (weapon2) {
            weapon2->drawPTX(deltaTime);
        }
        
        if (weapon3) {
            weapon3->drawPTX(deltaTime);
        }
    }
};