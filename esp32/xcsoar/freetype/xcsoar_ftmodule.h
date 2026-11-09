/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright The XCSoar Project */

/* The FreeType modules XCSoar needs for its TrueType fonts, used
   instead of FreeType's list of all of them: the other font formats
   would take about 200 kB of flash. */

FT_USE_MODULE( FT_Module_Class, autofit_module_class )
FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )
FT_USE_MODULE( FT_Renderer_Class, ft_raster1_renderer_class )
