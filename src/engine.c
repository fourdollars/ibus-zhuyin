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

#include <enchant.h>
#include "engine.h"
#include "zhuyin.h"

typedef struct _IBusZhuyinEngine IBusZhuyinEngine;
typedef struct _IBusZhuyinEngineClass IBusZhuyinEngineClass;

struct _IBusZhuyinEngine {
    IBusEngine parent;

    /* members */
    GString *preedit;
    gint cursor_pos;

    IBusLookupTable *table;
};

struct _IBusZhuyinEngineClass {
    IBusEngineClass parent;
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
static void ibus_zhuyin_engine_focus_in    (IBusEngine             *engine);
static void ibus_zhuyin_engine_focus_out   (IBusEngine             *engine);
static void ibus_zhuyin_engine_reset       (IBusEngine             *engine);
static void ibus_zhuyin_engine_enable      (IBusEngine             *engine);
static void ibus_zhuyin_engine_disable     (IBusEngine             *engine);
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_zhuyin_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
static void ibus_zhuyin_engine_page_up     (IBusEngine             *engine);
static void ibus_zhuyin_engine_page_down   (IBusEngine             *engine);
static void ibus_zhuyin_engine_cursor_up   (IBusEngine             *engine);
static void ibus_zhuyin_engine_cursor_down (IBusEngine             *engine);
static void ibus_zhuyin_property_activate  (IBusEngine             *engine,
                                            const gchar            *prop_name,
                                            gint                    prop_state);
static void ibus_zhuyin_engine_property_show (IBusEngine             *engine,
                                              const gchar            *prop_name);
static void ibus_zhuyin_engine_property_hide (IBusEngine             *engine,
                                              const gchar            *prop_name);

static void ibus_zhuyin_engine_commit_string (IBusZhuyinEngine      *zhuyin,
                                              const gchar            *string);
static void ibus_zhuyin_engine_update      (IBusZhuyinEngine      *zhuyin);

static EnchantBroker *broker = NULL;
static EnchantDict *dict = NULL;
static gchar* display[4] = {NULL};
static gchar input[4] = {0};

G_DEFINE_TYPE (IBusZhuyinEngine, ibus_zhuyin_engine, IBUS_TYPE_ENGINE)

static void
ibus_zhuyin_engine_class_init (IBusZhuyinEngineClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_zhuyin_engine_destroy;

    engine_class->process_key_event = ibus_zhuyin_engine_process_key_event;
    engine_class->enable            = ibus_zhuyin_engine_enable;
    engine_class->disable           = ibus_zhuyin_engine_disable;
    engine_class->reset             = ibus_zhuyin_engine_reset;
}

static void
ibus_zhuyin_engine_init (IBusZhuyinEngine *zhuyin)
{
    if (broker == NULL) {
        broker = enchant_broker_init ();
        dict = enchant_broker_request_dict (broker, "en");
    }

    zhuyin->preedit = g_string_new ("");
    zhuyin->cursor_pos = 0;

    zhuyin->table = ibus_lookup_table_new (10, 0, TRUE, TRUE);
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

    ((IBusObjectClass *) ibus_zhuyin_engine_parent_class)->destroy ((IBusObject *)zhuyin);
}

static void
ibus_zhuyin_engine_update_lookup_table (IBusZhuyinEngine *zhuyin)
{
    gchar ** sugs;
    gsize n_sug, i;
    gboolean retval;

    if (zhuyin->preedit->len == 0) {
        ibus_engine_hide_lookup_table ((IBusEngine *) zhuyin);
        return;
    }

    ibus_lookup_table_clear (zhuyin->table);
    
    sugs = enchant_dict_suggest (dict,
                                 zhuyin->preedit->str,
                                 zhuyin->preedit->len,
                                 &n_sug);

    if (sugs == NULL || n_sug == 0) {
        ibus_engine_hide_lookup_table ((IBusEngine *) zhuyin);
        return;
    }

    for (i = 0; i < n_sug; i++) {
        ibus_lookup_table_append_candidate (zhuyin->table, ibus_text_new_from_string (sugs[i]));
    }

    ibus_engine_update_lookup_table ((IBusEngine *) zhuyin, zhuyin->table, TRUE);

    if (sugs)
        enchant_dict_free_suggestions (dict, sugs);
}

static void
ibus_zhuyin_engine_update_preedit (IBusZhuyinEngine *zhuyin)
{
    IBusText *text;
    gint retval;

    text = ibus_text_new_from_static_string (zhuyin->preedit->str);
    text->attrs = ibus_attr_list_new ();
    
    ibus_attr_list_append (text->attrs,
                           ibus_attr_underline_new (IBUS_ATTR_UNDERLINE_SINGLE, 0, zhuyin->preedit->len));

    if (zhuyin->preedit->len > 0) {
        retval = enchant_dict_check (dict, zhuyin->preedit->str, zhuyin->preedit->len);
/*        if (retval != 0) {*/
/*            ibus_attr_list_append (text->attrs,*/
/*                               ibus_attr_foreground_new (0xff0000, 0, zhuyin->preedit->len));*/
/*        }*/
    }
    
    ibus_engine_update_preedit_text ((IBusEngine *)zhuyin,
                                     text,
                                     zhuyin->cursor_pos,
                                     TRUE);

}

/* commit preedit to client and update preedit */
static gboolean
ibus_zhuyin_engine_commit_preedit (IBusZhuyinEngine *zhuyin)
{
    if (zhuyin->preedit->len == 0)
        return FALSE;
    
    ibus_zhuyin_engine_commit_string (zhuyin, zhuyin->preedit->str);
    g_string_assign (zhuyin->preedit, "");
    zhuyin->cursor_pos = 0;

    ibus_zhuyin_engine_update (zhuyin);

    return TRUE;
}

/* commit candidate to client and update preedit */
static gboolean
ibus_zhuyin_engine_commit_candidate (IBusZhuyinEngine *zhuyin, gint candidate)
{
    if (zhuyin->preedit->len == 0)
        return FALSE;

    IBusText *ib_text = ibus_lookup_table_get_candidate (zhuyin->table, candidate);
    ibus_zhuyin_engine_commit_string (zhuyin, ib_text->text);
    g_string_assign (zhuyin->preedit, "");
    zhuyin->cursor_pos = 0;

    ibus_zhuyin_engine_update (zhuyin);

    return TRUE;
}

static void
ibus_zhuyin_engine_commit_string (IBusZhuyinEngine *zhuyin,
                                   const gchar       *string)
{
    IBusText *text;
    text = ibus_text_new_from_static_string (string);
    ibus_engine_commit_text ((IBusEngine *)zhuyin, text);
    g_debug("commit %s", text->text);
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
        input[i] = 0;
        display[i] = NULL;
    }

