/*
 *  GL Widgets, GLW_IMAGE widget
 *  Copyright (C) 2007 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "glw.h"
#include "glw_image.h"

static void
glw_image_dtor(glw_t *w)
{
  glw_image_t *gi = (void *)w;

  if(gi->gi_tex != NULL)
    glw_tex_deref(w->glw_root, gi->gi_tex);

  if(gi->gi_render_initialized)
    glw_render_free(&gi->gi_gr);
}

static void 
glw_image_render(glw_t *w, glw_rctx_t *rc)
{
  glw_image_t *gi = (void *)w;
  glw_loadable_texture_t *glt = gi->gi_tex;
  float alpha_self;
  glw_rctx_t rc0;
  glw_t *c;
  float xs, ys;

  if(glt == NULL || glt->glt_state != GLT_STATE_VALID)
    return;

  if(!glw_is_tex_inited(&glt->glt_texture))
    alpha_self = 0;
  else
    alpha_self = rc->rc_alpha * w->glw_alpha * gi->gi_alpha_self;

  if(w->glw_class == GLW_IMAGE || w->glw_class == GLW_ICON || 
     !gi->gi_border_scaling) {

    rc0 = *rc;

    glw_PushMatrix(&rc0, rc);

    //    glw_align_1(&rc0, w->glw_alignment);
      
    if(w->glw_class == GLW_IMAGE || w->glw_class == GLW_ICON)
      glw_rescale(&rc0, glt->glt_aspect);

    if(gi->gi_angle != 0)
      glw_Rotatef(&rc0, -gi->gi_angle, 0, 0, 1);

    //    glw_align_2(&rc0, w->glw_alignment);

    if(glw_is_focusable(w))
      glw_store_matrix(w, &rc0);
    
    if(alpha_self > 0.01)
      glw_render(&gi->gi_gr, &rc0, 
		 GLW_RENDER_MODE_QUADS, GLW_RENDER_ATTRIBS_TEX,
		 &glt->glt_texture,
		 gi->gi_color.r, gi->gi_color.g, gi->gi_color.b, alpha_self);
 
    if((c = TAILQ_FIRST(&w->glw_childs)) != NULL) {
      rc0.rc_alpha = rc->rc_alpha * w->glw_alpha;
      glw_render0(c, &rc0);
    }
    glw_PopMatrix();

  } else {

    if(glw_is_focusable(w))
      glw_store_matrix(w, rc);

    if(alpha_self > 0.01)
      glw_render(&gi->gi_gr, rc, 
		 GLW_RENDER_MODE_QUADS, GLW_RENDER_ATTRIBS_TEX,
		 &glt->glt_texture,
		 gi->gi_color.r, gi->gi_color.g, gi->gi_color.b, alpha_self);

    if((c = TAILQ_FIRST(&w->glw_childs)) != NULL) {

      rc0 = *rc;
      
      glw_PushMatrix(&rc0, rc);
      
      glw_Translatef(&rc0, gi->gi_child_xt, gi->gi_child_yt, 0.0f);

      xs = gi->gi_child_xs;
      ys = gi->gi_child_ys;

      glw_Scalef(&rc0, xs, ys, 1.0f);

      rc0.rc_size_x = rc->rc_size_x * xs;
      rc0.rc_size_y = rc->rc_size_y * ys;

      rc0.rc_alpha = rc->rc_alpha * w->glw_alpha;
      glw_render0(c, &rc0);

      glw_PopMatrix();
    }
  }
}


/**
 *
 */
