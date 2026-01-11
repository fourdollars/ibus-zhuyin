/* -*- coding: utf-8; indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*- */
/**
 * Copyright (C) 2012 Shih-Yuan Lee (FourDollars) <fourdollars@gmail.com>
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

#include "engine.h"
#include "zhuyin.h"
#include "punctuation.h"

#ifndef G_UNICHAR_MAX_BYTES
#define G_UNICHAR_MAX_BYTES 6
#endif

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _IBusZhuyinEngine IBusZhuyinEngine;
typedef struct _IBusZhuyinEngineClass IBusZhuyinEngineClass;

struct _IBusZhuyinEngine {
    IBusEngine parent;

    /* members */
    GString *preedit;
    gint mode;
    gint page_max;
    gint page_size;
    gchar* display[4];
    gchar input[4];
    gboolean valid;
    gchar** candidate_member;
    gchar** punctuation_candidate;
    guint candidate_number;

    IBusLookupTable *table;
};

struct _IBusZhuyinEngineClass {
    IBusEngineClass parent;
};

static GtkWidget *punctuation_window = NULL;
static IBusEngine *engine_instance = NULL;

// Variables for punctuation window dragging
static gint punctuation_window_x = -1;
static gint punctuation_window_y = -1;
static gboolean punctuation_window_dragging = FALSE;
static gint punctuation_window_drag_start_x = 0;
static gint punctuation_window_drag_start_y = 0;

enum {
    IBUS_ZHUYIN_MODE_NORMAL,
    IBUS_ZHUYIN_MODE_CANDIDATE,
    IBUS_ZHUYIN_MODE_LEADING
};

// Global Declarations for Punctuation Window
static const gchar *global_physical_keys[4][14] = {
    {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "\\"},
    {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]"},
    {"a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'"},
    {"z", "x", "c", "v", "b", "n", "m", ",", ".", "/"}
};

static const gchar *global_punctuation_keys[4][14] = {
    {"€", "┌", "┬", "┐", "〝", "〞", "‘", "’", "“", "”", "『", "』", "「", "」"},
    {"├", "┼", "┤", "※", "〈", "〉", "《", "》", "【", "】", "〔", "〕"},
    {"└", "┴", "┘", "○", "●", "↑", "↓", "！", "：", "；", "、"},
    {"─", "│", "◎", "§", "←", "→", "。", "，", "．", "？"}
};

/* functions prototype */
static void ibus_zhuyin_engine_class_init (IBusZhuyinEngineClass *klass);
static void ibus_zhuyin_engine_init (IBusZhuyinEngine *engine);
static void ibus_zhuyin_engine_destroy (IBusZhuyinEngine *engine);
static gboolean ibus_zhuyin_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint                  keyval,
                                             guint                  keycode,
                                             guint                  modifiers);
static void ibus_zhuyin_engine_reset       (IBusEngine             *engine);
static void ibus_zhuyin_engine_enable      (IBusEngine             *engine);
static void ibus_zhuyin_engine_disable     (IBusEngine             *engine);
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_zhuyin_engine_page_up     (IBusEngine             *engine);
static void ibus_zhuyin_engine_page_down   (IBusEngine             *engine);
static void ibus_zhuyin_engine_candidate_clicked (IBusEngine             *engine,
                                             guint                   index,
                                             guint                   button,
                                             guint                   state);

static gboolean ibus_zhuyin_engine_commit_candidate (IBusZhuyinEngine *zhuyin, gint candidate);
static void ibus_zhuyin_engine_commit_string (IBusZhuyinEngine      *zhuyin,
                                              const gchar            *string);
static void ibus_zhuyin_engine_update_aux_text(IBusZhuyinEngine *zhuyin);
static void _update_lookup_table_and_aux_text(IBusZhuyinEngine *zhuyin);
static void ibus_zhuyin_engine_update      (IBusZhuyinEngine      *zhuyin);

static gboolean ibus_zhuyin_preedit_phase (IBusZhuyinEngine *zhuyin,
                                           guint             keyval,
                                           guint             keycode,
                                           guint             modifiers);
static gboolean ibus_zhuyin_candidate_phase (IBusZhuyinEngine *zhuyin,
                                             guint             keyval,
                                             guint             keycode,
                                             guint             modifiers);
static gboolean ibus_zhuyin_leading_phase (IBusZhuyinEngine *zhuyin,
                                           guint             keyval,
                                           guint             keycode,
                                           guint             modifiers);

G_DEFINE_TYPE (IBusZhuyinEngine, ibus_zhuyin_engine, IBUS_TYPE_ENGINE)

static void on_punctuation_button_clicked(GtkButton *button, gpointer user_data) {
    const gchar *symbol = (const gchar *)user_data;
    if (engine_instance && symbol) {
        ibus_engine_commit_text(engine_instance, ibus_text_new_from_string(symbol));
    }
    if (punctuation_window) {
        gtk_widget_hide(punctuation_window);
    }
}

static gboolean on_punctuation_window_focus_out(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    if (punctuation_window && gtk_widget_get_visible(punctuation_window)) {
        gtk_widget_hide(punctuation_window);
    }
    return TRUE;
}

static gboolean on_punctuation_window_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button == 1) { // Left mouse button
        punctuation_window_dragging = TRUE;
        punctuation_window_drag_start_x = event->x_root;
        punctuation_window_drag_start_y = event->y_root;
        gtk_window_get_position(GTK_WINDOW(widget), &punctuation_window_x, &punctuation_window_y);
    }
    return TRUE;
}

