#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkscreen.h>
#include <cairo.h>
#include "calculator.h"

#define SPEC_BUTTONS "ac unar plot res backspace close minimize"
#define G_FUNCTIONS "log( ln( sqrt( cos( sin( tan( acos( asin( atan( ( "
#define DIGITS "1 2 3 4 5 6 7 8 9 0 "
#define G_OPERATORS "/ * - + % ^ "

void init_plot();

static GtkWindow *window;
static GtkWindow *plot;
static GObject *draw;
static GtkApplication *app;
static GtkTextView *text_view;
static GtkTextBuffer *buffer;
static int buffer_len = 0, dot = 0, l_brckt = 0, r_brckt = 0;
static char last_val[8];
static gchar rpn[256];
static char allowed[96];

size_t strrspn(const char *str1, const char *str2) {
  const char *s = str1 + strlen(str1);
  for (; s != str1 && strchr(str2, *s); s--) {
  }
  return s - str1;
}

static void set_allowed(const gchar *val) {
  if (buffer_len == 0 && strstr("0", last_val)) {
    snprintf(allowed, sizeof(allowed), "%s%s", ". x 1 2 3 4 5 6 7 8 9 ",
             G_FUNCTIONS);
  } else if (buffer_len != 0 && strstr(".", last_val)) {
    snprintf(allowed, sizeof(allowed), "%s", DIGITS);
    dot = 1;
  } else if (buffer_len != 0 && strstr(DIGITS, last_val)) {
    snprintf(allowed, sizeof(allowed), "%s%s%s%s", G_OPERATORS, DIGITS,
             l_brckt - r_brckt > 0 ? ") " : "", !dot ? ". " : "");
  } else if (buffer_len != 0 && strstr(G_OPERATORS, last_val)) {
    snprintf(allowed, sizeof(allowed), "%s%s%s", "x ", DIGITS, G_FUNCTIONS);
    dot = 0;
  } else if (buffer_len != 0 && strstr("x", last_val)) {
    snprintf(allowed, sizeof(allowed), "%s%s", ") ", G_OPERATORS);
  } else if (buffer_len != 0 && strchr(val, '(')) {
    snprintf(allowed, sizeof(allowed), "%s%s%s", "x ", DIGITS, G_FUNCTIONS);
  } else if (buffer_len != 0 && strchr(val, ')')) {
    if (!strchr(last_val, '('))
      snprintf(allowed, sizeof(allowed), "%s%s", G_OPERATORS,
               l_brckt - r_brckt != 0 ? ") " : "");
  } else if (buffer_len != 0 && strchr(last_val, ')')) {
    snprintf(allowed, sizeof(allowed), "%s", G_OPERATORS);
  }
}

static void print_button_handler(GtkWidget *widget, gpointer data) {
  const gchar *val = (gchar *)data;
  const int len = strlen(val);
  if (buffer_len + len < 256) {
    set_allowed(val);
    if (strstr(allowed, val)) {
      if (buffer_len == 0 && (val - 60 > 0)) {
        if (strstr(val, ".")) {
          gtk_text_buffer_insert_at_cursor(buffer, val, len);
          buffer_len += len;
        } else {
          gtk_text_buffer_set_text(buffer, val, len);
        }
      } else {
        gtk_text_buffer_insert_at_cursor(buffer, val, len);
      }
      if (strchr(val, '('))
        l_brckt++;
      else if (strchr(val, ')'))
        r_brckt++;
      buffer_len += len;
      snprintf(last_val, sizeof(last_val), "%s", val);
    }
  }
}

void input_reset(void) {
  snprintf(last_val, sizeof(last_val), "%s", "0");
  gtk_text_buffer_set_text(buffer, last_val, 1);
  buffer_len = dot = l_brckt = r_brckt = 0;
}

gchar *get_buffer_text(void) {
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buffer, &start, &end);
  return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void input_result(gchar *text) {
  if (r_brckt != l_brckt && strchr(".+-/*^%", text[buffer_len])) {
    gtk_text_buffer_set_text(buffer, "Error", 5);
  } else if (!strstr(text, "Error")) {
    sprintf(text, "%.15g", calculate(text, NULL));
    gtk_text_buffer_set_text(buffer, text, strlen(text));
  } else {
    input_reset();
  }
}

