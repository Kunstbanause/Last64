import numpy as np
from noise import pnoise2
import struct

# ----------------------
# Config matching your C++ version - TMEM-friendly sizes
# ----------------------
OCTAVES = 4
LACUNARITY = 2.7
GAIN = 0.5
TILE_SIZE = 32  # FBM noise texture size - reduced to fit TMEM
WARP_MAP_SIZE = 32  # Warp displacement map size
PALETTE_SIZE = 64  # Palette LUT size - reduced to fit TMEM

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

def save_as_n64_rgb555(filename, data):
    """Save data as N64 RGB555 format (big-endian)"""
    height, width = data.shape[:2]
    
    if len(data.shape) == 3:
        # RGB data
        r = (data[:, :, 0] >> 3) & 0x1F  # 5 bits for red
        g = (data[:, :, 1] >> 3) & 0x1F  # 5 bits for green
        b = (data[:, :, 2] >> 3) & 0x1F  # 5 bits for blue
    else:
        # Grayscale data
        gray = (data >> 3) & 0x1F
        r = g = b = gray
    
    # Combine into 16-bit values (RGB555 format: 0RRRRRGGGGGBBBBB)
    rgb555 = (r << 10) | (g << 5) | b
    
    # Save as big-endian binary file (N64 is big-endian)
    with open(filename, 'wb') as f:
        for row in rgb555:
            for pixel in row:
                # Write as big-endian 16-bit value
                f.write(struct.pack('>H', int(pixel)))

def generate_balatro_palette():
    """Generate the exact Balatro color palette as 1D LUT"""
    # Create 256 values from 0 to 1
    vals = np.linspace(0, 1, PALETTE_SIZE)
    
    # Balatro color stops
    colors = [
        (0.0, np.array([15, 20, 40])),    # dark navy
        (0.3, np.array([70, 120, 180])),  # muted blue
        (0.6, np.array([200, 90, 60])),   # muted orange/red
        (1.0, np.array([220, 220, 220]))  # soft white
    ]
    
    palette_colors = np.zeros((PALETTE_SIZE, 3), dtype=np.uint8)
    
    for i, val in enumerate(vals):
        # Find which color segment we're in
        for j in range(len(colors)-1):
            lv, lc = colors[j]
            rv, rc = colors[j+1]
            
            if val >= lv and val <= rv:
                t = (val - lv) / (rv - lv) if (rv - lv) > 0 else 0
                palette_colors[i, 0] = int(lerp(lc[0], rc[0], t))
                palette_colors[i, 1] = int(lerp(lc[1], rc[1], t))
                palette_colors[i, 2] = int(lerp(lc[2], rc[2], t))
                break
    
    # Reshape to 256x1x3 for saving
    return palette_colors.reshape((PALETTE_SIZE, 1, 3))

def generate_tileable_fbm(size, t=0):
    """Generate tileable FBM noise texture"""
    # Create seamless coordinates using sine/cosine wrapping
    i = np.arange(size)
    x_coords = np.cos(2 * np.pi * i / size) * size / (2 * np.pi)
    y_coords = np.sin(2 * np.pi * i / size) * size / (2 * np.pi)
    
    x, y = np.meshgrid(x_coords, y_coords)
    
    # Generate FBM noise
    noise = fbm(x, y, t=t)
    
    # Normalize to [0, 255] for grayscale
    noise = (noise - noise.min()) / (noise.max() - noise.min() + 1e-6)
    noise_gray = (noise * 255).astype(np.uint8)
    
    return noise_gray

def generate_warp_map(size, t=0):
    """Generate warp displacement map using RG channels"""
    # Create seamless coordinates
    i = np.arange(size)
    x_coords = np.cos(2 * np.pi * i / size) * size / (2 * np.pi)
    y_coords = np.sin(2 * np.pi * i / size) * size / (2 * np.pi)
    
    x, y = np.meshgrid(x_coords, y_coords)
    
    # Generate two channels of FBM noise for X and Y displacement
    warp_x = fbm(x, y, t=t)
    warp_y = fbm(x + 100, y + 100, t=t)  # Offset for different pattern
    
    # Normalize to [0, 1] range
    warp_x = (warp_x - warp_x.min()) / (warp_x.max() - warp_x.min() + 1e-6)
    warp_y = (warp_y - warp_y.min()) / (warp_y.max() - warp_y.min() + 1e-6)
    
    # Pack into RGB channels
    warp_map = np.zeros((size, size, 3), dtype=np.uint8)
    warp_map[:, :, 0] = (warp_x * 255).astype(np.uint8)  # R = X displacement
    warp_map[:, :, 1] = (warp_y * 255).astype(np.uint8)  # G = Y displacement
    warp_map[:, :, 2] = 128  # B = neutral (0.5 in normalized space)
    
    return warp_map

def generate_all_textures(t=0):
    """Generate all required N64 textures"""
    print("Generating N64 Balatro textures...")
    
    # Generate FBM noise texture (64x64 grayscale)
    print("- Generating FBM noise texture (64x64)...")
    fbm_texture = generate_tileable_fbm(TILE_SIZE, t)
    save_as_n64_rgb555("fbm_noise.rgb555", fbm_texture)
    
    # Generate warp displacement map (32x32 RG)
    print("- Generating warp displacement map (32x32)...")
    warp_map = generate_warp_map(WARP_MAP_SIZE, t)
    save_as_n64_rgb555("warp_map.rgb555", warp_map)
    
    # Generate Balatro color palette (256x1)
    print("- Generating Balatro palette LUT (256x1)...")
    palette_lut = generate_balatro_palette()
    save_as_n64_rgb555("palette_lut.rgb555", palette_lut)
    
    print("All textures saved!")
    print(f"- fbm_noise.rgb555: {TILE_SIZE}x{TILE_SIZE} RGB555 ({TILE_SIZE*TILE_SIZE*2} bytes)")
    print(f"- warp_map.rgb555: {WARP_MAP_SIZE}x{WARP_MAP_SIZE} RGB555 ({WARP_MAP_SIZE*WARP_MAP_SIZE*2} bytes)")
    print(f"- palette_lut.rgb555: {PALETTE_SIZE}x1 RGB555 ({PALETTE_SIZE*2} bytes)")
    print(f"Total texture memory: {(TILE_SIZE*TILE_SIZE + WARP_MAP_SIZE*WARP_MAP_SIZE + PALETTE_SIZE) * 2} bytes")

def generate_test_variations():
    """Generate multiple texture variations for testing"""
    print("Generating texture variations...")
    
    # Generate a few time-based variations
    times = [0.0, 1.0, 2.0, 3.0]
    
    for i, t in enumerate(times):
        print(f"- Generating variation {i+1} (t={t})...")
        
        fbm_texture = generate_tileable_fbm(TILE_SIZE, t)
        warp_map = generate_warp_map(WARP_MAP_SIZE, t)
        
        save_as_n64_rgb555(f"fbm_noise_v{i+1}.rgb555", fbm_texture)
        save_as_n64_rgb555(f"warp_map_v{i+1}.rgb555", warp_map)
    
    print("Variations saved!")

if __name__ == "__main__":
    # Generate the main textures
    generate_all_textures(t=0.0)
    
    # Optionally generate test variations
    # generate_test_variations()
    
    print("\nReady for N64! Copy the .rgb555 files to your ROM directory.")