static gboolean on_punctuation_window_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    if (punctuation_window_dragging) {
        gint dx = event->x_root - punctuation_window_drag_start_x;
        gint dy = event->y_root - punctuation_window_drag_start_y;
        gtk_window_move(GTK_WINDOW(widget), punctuation_window_x + dx, punctuation_window_y + dy);
    }
    return TRUE;
}

static gboolean on_punctuation_window_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->button == 1) { // Left mouse button
        punctuation_window_dragging = FALSE;
        // Store the final position
        gtk_window_get_position(GTK_WINDOW(widget), &punctuation_window_x, &punctuation_window_y);
    }
    return TRUE;
}

static gboolean create_punctuation_window_idle(gpointer user_data) {
    IBusEngine *engine = (IBusEngine *)user_data;
    GtkWidget *grid;
    GtkWidget *button;
    
    punctuation_window = gtk_window_new(GTK_WINDOW_POPUP);
    g_object_add_weak_pointer(G_OBJECT(punctuation_window), (gpointer *)&punctuation_window);

    // Enable motion events
    gtk_widget_set_events(punctuation_window, gtk_widget_get_events(punctuation_window) | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

    // Connect event handlers
    g_signal_connect(G_OBJECT(punctuation_window), "button-press-event", G_CALLBACK(on_punctuation_window_button_press), NULL);
    g_signal_connect(G_OBJECT(punctuation_window), "motion-notify-event", G_CALLBACK(on_punctuation_window_motion_notify), NULL);
    g_signal_connect(G_OBJECT(punctuation_window), "button-release-event", G_CALLBACK(on_punctuation_window_button_release), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 0);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 0);
    gtk_container_add(GTK_CONTAINER(punctuation_window), grid);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 14 && global_physical_keys[i][j] != NULL; j++) {
            GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            
            GtkWidget *phys_label = gtk_label_new(NULL);
            gchar *phys_markup = g_strdup_printf("<span size='x-small' foreground='#888888'>%s</span>", global_physical_keys[i][j]);
            gtk_label_set_markup(GTK_LABEL(phys_label), phys_markup);
            g_free(phys_markup);
            gtk_label_set_xalign(GTK_LABEL(phys_label), 0.0);

            GtkWidget *punc_label = gtk_label_new(NULL);
            gchar *punc_markup = g_strdup_printf("<span size='xx-large' weight='bold'>%s</span>", global_punctuation_keys[i][j]);
            gtk_label_set_markup(GTK_LABEL(punc_label), punc_markup);
            g_free(punc_markup);
            gtk_label_set_xalign(GTK_LABEL(punc_label), 1.0);

            gtk_box_pack_start(GTK_BOX(vbox), phys_label, TRUE, TRUE, 0);
            gtk_box_pack_end(GTK_BOX(vbox), punc_label, TRUE, TRUE, 0);
            
            button = gtk_button_new();
            gtk_widget_set_hexpand(button, TRUE);
            gtk_widget_set_vexpand(button, TRUE);
            gtk_container_add(GTK_CONTAINER(button), vbox);
            
            gint j_offset = 0;
            if (i > 0) {
                j_offset = 1;
            }
            
            g_signal_connect(button, "clicked", G_CALLBACK(on_punctuation_button_clicked), (gpointer)global_punctuation_keys[i][j]);
            gtk_grid_attach(GTK_GRID(grid), button, j + j_offset, i, 1, 1);
        }
    }
    gtk_widget_show_all(grid);

    // After creation, show the window
    if (punctuation_window_x != -1 && punctuation_window_y != -1) {
        gtk_window_move(GTK_WINDOW(punctuation_window), punctuation_window_x, punctuation_window_y);
    } else {
        GdkScreen *screen = gdk_screen_get_default();
        if (screen) {
            GdkDisplay *display = gdk_display_get_default();
            GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
            GdkRectangle geometry;
            gdk_monitor_get_geometry(monitor, &geometry);
            gint screen_width = geometry.width;
            gint screen_height = geometry.height;

            GtkRequisition requisition;
            gtk_widget_get_preferred_size(punctuation_window, &requisition, NULL);

            gint window_width = requisition.width;
            gint window_height = requisition.height;

            gint x = screen_width - window_width - 10; // 10 pixels margin from right
            gint y = screen_height - window_height - 10; // 10 pixels margin from bottom

            gtk_window_move(GTK_WINDOW(punctuation_window), x, y);
        } else {
            // Fallback if screen info is not available
            gtk_window_set_position(GTK_WINDOW(punctuation_window), GTK_WIN_POS_CENTER);
        }
    }
    gtk_widget_show(punctuation_window);
    
    return G_SOURCE_REMOVE;
}

static gboolean show_punctuation_window_idle(gpointer user_data) {
    if (punctuation_window) {
        if (punctuation_window_x != -1 && punctuation_window_y != -1) {
            gtk_window_move(GTK_WINDOW(punctuation_window), punctuation_window_x, punctuation_window_y);
        } else {
            GdkScreen *screen = gdk_screen_get_default();
            if (screen) {
            GdkDisplay *display = gdk_display_get_default();
            GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
            GdkRectangle geometry;
            gdk_monitor_get_geometry(monitor, &geometry);
            gint screen_width = geometry.width;
            gint screen_height = geometry.height;

                GtkRequisition requisition;
                gtk_widget_get_preferred_size(punctuation_window, &requisition, NULL);

                gint window_width = requisition.width;
                gint window_height = requisition.height;

                gint x = screen_width - window_width - 10; // 10 pixels margin from right
                gint y = screen_height - window_height - 10; // 10 pixels margin from bottom

                gtk_window_move(GTK_WINDOW(punctuation_window), x, y);
            } else {
                // Fallback if screen info is not available
                gtk_window_set_position(GTK_WINDOW(punctuation_window), GTK_WIN_POS_CENTER);
            }
        }
        gtk_widget_show(punctuation_window);
    }
    return G_SOURCE_REMOVE;
}

