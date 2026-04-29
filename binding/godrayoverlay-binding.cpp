/*
** godrayoverlay-binding.cpp
*/

#include "binding-types.h"
#include "binding-util.h"
#include "disposable-binding.h"
#include "godrayoverlay.h"
#include "viewportelement-binding.h"

#if RAPI_FULL > 187
DEF_TYPE(GodRayOverlay);
#else
DEF_ALLOCFUNC(GodRayOverlay);
#endif

RB_METHOD(godRayOverlayInitialize)
{
    GodRayOverlay* overlay = viewportElementInitialize<GodRayOverlay>(argc, argv, self);
    setPrivateData(self, overlay);
    return self;
}

DEF_GFX_PROP_F(GodRayOverlay, LightX)
DEF_GFX_PROP_F(GodRayOverlay, LightY)
DEF_GFX_PROP_F(GodRayOverlay, Intensity)
DEF_GFX_PROP_F(GodRayOverlay, RayColorR)
DEF_GFX_PROP_F(GodRayOverlay, RayColorG)
DEF_GFX_PROP_F(GodRayOverlay, RayColorB)
DEF_GFX_PROP_F(GodRayOverlay, Decay)
DEF_GFX_PROP_F(GodRayOverlay, RayDensity)
DEF_GFX_PROP_F(GodRayOverlay, RaySharpness)
DEF_GFX_PROP_F(GodRayOverlay, Phase)
DEF_GFX_PROP_I(GodRayOverlay, Opacity)

void godRayOverlayBindingInit()
{
    const VALUE klass = rb_define_class("GodRayOverlay", rb_cObject);
#if RAPI_FULL > 187
    rb_define_alloc_func(klass, classAllocate<&GodRayOverlayType>);
#else
    rb_define_alloc_func(klass, GodRayOverlayAllocate);
#endif

    disposableBindingInit<GodRayOverlay>(klass);
    viewportElementBindingInit<GodRayOverlay>(klass);

    _rb_define_method(klass, "initialize", godRayOverlayInitialize);

    INIT_PROP_BIND(GodRayOverlay, LightX, "light_x");
    INIT_PROP_BIND(GodRayOverlay, LightY, "light_y");
    INIT_PROP_BIND(GodRayOverlay, Intensity, "intensity");
    INIT_PROP_BIND(GodRayOverlay, RayColorR, "ray_color_r");
    INIT_PROP_BIND(GodRayOverlay, RayColorG, "ray_color_g");
    INIT_PROP_BIND(GodRayOverlay, RayColorB, "ray_color_b");
    INIT_PROP_BIND(GodRayOverlay, Decay, "decay");
    INIT_PROP_BIND(GodRayOverlay, RayDensity, "ray_density");
    INIT_PROP_BIND(GodRayOverlay, RaySharpness, "ray_sharpness");
    INIT_PROP_BIND(GodRayOverlay, Phase, "phase");
    INIT_PROP_BIND(GodRayOverlay, Opacity, "opacity");
}
