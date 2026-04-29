/*
** godrayoverlay.cpp
*/

#include "godrayoverlay.h"

#include "quad.h"

struct GodRayOverlayPrivate
{
    float lightX;
    float lightY;
    float intensity;
    float rayColorR;
    float rayColorG;
    float rayColorB;
    float decay;
    float rayDensity;
    float raySharpness;
    float phase;
    NormValue opacity;

    Vec2i screenSize;
    Quad quad;

    GodRayOverlayPrivate()
        : lightX(0.5f),
          lightY(0.0f),
          intensity(0.5f),
          rayColorR(1.0f),
          rayColorG(0.94f),
          rayColorB(0.78f),
          decay(2.0f),
          rayDensity(12.0f),
          raySharpness(1.0f),
          phase(0.0f),
          opacity(160),
          screenSize(1, 1)
    {
    }
};


GodRayOverlay::GodRayOverlay(Viewport* viewport)
    : ViewportElement(viewport)
{
    pointer = new GodRayOverlayPrivate();
    GodRayOverlay::onGeometryChange(scene->getGeometry());
}

GodRayOverlay::~GodRayOverlay()
{
    dispose();
}

DEF_ATTR_SIMPLE(GodRayOverlay, LightX, float, pointer->lightX);
DEF_ATTR_SIMPLE(GodRayOverlay, LightY, float, pointer->lightY);
DEF_ATTR_SIMPLE(GodRayOverlay, Intensity, float, pointer->intensity);
DEF_ATTR_SIMPLE(GodRayOverlay, RayColorR, float, pointer->rayColorR);
DEF_ATTR_SIMPLE(GodRayOverlay, RayColorG, float, pointer->rayColorG);
DEF_ATTR_SIMPLE(GodRayOverlay, RayColorB, float, pointer->rayColorB);
DEF_ATTR_SIMPLE(GodRayOverlay, Decay, float, pointer->decay);
DEF_ATTR_SIMPLE(GodRayOverlay, RayDensity, float, pointer->rayDensity);
DEF_ATTR_SIMPLE(GodRayOverlay, RaySharpness, float, pointer->raySharpness);
DEF_ATTR_SIMPLE(GodRayOverlay, Phase, float, pointer->phase);
DEF_ATTR_SIMPLE(GodRayOverlay, Opacity, int, pointer->opacity)

void GodRayOverlay::draw()
{
    if (!pointer->opacity)
    {
        return;
    }

    GodRayShader& shader = shState->shaders().godRay;
    shader.bind();
    shader.applyViewportProj();
    shader.setTranslation(Vec2i());

    /* texSizeInv normalizes the quad's texCoord (set in pixel
     * coordinates) to [0, 1] range in the vertex shader, giving
     * the fragment shader a clean v_texCoord for screen-space
     * ray computation. */
    shader.setTexSize(pointer->screenSize);

    shader.setLightPosition(Vec2(pointer->lightX, pointer->lightY));
    shader.setRayColor(Vec4(pointer->rayColorR, pointer->rayColorG, pointer->rayColorB, 0));
    shader.setIntensity(pointer->intensity);
    shader.setDecay(pointer->decay);
    shader.setRayDensity(pointer->rayDensity);
    shader.setRaySharpness(pointer->raySharpness);
    shader.setPhase(pointer->phase);
    shader.setOpacity(pointer->opacity.norm);

    /* Additive blending: Rays brighten the scene. */
    glState.blendMode.pushSet(BlendAddition);

    pointer->quad.draw();
    glState.blendMode.pop();
}

void GodRayOverlay::onGeometryChange(const Scene::Geometry& geometry)
{
    /* Tex rect matches pos rect in pixel coordinates.
     * simple.vert multiplies by textSizeInv to normalize. */
    pointer->quad.setTexPosRect(FloatRect(geometry.rect), FloatRect(geometry.rect));
    pointer->screenSize = Vec2i(geometry.rect.w, geometry.rect.h);
}

void GodRayOverlay::releaseResources()
{
    unlink();
    delete pointer;
}