static gboolean hide_punctuation_window_idle(gpointer user_data) {
    if (punctuation_window) {
        gtk_widget_hide(punctuation_window);
    }
    return G_SOURCE_REMOVE;
}


static void
ibus_zhuyin_engine_page_down (IBusEngine *engine)
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *) engine;

    if (zhuyin->mode != IBUS_ZHUYIN_MODE_CANDIDATE)
        return;

    ibus_lookup_table_page_down(zhuyin->table);
    _update_lookup_table_and_aux_text (zhuyin);
}

static void
ibus_zhuyin_engine_page_up (IBusEngine *engine)
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *) engine;

    if (zhuyin->mode != IBUS_ZHUYIN_MODE_CANDIDATE)
        return;

    ibus_lookup_table_page_up(zhuyin->table);
    _update_lookup_table_and_aux_text (zhuyin);
}

static void
ibus_zhuyin_engine_update_aux_text(IBusZhuyinEngine *zhuyin)
{
    if (zhuyin->candidate_number > zhuyin->page_size) {
        gint pos = ibus_lookup_table_get_cursor_pos(zhuyin->table);
        gint page = pos / zhuyin->page_size;
        gchar *page_str = g_strdup_printf("%d / %d", page + 1, zhuyin->page_max + 1);
        IBusText *text = ibus_text_new_from_string(page_str);
        ibus_engine_update_auxiliary_text((IBusEngine *)zhuyin, text, TRUE);
        g_free(page_str);
    } else {
        ibus_engine_update_auxiliary_text((IBusEngine *)zhuyin, ibus_text_new_from_string(""), FALSE);
    }
}
static void
_update_lookup_table_and_aux_text(IBusZhuyinEngine *zhuyin)
{
    ibus_engine_update_lookup_table((IBusEngine *)zhuyin, zhuyin->table, TRUE);
    ibus_zhuyin_engine_update_aux_text(zhuyin);
}

static void
ibus_zhuyin_engine_candidate_clicked (IBusEngine *engine,
                                      guint       index,
                                      guint       button,
                                      guint       state)
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;

    /* We only care about left click */
    if (button != 1)
        return;

    if (index >= zhuyin->candidate_number)
        return;

    ibus_zhuyin_engine_commit_candidate (zhuyin, index);
}

static void
ibus_zhuyin_engine_class_init (IBusZhuyinEngineClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_zhuyin_engine_destroy;

    engine_class->candidate_clicked   = ibus_zhuyin_engine_candidate_clicked;
    engine_class->disable             = ibus_zhuyin_engine_disable;
    engine_class->enable              = ibus_zhuyin_engine_enable;
    engine_class->page_down           = ibus_zhuyin_engine_page_down;
    engine_class->page_up             = ibus_zhuyin_engine_page_up;
    engine_class->process_key_event   = ibus_zhuyin_engine_process_key_event;
    engine_class->reset               = ibus_zhuyin_engine_reset;
    engine_class->set_cursor_location = ibus_engine_set_cursor_location;
}

static void
ibus_zhuyin_engine_init (IBusZhuyinEngine *zhuyin)
{
    engine_instance = (IBusEngine *)zhuyin;
    zhuyin->preedit = g_string_new ("");
    zhuyin->mode = IBUS_ZHUYIN_MODE_NORMAL;
    zhuyin->page_size = 9;
    zhuyin->punctuation_candidate = NULL;

    zhuyin->table = ibus_lookup_table_new (zhuyin->page_size, 0, TRUE, TRUE);
    ibus_lookup_table_set_orientation(zhuyin->table, IBUS_ORIENTATION_HORIZONTAL);
    g_object_ref_sink (zhuyin->table);
    zhuyin_init();
}

static void
ibus_zhuyin_engine_destroy (IBusZhuyinEngine *zhuyin)
{
    if (zhuyin->preedit) {
        g_string_free (zhuyin->preedit, TRUE);
        zhuyin->preedit = NULL;
    }

    if (zhuyin->table) {
        g_object_unref (zhuyin->table);
        zhuyin->table = NULL;
    }

    if (zhuyin->punctuation_candidate) {
        g_strfreev (zhuyin->punctuation_candidate);
        zhuyin->punctuation_candidate = NULL;
    }
    
    if (punctuation_window) {
        gtk_widget_destroy(punctuation_window);
        punctuation_window = NULL;
    }
    engine_instance = NULL;

    ((IBusObjectClass *) ibus_zhuyin_engine_parent_class)->destroy ((IBusObject *)zhuyin);
}

static void
ibus_zhuyin_engine_update_lookup_table (IBusZhuyinEngine *zhuyin)
{
    gchar ** sugs;
    gsize n_sug, i;
    gboolean retval;

    ibus_lookup_table_clear (zhuyin->table);
    
    sugs = zhuyin->candidate_member;
    n_sug = zhuyin->candidate_number;

    if (sugs == NULL) {
        ibus_engine_hide_lookup_table ((IBusEngine *) zhuyin);
        return;
    }

    for (i = 0; i < n_sug; i++) {
        ibus_lookup_table_append_candidate (zhuyin->table, ibus_text_new_from_string (sugs[i]));
    }

    _update_lookup_table_and_aux_text (zhuyin);
}

