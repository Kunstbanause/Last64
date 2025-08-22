import pygame
import numpy as np
import time
from noise import pnoise2

# ----------------------
# Config
# ----------------------
W, H = 320, 240
OCTAVES = 4
LACUNARITY = 2.7
GAIN = 0.5
SCROLL_SPEED = 0.05
MARBLE_FREQ = 1.5
SPIRAL_STRENGTH = 0.8
ROTATE_SPEED = 0.3
CENTER_BIAS = 2.0

# N64-specific configs
TILE_SIZE = 64  # Size of the repeating tileable noise texture
WARP_MAP_SIZE = 32  # Size of the warp displacement map

pygame.init()
screen = pygame.display.set_mode((W, H))
clock = pygame.time.Clock()

# ----------------------
# Helper functions
# ----------------------
def fbm(x, y, octaves=OCTAVES, lacunarity=LACUNARITY, gain=GAIN, t=0):
    """Fractional Brownian motion"""
    value = np.zeros_like(x)
    amp = 1.0
    freq = 1.0
    for _ in range(octaves):
        value += amp * np.vectorize(pnoise2)(x*freq + t, y*freq + t)
        freq *= lacunarity
        amp *= gain
    return value

def lerp(a, b, t):
    return a + (b - a) * t

def palette(val):
    """Balatro-inspired subdued palette using gradient"""
    val = np.clip(val, 0, 1)
    colors = [
        (0.0, np.array([15, 20, 40])),    # dark navy
        (0.3, np.array([70, 120, 180])),  # muted blue
        (0.6, np.array([200, 90, 60])),   # muted orange/red
        (1.0, np.array([220, 220, 220]))  # soft white
    ]
    r = np.zeros_like(val)
    g = np.zeros_like(val)
    b = np.zeros_like(val)
    for i in range(len(colors)-1):
        lv, lc = colors[i]
        rv, rc = colors[i+1]
        mask = (val >= lv) & (val <= rv)
        t = (val[mask] - lv) / (rv - lv)
        r[mask] = lerp(lc[0], rc[0], t)
        g[mask] = lerp(lc[1], rc[1], t)
        b[mask] = lerp(lc[2], rc[2], t)
    return np.dstack([r,g,b]).astype(np.uint8)

def save_as_n64_rgb555(filename, data):
    """Save data as N64 RGB555 format"""
    # Convert to RGB555 format (16-bit)
    # Format: 0RRRRRGGGGGBBBBB
    r = (data[:, :, 0] >> 3) & 0x1F  # 5 bits for red
    g = (data[:, :, 1] >> 3) & 0x1F  # 5 bits for green
    b = (data[:, :, 2] >> 3) & 0x1F  # 5 bits for blue
    
    # Combine into 16-bit values
    rgb555 = (r << 10) | (g << 5) | b
    
    # Save as binary file
    with open(filename, 'wb') as f:
        for row in rgb555:
            for pixel in row:
                f.write(pixel.astype(np.uint16).tobytes())

def generate_tileable_fbm(size, t=0):
    """Generate a tileable FBM noise texture"""
    # Create coordinate grids
    x = np.linspace(0, 4, size)
    y = np.linspace(0, 4, size)
    x, y = np.meshgrid(x, y)
    
    # Generate FBM noise
    noise = fbm(x, y, t=t)
    
    return noise

def generate_warp_map(size, t=0):
    """Generate a warp displacement map"""
    # Create coordinate grids
    x = np.linspace(0, 4, size)
    y = np.linspace(0, 4, size)
    x, y = np.meshgrid(x, y)
    
    # Generate two channels of FBM noise for displacement
    warp_x = fbm(x, y, t=t)
    warp_y = fbm(x+100, y+100, t=t)
    
    # Normalize to [0, 1] range
    warp_x = (warp_x - warp_x.min()) / (warp_x.max() - warp_x.min() + 1e-6)
    warp_y = (warp_y - warp_y.min()) / (warp_y.max() - warp_y.min() + 1e-6)
    
    # Convert to RGB (using R and G channels for displacement)
    warp_map = np.zeros((size, size, 3), dtype=np.uint8)
    warp_map[:, :, 0] = (warp_x * 255).astype(np.uint8)  # R channel
    warp_map[:, :, 1] = (warp_y * 255).astype(np.uint8)  # G channel
    warp_map[:, :, 2] = 0  # B channel (unused)
    
    return warp_map