void inverse_last_value() {
  gchar buf[256], tmp[256], output[512], *text = get_buffer_text();
  if (!strchr(last_val, '.')) {
    int unar = 0;
    size_t pos = strrspn(text, "1234567890.)");
    if (strchr(text + pos, '-') && strchr(text + pos - 1, '(')) {
      unar = 1;
      pos -= pos > 0 ? 1 : 0;
    }
    if (!unar)
      pos += pos == 0 ? 0 : 1;
    snprintf(buf, sizeof(buf), "%s", (text + pos));
    if (strlen((text + pos)) != 0) {
      if (unar == 0) {
        snprintf(last_val, sizeof(last_val), "%s", ")");
        snprintf(buf, sizeof(buf), "%s%s%s", "(-", (text + pos), last_val);
      } else {
        size_t len = strlen((text + pos + 2));
        snprintf(buf, len, "%s", (text + pos + 2));
        snprintf(last_val, sizeof(last_val), "%s", buf + len - 2);
      }
      snprintf(tmp, pos + 1, "%s", text);
      snprintf(output, sizeof(output), "%s%s", tmp, buf);
      gtk_text_buffer_set_text(buffer, output, strlen(output));
    }
  }
  free(text);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
  GdkRectangle da;
  gdouble dx = 0.05, dy = 0.05; /* Pixels between each point */
  double i = 0, clip_x1 = 0.0, clip_y1 = 0.0, clip_x2 = 0.0, clip_y2 = 0.0;

  /* Determine GtkDrawingArea dimensions */
  gdk_window_get_geometry(gtk_widget_get_window(widget), &da.x, &da.y,
                          &da.width, &da.height);

  // background
  cairo_set_source_rgb(cr, 0.898039, 0.898039, 0.898039);
  cairo_paint(cr);

  // centered
  cairo_translate(cr, da.width / 2, da.height / 2);
  cairo_scale(cr, 30, -30);

  /* Determine the data points to calculate (ie. those in the clipping zone */
  cairo_device_to_user_distance(cr, &dx, &dy);
  cairo_clip_extents(cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);
  cairo_set_line_width(cr, 0.035);

  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);

  // axis
  cairo_set_source_rgb(cr, 0.215686274509804, 0.058823529411765,
                       0.27843137254902);
  cairo_move_to(cr, clip_x1, 0.0);
  cairo_line_to(cr, clip_x2, 0.0);
  cairo_move_to(cr, 0.0, clip_y1);
  cairo_line_to(cr, 0.0, clip_y2);
  cairo_stroke(cr);

  /* Link each data point */
  char *log = strchr(rpn, 'l');
  char *ln = strchr(rpn, 'n');
  char *sqrt = strchr(rpn, 'q');
  int cliped = 0, once = 0;
  for (i = clip_x1; i < clip_x2; i += dx) {
    double y = calculate(rpn, &i);
    if (i == clip_x1) cairo_move_to(cr, i, y);

    if (log || ln || sqrt) {
      if (y == 0.0) {
        cairo_move_to(cr, 0.0, 0.0);
      } else if (i < 0) {
        cairo_move_to(cr, 0.0, clip_y1);
        continue;
      }
    }

    if (y < clip_y1)
      cliped = -1;
    else if (y > clip_y2)
      cliped = 1;

    if (cliped != 0 && once == 0) {
      once = 1;
      cairo_line_to(cr, i, y);
    }

    if (y > clip_y1 && y < clip_y2) {
      if (cliped == -1)
        cairo_move_to(cr, i, clip_y1);
      else if (cliped == 1)
        cairo_move_to(cr, i, clip_y2);
      cliped = once = 0;
      cairo_line_to(cr, i, y);
    }
  }

  /* Draw the curve */
  cairo_set_line_width(cr, 0.05);
  cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.6);
  cairo_stroke(cr);

  cairo_scale(cr, 1, -1);
  cairo_set_font_size(cr, 0.35);
  for (int i = clip_x1; i < clip_x2; i += 1) {
    cairo_set_source_rgba(cr, 0.215686274509804, 0.058823529411765,
                       0.27843137254902, 0.2);
    cairo_move_to(cr, i, clip_y1);
    cairo_line_to(cr, i, clip_y2);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.215686274509804, 0.058823529411765,
                       0.27843137254902);
    cairo_move_to(cr, i, 0.1);
    cairo_line_to(cr, i, -0.1);
    cairo_stroke(cr);
    if (i == 0) continue;
    cairo_move_to(cr, i - 0.25, i > 0 ? -0.25 : 0.55);
    char _n[4];
    sprintf(_n, "%d", i);
    cairo_show_text(cr, _n);
  }

  for (int i = clip_y1; i < clip_y2; i += 1) {
    cairo_set_source_rgba(cr, 0.215686274509804, 0.058823529411765,
                       0.27843137254902, 0.2);
    cairo_move_to(cr, clip_x1, i);
    cairo_line_to(cr, clip_x2, i);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.215686274509804, 0.058823529411765,
                       0.27843137254902);
    cairo_move_to(cr, 0.1, i);
    cairo_line_to(cr, -0.1, i);
    cairo_stroke(cr);
    if (i == 0) continue;
    cairo_move_to(cr, i > 0 ? 0.35 : -0.75, i + 0.25);
    char _n[4];
    sprintf(_n, "%d", -1 * i);
    cairo_show_text(cr, _n);
  }

  return FALSE;
}

gboolean window_handler(GtkWidget *widget, GdkEventButton *event,
                        GdkWindowEdge edge) {
  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                               event->button, event->x_root, event->y_root,
                               event->time);
  }
  return TRUE;
}

static void close_window(GtkWidget *button, gpointer data) {
  plot = NULL;
  gtk_window_close((GtkWindow *)data);
}