static void
ibus_zhuyin_engine_update_preedit (IBusZhuyinEngine *zhuyin)
{
    IBusText *text;
    gint retval;

    text = ibus_text_new_from_string (zhuyin->preedit->str);
    text->attrs = ibus_attr_list_new ();
    
    ibus_attr_list_append (text->attrs,
                           ibus_attr_underline_new (IBUS_ATTR_UNDERLINE_SINGLE, 0, zhuyin->preedit->len));

    ibus_engine_update_preedit_text ((IBusEngine *)zhuyin,
                                     text,
                                     ibus_lookup_table_get_cursor_pos(zhuyin->table),
                                     TRUE);

}

/* commit preedit to client and update preedit */
static gboolean
ibus_zhuyin_engine_commit_preedit (IBusZhuyinEngine *zhuyin)
{
    if (zhuyin->preedit->len == 0)
        return FALSE;
    
    ibus_zhuyin_engine_commit_string (zhuyin, zhuyin->preedit->str);
    ibus_zhuyin_engine_reset((IBusEngine *) zhuyin);

    return TRUE;
}

/* commit candidate to client and update preedit */
static gboolean
ibus_zhuyin_engine_commit_candidate (IBusZhuyinEngine *zhuyin, gint candidate)
{
    IBusText *ib_text = ibus_lookup_table_get_candidate (zhuyin->table, candidate);

    if (ib_text == NULL)
        return FALSE;

    ibus_zhuyin_engine_commit_string (zhuyin, ib_text->text);
    ibus_zhuyin_engine_reset((IBusEngine *) zhuyin);

    return TRUE;
}

static void
ibus_zhuyin_engine_commit_string (IBusZhuyinEngine *zhuyin,
                                   const gchar       *string)
{
    IBusText *text;
    text = ibus_text_new_from_string (string);
    ibus_engine_commit_text ((IBusEngine *)zhuyin, text);
}

static void
ibus_zhuyin_engine_update (IBusZhuyinEngine *zhuyin)
{
    ibus_zhuyin_engine_update_preedit (zhuyin);
    ibus_engine_hide_lookup_table ((IBusEngine *)zhuyin);
}

static void
ibus_zhuyin_engine_reset (IBusEngine *engine) 
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *) engine;
    gsize i = 0;

    for (i = 0; i < 4; i++) {
        zhuyin->input[i] = 0;
        zhuyin->display[i] = NULL;
    }

    g_string_assign (zhuyin->preedit, "");
    zhuyin->mode = IBUS_ZHUYIN_MODE_NORMAL;
    zhuyin->valid = FALSE;
    zhuyin->candidate_number = 0;
    if (punctuation_window && gtk_widget_get_visible(punctuation_window)) {
        g_idle_add(hide_punctuation_window_idle, NULL);
    }

    ibus_zhuyin_engine_update (zhuyin);
    ibus_zhuyin_engine_update_aux_text(zhuyin);
}

static void
ibus_zhuyin_engine_redraw (IBusZhuyinEngine *zhuyin)
{
    gsize i = 0;
    g_string_assign (zhuyin->preedit, "");

    for (i = 0; i < 4; i++) {
        if (zhuyin->display[i] != NULL && zhuyin->input[i] > 0) {
            g_string_insert (zhuyin->preedit,
                    zhuyin->preedit->len,
                    zhuyin->display[i] );
        }
    }

    ibus_zhuyin_engine_update (zhuyin);
}

static gboolean
ibus_zhuyin_punctuation_phase (IBusZhuyinEngine *zhuyin,
                               guint             keyval,
                               guint             keycode,
                               guint             modifiers)
{
    gchar* punctuation = NULL;

    switch (keyval) {
        case '`':
            punctuation = "‘";
            break;
        case '~':
            punctuation = "～";
            break;
        case '!':
            punctuation = "！";
            break;
        case '@':
            punctuation = "＠";
            break;
        case '#':
            punctuation = "＃";
            break;
        case '$':
            punctuation = "＄";
            break;
        case '%':
            punctuation = "％";
            break;
        case '^':
            punctuation = "︿";
            break;
        case '&':
            punctuation = "＆";
            break;
        case '*':
            punctuation = "＊";
            break;
        case '(':
            punctuation = "（";
            break;
        case ')':
            punctuation = "）";
            break;
        case '_':
            punctuation = "－ ＿ ￣";
            break;
        case '+':
            punctuation = "＋ ＝";
            break;
        case '[':
            punctuation = "「";
            break;
        case '{':
            punctuation = "『 〈 《 ［ ｛ 【 〖 〔 〘 〚";
            break;
        case ']':
            punctuation = "」";
            break;
        case '}':
            punctuation = "』 〉 》 ］ ｝ 】 〗 〕 〙 〛";
            break;
        case '\\':
            punctuation = "＼ ／";
            break;
        case '|':
            punctuation = "｜";
            break;
        case ':':
            punctuation = "： ；";
            break;
        case '\'':
            punctuation = "’";
            break;
        case '"':
            punctuation = "、 “ ” ‘ ’";
            break;
        case '<':
            punctuation = "，";
            break;
        case '>':
            punctuation = "。";
            break;
        case '?':
            punctuation = "？";
            break;
    }

    if (zhuyin->punctuation_candidate != NULL) {
        g_strfreev(zhuyin->punctuation_candidate);
        zhuyin->punctuation_candidate = NULL;
    }

    if (punctuation == NULL) {
        return FALSE;
    }

    zhuyin->punctuation_candidate = g_strsplit (punctuation, " ", 0);
    zhuyin->candidate_number = g_strv_length (zhuyin->punctuation_candidate);
    if (zhuyin->candidate_number > 1) {
        zhuyin->candidate_member = zhuyin->punctuation_candidate;
        zhuyin->display[0] = zhuyin->punctuation_candidate[0];
        zhuyin->mode = IBUS_ZHUYIN_MODE_CANDIDATE;
        ibus_zhuyin_engine_redraw (zhuyin);
        ibus_zhuyin_engine_update_lookup_table (zhuyin);
        return TRUE;
    }

    if (zhuyin->punctuation_candidate) {
        g_strfreev(zhuyin->punctuation_candidate);
        zhuyin->punctuation_candidate = NULL;
    }

    /* commit the single punctuation */
    zhuyin->candidate_number = 0;
    ibus_zhuyin_engine_commit_string (zhuyin, punctuation);
    return TRUE;
}

