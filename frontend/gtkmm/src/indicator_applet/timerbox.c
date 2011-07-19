/*
 * workrave-timerbox.c
 *
 * Copyright (C) 2011 Rob Caelers <robc@krandor.nl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "timerbox.h"

#include <cairo.h>
#include <gtk/gtk.h>

#include "timebar.h"

#define WORKRAVE_TIMERBOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WORKRAVE_TYPE_TIMERBOX, WorkraveTimerboxPrivate))

static void workrave_timerbox_class_init(WorkraveTimerboxClass *klass);
static void workrave_timerbox_init(WorkraveTimerbox *self);
static void workrave_timerbox_dispose(GObject *gobject);
static void workrave_timerbox_finalize(GObject *gobject);
static void workrave_timerbox_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static void workrave_timerbox_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec);

static void workrave_timerbox_update_sheep(WorkraveTimerbox *self, cairo_t *cr);
static void workrave_timerbox_update_time_bars(WorkraveTimerbox *self, cairo_t *cr, cairo_surface_t *surface);

G_DEFINE_TYPE(WorkraveTimerbox, workrave_timerbox, G_TYPE_OBJECT);

enum
{
  PROP_0,
  PROP_NAME
};


const int PADDING_X = 2;
const int PADDING_Y = 2;

struct _WorkraveTimerboxPrivate
{
  gchar *name;
  GdkPixbuf *sheep_icon;
  WorkraveTimebar *slot_to_time_bar[BREAK_ID_SIZEOF];
  GdkPixbuf *break_to_icon[BREAK_ID_SIZEOF];
  BreakId slot_to_break[BREAK_ID_SIZEOF];
  short break_to_slot[BREAK_ID_SIZEOF];
  gboolean break_visible[BREAK_ID_SIZEOF];
  gboolean enabled;
  short filled_slots;
  int width;
  int height;
};


static void
workrave_timerbox_class_init(WorkraveTimerboxClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec;

  gobject_class->dispose = workrave_timerbox_dispose;
  gobject_class->finalize = workrave_timerbox_finalize;
  gobject_class->set_property = workrave_timerbox_set_property;
  gobject_class->get_property = workrave_timerbox_get_property;

  pspec = g_param_spec_string("workrave",
                              "Workrave construct prop",
                              "Set workrave's name",
                              "no-name-set" /* default value */,
                              G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_property(gobject_class,
                                  PROP_NAME,
                                  pspec);

  g_type_class_add_private(klass, sizeof(WorkraveTimerboxPrivate));
}


static void
workrave_timerbox_init(WorkraveTimerbox *self)
{
  self->priv = WORKRAVE_TIMERBOX_GET_PRIVATE(self); 

  const char *icons[] = { "timer-micro-break.png", "timer-rest-break.png", "timer-daily.png" };
  self->priv->sheep_icon = gdk_pixbuf_new_from_file(WORKRAVE_PKGDATADIR "/images/workrave.png", NULL);

  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      self->priv->slot_to_time_bar[i] = g_object_new(WORKRAVE_TYPE_TIMEBAR, NULL);

      GString *filename = g_string_new("");
      g_string_printf(filename, "%s/images/%s", WORKRAVE_PKGDATADIR, icons[i]);
      self->priv->break_to_icon[i] = gdk_pixbuf_new_from_file(filename->str, NULL);
      g_string_free(filename, TRUE);
      
      self->priv->break_visible[i] = FALSE;
      self->priv->slot_to_break[i] = BREAK_ID_NONE;
      self->priv->break_to_slot[i] = -1;
    }
  self->priv->filled_slots = 0;
  self->priv->enabled = TRUE;
  
  /* initialize all public and private members to reasonable default values. */

  /* If you need specific construction properties to complete initialization,
   * delay initialization completion until the property is set. 
   */
}



static void
workrave_timerbox_dispose(GObject *gobject)
{
  WorkraveTimerbox *self = WORKRAVE_TIMERBOX(gobject);

  g_object_unref(self->priv->sheep_icon);
  
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      g_object_unref(self->priv->break_to_icon[i]);
      g_object_unref(self->priv->slot_to_time_bar[i]);
    }

  /* 
   * In dispose, you are supposed to free all types referenced from this
   * object which might themselves hold a reference to self. Generally,
   * the most simple solution is to unref all members on which you own a 
   * reference.
   */

  /* dispose might be called multiple times, so we must guard against
   * calling g_object_unref() on an invalid GObject.
   */

  /* Chain up to the parent class */
  G_OBJECT_CLASS(workrave_timerbox_parent_class)->dispose(gobject);
}


static void
workrave_timerbox_finalize(GObject *gobject)
{
  //  WorkraveTimerbox *self = WORKRAVE_TIMERBOX(gobject);

  /* Chain up to the parent class */
  G_OBJECT_CLASS(workrave_timerbox_parent_class)->finalize(gobject);
}


void
workrave_timerbox_do_action(WorkraveTimerbox *self, int param)
{
  g_return_if_fail(WORKRAVE_IS_TIMERBOX(self));

  /* do stuff here. */
}


static void
workrave_timerbox_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
  WorkraveTimerbox *self = WORKRAVE_TIMERBOX(gobject);
  //GObject *obj;

  switch (property_id)
    {
    case PROP_NAME:
      g_free (self->priv->name);
      self->priv->name = g_value_dup_string(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
      break;
    }
}

