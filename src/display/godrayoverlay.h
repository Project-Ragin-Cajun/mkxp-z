/*
** godrayoverlay.h
**
** Procedural crepuscular ray overlay. Draws a fullscreen quad
** with a fragment shader that generates light rays from a
** screen-space source point. Intended to be driven from Ruby
** via sun position data each frame.
*/

#ifndef GODRAYOVERLAY_H
#define GODRAYOVERLAY_H

#include "disposable.h"
#include "viewport.h"

struct GodRayOverlayPrivate;

class GodRayOverlay : public ViewportElement, public Disposable
{
public:
    explicit GodRayOverlay(Viewport* viewport = nullptr);
    ~GodRayOverlay() override;

    /* Normalized [0, 1] screen-space position of the light source.
     * (0, 0) = top-left, (1, 1) = bottom-right. Values outside
     * [0, 1] are valid because the source can be off-screen. */
    DECL_ATTR(LightX, float);
    DECL_ATTR(LightY, float);

    /* Overall ray brightness multiplier. */
    DECL_ATTR(Intensity, float);

    /* RGB tint of the rays (0.0-1.0 per channel).
     * Dawn gold, noon white, dusk amber. */
    DECL_ATTR(RayColorR, float);
    DECL_ATTR(RayColorG, float);
    DECL_ATTR(RayColorB, float);

    /* Radial falloff exponent. Higher = rays fade faster. */
    DECL_ATTR(Decay, float);

    /* Number of angular ray bands per full revolution. */
    DECL_ATTR(RayDensity, float);

    /* Edge hardness of each ray band. Higher = sharper edges. */
    DECL_ATTR(RaySharpness, float);

    /* Animation phase in radians. Increment each frame for sway. */
    DECL_ATTR(Phase, float);

    /* Standard RGSS opacity (0-255). */
    DECL_ATTR(Opacity, int);

private:
    GodRayOverlayPrivate* pointer = nullptr;

    void draw() override;;
    void onGeometryChange(const Scene::Geometry& geometry) override;

    void releaseResources() override;
    const char* klassName() const { return "GodRayOverlay"; }

    ABOUT_TO_ACCESS_DISP
};

#endif