static gboolean
ibus_zhuyin_preedit_phase (IBusZhuyinEngine *zhuyin,
                           guint             keyval,
                           guint             keycode,
                           guint             modifiers)
{
    gchar* phonetic = NULL;
    gint   type = 0;

    switch (keyval) {
        case '1':
            phonetic = "ㄅ";
            type = 1;
            break;
        case 'q':
            phonetic = "ㄆ";
            type = 1;
            break;
        case 'a':
            phonetic = "ㄇ";
            type = 1;
            break;
        case 'z':
            phonetic = "ㄈ";
            type = 1;
            break;
        case '2':
            phonetic = "ㄉ";
            type = 1;
            break;
        case 'w':
            phonetic = "ㄊ";
            type = 1;
            break;
        case 's':
            phonetic = "ㄋ";
            type = 1;
            break;
        case 'x':
            phonetic = "ㄌ";
            type = 1;
            break;
        case 'e':
            phonetic = "ㄍ";
            type = 1;
            break;
        case 'd':
            phonetic = "ㄎ";
            type = 1;
            break;
        case 'c':
            phonetic = "ㄏ";
            type = 1;
            break;
        case 'r':
            phonetic = "ㄐ";
            type = 1;
            break;
        case 'f':
            phonetic = "ㄑ";
            type = 1;
            break;
        case 'v':
            phonetic = "ㄒ";
            type = 1;
            break;
        case '5':
            phonetic = "ㄓ";
            type = 1;
            break;
        case 't':
            phonetic = "ㄔ";
            type = 1;
            break;
        case 'g':
            phonetic = "ㄕ";
            type = 1;
            break;
        case 'b':
            phonetic = "ㄖ";
            type = 1;
            break;
        case 'y':
            phonetic = "ㄗ";
            type = 1;
            break;
        case 'h':
            phonetic = "ㄘ";
            type = 1;
            break;
        case 'n':
            phonetic = "ㄙ";
            type = 1;
            break;
        case 'u':
            phonetic = "ㄧ";
            type = 2;
            break;
        case 'j':
            phonetic = "ㄨ";
            type = 2;
            break;
        case 'm':
            phonetic = "ㄩ";
            type = 2;
            break;
        case '8':
            phonetic = "ㄚ";
            type = 3;
            break;
        case 'i':
            phonetic = "ㄛ";
            type = 3;
            break;
        case 'k':
            phonetic = "ㄜ";
            type = 3;
            break;
        case ',':
            phonetic = "ㄝ";
            type = 3;
            break;
        case '9':
            phonetic = "ㄞ";
            type = 3;
            break;
        case 'o':
            phonetic = "ㄟ";
            type = 3;
            break;
        case 'l':
            phonetic = "ㄠ";
            type = 3;
            break;
        case '.':
            phonetic = "ㄡ";
            type = 3;
            break;
        case '0':
            phonetic = "ㄢ";
            type = 3;
            break;
        case 'p':
            phonetic = "ㄣ";
            type = 3;
            break;
        case ';':
            phonetic = "ㄤ";
            type = 3;
            break;
        case '/':
            phonetic = "ㄥ";
            type = 3;
            break;
        case '-':
            phonetic = "ㄦ";
            type = 3;
            break;
        case '3':
            phonetic = "ˇ";
            type = 4;
            break;
        case '4':
            phonetic = "ˋ";
            type = 4;
            break;
        case '6':
            phonetic = "ˊ";
            type = 4;
            break;
        case '7':
            phonetic = "˙";
            type = 4;
            break;
        case IBUS_space:
            if (zhuyin->valid == TRUE) {
                zhuyin->mode = IBUS_ZHUYIN_MODE_CANDIDATE;
                if (zhuyin->candidate_number == 1) {
                    gint i = 3;
                    while (i > 0) {
                        if (zhuyin->input[i] > 0) {
                            zhuyin->input[i] = 0;
                            zhuyin->display[i] = NULL;
                        }
                        i--;
                    }
                    if (zhuyin->input[0] == 0)
                        zhuyin->input[0] = 1;
                    zhuyin->display[0] = zhuyin->candidate_member[0];
                    ibus_zhuyin_engine_redraw (zhuyin);
                    zhuyin->display[0] = NULL;
                    return ibus_zhuyin_engine_commit_preedit (zhuyin);
                } else {
                    ibus_zhuyin_engine_update_lookup_table (zhuyin);
                    return TRUE;
                }
            }
        case IBUS_Escape:
        case IBUS_Delete:
            if (zhuyin->preedit->len == 0)
                return FALSE;

            ibus_zhuyin_engine_reset ((IBusEngine *)zhuyin);
            return TRUE;
        case IBUS_BackSpace:
            if (zhuyin->preedit->len == 0) {
                return FALSE;
            } else {
                gint i = 3;
                while (i >= 0) {
                    if (zhuyin->input[i] > 0) {
                        zhuyin->input[i] = 0;
                        zhuyin->display[i] = NULL;
                        break;
                    }
                    i--;
                }
                ibus_zhuyin_engine_redraw (zhuyin);
                return TRUE;
            }
        case IBUS_Return:
            return ibus_zhuyin_engine_commit_preedit (zhuyin);
        default:
            return FALSE;
    }

    if (type > 0) {
        guint i = 0;
        guint stanza = 0;
        zhuyin->display[type - 1] = phonetic;
        zhuyin->input[type - 1] = keyval;

        ibus_zhuyin_engine_redraw (zhuyin);

        for (i = 0; i < 4; i++) {
            switch (zhuyin->input[i]) {
                case '1':
                    stanza = stanza | 1;
                    break;
                case 'q':
                    stanza = stanza | 2;
                    break;
                case 'a':
                    stanza = stanza | 3;
                    break;
                case 'z':
                    stanza = stanza | 4;
                    break;
                case '2':
                    stanza = stanza | 5;
                    break;
                case 'w':
                    stanza = stanza | 6;
                    break;
                case 's':
                    stanza = stanza | 7;
                    break;
                case 'x':
                    stanza = stanza | 8;
                    break;
                case 'e':
                    stanza = stanza | 9;
                    break;
                case 'd':
                    stanza = stanza | 10;
                    break;
                case 'c':
                    stanza = stanza | 11;
                    break;
                case 'r':
                    stanza = stanza | 12;
                    break;
                case 'f':
                    stanza = stanza | 13;
                    break;
                case 'v':
                    stanza = stanza | 14;
                    break;
                case '5':
                    stanza = stanza | 15;
                    break;
                case 't':
                    stanza = stanza | 16;
                    break;
                case 'g':
                    stanza = stanza | 17;
                    break;
                case 'b':
                    stanza = stanza | 18;
                    break;
                case 'y':
                    stanza = stanza | 19;
                    break;
                case 'h':
                    stanza = stanza | 20;
                    break;
                case 'n':
                    stanza = stanza | 21;
                    break;
                case 'u':
                    stanza = stanza | (1 << 8);
                    break;
                case 'j':
                    stanza = stanza | (2 << 8);
                    break;
                case 'm':
                    stanza = stanza | (3 << 8);
                    break;
                case '8':
                    stanza = stanza | (1 << 16);
                    break;
                case 'i':
                    stanza = stanza | (2 << 16);
                    break;
                case 'k':
                    stanza = stanza | (3 << 16);
                    break;
                case ',':
                    stanza = stanza | (4 << 16);
                    break;
                case '9':
                    stanza = stanza | (5 << 16);
                    break;
                case 'o':
                    stanza = stanza | (6 << 16);
                    break;
                case 'l':
                    stanza = stanza | (7 << 16);
                    break;
                case '.':
                    stanza = stanza | (8 << 16);
                    break;
                case '0':
                    stanza = stanza | (9 << 16);
                    break;
                case 'p':
                    stanza = stanza | (10 << 16);
                    break;
                case ';':
                    stanza = stanza | (11 << 16);
                    break;
                case '/':
                    stanza = stanza | (12 << 16);
                    break;
                case '-':
                    stanza = stanza | (13 << 16);
                    break;
                case '6':
                    stanza = stanza | (1 << 24);
                    break;
                case '3':
                    stanza = stanza | (2 << 24);
                    break;
                case '4':
                    stanza = stanza | (3 << 24);
                    break;
                case '7':
                    stanza = stanza | (4 << 24);
                    break;
            }
        }

        zhuyin->candidate_member = zhuyin_candidate(stanza, &i);
        zhuyin->candidate_number = i;
        if (zhuyin->candidate_number % zhuyin->page_size)
            zhuyin->page_max = zhuyin->candidate_number / zhuyin->page_size;
        else
            zhuyin->page_max = zhuyin->candidate_number / zhuyin->page_size - 1;

        /* directly commit when only one candidate. */
        if (type == 4 && zhuyin->candidate_number == 1) {
            ibus_zhuyin_engine_commit_string (zhuyin, zhuyin->candidate_member[0]);
            ibus_zhuyin_engine_reset ((IBusEngine *)zhuyin);
            return TRUE;
        }

        if (zhuyin->candidate_member == NULL) {
            zhuyin->valid = FALSE;
        } else {
            zhuyin->valid = TRUE;
            if (zhuyin->display[3] != NULL) {
                zhuyin->mode = IBUS_ZHUYIN_MODE_CANDIDATE;
                ibus_zhuyin_engine_update_lookup_table (zhuyin);
            }
        }
    }

    return TRUE;
}