def generate_palette_lut():
    """Generate the 256-color palette LUT"""
    # Create a 1D array of values from 0 to 1
    vals = np.linspace(0, 1, 256)
    
    # Apply palette mapping
    palette_colors = np.zeros((256, 3), dtype=np.uint8)
    for i, val in enumerate(vals):
        # Simple linear mapping for now (will be replaced with actual palette)
        palette_colors[i] = [int(val * 255), int(val * 255), int(val * 255)]
    
    # Reshape to 256x1 image
    palette_colors = palette_colors.reshape((256, 1, 3))
    
    return palette_colors

# ----------------------
# Texture generation
# ----------------------
def generate_textures(t):
    """Generate all required textures"""
    # Generate tileable FBM noise texture
    fbm_noise = generate_tileable_fbm(TILE_SIZE, t)
    # Normalize to [0, 1] and convert to RGB
    fbm_noise = (fbm_noise - fbm_noise.min()) / (fbm_noise.max() - fbm_noise.min() + 1e-6)
    fbm_rgb = np.zeros((TILE_SIZE, TILE_SIZE, 3), dtype=np.uint8)
    fbm_rgb[:, :, 0] = (fbm_noise * 255).astype(np.uint8)
    fbm_rgb[:, :, 1] = (fbm_noise * 255).astype(np.uint8)
    fbm_rgb[:, :, 2] = (fbm_noise * 255).astype(np.uint8)
    
    # Generate warp displacement map
    warp_map = generate_warp_map(WARP_MAP_SIZE, t)
    
    # Generate palette LUT
    palette_lut = generate_palette_lut()
    
    return fbm_rgb, warp_map, palette_lut

# ----------------------
# Main frame generation
# ----------------------
def generate_frame(t):
    y, x = np.mgrid[0:H, 0:W]
    
    # Normalize coordinates
    x = x / W * 4.0
    y = y / H * 4.0
    
    # ----------------------
    # Scrolling FBM noise
    # ----------------------
    scroll = t * SCROLL_SPEED
    warp_x = fbm(x, y, t=scroll)
    warp_y = fbm(x+100, y+100, t=scroll)
    
    # ----------------------
    # Spiral / center bias
    # ----------------------
    cx, cy = 2.0, 2.0  # center in normalized coordinates
    dx = x - cx
    dy = y - cy
    r = np.sqrt(dx**2 + dy**2)
    angle = SPIRAL_STRENGTH * r + t*ROTATE_SPEED
    cos_a = np.cos(angle)
    sin_a = np.sin(angle)
    
    # Rotate coordinates
    x_rot = cx + dx*cos_a - dy*sin_a + warp_x*0.5
    y_rot = cy + dx*sin_a + dy*cos_a + warp_y*0.5
    
    # ----------------------
    # Marble pattern
    # ----------------------
    marble = np.sin(MARBLE_FREQ * x_rot + fbm(x_rot, y_rot, t=t*0.05)*2.0 + t)
    
    # ----------------------
    # Improved center bias - preserve detail, just shift brightness range
    # ----------------------
    center_falloff = 1 - np.power(r / 2.0, CENTER_BIAS)
    marble = marble * 0.7 + center_falloff * 0.3
    
    # Normalize for palette
    val = (marble - marble.min()) / (marble.max() - marble.min() + 1e-6)
    
    return palette(val)

# ----------------------
# Main loop
# ----------------------
running = True
start = time.time()
texture_gen_counter = 0

while running:
    t = time.time() - start
    frame = generate_frame(t)
    surf = pygame.surfarray.make_surface(frame.swapaxes(0, 1))
    screen.blit(surf, (0, 0))
    pygame.display.flip()
    clock.tick(30)
    
    # Generate N64 textures every 2 seconds
    if int(t) > texture_gen_counter:
        texture_gen_counter = int(t)
        print(f"Generating N64 textures at t={t}")
        
        # Generate textures
        fbm_rgb, warp_map, palette_lut = generate_textures(t)
        
        # Save as N64 RGB555 format
        save_as_n64_rgb555("fbm_noise.rgb555", fbm_rgb)
        save_as_n64_rgb555("warp_map.rgb555", warp_map)
        save_as_n64_rgb555("palette_lut.rgb555", palette_lut)
        
        print("Textures saved!")
    
    for e in pygame.event.get():
        if e.type == pygame.QUIT:
            running = False

pygame.quit()