static void
glw_image_layout_tesselated(glw_rctx_t *rc, glw_image_t *gi, 
			    glw_loadable_texture_t *glt)
{
  float tex[4][2];
  float vex[4][2];
  int x, y, i = 0;

  gi->gi_saved_size_x = rc->rc_size_x;
  gi->gi_saved_size_y = rc->rc_size_y;

  tex[0][0] = 0.0;
  tex[1][0] = 0.0 + gi->gi_border_left  / glt->glt_xs;
  tex[2][0] = 1.0 - gi->gi_border_right / glt->glt_xs;
  tex[3][0] = 1.0;

  tex[0][1] = 0.0;
  tex[1][1] = 0.0 + gi->gi_border_top    / glt->glt_ys;
  tex[2][1] = 1.0 - gi->gi_border_bottom / glt->glt_ys;
  tex[3][1] = 1.0;

  vex[0][0] =         -1.0;
  vex[1][0] = GLW_MIN(-1.0 + 2.0 * gi->gi_border_left  / rc->rc_size_x, 0.0);
  vex[2][0] = GLW_MAX( 1.0 - 2.0 * gi->gi_border_right / rc->rc_size_x, 0.0);
  vex[3][0] =          1.0;
    
  vex[0][1] =          1.0;
  vex[1][1] = GLW_MAX( 1.0 - 2.0 * gi->gi_border_top    / rc->rc_size_y, 0.0);
  vex[2][1] = GLW_MIN(-1.0 + 2.0 * gi->gi_border_bottom / rc->rc_size_y, 0.0);
  vex[3][1] =         -1.0;

  glw_render_set_pre(&gi->gi_gr);

  for(y = 0; y < 3; y++) {
    for(x = 0; x < 3; x++) {

      glw_render_vtx_pos(&gi->gi_gr, i, vex[x + 0][0], vex[y + 1][1], 0.0f);
      glw_render_vtx_st (&gi->gi_gr, i, tex[x + 0][0], tex[y + 1][1]);
      i++;

      glw_render_vtx_pos(&gi->gi_gr, i, vex[x + 1][0], vex[y + 1][1], 0.0f);
      glw_render_vtx_st (&gi->gi_gr, i, tex[x + 1][0], tex[y + 1][1]);
      i++;

      glw_render_vtx_pos(&gi->gi_gr, i, vex[x + 1][0], vex[y + 0][1], 0.0f);
      glw_render_vtx_st (&gi->gi_gr, i, tex[x + 1][0], tex[y + 0][1]);
      i++;

      glw_render_vtx_pos(&gi->gi_gr, i, vex[x + 0][0], vex[y + 0][1], 0.0f);
      glw_render_vtx_st (&gi->gi_gr, i, tex[x + 0][0], tex[y + 0][1]);
      i++;
    }
  }

  glw_render_set_post(&gi->gi_gr);
    

  gi->gi_child_xt = (vex[2][0] + vex[1][0]) * 0.5f;
  gi->gi_child_yt = (vex[1][1] + vex[2][1]) * 0.5f;

  gi->gi_child_xs = (vex[2][0] - vex[1][0]) * 0.5f;
  gi->gi_child_ys = (vex[1][1] - vex[2][1]) * 0.5f;
}


/**
 *
 */
static void
glw_image_update_constraints(glw_image_t *gi)
{
  glw_loadable_texture_t *glt = gi->gi_tex;
  glw_t *c;
  int y = 0, x = 0;
  glw_root_t *gr = gi->w.glw_root;

  if(gi->w.glw_class == GLW_BACKDROP) {

    c = TAILQ_FIRST(&gi->w.glw_childs);

    if(c != NULL) {

      if(c->glw_req_size_y)
	y = c->glw_req_size_y + gi->gi_border_top + gi->gi_border_bottom;

      if(c->glw_req_size_x)
	x = c->glw_req_size_x + gi->gi_border_left + gi->gi_border_right;
    }

    glw_set_constraint_xy(&gi->w, x, y);

  } else if(gi->w.glw_class == GLW_ICON) {

    glw_set_constraint_xy(&gi->w, 
			  gi->gi_size * gr->gr_fontsize_px, 
			  gi->gi_size * gr->gr_fontsize_px); 

  } else {

    glw_set_constraint_aspect(&gi->w, 
			      glt && glt->glt_state == GLT_STATE_VALID ? 
			      glt->glt_aspect : 1);
  }
}


/**
 *
 */
static void 
glw_image_layout(glw_t *w, glw_rctx_t *rc)
{
  glw_image_t *gi = (void *)w;
  glw_loadable_texture_t *glt = gi->gi_tex;
  glw_rctx_t rc0;
  glw_t *c;

  if(glt == NULL)
    return;

  glw_tex_layout(w->glw_root, glt);

  if(glt->glt_state == GLT_STATE_VALID) {
    if(gi->gi_render_init == 1) {
      gi->gi_render_init = 0;

      if(gi->gi_render_initialized)
	glw_render_free(&gi->gi_gr);

      glw_render_init(&gi->gi_gr, 4 * (gi->gi_border_scaling ? 9 : 1),
		      GLW_RENDER_ATTRIBS_TEX);
      gi->gi_render_initialized = 1;

      if(!gi->gi_border_scaling) {

	glw_render_vtx_pos(&gi->gi_gr, 0, -1.0, -1.0, 0.0);
	glw_render_vtx_st (&gi->gi_gr, 0,  0.0,  1.0);

	glw_render_vtx_pos(&gi->gi_gr, 1,  1.0, -1.0, 0.0);
	glw_render_vtx_st (&gi->gi_gr, 1,  1.0,  1.0);

	glw_render_vtx_pos(&gi->gi_gr, 2,  1.0,  1.0, 0.0);
	glw_render_vtx_st (&gi->gi_gr, 2,  1.0,  0.0);

	glw_render_vtx_pos(&gi->gi_gr, 3, -1.0,  1.0, 0.0);
	glw_render_vtx_st (&gi->gi_gr, 3,  0.0,  0.0);
      } else {
	glw_image_layout_tesselated(rc, gi, glt);
      }
    } else if(gi->gi_border_scaling &&
	      (gi->gi_saved_size_x != rc->rc_size_x ||
	       gi->gi_saved_size_y != rc->rc_size_y)) {
      glw_image_layout_tesselated(rc, gi, glt);
    }
  }

  if(!gi->gi_border_scaling)
    glw_image_update_constraints(gi);

  if((c = TAILQ_FIRST(&w->glw_childs)) != NULL) {
    rc0 = *rc;
    rc0.rc_size_x = rc->rc_size_x * gi->gi_child_xs;
    rc0.rc_size_y = rc->rc_size_y * gi->gi_child_ys;
    glw_layout0(c, &rc0);
  }
}