static gboolean
ibus_zhuyin_candidate_phase (IBusZhuyinEngine *zhuyin,
                             guint             keyval,
                             guint             keycode,
                             guint             modifiers)
{
    /* Choose candidate character */
    gint candidate = -1;
    gint index = ibus_lookup_table_get_cursor_pos(zhuyin->table);
    gint page = index / zhuyin->page_size;
    switch (keyval) {
        case IBUS_1:
        case IBUS_a:
            candidate = page * zhuyin->page_size;
            break;
        case IBUS_2:
        case IBUS_s:
            candidate = page * zhuyin->page_size + 1;
            break;
        case IBUS_3:
        case IBUS_d:
            candidate = page * zhuyin->page_size + 2;
            break;
        case IBUS_4:
        case IBUS_f:
            candidate = page * zhuyin->page_size + 3;
            break;
        case IBUS_5:
        case IBUS_g:
            candidate = page * zhuyin->page_size + 4;
            break;
        case IBUS_6:
        case IBUS_h:
            candidate = page * zhuyin->page_size + 5;
            break;
        case IBUS_7:
        case IBUS_j:
            candidate = page * zhuyin->page_size + 6;
            break;
        case IBUS_8:
        case IBUS_k:
            candidate = page * zhuyin->page_size + 7;
            break;
        case IBUS_9:
        case IBUS_l:
            candidate = page * zhuyin->page_size + 8;
            break;
        default:break;
    }

    if (candidate != -1 && candidate < zhuyin->candidate_number) {
        return ibus_zhuyin_engine_commit_candidate (zhuyin, candidate);
    }

    modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

    /* Functional shortcuts */
    if (modifiers == IBUS_CONTROL_MASK && keyval == IBUS_s) {
        _update_lookup_table_and_aux_text (zhuyin);
        return TRUE;
    }

    if (modifiers == IBUS_CONTROL_MASK && keyval == IBUS_a) {
        ibus_engine_show_preedit_text ((IBusEngine *) zhuyin);
        return TRUE;
    }

    if (modifiers == IBUS_CONTROL_MASK && keyval == IBUS_b) {
        ibus_engine_hide_preedit_text ((IBusEngine *) zhuyin);
        return TRUE;
    }

    if (modifiers != 0) {
        if (zhuyin->preedit->len == 0)
            return FALSE;
        else
            return TRUE;
    }

#if IBUS_CHECK_VERSION(1, 5, 0)
    IBusOrientation orientation = ibus_lookup_table_get_orientation(zhuyin->table);
#else
    IBusOrientation orientation = IBUS_ORIENTATION_VERTICAL;
#endif

    switch (keyval) {
        case IBUS_Return: {
            gint index = ibus_lookup_table_get_cursor_pos(zhuyin->table);
            if (index < zhuyin->candidate_number) {
                return ibus_zhuyin_engine_commit_candidate (zhuyin, index);
            }
            /* if no candidate is selected, commit preedit. */
            return ibus_zhuyin_engine_commit_preedit (zhuyin);
        }

        case IBUS_Escape:
        case IBUS_Delete:
            if (zhuyin->preedit->len == 0)
                return FALSE;

            ibus_zhuyin_engine_reset ((IBusEngine *)zhuyin);
            return TRUE;

        case IBUS_Up:
            if (orientation == IBUS_ORIENTATION_VERTICAL) {
                ibus_lookup_table_cursor_up(zhuyin->table);
            } else { // HORIZONTAL
                ibus_lookup_table_page_up(zhuyin->table);
            }
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_Down:
            if (orientation == IBUS_ORIENTATION_VERTICAL) {
                ibus_lookup_table_cursor_down(zhuyin->table);
            } else { // HORIZONTAL
                ibus_lookup_table_page_down(zhuyin->table);
            }
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_Left:
            if (orientation == IBUS_ORIENTATION_HORIZONTAL) {
                ibus_lookup_table_cursor_up(zhuyin->table);
            } else { // VERTICAL
                ibus_lookup_table_page_up(zhuyin->table);
            }
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_Right:
            if (orientation == IBUS_ORIENTATION_HORIZONTAL) {
                ibus_lookup_table_cursor_down(zhuyin->table);
            } else { // VERTICAL
                ibus_lookup_table_page_down(zhuyin->table);
            }
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_Page_Up:
            ibus_lookup_table_page_up(zhuyin->table);
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_Page_Down:
            ibus_lookup_table_page_down(zhuyin->table);
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_Home:
            ibus_lookup_table_set_cursor_pos(zhuyin->table, 0);
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_End: {
            ibus_lookup_table_set_cursor_pos(zhuyin->table, zhuyin->candidate_number - 1);
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;
        }
        case IBUS_space:
            ibus_lookup_table_page_down(zhuyin->table);
            _update_lookup_table_and_aux_text (zhuyin);
            return TRUE;

        case IBUS_BackSpace:
            zhuyin->mode = IBUS_ZHUYIN_MODE_NORMAL;
            if (zhuyin->preedit->len == 0) {
                return FALSE;
            } else {
                gint i = 3;
                while (i >= 0) {
                    if (zhuyin->input[i] > 0) {
                        zhuyin->input[i] = 0;
                        zhuyin->display[i] = NULL;
                        break;
                    }
                    i--;
                }
                ibus_zhuyin_engine_redraw (zhuyin);
                return TRUE;
            }
    }

    return TRUE;
}

