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
   ui_meta ("visible", "! huemode")

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
  ui_range      (0.0, 8.0)
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

property_boolean (clipbugpolicy, _("Disable clipping (Allows color update delay bug)"), TRUE)
  description    (_("Hidden setting - This checkbox removes the shadow clip bug for compliance with GIMP 3's non-destructive text editing. If enabled this will triger another bug only seen after using Neon Border heavily, said bug appears usually after a few minutes of usage and will cause Neon Border to delay a selected color update until another slider is moved. It is suggested to enable this once one applies the filter. But keep it disabled while editing Neon Border unless you can tolerate a delayed color update."))
    ui_meta     ("role", "output-extent")

enum_start (gegl_neon_mode_typebeavneon)
  enum_value (GEGL_NEON,      "sept2025",
              N_("Sept 2025 Graph"))
  enum_value (GEGL_NEON_CLASSIC,      "classic",
              N_("Classic"))
enum_end (geglneonmodetypebeavneon)

property_enum (type, _("Setting for Neon"),
    geglneonmodetypebeavneon, gegl_neon_mode_typebeavneon,
    GEGL_NEON)
    description (_("Hidden Setting - Version of Neon border to use"))
    ui_meta     ("role", "output-extent")

property_boolean (offcanvasclip, _("Off canvas clipping"), TRUE)
  description    (_("A trade off between two technical settings, default TRUE is best for text styles to prevent the glowing border outlines from clipping, but the consequence is canvas bordering content will always clip with a glowing line.  When disabled FALSE Neon Border doesn't clip at the end of the canvas so opaque pixels touching the canvas canvas don't have a glowing line, but text styles will clip if the outline borders are large, thus making the text style bad. In example, when TRUE said glowing line clip will present itself on a transparent covered image of a human that cuts off at the ankles, because the ankles are the canvas edge. Said image of person's ankles will clip if TRUE, and not clip of FALSE. This exist because there is no perfect GEGL graph only trade offs. Put simply, consider making FALSE for human legs or any content touching the canvas, for text styles on canvas keep TRUE "))


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
/*  GeglNode *stroke2;*/
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
  GeglNode *cliphere;
  GeglNode *cliphere2;
  GeglNode *colorblurstatic;
  GeglNode *colorwhitelock;
  GeglNode *g1blur;
  GeglNode *g1input;
  GeglNode *g1median;
  GeglNode *g1opacity;
  GeglNode *g1behind;
  GeglNode *g1repair;
  GeglNode *g1color;
  GeglNode *g2input;
  GeglNode *g2blur;
  GeglNode *g2median;
  GeglNode *g2opacity;
  GeglNode *g2behind;
  GeglNode *g2repair;
  GeglNode *g2color;
  GeglNode *g2coloralt;
  GeglNode *cropfinal;
  GeglNode *idreffinal;
  GeglNode *blurout;
  GeglNode *opacityintense;
  GeglNode *repairblurout;
  GeglNode *erase;
  GeglNode *ocg1median;
  GeglNode *ocg2median;
  GeglNode *ocblurout;
  GeglNode *ocblurout2;
  GeglNode *blurout2;
  GeglNode *opacityintense2;
  GeglNode *erase2;
  GeglNode *repairblurout2;
GeglNode *classicbehind;
GeglNode *classicbox;
GeglNode *classicc2a;
GeglNode *classiccolor;
GeglNode *classiccolorblur;
GeglNode *classiccoloroverlay;
GeglNode *classiccrop2;
GeglNode *classicgaussian;
GeglNode *classicnop;
GeglNode *classicopacity;
GeglNode *classicstroke;
GeglNode *classicstroke2;
GeglNode *classicinput;
GeglNode *classicoutput;

} State; 


