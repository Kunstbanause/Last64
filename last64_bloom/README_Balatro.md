# Balatro-Style Background for N64

This document explains how to generate the required textures for the Balatro-style background effect on the N64.

## Required Textures

1. **FBM Noise Texture (64x64 RGB555)**
   - Tileable fractional Brownian motion noise
   - Used as the base for the marble effect

2. **Warp Displacement Map (32x32 RGB555)**
   - Controls the warping/spiral effect
   - Red channel controls X displacement
   - Green channel controls Y displacement

3. **Palette LUT (256x1 RGB555)**
   - Color palette for the final effect
   - Indexed by the noise values

## Generating Textures

Run the Python script in the assets directory:

```bash
cd assets
python3 background.py
```

This will generate three files:
- `fbm_noise.rgb555` - 64x64 RGB555 texture
- `warp_map.rgb555` - 32x32 RGB555 texture
- `palette_lut.rgb555` - 256x1 RGB555 texture

## Texture Format

All textures are saved in RGB555 format (16-bit):
- 5 bits for Red
- 5 bits for Green
- 5 bits for Blue
- 1 unused bit

The format is compatible with the N64 RDP.

## Implementation Details

The effect is achieved through:
1. Scrolling the FBM noise texture
2. Rotating the warp displacement map
3. Using the RDP combiner to blend textures
4. Applying a radial gradient overlay
5. Using the palette LUT for final colors

## Loading Textures in Code

The textures are loaded using a custom utility function that reads the binary RGB555 files and creates libdragon surface objects. The Makefile has been updated to include these files in the ROM filesystem.

## Future Improvements

- Implement the full multi-texture combiner setup for the warping effect
- Add a radial gradient texture for the center bias effect
- Optimize the combiner settings for better performance
- Add parameters to control the effect in real-time