static gboolean
ibus_zhuyin_leading_phase (IBusZhuyinEngine *zhuyin,
                           guint             keyval,
                           guint             keycode,
                           guint             modifiers)
{
    gchar* punctuation = NULL;
    int i;
    for (i = 0; leading_key_punctuation[i].candidates != NULL; i++) {
        if (leading_key_punctuation[i].keyval == keyval) {
            punctuation = (gchar*)leading_key_punctuation[i].candidates;
            break;
        }
    }

    if (punctuation == NULL) {
        zhuyin->mode = IBUS_ZHUYIN_MODE_NORMAL;
        return ibus_zhuyin_preedit_phase(zhuyin, keyval, keycode, modifiers);
    }
    if (zhuyin->punctuation_candidate != NULL) {
        g_strfreev(zhuyin->punctuation_candidate);
        zhuyin->punctuation_candidate = NULL;
    }
    zhuyin->punctuation_candidate = g_strsplit (punctuation, " ", 0);
    zhuyin->candidate_number = g_strv_length (zhuyin->punctuation_candidate);
    if (zhuyin->candidate_number % zhuyin->page_size)
        zhuyin->page_max = zhuyin->candidate_number / zhuyin->page_size;
    else
        zhuyin->page_max = zhuyin->candidate_number / zhuyin->page_size - 1;
    zhuyin->candidate_member = zhuyin->punctuation_candidate;
    zhuyin->display[0] = zhuyin->punctuation_candidate[0];
    zhuyin->mode = IBUS_ZHUYIN_MODE_CANDIDATE;
    ibus_zhuyin_engine_redraw (zhuyin);
    ibus_zhuyin_engine_update_lookup_table (zhuyin);
    return TRUE;
}