static void minimize_window(GtkWidget *button, gpointer data) {
  plot = NULL;
  gtk_window_iconify((GtkWindow *)data);
}

void init_plot(gchar *text) {
  if (plot == NULL) {
    strcpy(rpn, text);
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "assets/ui/plot.ui", NULL);
    plot = GTK_WINDOW(gtk_builder_get_object(builder, "plot_win"));
    draw = gtk_builder_get_object(builder, "draw_area");
    GObject *close = gtk_builder_get_object(builder, "close_plot");
    GObject *minimize = gtk_builder_get_object(builder, "minimize_plot");
    g_signal_connect(draw, "draw", G_CALLBACK(on_draw), text);
    g_signal_connect(GTK_WIDGET(draw), "button-press-event",
                     G_CALLBACK(window_handler), NULL);
    g_signal_connect(GTK_WIDGET(close), "clicked", G_CALLBACK(close_window),
                     plot);
    g_signal_connect(GTK_WIDGET(minimize), "clicked", G_CALLBACK(minimize_window),
                     plot);
    gtk_widget_show(GTK_WIDGET(plot));
    gtk_widget_set_opacity(GTK_WIDGET(plot), 0.97);
    g_object_unref(builder);
  }
}

void backspace(gchar *text) {
  if (buffer_len > 0) {
    do {
      text[buffer_len--] = '\0';
      if (text[buffer_len] == '(')
        l_brckt--;
      else if (text[buffer_len] == ')')
        r_brckt--;
      snprintf(last_val, sizeof(last_val), "%c", text[buffer_len]);
      set_allowed(last_val);
    } while (strchr("socialtnqrg", text[buffer_len - 1]));
    gtk_text_buffer_set_text(buffer, text, buffer_len);
    if (buffer_len <= 0) {
      input_reset();
      buffer_len = 0;
    }
  }
}

#define AC_OFFSET 0
#define UNAR_OFFSET 3
#define RESULT_OFFSET 13
#define BACKSPACE_OFFSET 17
#define CLOSE_OFFSET 27
#define MINIMIZE_OFFSET 33

static void special_button_handler(GtkWidget *widget, gpointer data) {
  const gchar *val = (gchar *)data;
  gchar *text = get_buffer_text();
  char *result = NULL;
  switch (strstr(SPEC_BUTTONS, val) - SPEC_BUTTONS) {
  case AC_OFFSET:
    input_reset();
    break;
  case UNAR_OFFSET:
    inverse_last_value();
    break;
  case BACKSPACE_OFFSET:
    backspace(text);
    break;
  case CLOSE_OFFSET:
    gtk_window_close(window);
    break;
  case MINIMIZE_OFFSET:
    gtk_window_iconify(window);
    break;
  case 8:
    result = (char *)malloc(1024);
    infix_to_postfix(text, result);
    init_plot(result);
    break;
  case RESULT_OFFSET:
    result = (char *)malloc(1024);
    infix_to_postfix(text, result);
    if (strchr(text, 'x'))
      init_plot(result);
    else
      input_result(result);
    break;
  }
  if (result != NULL)
    free(result);
  free(text);
}

static void init_buttons(GtkWidget *widget) {
  const gchar *label = gtk_button_get_label(GTK_BUTTON(widget)),
              *name = gtk_widget_get_name(widget);
  if (label != NULL)
    g_signal_connect(widget, "clicked", G_CALLBACK(print_button_handler),
                     (gpointer *)label);
  else
    g_signal_connect(widget, "clicked", G_CALLBACK(special_button_handler),
                     (gpointer *)name);
}

static void init_css(void) {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_style_context_add_provider_for_screen(
      gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_path(provider, "assets/css/style.css", NULL);
  g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
  init_css();
  snprintf(last_val, sizeof(last_val), "%s", "0");
  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "assets/ui/main.ui", NULL);
  window = GTK_WINDOW(gtk_builder_get_object(builder, "main"));
  GSList *objects = gtk_builder_get_objects(builder);
  for (GSList *lp = objects; lp != NULL; lp = lp->next) {
    if (GTK_IS_BUTTON(lp->data)) {
      init_buttons(lp->data);
    } else if (GTK_IS_TEXT_VIEW(lp->data)) {
      text_view = lp->data;
      buffer = gtk_text_view_get_buffer(text_view);
      g_signal_connect(lp->data, "button-press-event",
                       G_CALLBACK(window_handler), NULL);
    } else if (GTK_IS_WINDOW(lp->data)) {
      window = lp->data;
      gtk_window_set_application(GTK_WINDOW(lp->data), app);
      gtk_window_set_resizable(GTK_WINDOW(lp->data), FALSE);
      gtk_widget_show(GTK_WIDGET(lp->data));
      gtk_widget_set_opacity(GTK_WIDGET(lp->data), 0.97);
    }
  }
  g_object_unref(builder);
}

int main(int argc, char *argv[]) {
  app = gtk_application_new("org.gtk.calc", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