static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglColor *neonhiddencolor1 = gegl_color_new ("#ff2000");
  GeglColor *neonhiddencolor2 = gegl_color_new ("#ff2000");
  GeglColor *staticcolor = gegl_color_new ("#00ff27");
  GeglColor *staticcolorblur = gegl_color_new ("#96f8d0");
  GeglColor *white = gegl_color_new ("#ffffff");
  GeglColor *white2 = gegl_color_new ("#ffffff");
  GeglColor *classicneonhiddencolor1 = gegl_color_new ("#ff2000");
  GeglColor *classicneonhiddencolor2 = gegl_color_new ("#ff2000");
  GeglColor *classicwhite = gegl_color_new ("#ffffff");


  State *state = o->user_data = g_malloc0 (sizeof (State));

    state->input    = gegl_node_get_input_proxy (gegl, "input");
    state->output   = gegl_node_get_output_proxy (gegl, "output");


    state->color   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay", "value", white,
                                  NULL);

    state->colorwhitelock   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay", "value", white2,
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
                                  "operation", "gegl:gaussian-blur",  "abyss-policy", 1,  "clip-extent", FALSE,                 
                                   NULL);



/*    
    state->stroke      = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);
 

* state->stroke2     = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL); */
                                
  state->box    = gegl_node_new_child (gegl,
                                  "operation", "gegl:box-blur", "radius", 1,   NULL);

  state->nop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->cliphere    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->cliphere2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->crop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

  state->crop2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop", 
                                  NULL);

  state->newnop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop", 
                                  NULL);

  state->replace    = gegl_node_new_child (gegl,
                                  "operation", "gegl:src",   NULL);

  state->normal    = gegl_node_new_child (gegl,
                                  "operation", "gegl:over",
                                  NULL);
                         
  state->hue    = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);

  state->nothinghue    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

   state->g1behind   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",  NULL);



   state->g1input   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",  NULL);


   state->g1opacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",  NULL);



   state->g1color   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",  NULL);

    state->g1blur      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "clip-extent", FALSE,
                                         "abyss-policy", 1, NULL);



    state->g1median     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "percentile",       100.0,
                                         "alpha-percentile", 100.0,
                                         "abyss-policy",     0,
                                         NULL);

    state->ocg1median     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "percentile",       100.0,
                                         "alpha-percentile", 100.0,
                                         "abyss-policy",     1,
                                         NULL);

   state->g1repair   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",   NULL);


   state->g2behind   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",  NULL);


   state->g2input   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",  NULL);



   state->g2opacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",  NULL);


   state->g2color   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",  NULL); 

   state->g2coloralt   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay", "value", staticcolor,  NULL);  

    state->g2blur      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "clip-extent", FALSE,
                                         "abyss-policy", 1, NULL);


    state->g2median     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "percentile",       100.0,
                                         "alpha-percentile", 100.0,
                                         "abyss-policy",     0,
                                         NULL);

    state->ocg2median     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "percentile",       100.0,
                                         "alpha-percentile", 100.0,
                                         "abyss-policy",     1,
                                         NULL);

   state->g2repair   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",       NULL);

   state->cropfinal   = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",  NULL);


   state->idreffinal   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",  NULL);


   state->erase   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-out",       NULL);

   state->opacityintense   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 2.9,       NULL);

    state->blurout      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "std-dev-x", 1.0, "std-dev-y", 1.0,
                                         "clip-extent", FALSE,
                                         "abyss-policy", 0, NULL);

    state->ocblurout      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "std-dev-x", 1.0, "std-dev-y", 1.0,
                                         "clip-extent", FALSE,
                                         "abyss-policy", 1, NULL);



    state->repairblurout     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "radius",       0,
                                         "abyss-policy",     1,
                                         NULL);

   state->erase2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-out",       NULL);

   state->opacityintense2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 0.7,       NULL);

    state->blurout2      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "std-dev-x", 1.0, "std-dev-y", 1.0,
                                         "clip-extent", FALSE,
                                         "abyss-policy", 0, NULL);

    state->ocblurout2      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "std-dev-x", 1.0, "std-dev-y", 1.0,
                                         "clip-extent", FALSE,
                                         "abyss-policy", 0, NULL);


    state->repairblurout2     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "radius",       0,
                                         "abyss-policy",     1,
                                         NULL);