static gboolean
ibus_zhuyin_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;

    /* Ignore key release event */
    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    if ((modifiers & (IBUS_CONTROL_MASK | IBUS_MOD1_MASK)) == (IBUS_CONTROL_MASK | IBUS_MOD1_MASK) && keyval == IBUS_comma) {
        if (!punctuation_window) {
            g_idle_add(create_punctuation_window_idle, engine);
        } else {
            if (gtk_widget_get_visible(punctuation_window)) {
                g_idle_add(hide_punctuation_window_idle, NULL);
            } else {
                g_idle_add(show_punctuation_window_idle, NULL);
            }
        }
        return TRUE;
    }

    if (punctuation_window && gtk_widget_get_visible(punctuation_window)) {
        if (keyval == IBUS_Escape) {
            g_idle_add(hide_punctuation_window_idle, NULL);
            return TRUE;
        }

        // Handle key presses in punctuation window
        gunichar unicode_char = gdk_keyval_to_unicode(keyval);
        if (unicode_char != 0) { // If it's a printable character
            gchar char_str[G_UNICHAR_MAX_BYTES + 1];
            gint len = g_unichar_to_utf8(unicode_char, char_str);
            char_str[len] = '\0'; // Null-terminate the string

            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 14; j++) {
                    if (global_physical_keys[i][j] != NULL &&
                        strcmp(char_str, global_physical_keys[i][j]) == 0) {
                        ibus_zhuyin_engine_commit_string(zhuyin, global_punctuation_keys[i][j]);
                        g_idle_add(hide_punctuation_window_idle, NULL);
                        return TRUE;
                    }
                }
            }

            // If no specific mapping is found, commit the character itself
            ibus_zhuyin_engine_commit_string(zhuyin, char_str);
            g_idle_add(hide_punctuation_window_idle, NULL);
            return TRUE;
        }
        // If the key was not a printable character or Escape,
        // it means we don't want to pass it to other phases when punctuation window is open.
        // So we return TRUE to consume the event.
        return TRUE;
    }

    switch (zhuyin->mode) {
        case IBUS_ZHUYIN_MODE_NORMAL:
            if (zhuyin->preedit->len == 0) {
                // Ctrl + `
                if ((modifiers & IBUS_CONTROL_MASK) == IBUS_CONTROL_MASK && keyval == IBUS_grave) {
                    zhuyin->mode = IBUS_ZHUYIN_MODE_LEADING;
                    return TRUE;
                }
                if (ibus_zhuyin_punctuation_phase(zhuyin, keyval, keycode, modifiers)) {
                    return TRUE;
                }
            } else {
                // If preedit is not empty, check if it's a punctuation key.
                // If it is, commit preedit and re-process.
                if (keyval == '{' || keyval == '}' || keyval == '\\' ||
                    keyval == '_' || keyval == '+' || keyval == ':' || keyval == '"') {
                    ibus_zhuyin_engine_commit_preedit (zhuyin); // This will also reset preedit
                    // Now, re-call ibus_zhuyin_punctuation_phase with empty preedit
                    return ibus_zhuyin_punctuation_phase(zhuyin, keyval, keycode, modifiers);
                }
            }
            return ibus_zhuyin_preedit_phase(zhuyin, keyval, keycode, modifiers);
        case IBUS_ZHUYIN_MODE_CANDIDATE:
            return ibus_zhuyin_candidate_phase(zhuyin, keyval, keycode, modifiers);
        case IBUS_ZHUYIN_MODE_LEADING:
            return ibus_zhuyin_leading_phase(zhuyin, keyval, keycode, modifiers);
        default:
            break;
    }
    return TRUE;
}

static void ibus_zhuyin_engine_enable (IBusEngine *engine)
{
    ibus_zhuyin_engine_reset (engine);
    engine_instance = engine;
}

static void ibus_zhuyin_engine_disable (IBusEngine *engine)
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;
    if (punctuation_window && gtk_widget_get_visible(punctuation_window)) {
        g_idle_add(hide_punctuation_window_idle, NULL);
    }
    ibus_zhuyin_engine_commit_preedit (zhuyin);
    engine_instance = NULL;
}

static void
ibus_engine_set_cursor_location (IBusEngine *engine,
                                 gint        x,
                                 gint        y,
                                 gint        w,
                                 gint        h)
{
    if (punctuation_window && gtk_widget_get_visible(punctuation_window)) {
    }
}

/* vim:set fileencodings=utf-8 tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
