/* Tile shadow projection via height map ray march.
 *
 * Paired with simple.vert. For each screen fragment, converts
 * to tile space, marches toward the sun, and checks the height
 * map for shadow casters. If a caster's shadow reach covers
 * the fragment, the fragment is darkened.
 *
 * Blended normally (BlendNormal): outputs semi-transparent
 * black to darken shadowed areas. */

varying vec2 v_texCoord;

/* Height map: one pixel per tile, red channel stores height
 * scaled to 0-1 (0 = no caster, 1 = full tile height). */
uniform sampler2D heightMap;

/* Map dimensions in tiles. Used to normalize tile coordinates
 * into height map UV space. */
uniform vec2 mapSize;

/* Screen dimensions in pixels. Converts normalized screen
 * coordinates back to pixel space for world position math. */
uniform vec2 screenSize;

/* Current scroll offset in pixels. Converts screen-space
 * fragments to world-space tile positions. */
uniform vec2 scrollOffset;

/* Normalized direction from fragment toward the sun.
 * The march follows this vector looking for casters
 * between the fragment and the light source. */
uniform vec2 sunDirection;

/* Maximum shadow reach in tiles at the current sun
 * elevation. Low sun = long reach, high sun = short. */
uniform float shadowLength;

/* Master opacity for the shadow overlay. */
uniform float shadowOpacity;

/* Tile size in pixels. */
const float TILE_SIZE = 32.0;

/* Maximum march steps. 16 tiles covers any reasonable
 * shadow length at any screen resolution. */
const int MAX_STEPS = 16;

void main()
{
/* Convert screen position to world tile coordinates. */
    vec2 world_pixel = v_texCoord * screenSize + scrollOffset;
    vec2 tile_position = world_pixel / TILE_SIZE;

/* March toward the sun looking for shadow casters. */
    float shadow = 0.0;

    for (int step = 1; step <= MAX_STEPS; step++)
    {
        float march_distance = float(step);
        if (march_distance > shadowLength)
        break;

        vec2 sample_tile = tile_position + sunDirection * march_distance;
        vec2 height_uv = (sample_tile + 0.5) / mapSize;

    /* Skip samples outside the map. */
        if (height_uv.x < 0.0 || height_uv.x > 1.0 ||
        height_uv.y < 0.0 || height_uv.y > 1.0)
        {
            continue;
        }

        float tile_height = texture2D(heightMap, height_uv).r;

        if (tile_height > 0.0)
        {
        /* Shadow reach scales with tile height.
         * A full-height tile covers the full shadowLength.
         * A half-height tile covers half. */
            float reach = tile_height * shadowLength;

            if (march_distance <= reach)
            {
            /* Soft falloff near the shadow edge. */
                float edge = 1.0 - (march_distance / reach);
                shadow = max(shadow, edge);
                break;
            }
        }
    }

    gl_FragColor = vec4(0.0, 0.0, 0.0, shadow * shadowOpacity);
}