/*classic*/

    state->classiccolor   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay", "value", classicwhite,
                                  NULL);



    state->classiccolorblur   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);





    state->classicopacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL);

     state->classiccoloroverlay   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                   "value", classicneonhiddencolor1, NULL);
                                  

    state->classicc2a   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-to-alpha",  "transparency-threshold", 0.5,
                                   "color", classicneonhiddencolor2, NULL);
                          
   state->classicgaussian    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur",  "abyss-policy", 1,  "clip-extent", FALSE,              

   
                                   NULL);

    state->classicstroke      = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);

    state->classicstroke2     = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);


                                         
  state->classicbox    = gegl_node_new_child (gegl,
                                  "operation", "gegl:box-blur", "radius", 1,  NULL);

  state->classicnop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->classiccrop2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

  state->classicbehind    = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",
                                  NULL);

    state->classicinput    = gegl_node_get_input_proxy (gegl, "input");
    state->classicoutput   = gegl_node_get_output_proxy (gegl, "output");



  gegl_operation_meta_redirect (operation, "gaus", state->gaussian, "std-dev-x");
  gegl_operation_meta_redirect (operation, "gaus2", state->gaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "colorneon", state->color, "value");
  gegl_operation_meta_redirect (operation, "colorneon2", state->g2color, "value");
  gegl_operation_meta_redirect (operation, "stroke", state->g1median, "radius");
  gegl_operation_meta_redirect (operation, "stroke", state->ocg1median, "radius");
  gegl_operation_meta_redirect (operation, "blurstroke", state->g1blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "blurstroke", state->g1blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "stroke2", state->g2median, "radius");
  gegl_operation_meta_redirect (operation, "stroke2", state->ocg2median, "radius");
  gegl_operation_meta_redirect (operation, "blurstroke2", state->g2blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "blurstroke2", state->g2blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "opacity", state->g1opacity, "value");
  gegl_operation_meta_redirect (operation, "opacity2", state->g2opacity, "value");
  gegl_operation_meta_redirect (operation, "colorblur", state->colorblur, "value");
  gegl_operation_meta_redirect (operation, "opacityglow", state->opacity, "value");
  gegl_operation_meta_redirect (operation, "hue", state->hue, "hue");


  gegl_operation_meta_redirect (operation, "gaus", state->classicgaussian, "std-dev-x");
  gegl_operation_meta_redirect (operation, "gaus2", state->classicgaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "colorneon", state->classiccolor, "value");
  gegl_operation_meta_redirect (operation, "colorneon2", state->classicstroke2, "color");
  gegl_operation_meta_redirect (operation, "stroke", state->classicstroke, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke", state->classicstroke, "radius");
  gegl_operation_meta_redirect (operation, "stroke2", state->classicstroke2, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke2", state->classicstroke2, "radius");
  gegl_operation_meta_redirect (operation, "opacity", state->classicstroke, "opacity");
  gegl_operation_meta_redirect (operation, "opacity2", state->classicstroke2, "opacity");
  gegl_operation_meta_redirect (operation, "colorblur", state->classiccolorblur, "value");
  gegl_operation_meta_redirect (operation, "opacityglow", state->classicopacity, "value");



/*
  gegl_operation_meta_redirect (operation, "stroke2", state->stroke2alt, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke2", state->stroke2alt, "radius");
  gegl_operation_meta_redirect (operation, "opacity2", state->stroke2alt, "opacity");
*/
}


static void update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;  
  GeglNode *crop = state->crop;
  GeglNode *borderpolicy = state->replace;
  GeglNode *colorblur = state->colorblur;
/*  GeglNode *stroke2 = state->stroke2; */
  GeglNode *huechoice = state->hue;
  GeglNode *colorwhites = state->color;
  GeglNode *stroke2 = state->g2color;
  GeglNode *g1median = state->g1median;
  GeglNode *g2median = state->g2median;
  GeglNode *blurout = state->blurout;
  GeglNode *blurout2 = state->blurout2;


  if (o->clipbugpolicy) crop = state->newnop;
  if (!o->clipbugpolicy) crop = state->crop; 
  if (o->policy) borderpolicy = state->normal;  
  if (o->huemode) huechoice = state->hue;  
  if (o->huemode) stroke2 = state->g2coloralt;  
  if (o->huemode) colorblur = state->colorblurstatic;  
  if (o->huemode) colorwhites = state->colorwhitelock;
  if (!o->offcanvasclip) g1median = state->ocg1median;
  if (!o->offcanvasclip)  g2median = state->ocg2median;
  if (!o->offcanvasclip)  blurout = state->ocblurout;
  if (!o->offcanvasclip)  blurout2 = state->ocblurout2;


 if (!o->policy) borderpolicy = state->replace;
 if (!o->huemode) huechoice = state->nothinghue;
 if (!o->huemode) stroke2 = state->g2color;
 if (!o->huemode) colorblur = state->colorblur;
 if (!o->huemode) colorwhites = state->color;

 if (o->offcanvasclip) g1median = state->g1median;
 if (o->offcanvasclip)  g2median = state->g2median;
 if (o->offcanvasclip)  blurout = state->blurout;
 if (o->offcanvasclip)  blurout2 = state->blurout2;



switch (o->type) {



    case GEGL_NEON:
gegl_node_link_many (state->input,    borderpolicy, state->idreffinal, state->cropfinal,  state->output, NULL);
gegl_node_link_many (state->input, state->g1input, state->g1behind, state->cliphere, state->crop2,  colorwhites, state->erase, state->g2input, state->g2behind, state->cliphere2, crop, state->box, state->nop, state->behind, huechoice,   NULL);
gegl_node_link_many (state->nop, state->opacity, colorblur, state->gaussian, state->erase2, NULL);
gegl_node_connect (state->behind, "aux", state->erase2, "output"); 
gegl_node_connect (state->crop2, "aux", state->cliphere, "output");  
gegl_node_connect (crop, "aux", state->cliphere2, "output"); 
gegl_node_connect (borderpolicy, "aux", huechoice, "output"); 
gegl_node_connect (state->g1behind, "aux", state->g1repair, "output"); 
gegl_node_link_many (state->input, g1median, state->g1blur, state->g1opacity,  state->g1color, state->g1repair,  NULL);
gegl_node_connect (state->g2behind, "aux", state->g2repair, "output"); 
gegl_node_link_many (state->g2input, g2median, state->g2blur, state->g2opacity,  stroke2, state->g2repair,  NULL);
gegl_node_connect (state->cropfinal, "aux", state->idreffinal, "output"); 
gegl_node_connect (state->erase, "aux", state->repairblurout, "output"); 
gegl_node_link_many (state->input, state->opacityintense, blurout, state->repairblurout,   NULL);
gegl_node_connect (state->erase2, "aux", state->repairblurout2, "output"); 
gegl_node_link_many (state->input, state->opacityintense2, blurout2, state->repairblurout2,   NULL);
        break;
    case GEGL_NEON_CLASSIC:
gegl_node_link_many (state->classicinput, state->classiccoloroverlay, state->classicstroke, state->classicc2a, state->classiccolor, state->classicstroke2, state->classicbox, state->classicnop, state->classicbehind, state->classicoutput, NULL);
gegl_node_link_many (state->classicnop, state->classicopacity, state->classiccolorblur, state->classicgaussian, NULL);
gegl_node_connect (state->classicbehind, "aux", state->classicgaussian, "output"); 



    }
}
    
/*2025, Neon Border is the  first GEGL plugin of mine that uses dispose (seen below), something that GIMPs team seems to do all the time for node based plugins
but my first implementation of this practice is here, perhaps in the future GEGL Effects and even all my other plugins could follow along 
 */

static void
dispose (GObject *object)
{
   GeglProperties  *o = GEGL_PROPERTIES (object);
   g_clear_pointer (&o->user_data, g_free);
   G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class      = GEGL_OPERATION_CLASS (klass);
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  operation_class->attach      = attach;
  operation_meta_class->update = update_graph;

  object_class               = G_OBJECT_CLASS (klass);
  object_class->dispose      = dispose;

  gegl_operation_class_set_keys (operation_class,
    "name",        "lb:neon-border",
    "title",       _("Neon Border"),
    "reference-hash", "33do6a1h2a00xn3v25sb2ac",
    "description", _("A glowing neon border logo for your text"
                     ""),
    "gimp:menu-path", "<Image>/Filters/Text Styling",
    "gimp:menu-label", _("Neon Border..."),
    NULL);
}

#endif
