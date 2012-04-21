/* vim:set et sts=4: */

#include <enchant.h>
#include "engine.h"

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
static void	ibus_zhuyin_engine_class_init	(IBusZhuyinEngineClass	*klass);
static void	ibus_zhuyin_engine_init		(IBusZhuyinEngine		*engine);
static void	ibus_zhuyin_engine_destroy		(IBusZhuyinEngine		*engine);
static gboolean 
			ibus_zhuyin_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint               	 keyval,
                                             guint               	 keycode,
                                             guint               	 modifiers);
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
static void ibus_zhuyin_engine_property_show
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
static void ibus_zhuyin_engine_property_hide
											(IBusEngine             *engine,
                                             const gchar            *prop_name);

static void ibus_zhuyin_engine_commit_string
                                            (IBusZhuyinEngine      *zhuyin,
                                             const gchar            *string);
static void ibus_zhuyin_engine_update      (IBusZhuyinEngine      *zhuyin);

static EnchantBroker *broker = NULL;
static EnchantDict *dict = NULL;

G_DEFINE_TYPE (IBusZhuyinEngine, ibus_zhuyin_engine, IBUS_TYPE_ENGINE)

static void
ibus_zhuyin_engine_class_init (IBusZhuyinEngineClass *klass)
{
	IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
	IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);
	
	ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_zhuyin_engine_destroy;

    engine_class->process_key_event = ibus_zhuyin_engine_process_key_event;
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

    zhuyin->table = ibus_lookup_table_new (9, 0, TRUE, TRUE);
    g_object_ref_sink (zhuyin->table);
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
        if (retval != 0) {
            ibus_attr_list_append (text->attrs,
                               ibus_attr_foreground_new (0xff0000, 0, zhuyin->preedit->len));
        }
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

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))

static gboolean 
ibus_zhuyin_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
    IBusText *text;
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

    if (modifiers == IBUS_CONTROL_MASK && keyval == IBUS_s) {
        ibus_zhuyin_engine_update_lookup_table (zhuyin);
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
        if (zhuyin->preedit->len == 0)
            return FALSE;

        g_string_assign (zhuyin->preedit, "");
        zhuyin->cursor_pos = 0;
        ibus_zhuyin_engine_update (zhuyin);
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
        if (zhuyin->preedit->len == 0)
            return FALSE;
        if (zhuyin->cursor_pos > 0) {
            zhuyin->cursor_pos --;
            g_string_erase (zhuyin->preedit, zhuyin->cursor_pos, 1);
            ibus_zhuyin_engine_update (zhuyin);
        }
        return TRUE;
    
    case IBUS_Delete:
        if (zhuyin->preedit->len == 0)
            return FALSE;
        if (zhuyin->cursor_pos < zhuyin->preedit->len) {
            g_string_erase (zhuyin->preedit, zhuyin->cursor_pos, 1);
            ibus_zhuyin_engine_update (zhuyin);
        }
        return TRUE;
    }

    if (is_alpha (keyval)) {
        g_string_insert_c (zhuyin->preedit,
                           zhuyin->cursor_pos,
                           keyval);

        zhuyin->cursor_pos ++;
        ibus_zhuyin_engine_update (zhuyin);
        
        return TRUE;
    }

    return FALSE;
}
