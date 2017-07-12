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

typedef struct _IBusZhuyinEngine IBusZhuyinEngine;
typedef struct _IBusZhuyinEngineClass IBusZhuyinEngineClass;

struct _IBusZhuyinEngine {
    IBusEngine parent;

    /* members */
    GString *preedit;
    gint mode;
    gint cursor_pos;
    gint page;
    gint page_max;
    gint page_size;
    gchar* display[4];
    gchar input[4];
    gboolean valid;
    gchar** candidate_member;
    guint candidate_number;

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

static gboolean ibus_zhuyin_punctuation_phase (IBusZhuyinEngine *zhuyin,
                                               guint             keyval,
                                               guint             keycode,
                                               guint             modifiers);

static gboolean ibus_zhuyin_preedit_phase (IBusZhuyinEngine *zhuyin,
                                           guint             keyval,
                                           guint             keycode,
                                           guint             modifiers);
static gboolean ibus_zhuyin_candidate_phase (IBusZhuyinEngine *zhuyin,
                                             guint             keyval,
                                             guint             keycode,
                                             guint             modifiers);

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
    zhuyin->preedit = g_string_new ("");
    zhuyin->cursor_pos = 0;
    zhuyin->mode = 0;
    zhuyin->page_size = 9;

    zhuyin->table = ibus_lookup_table_new (zhuyin->page_size, 0, FALSE, TRUE);
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
    
    sugs = zhuyin->candidate_member;
    n_sug = zhuyin->candidate_number;

    if (sugs == NULL) {
        ibus_engine_hide_lookup_table ((IBusEngine *) zhuyin);
        return;
    }

    for (i = 0; i < n_sug; i++) {
        ibus_lookup_table_append_candidate (zhuyin->table, ibus_text_new_from_string (sugs[i]));
    }

    ibus_engine_update_lookup_table ((IBusEngine *) zhuyin, zhuyin->table, TRUE);
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
    ibus_zhuyin_engine_reset((IBusEngine *) zhuyin);

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
    ibus_zhuyin_engine_reset((IBusEngine *) zhuyin);

    return TRUE;
}

static void
ibus_zhuyin_engine_commit_string (IBusZhuyinEngine *zhuyin,
                                   const gchar       *string)
{
    IBusText *text;
    text = ibus_text_new_from_static_string (string);
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
    zhuyin->cursor_pos = 0;
    zhuyin->mode = 0;
    zhuyin->page = 0;
    zhuyin->valid = FALSE;

    ibus_zhuyin_engine_update (zhuyin);
}

