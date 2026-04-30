/*
** tileshadowoverlay.h
**
** Screen-space tile shadow projection driven by a height map
** texture. Draws a fullscreen quad with a fragment shader that
** ray-marches through the height map to determine shadowed
** areas.
*/

#ifndef TILESHADOWOVERLAY_H
#define TILESHADOWOVERLAY_H

#include "disposable.h"
#include "viewport.h"

class Bitmap;
struct TileShadowOverlayPrivate;

class TileShadowOverlay : public ViewportElement, public Disposable
{
public:
    explicit TileShadowOverlay(Viewport* viewport = nullptr);
    ~TileShadowOverlay() override;

    /* Height map bitmap (one pixel per tile, red = height)
     * Ownership stays with the caller. */
    void setHeightMap(Bitmap* bitmap) const;

    /* Map dimension in tiles. */
    DECL_ATTR(MapWidth, int);
    DECL_ATTR(MapHeight, int);

    /* Normalized direction from fragment toward the sun. */
    DECL_ATTR(SunDirectionX, float);
    DECL_ATTR(SunDirectionY, float);

    /* Maximum shadow reach in tiles at current elevation. */
    DECL_ATTR(ShadowLength, float);

    /* Current scroll offset in pixels. */
    DECL_ATTR(ScrollX, float);
    DECL_ATTR(ScrollY, float);

    /* Master opacity (0-255). */
    DECL_ATTR(Opacity, int);

private:
    TileShadowOverlayPrivate* pointer = nullptr;

    void draw() override;
    void onGeometryChange(const Scene::Geometry& geometry) override;

    void releaseResources() override;
    const char* klassName() const { return "TileShadowOverlay"; }

    ABOUT_TO_ACCESS_DISP
};

#endif
