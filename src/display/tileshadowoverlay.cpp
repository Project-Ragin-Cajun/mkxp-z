/*
** tileshadowoverlay.cpp
*/

#include "tileshadowoverlay.h"

#include "bitmap.h"
#include "quad.h"

struct TileShadowOverlayPrivate
{
    Bitmap* heightMap;

    int mapWidth;
    int mapHeight;
    float sunDirectionX;
    float sunDirectionY;
    float shadowLength;
    float scrollX;
    float scrollY;
    NormValue opacity;

    Vec2i screenSize;
    Quad quad;

    TileShadowOverlayPrivate()
        : heightMap(nullptr),
          mapWidth(1),
          mapHeight(1),
          sunDirectionX(0.0f),
          sunDirectionY(-1.0f),
          shadowLength(3.0f),
          scrollX(0.0f),
          scrollY(0.0f),
          opacity(160),
          screenSize(1, 1)
    {
    }
};

TileShadowOverlay::TileShadowOverlay(Viewport* viewport)
    : ViewportElement(viewport)
{
    pointer = new TileShadowOverlayPrivate();
    TileShadowOverlay::onGeometryChange(scene->getGeometry());
}

TileShadowOverlay::~TileShadowOverlay()
{
    dispose();
}

void TileShadowOverlay::setHeightMap(Bitmap* bitmap) const
{
    pointer->heightMap = bitmap;
}

DEF_ATTR_SIMPLE(TileShadowOverlay, MapWidth, int, pointer->mapWidth)
DEF_ATTR_SIMPLE(TileShadowOverlay, MapHeight, int, pointer->mapHeight)
DEF_ATTR_SIMPLE(TileShadowOverlay, SunDirectionX, float, pointer->sunDirectionX)
DEF_ATTR_SIMPLE(TileShadowOverlay, SunDirectionY, float, pointer->sunDirectionY)
DEF_ATTR_SIMPLE(TileShadowOverlay, ShadowLength, float, pointer->shadowLength)
DEF_ATTR_SIMPLE(TileShadowOverlay, ScrollX, float, pointer->scrollX)
DEF_ATTR_SIMPLE(TileShadowOverlay, ScrollY, float, pointer->scrollY)
DEF_ATTR_SIMPLE(TileShadowOverlay, Opacity, int, pointer->opacity)

void TileShadowOverlay::draw()
{
    if (!pointer->opacity || !pointer->heightMap)
    {
        return;
    }

    TileShadowShader& shader = shState->shaders().tileShadow;
    shader.bind();
    shader.applyViewportProj();
    shader.setTranslation(Vec2i());
    shader.setTexSize(pointer->screenSize);

    /* Bind the height map to texture unit 1 with nearest
     * filtering. Each pixel represents one tile, so linear
     * interpolation would blend adjacent tile heights. */
    shader.setHeightMap(pointer->heightMap->getGLTypes().tex);
    TEX::setSmooth(false);

    shader.setMapSize(Vec2(pointer->mapWidth, pointer->mapHeight));
    shader.setScreenSize(Vec2(pointer->screenSize.x, pointer->screenSize.y));
    shader.setScrollOffset(Vec2(pointer->scrollX, pointer->scrollY));
    shader.setSunDirection(Vec2(pointer->sunDirectionX, pointer->sunDirectionY));
    shader.setShadowLength(pointer->shadowLength);
    shader.setOpacity(pointer->opacity.norm);

    /* Normal blending: Semi-transparent black darkens the scene. */
    glState.blendMode.pushSet(BlendNormal);

    pointer->quad.draw();
    glState.blendMode.pop();
}

void TileShadowOverlay::onGeometryChange(const Scene::Geometry& geometry)
{
    pointer->quad.setTexPosRect(FloatRect(geometry.rect), FloatRect(geometry.rect));
    pointer->screenSize = Vec2i(geometry.rect.w, geometry.rect.h);
}

void TileShadowOverlay::releaseResources()
{
    unlink();
    delete pointer;
}
