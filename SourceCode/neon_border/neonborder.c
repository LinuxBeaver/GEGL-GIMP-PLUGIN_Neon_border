/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Credit to Øyvind Kolås (pippin) for major GEGL contributions
 * 2022 Beaver (GEGL neon border)
 */

/* 
GEGL GRAPH OF NEON BORDER, *may not be 100% accurate. If you paste below in Gimp's
GEGL Graph you can test this filter without installing it.

color-overlay value=#ff2000
dropshadow x=0 y=0 grow-radius=9 radius=2.2 opacity=1
color-to-alpha transparency-threshold=0.5  color=#ff2000
color-overlay value=#ffffff
dropshadow x=0 y=0 grow-radius=10 radius=12 color=#00d8ff opacity=1
id=1 dst-over aux=[ ref=1 gaussian-blur std-dev-y=50 std-dev-x=5 opacity value=0.6 ] */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (policy, _("Merge with Image"), FALSE)
  description    (_("Should the neon border merge with the image content or be alone? "))

property_boolean (huemode, _("Hue rotation mode"), FALSE)
  description    (_("Forget about some color pickers and just rotate the hue"))

property_double (hue, _("Hue Rotation"), 0.0)
  value_range   (-180, 180)
  ui_range      (-180, 180)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  ui_meta     ("sensitive", " huemode")
   ui_meta ("visible", " huemode")

property_color (colorneon, _("Color (recommended white)"), "#ffffff")
  description   (_("Changing this will ruin the neon border effect but feel free to do whatever you want"))


property_color  (colorneon2, _("Color 2"), "#00ff27")
  description   (_("The glow's color (defaults to a green)"))
  ui_meta     ("sensitive", "! huemode")
   ui_meta ("visible", "! huemode")

/*This is having a problem. The color sometimes does not update
until Gimp's clipping setting is changed from Adjust to Clip.
I think it needs to be inside a composer - beaver, UPDATE BUG SOLVED IN MID 2023*/

property_double (blurstroke, _("Blur radius"), 2.2)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 14.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")

property_double (blurstroke2, _("Blur radius 2"), 4.3)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 14.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")