    g_string_assign (zhuyin->preedit, "");
    zhuyin->cursor_pos = 0;

    ibus_zhuyin_engine_update (zhuyin);
}

static void
ibus_zhuyin_engine_redraw (IBusZhuyinEngine *zhuyin)
{
    gsize i = 0;
/*    g_string_assign (zhuyin->preedit, "");*/
/*    zhuyin->cursor_pos = 0;*/

    for (i = 0; i < 4; i++) {
        if (display[i] != NULL) {
            gsize old_len = zhuyin->preedit->len;
            g_string_insert (zhuyin->preedit,
                    zhuyin->cursor_pos,
                    display[i] );
            zhuyin->cursor_pos += zhuyin->preedit->len - old_len;
        }
    }

    ibus_zhuyin_engine_update (zhuyin);
}

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))

static gboolean 
ibus_zhuyin_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
    IBusText *text;
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;

    /* Ignore key release event */
    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    /* Choose candidate character */
    if (modifiers & IBUS_SHIFT_MASK) {
        switch (keyval) {
            case IBUS_exclam:      // shift + 1
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 0);
            case IBUS_at:          // shift + 2
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 1);
            case IBUS_numbersign:  // shift + 3
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 2);
            case IBUS_dollar:      // shift + 4
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 3);
            case IBUS_percent:     // shift + 5
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 4);
            case IBUS_asciicircum: // shift + 6
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 5);
            case IBUS_ampersand:   // shift + 7
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 6);
            case IBUS_asterisk:    // shift + 8
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 7);
            case IBUS_parenleft:   // shift + 9
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 8);
            case IBUS_parenright:  // shift + 0
                return ibus_zhuyin_engine_commit_candidate (zhuyin, 9);
                break;
            default:break;
        }
    }

    modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

    /* Functional shortcuts */
    if (modifiers == IBUS_CONTROL_MASK && keyval == IBUS_s) {
        ibus_zhuyin_engine_update_lookup_table (zhuyin);
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


    switch (keyval) {
        case IBUS_space:
            g_string_append (zhuyin->preedit, " ");
            return ibus_zhuyin_engine_commit_preedit (zhuyin);
        case IBUS_Return:
            return ibus_zhuyin_engine_commit_preedit (zhuyin);

        case IBUS_Escape:
        case IBUS_Delete:
            if (zhuyin->preedit->len == 0)
                return FALSE;

            ibus_zhuyin_engine_reset ((IBusEngine *)zhuyin);
            return TRUE;

        case IBUS_Left:
            if (zhuyin->preedit->len == 0)
                return FALSE;
            if (zhuyin->cursor_pos > 0) {
                zhuyin->cursor_pos --;
                ibus_zhuyin_engine_update (zhuyin);
            }
            return TRUE;

        case IBUS_Right:
            if (zhuyin->preedit->len == 0)
                return FALSE;
            if (zhuyin->cursor_pos < zhuyin->preedit->len) {
                zhuyin->cursor_pos ++;
                ibus_zhuyin_engine_update (zhuyin);
            }
            return TRUE;

        case IBUS_Up:
            if (zhuyin->preedit->len == 0)
                return FALSE;
            if (zhuyin->cursor_pos != 0) {
                zhuyin->cursor_pos = 0;
                ibus_zhuyin_engine_update (zhuyin);
            }
            return TRUE;

        case IBUS_Down:
            if (zhuyin->preedit->len == 0)
                return FALSE;

            if (zhuyin->cursor_pos != zhuyin->preedit->len) {
                zhuyin->cursor_pos = zhuyin->preedit->len;
                ibus_zhuyin_engine_update (zhuyin);
            }

            return TRUE;

        case IBUS_BackSpace:
            if (zhuyin->preedit->len == 0) {
                return FALSE;
            } else {
                gsize i = 3;
                while (i >= 0) {
                    if (input[i] > 0) {
                        input[i] = 0;
                        display[i] = NULL;
                        break;
                    }
                    i--;
                }
                ibus_zhuyin_engine_redraw (zhuyin);
                return TRUE;
            }

    }

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
        default:
            break;
    }

    if (type > 0) {
        guint stanza = 0;
        gsize i = 0;
        gchar* old_display = display[type - 1];
        gchar old_input = input[type - 1];
        display[type - 1] = phonetic;
        input[type - 1] = keyval;

        for (i = 0; i < 4; i++) {
            if (input[i] != 0) {
                stanza = (stanza << 8) | input[i];
            }
        }

        i = zhuyin_check(stanza);

        if (i == -1) {
            input[type - 1] = old_input;
            display[type - 1] = old_display;
        }

        ibus_zhuyin_engine_redraw (zhuyin);
    }
    return TRUE;
}

static void ibus_zhuyin_engine_enable (IBusEngine *engine)
{
    ibus_zhuyin_engine_reset (engine);
}

static void ibus_zhuyin_engine_disable (IBusEngine *engine)
{
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;
    ibus_zhuyin_engine_commit_preedit (zhuyin);
}

/* vim:set fileencodings=utf-8 tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