static void
ibus_zhuyin_engine_redraw (IBusZhuyinEngine *zhuyin)
{
    gsize i = 0;
    g_string_assign (zhuyin->preedit, "");
    zhuyin->cursor_pos = 0;

    for (i = 0; i < 4; i++) {
        if (zhuyin->display[i] != NULL) {
            gsize old_len = zhuyin->preedit->len;
            g_string_insert (zhuyin->preedit,
                    zhuyin->cursor_pos,
                    zhuyin->display[i] );
            zhuyin->cursor_pos += zhuyin->preedit->len - old_len;
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
    static gchar** punctuation_candidate = NULL;
    gboolean more_punctuation = FALSE;
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
            more_punctuation = TRUE;
            zhuyin->candidate_number = 3;
            break;
        case '+':
            punctuation = "＋ ＝";
            more_punctuation = TRUE;
            zhuyin->candidate_number = 2;
            break;
        case '[':
            punctuation = "「";
            break;
        case '{':
            punctuation = "『 〈 《 ［ ｛ 【 〖 〔 〘 〚";
            more_punctuation = TRUE;
            zhuyin->candidate_number = 10;
            break;
        case ']':
            punctuation = "」";
            break;
        case '}':
            punctuation = "』 〉 》 ］ ｝ 】 〗 〕 〙 〛";
            more_punctuation = TRUE;
            zhuyin->candidate_number = 10;
            break;
        case '\\':
            punctuation = "＼ ／";
            more_punctuation = TRUE;
            zhuyin->candidate_number = 2;
            break;
        case '|':
            punctuation = "｜";
            break;
        case ':':
            punctuation = "： ；";
            more_punctuation = TRUE;
            zhuyin->candidate_number = 2;
            break;
        case '\'':
            punctuation = "’";
            break;
        case '"':
            punctuation = "、 “ ” ‘ ’";
            more_punctuation = TRUE;
            zhuyin->candidate_number = 5;
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

    if (more_punctuation == TRUE) {
        if (punctuation_candidate != NULL) {
            g_strfreev(punctuation_candidate);
        }
        punctuation_candidate = g_strsplit (punctuation, " ", 0);
        zhuyin->candidate_member = punctuation_candidate;
        zhuyin->display[0] = punctuation_candidate[0];
        zhuyin->mode = 1;
        ibus_zhuyin_engine_redraw (zhuyin);
        ibus_zhuyin_engine_update_lookup_table (zhuyin);
        return TRUE;
    }

    if (punctuation != NULL) {
        ibus_zhuyin_engine_commit_string (zhuyin, punctuation);
        return TRUE;
    }

    return FALSE;
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
                zhuyin->mode = 1;
                if (zhuyin->candidate_number == 1) {
                    gsize i = 3;
                    while (i > 0) {
                        if (zhuyin->input[i] > 0) {
                            zhuyin->input[i] = 0;
                            zhuyin->display[i] = NULL;
                        }
                        i--;
                    }
                    zhuyin->input[i] = 0;
                    zhuyin->display[i] = zhuyin->candidate_member[0];
                    ibus_zhuyin_engine_redraw (zhuyin);
                    zhuyin->display[i] = NULL;
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
                gsize i = 3;
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
            gsize i = 3;
            while (i > 0) {
                if (zhuyin->input[i] > 0) {
                    zhuyin->input[i] = 0;
                    zhuyin->display[i] = NULL;
                }
                i--;
            }
            zhuyin->input[i] = 0;
            zhuyin->display[i] = zhuyin->candidate_member[0];
            ibus_zhuyin_engine_redraw (zhuyin);
            zhuyin->display[i] = NULL;
            return ibus_zhuyin_engine_commit_preedit (zhuyin);
        }

        if (zhuyin->candidate_member == NULL) {
            zhuyin->valid = FALSE;
        } else {
            zhuyin->valid = TRUE;
            if (zhuyin->display[3] != NULL) {
                zhuyin->mode = 1;
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
    switch (keyval) {
        case IBUS_1:
        case IBUS_a:
            candidate = zhuyin->page * zhuyin->page_size;
            break;
        case IBUS_2:
        case IBUS_s:
            candidate = zhuyin->page * zhuyin->page_size + 1;
            break;
        case IBUS_3:
        case IBUS_d:
            candidate = zhuyin->page * zhuyin->page_size + 2;
            break;
        case IBUS_4:
        case IBUS_f:
            candidate = zhuyin->page * zhuyin->page_size + 3;
            break;
        case IBUS_5:
        case IBUS_g:
            candidate = zhuyin->page * zhuyin->page_size + 4;
            break;
        case IBUS_6:
        case IBUS_h:
            candidate = zhuyin->page * zhuyin->page_size + 5;
            break;
        case IBUS_7:
        case IBUS_j:
            candidate = zhuyin->page * zhuyin->page_size + 6;
            break;
        case IBUS_8:
        case IBUS_k:
            candidate = zhuyin->page * zhuyin->page_size + 7;
            break;
        case IBUS_9:
        case IBUS_l:
            candidate = zhuyin->page * zhuyin->page_size + 8;
            break;
        default:break;
    }

    if (candidate != -1 && candidate < zhuyin->candidate_number) {
        return ibus_zhuyin_engine_commit_candidate (zhuyin, candidate);
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
        case IBUS_Return:
            return ibus_zhuyin_engine_commit_preedit (zhuyin);

        case IBUS_Escape:
        case IBUS_Delete:
            if (zhuyin->preedit->len == 0)
                return FALSE;

            ibus_zhuyin_engine_reset ((IBusEngine *)zhuyin);
            return TRUE;

        case IBUS_Left:
        case IBUS_Up:
            ibus_lookup_table_page_up(zhuyin->table);
            zhuyin->page--;
            if (zhuyin->page < 0) {
                zhuyin->page = zhuyin->page_max;
            }
            ibus_engine_update_lookup_table ((IBusEngine *) zhuyin, zhuyin->table, TRUE);
            return TRUE;

        case IBUS_space:
        case IBUS_Right:
        case IBUS_Down:
            ibus_lookup_table_page_down(zhuyin->table);
            zhuyin->page++;
            if (zhuyin->page > zhuyin->page_max) {
                zhuyin->page = 0;
            }
            ibus_engine_update_lookup_table ((IBusEngine *) zhuyin, zhuyin->table, TRUE);
            return TRUE;

        case IBUS_BackSpace:
            zhuyin->mode = 0;
            if (zhuyin->preedit->len == 0) {
                return FALSE;
            } else {
                gsize i = 3;
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

    switch (zhuyin->mode) {
        case 0:
            if (zhuyin->preedit->len == 0) {
                if (ibus_zhuyin_punctuation_phase(zhuyin, keyval, keycode, modifiers) == FALSE) {
                    return ibus_zhuyin_preedit_phase(zhuyin, keyval, keycode, modifiers);
                }
            } else {
                return ibus_zhuyin_preedit_phase(zhuyin, keyval, keycode, modifiers);
            }
        case 1:
            return ibus_zhuyin_candidate_phase(zhuyin, keyval, keycode, modifiers);
        default:
            break;
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