property_double (stroke, _("Grow radius 1"), 9.0)
  value_range   (0, 50.0)
  ui_range      (0, 20.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the shadow before blurring; a negative value will contract the shadow instead"))

property_double (stroke2, _("Grow radius 2"), 2.1)
  value_range   (0, 12.0)
  ui_range      (0, 10.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the shadow before blurring; a negative value will contract the shadow instead"))

property_double (opacity, _("Opacity"), 1.0)
  value_range   (0.0, 1.0)
  ui_steps      (0.01, 0.10)

property_double (opacity2, _("Opacity 2"), 1.0)
  value_range   (0.0, 1.0)
  ui_steps      (0.01, 0.10)

property_color (colorblur, _("Color of glow"), "#96f8d0")
    description (_("The color to paint over the input"))
  ui_meta     ("sensitive", "! huemode")
   ui_meta ("visible", "! huemode")

property_double (gaus, _("Gaussian Glow X"), 10)
   description (_("Standard deviation for the horizontal axis"))
   value_range (0.0, 100.0)

property_double (gaus2, _("Gaussian Glow Y"), 65)
   description (_("Standard deviation for the vertical axis"))
   value_range (0.0, 100.0)

property_double (opacityglow, _("Opacity of Glow"), 0.40)
    description (_("Global opacity value that is always used on top of the optional auxiliary input buffer."))
    value_range (0.00, 1.00)
    ui_range    (0.00, 1.00)

property_boolean (clipbugpolicy, _("Disable clipping (Allows color update delay bug)"), FALSE)
  description    (_("This checkbox removes the shadow clip bug for compliance with Gimp 3's non-destructive text editing. If enabled this will triger another bug only seen after using GEGL Effects heavily, said bug appears usually after a few minutes of usage and will cause GEGL Effects to delay a selected color update until another slider is moved. It is suggested to enable this once one applies the filter. But keep it disabled while editing GEGL Effects unless you can tolerate a delayed color update."))


#else

#define GEGL_OP_META
#define GEGL_OP_NAME     neonborder
#define GEGL_OP_C_SOURCE neonborder.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *output;
  GeglNode *newnop;
  GeglNode *coloroverlay;
  GeglNode *stroke;
  GeglNode *crop;
  GeglNode *stroke2;
  GeglNode *c2a;
  GeglNode *color;
  GeglNode *replace;
  GeglNode *normal;
  GeglNode *colorblur;
  GeglNode *box;
  GeglNode *huelight;
  GeglNode *nop;
  GeglNode *crop2;
  GeglNode *behind;
  GeglNode *opacity;
  GeglNode *nothinghue;
  GeglNode *hue;
  GeglNode *gaussian;
  GeglNode *stroke2alt;
 GeglNode  *colorblurstatic;
} State; 


static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglColor *neonhiddencolor1 = gegl_color_new ("#ff2000");
  GeglColor *neonhiddencolor2 = gegl_color_new ("#ff2000");
  GeglColor *staticcolor = gegl_color_new ("#00ff27");
  GeglColor *staticcolorblur = gegl_color_new ("#96f8d0");


  State *state = o->user_data = g_malloc0 (sizeof (State));

    state->input    = gegl_node_get_input_proxy (gegl, "input");
    state->output   = gegl_node_get_output_proxy (gegl, "output");


    state->color   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay", 
                                  NULL);

    state->colorblur   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);



    state->colorblurstatic   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",  "value", staticcolorblur,
                                  NULL);



    state->behind   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",
                                  NULL);

    state->opacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL);

     state->coloroverlay   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                   "value", neonhiddencolor1, NULL);
                                  

    state->c2a   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-to-alpha",  "transparency-threshold", 0.5,
                                   "color", neonhiddencolor2, NULL);
                          
   state->gaussian    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur",  "abyss-policy", 0,  "clip-extent", FALSE,                 
                                   NULL);

    state->stroke      = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);

    state->stroke2     = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);

    state->stroke2alt     = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0, "color", staticcolor,  NULL);


                                         
  state->box    = gegl_node_new_child (gegl,
                                  "operation", "gegl:box-blur", "radius", 1,  NULL);

  state->nop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->crop2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);
  state->newnop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);
  state->replace    = gegl_node_new_child (gegl,
                                  "operation", "gegl:src",   NULL);

  state->normal    = gegl_node_new_child (gegl,
                                  "operation", "gegl:over",
                                  NULL);
                         

  state->crop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

  state->hue    = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);

  state->nothinghue    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  gegl_operation_meta_redirect (operation, "gaus", state->gaussian, "std-dev-x");
  gegl_operation_meta_redirect (operation, "gaus2", state->gaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "colorneon", state->color, "value");
  gegl_operation_meta_redirect (operation, "colorneon2", state->stroke2, "color");
  gegl_operation_meta_redirect (operation, "stroke", state->stroke, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke", state->stroke, "radius");
  gegl_operation_meta_redirect (operation, "stroke2", state->stroke2, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke2", state->stroke2, "radius");
  gegl_operation_meta_redirect (operation, "opacity", state->stroke, "opacity");
  gegl_operation_meta_redirect (operation, "opacity2", state->stroke2, "opacity");
  gegl_operation_meta_redirect (operation, "colorblur", state->colorblur, "value");
  gegl_operation_meta_redirect (operation, "opacityglow", state->opacity, "value");
  gegl_operation_meta_redirect (operation, "hue", state->hue, "hue");



  gegl_operation_meta_redirect (operation, "stroke2", state->stroke2alt, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke2", state->stroke2alt, "radius");
  gegl_operation_meta_redirect (operation, "opacity2", state->stroke2alt, "opacity");

}


static void update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;  
  GeglNode *crop = state->crop;
  GeglNode *borderpolicy = state->replace;
  GeglNode *colorblur = state->colorblur;
  GeglNode *stroke2 = state->stroke2;
  GeglNode *huechoice = state->hue;

  if (o->clipbugpolicy) crop = state->newnop;
  if (!o->clipbugpolicy) crop = state->crop;


  if (!o->policy) borderpolicy = state->replace;
  if (o->policy) borderpolicy = state->normal;

  if (!o->huemode) huechoice = state->nothinghue;
  if (o->huemode) huechoice = state->hue;

  if (!o->huemode) stroke2 = state->stroke2;
  if (o->huemode) stroke2 = state->stroke2alt;

  if (!o->huemode) colorblur = state->colorblur;
  if (o->huemode) colorblur = state->colorblurstatic;

gegl_node_link_many (state->input, borderpolicy, state->output, NULL);
gegl_node_link_many (state->input, state->coloroverlay, state->stroke, state->crop2, state->c2a, state->color, stroke2, crop, state->box, state->nop, state->behind, huechoice, NULL);
gegl_node_link_many (state->nop, state->opacity, colorblur, state->gaussian, NULL);
gegl_node_connect (state->behind, "aux", state->gaussian, "output"); 
gegl_node_connect (state->crop2, "aux", state->input, "output"); 
gegl_node_connect (borderpolicy, "aux", huechoice, "output"); 


}
    

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:neon-border",
    "title",       _("Neon Border"),
    "reference-hash", "33do6a1h2a00xn3v25sb2ac",
    "description", _("Neon Border text styling"
                     ""),
    "gimp:menu-path", "<Image>/Filters/Text Styling",
    "gimp:menu-label", _("Neon Border..."),
    NULL);
}

#endif