static void
workrave_timerbox_get_property (GObject *gobject, guint property_id, GValue *value, GParamSpec *pspec)
{
  WorkraveTimerbox *self = WORKRAVE_TIMERBOX(gobject);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string(value, self->priv->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
      break;
    }
}


void
workrave_timerbox_set_slot(WorkraveTimerbox *self, int slot, BreakId brk)
{
  WorkraveTimerboxPrivate *priv = self->priv;
  
  BreakId old_brk = priv->slot_to_break[slot];
  if (old_brk != brk)
    {
      if (old_brk != BREAK_ID_NONE)
        {
          priv->break_visible[old_brk] = FALSE;
          priv->break_to_slot[old_brk] = -1;
          priv->filled_slots--;
        }
      priv->slot_to_break[slot] = brk;
      if (brk != BREAK_ID_NONE)
        {
          int old_slot = priv->break_to_slot[brk];
          if (old_slot >= 0)
            {
              priv->slot_to_break[old_slot] = BREAK_ID_NONE;
            }
          else
            {
              priv->filled_slots++;
            }
          priv->break_visible[brk] = TRUE;
          priv->break_to_slot[brk] = slot;
        }
    }
}


void
workrave_timerbox_update(WorkraveTimerbox *self, GtkImage *image)
{
  int win_w = 3 * 60;
  int win_h = 16;

  cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, win_w, win_h);
  cairo_t *cr = cairo_create (surface);

  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr);
  cairo_restore(cr);   
  
  workrave_timerbox_update_time_bars(self, cr, surface);
  workrave_timerbox_update_sheep(self, cr);

  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, win_w, win_h);
  gtk_image_set_from_pixbuf(image, pixbuf);

  cairo_surface_destroy(surface);
  cairo_destroy(cr);
}


void
workrave_timerbox_update_sheep(WorkraveTimerbox *self, cairo_t *cr)
{
  WorkraveTimerboxPrivate *priv = self->priv;

  if (!priv->enabled || priv->filled_slots == 0)
    {
      gdk_cairo_set_source_pixbuf(cr, priv->sheep_icon, 0, 0);
    }
}


void
workrave_timerbox_update_time_bars(WorkraveTimerbox *self, cairo_t *cr, cairo_surface_t *surface)
{
  WorkraveTimerboxPrivate *priv = self->priv;

  if (priv->enabled)
    {
      int x = 0, y = 0;
      int bar_w = 40, bar_h = 16;

      int icon_width = gdk_pixbuf_get_width(priv->break_to_icon[0]);
      int icon_height = gdk_pixbuf_get_height(priv->break_to_icon[0]);

      int icon_bar_w = icon_width + 2 * PADDING_X + bar_w;
      int icon_dy = 0;
      int bar_dy = 0;

      if (bar_h > icon_height)
        {
          icon_dy = (bar_h - icon_height + 1) / 2;
        }
      else
        {
          bar_dy = (icon_height - bar_h + 1) / 2;
        }
       
      for (int i = 0; i < BREAK_ID_SIZEOF; i++)
        {
          BreakId bid = priv->slot_to_break[i];

          if (bid != BREAK_ID_NONE)
            {
              WorkraveTimebar *bar = priv->slot_to_time_bar[bid];

              cairo_surface_t *bar_surface = cairo_surface_create_for_rectangle(surface, x+icon_width+PADDING_X, y + bar_dy, bar_w, bar_h);
              cairo_t *bar_cr = cairo_create(bar_surface);
              workrave_timebar_draw(bar, bar_cr, 0, 0, bar_w, bar_h);
              cairo_surface_destroy(bar_surface);
              cairo_destroy(bar_cr);
              
              gdk_cairo_set_source_pixbuf(cr, priv->break_to_icon[bid], x, y + icon_dy);
              cairo_fill(cr);
              cairo_paint(cr);
              
              x += icon_bar_w + 2 * PADDING_X;
            }
        }
    }
  
  /* for (int h = 0; h < BREAK_ID_SIZEOF; h++) */
  /*   { */
  /*     if ((! enabled) || (! break_visible[h])) */
  /*       { */
  /*         ctrl.HideWindow(break_to_icon[h]->get_handle()); */
  /*       } */
  /*   } */
  /* for (int j = enabled ? filled_slots : 0; j < BREAK_ID_SIZEOF; j++) */
  /*   { */
  /*     ctrl.HideWindow(slot_to_time_bar[j]->get_handle()); */
  /*   } */
}


WorkraveTimebar *
workrave_timerbox_get_time_bar(WorkraveTimerbox *self, BreakId timer)
{
  WorkraveTimerboxPrivate *priv = self->priv;

  WorkraveTimebar *ret = NULL;
  int slot = priv->break_to_slot[timer];
  if (slot >= 0)
    {
      ret = priv->slot_to_time_bar[slot];
    }
  return ret;
}


/* void */
/* TimerBox::set_enabled(bool ena) */
/* { */
/*   TRACE_ENTER_MSG("TimerBox::set_enabled", ena); */
/*   enabled = ena; */
/*   TRACE_EXIT(); */
/* } */

