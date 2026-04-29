/* Procedural crepuscular ray generator.
 *
 * Paired with simple.vert. Receives v_texCoord in [0, 1]
 * normalized screen space. Generates angular ray bands
 * emanating from a light source point, with multi-octave
 * variation for organic edges and animated sway.
 *
 * Blended additively (BlendAddition): src_alpha modulates
 * how much rayColor is added to the scene. */

varying vec2 v_texCoord;

/* Light source position in normalized [0, 1] screen space.
 * Values outside [0,1] are valid (off-screen source). */
uniform vec2 lightPos;

/* RGBA tint of the rays (uses .rgb only; .a unused).
 * Driven by time-of-day from Ruby:
 * warm gold at dawn, pale white at noon, amber at dusk. */
uniform vec4 rayColor;

/* Overall brightness multiplier. */
uniform float intensity;

/* Radial falloff rate. Higher = rays fade faster with distance. */
uniform float decay;

/* Angular frequency: Number of ray bands per revolution. */
uniform float rayDensity;

/* Edge hardness exponent. Higher = sharper ray boundaries. */
uniform float raySharpness;

/* Animation offset in radians. Increment for sway. */
uniform float phase;

/* Master opacity [0, 1] (from RGSS 0-255 via NormValue). */
uniform float opacity;

void main()
{
    vec2 delta = v_texCoord - lightPos;
    float dist = length(delta);
    float angle = atan(delta.y, delta.x);

/* Multi-octave angular pattern.
* Three sine waves at incommensurate frequencies produce
* irregular, naturalistic ray widths. No two rays look
* identical. Phase offsets are baked per octave so the
* sway animation doesn't move them in lockstep. */
    float rays = 0.0;
    rays += sin(angle * rayDensity + phase) * 0.5 + 0.5;
    rays += sin(angle * rayDensity * 0.6 + phase * 1.3 + 2.0) * 0.3 + 0.3;
    rays += sin(angle * rayDensity * 1.8 - phase * 0.8 + 4.0) * 0.15 + 0.15;
    rays = clamp(rays / 0.95, 0.0, 1.0);

/* Sharpness: pow() steepens the falloff between ray and
* gap, controlling whether edges are soft/hazy or crisp. */
    rays = pow(rays, raySharpness);

/* Inverse-distance radial falloff.
* Atmospheric scattering: 1 / (1 + d * k) gives a long tail that reads as atmospheric
* Exponential: (exp(-d * k)) cuts harder for shorter rays. */
    float falloff = 1.0 / (1.0 + dist * decay);

/* Suppress the hot spot directly at the light source.
* Without this, the overlapping ray bases form a solid
* bright disc. */
    float innerFade = smoothstep(0.0, 0.05, dist);

    float result = rays * falloff * innerFade * intensity;

/* Alpha drives additive contribution via BlendAddition's
* GL_SRC_ALPHA factor. RGB is the ray tint. */
    gl_FragColor = vec4(rayColor.rgb, result * opacity);
}
