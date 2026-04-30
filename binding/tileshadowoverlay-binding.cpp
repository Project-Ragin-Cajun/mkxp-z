/*
** tileshadowoverlay-binding.cpp
*/

#include "binding-types.h"
#include "binding-util.h"
#include "disposable-binding.h"
#include "tileshadowoverlay.h"
#include "viewportelement-binding.h"
#include "bitmap.h"

#if RAPI_FULL > 187
DEF_TYPE(TileShadowOverlay);
#else
DEF_ALLOCFUNC(TileShadowOverlay);
#endif

RB_METHOD(tileShadowOverlayInitialize)
{
    TileShadowOverlay* overlay = viewportElementInitialize<TileShadowOverlay>(argc, argv, self);
    setPrivateData(self, overlay);
    return self;
}

/* height_map= accepts a Bitmap. */
RB_METHOD_GUARD(tileShadowOverlaySetHeightMap)
    {
        rb_check_argc(argc, 1);
        const TileShadowOverlay* overlay = getPrivateData<TileShadowOverlay>(self);
        const VALUE bitmapObj = *argv;
        Bitmap* bitmap = nullptr;

        if (!NIL_P(bitmapObj))
        {
            bitmap = getPrivateDataCheck<Bitmap>(bitmapObj, BitmapType);
        }

        GFX_GUARD_EXC(overlay->setHeightMap(bitmap);)
        rb_iv_set(self, "height_map", bitmapObj);

        return bitmapObj;
    }
RB_METHOD_GUARD_END

RB_METHOD(tileShadowOverlayGetHeightMap)
{
    RB_UNUSED_PARAM;
    return rb_iv_get(self, "height_map");
}

DEF_GFX_PROP_I(TileShadowOverlay, MapWidth)
DEF_GFX_PROP_I(TileShadowOverlay, MapHeight)
DEF_GFX_PROP_F(TileShadowOverlay, SunDirectionX)
DEF_GFX_PROP_F(TileShadowOverlay, SunDirectionY)
DEF_GFX_PROP_F(TileShadowOverlay, ShadowLength)
DEF_GFX_PROP_F(TileShadowOverlay, ScrollX)
DEF_GFX_PROP_F(TileShadowOverlay, ScrollY)
DEF_GFX_PROP_I(TileShadowOverlay, Opacity)

void tileShadowOverlayBindingInit()
{
    const VALUE klass = rb_define_class("TileShadowOverlay", rb_cObject);
#if RAPI_FULL > 187
    rb_define_alloc_func(klass, classAllocate<&TileShadowOverlayType>);
#else
    rb_define_alloc_func(klass, TileShadowOverlayAllocate);
#endif

    disposableBindingInit<TileShadowOverlay>(klass);
    viewportElementBindingInit<TileShadowOverlay>(klass);

    _rb_define_method(klass, "initialize", tileShadowOverlayInitialize);

    INIT_PROP_BIND(TileShadowOverlay, MapWidth, "map_width");
    INIT_PROP_BIND(TileShadowOverlay, MapHeight, "map_height");
    INIT_PROP_BIND(TileShadowOverlay, SunDirectionX, "sun_direction_x");
    INIT_PROP_BIND(TileShadowOverlay, SunDirectionY, "sun_direction_y");
    INIT_PROP_BIND(TileShadowOverlay, ShadowLength, "shadow_length");
    INIT_PROP_BIND(TileShadowOverlay, ScrollX, "scroll_x");
    INIT_PROP_BIND(TileShadowOverlay, ScrollY, "scroll_y");
    INIT_PROP_BIND(TileShadowOverlay, Opacity, "opacity");

    _rb_define_method(klass, "height_map", tileShadowOverlayGetHeightMap);
    _rb_define_method(klass, "height_map=", tileShadowOverlaySetHeightMap);
}