/*
 *
 */
static int
glw_image_callback(glw_t *w, void *opaque, glw_signal_t signal,
		    void *extra)
{
  glw_t *c;
  switch(signal) {
  default:
    break;
  case GLW_SIGNAL_LAYOUT:
    glw_image_layout(w, extra);
    break;
  case GLW_SIGNAL_RENDER:
    glw_image_render(w, extra);
    return 1;
  case GLW_SIGNAL_DTOR:
    glw_image_dtor(w);
    break;
  case GLW_SIGNAL_EVENT:
    TAILQ_FOREACH(c, &w->glw_childs, glw_parent_link)
      if(glw_signal0(c, GLW_SIGNAL_EVENT, extra))
	return 1;
    break;

  case GLW_SIGNAL_CHILD_CONSTRAINTS_CHANGED:
  case GLW_SIGNAL_CHILD_CREATED:
    glw_image_update_constraints((glw_image_t *)w);
    return 1;
  case GLW_SIGNAL_CHILD_DESTROYED:
    glw_set_constraint_xy(w, 0, 0);
    return 1;

  }
  return 0;
}

/*
 *
 */
void 
glw_image_ctor(glw_t *w, int init, va_list ap)
{
  glw_image_t *gi = (void *)w;
  glw_attribute_t attrib;
  const char *filename = NULL;
  glw_root_t *gr = w->glw_root;

  if(init) {
    glw_signal_handler_int(w, glw_image_callback);
    gi->gi_alpha_self = 1;
    gi->gi_color.r = 1.0;
    gi->gi_color.g = 1.0;
    gi->gi_color.b = 1.0;
    gi->gi_size = 1.0;

    if(w->glw_class == GLW_IMAGE)
      glw_set_constraint_aspect(&gi->w, 1); 

  }

  do {
    attrib = va_arg(ap, int);
    switch(attrib) {
    case GLW_ATTRIB_BORDER_SIZE:
      gi->gi_border_scaling = 1;
      gi->gi_border_left   = va_arg(ap, double);
      gi->gi_border_top    = va_arg(ap, double);
      gi->gi_border_right  = va_arg(ap, double);
      gi->gi_border_bottom = va_arg(ap, double);
      gi->gi_render_init = 1;
      glw_image_update_constraints(gi);
      break;

    case GLW_ATTRIB_ANGLE:
      gi->gi_angle = va_arg(ap, double);
      break;

    case GLW_ATTRIB_ALPHA_SELF:
      gi->gi_alpha_self = va_arg(ap, double);
      break;
      
    case GLW_ATTRIB_SOURCE:
      filename = va_arg(ap, char *);

      if(gi->gi_tex != NULL)
	glw_tex_deref(w->glw_root, gi->gi_tex);

      gi->gi_tex = filename ? glw_tex_create(w->glw_root, filename) : NULL;
      gi->gi_render_init = 1;
      break;

    case GLW_ATTRIB_PIXMAP:
      gi->gi_tex = glw_tex_create_from_pixmap(w->glw_root, 
					      va_arg(ap, prop_pixmap_t *));
      gi->gi_render_init = 1;
      break;

    case GLW_ATTRIB_MIRROR:
      gi->gi_mirror = va_arg(ap, int);
      gi->gi_render_init = 1;
      break;

    case GLW_ATTRIB_RGB:
      gi->gi_color.r = va_arg(ap, double);
      gi->gi_color.g = va_arg(ap, double);
      gi->gi_color.b = va_arg(ap, double);
      break;

    case GLW_ATTRIB_SIZE:
      gi->gi_size = va_arg(ap, double);
      break;

    default:
      GLW_ATTRIB_CHEW(attrib, ap);
      break;
    }
  } while(attrib);

  if(w->glw_class == GLW_ICON)
    glw_set_constraint_xy(&gi->w, 
			  gi->gi_size * gr->gr_fontsize_px, 
			  gi->gi_size * gr->gr_fontsize_px); 
}
