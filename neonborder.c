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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
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


property_color (colorneon, _("Color (recommended white)"), "#ffffff")


property_color  (colorneon2, _("Color 2"), "#00ff27")
  description   (_("The glow's color (defaults to a green)"))
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

property_double (opacity, _("Opacity"), 1.2)
  value_range   (0.0, 1.3)
  ui_steps      (0.01, 0.10)

property_double (opacity2, _("Opacity 2"), 1.2)
  value_range   (0.0, 1.3)
  ui_steps      (0.01, 0.10)

property_color (colorblur, _("Color of glow"), "#96f8d0")
    description (_("The color to paint over the input"))


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


#else

#define GEGL_OP_META
#define GEGL_OP_NAME     neonborder
#define GEGL_OP_C_SOURCE neonborder.c

#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input, *output, *zzoutline, *stroke, *crop, *stroke2, *zzoutline2, *color, *colorblur, *box, *nop, *behind, *opacity, *gaussian;
  GeglColor *neonhiddencolor1 = gegl_color_new ("#ff2000");
  GeglColor *neonhiddencolor2 = gegl_color_new ("#ff2000");


  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");


  color   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);

  colorblur   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);


  behind   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",
                                  NULL);

  opacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL);

   zzoutline   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                   "value", neonhiddencolor1, NULL);
                                  

  zzoutline2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-to-alpha",  "transparency-threshold", 0.5,
                                   "color", neonhiddencolor2, NULL);
                          
 gaussian    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur",
                                  NULL);

  stroke      = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);

  stroke2     = gegl_node_new_child (gegl, "operation", "gegl:dropshadow", "x", 0.0, "y", 0.0,  NULL);

                                         
  box    = gegl_node_new_child (gegl,
                                  "operation", "gegl:box-blur", "radius", 1,  NULL);
  nop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  crop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);


  gegl_operation_meta_redirect (operation, "gaus", gaussian, "std-dev-x");
  gegl_operation_meta_redirect (operation, "gaus2", gaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "colorneon", color, "value");
  gegl_operation_meta_redirect (operation, "colorneon2", stroke2, "color");
  gegl_operation_meta_redirect (operation, "stroke", stroke, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke", stroke, "radius");
  gegl_operation_meta_redirect (operation, "stroke2", stroke2, "grow-radius");
  gegl_operation_meta_redirect (operation, "blurstroke2", stroke2, "radius");
  gegl_operation_meta_redirect (operation, "opacity", stroke, "opacity");
  gegl_operation_meta_redirect (operation, "opacity2", stroke2, "opacity");
  gegl_operation_meta_redirect (operation, "colorblur", colorblur, "value");
  gegl_operation_meta_redirect (operation, "opacityglow", opacity, "value");



  gegl_node_link_many (input, zzoutline, stroke, zzoutline2, color, stroke2, crop, box, nop, behind, output, NULL);
 gegl_node_link_many (nop, opacity, colorblur, gaussian, NULL);
gegl_node_connect_from (behind, "aux", gaussian, "output"); 


}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:neon-border",
    "title",       _("Neon Border"),
    "categories",  "Artistic",
    "reference-hash", "33do6a1h2a00xn3v25sb2ac",
    "description", _("Neon Border text styling filter."
                     ""),
    "gimp:menu-path", "<Image>/Filters/Text Styling",
    "gimp:menu-label", _("Neon Border..."),
    NULL);
}

#endif
