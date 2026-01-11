#include <glib.h>
#include <ibus.h>
#include "engine.h"
#include "zhuyin.h"

// Mocking IBus functions
static gchar *committed_text = NULL;
static gchar *current_preedit = NULL;

// Mocking GTK functions for punctuation window
typedef void GtkWidget;

// Prototype for helper in engine.c
void test_set_punctuation_window(GtkWidget *widget);

static GtkWidget *punctuation_window_mock = (GtkWidget *)1; // dummy non-NULL pointer
static gboolean punctuation_window_mock_visible = FALSE;

gboolean gtk_widget_get_visible(GtkWidget *widget) {
    return punctuation_window_mock_visible;
}

guint g_idle_add(GSourceFunc function, gpointer data) {
    // Do nothing in test
    return 1;
}

void ibus_engine_commit_text(IBusEngine *engine, IBusText *text) {
    if (committed_text) g_free(committed_text);
    committed_text = g_strdup(text->text);
}

void ibus_engine_update_preedit_text(IBusEngine *engine, IBusText *text, guint cursor_pos, gboolean visible) {
    if (current_preedit) g_free(current_preedit);
    current_preedit = g_strdup(text->text);
}

void ibus_engine_hide_lookup_table(IBusEngine *engine) {}
void ibus_engine_update_lookup_table(IBusEngine *engine, IBusLookupTable *table, gboolean visible) {}
void ibus_engine_update_auxiliary_text(IBusEngine *engine, IBusText *text, gboolean visible) {}
void ibus_engine_hide_preedit_text(IBusEngine *engine) {}
void ibus_engine_show_preedit_text(IBusEngine *engine) {}

// Dummy implementation for ibus_attr_list related functions if needed by engine.c
// engine.c uses ibus_attr_list_new, ibus_attr_list_append, ibus_attr_underline_new.
// These are usually in libibus. We link against libibus so it should be fine.

static void test_h_9_space() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);
    
    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }
    
    // 'h' -> ㄘ
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'h', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄘ");
    
    // '9' -> ㄞ
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '9', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄘㄞ");
    
    // Space -> Commit "猜" (assuming single candidate)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_space, 0, 0);
    
    // Verify commit
    g_assert_cmpstr(committed_text, ==, "猜");
    
    g_object_unref(engine);
}

static void test_w_8_7() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);
    
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // 'w' -> ㄊ
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'w', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄊ");
    
    // '8' -> ㄚ
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '8', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄊㄚ");
    
    // '7' -> ˙ (Tone 5)
    // If "遢" is the only candidate for ㄊㄚ˙, it should commit immediately.
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '7', 0, 0);
    
    g_assert_cmpstr(committed_text, ==, "遢");
    
    g_object_unref(engine);
}

static void test_punctuation_window_m() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // 1. Simulate triggering the punctuation window (Ctrl+Alt+,)
    gboolean handled = IBUS_ENGINE_GET_CLASS(engine)->process_key_event(
        engine, 
        IBUS_comma, 
        0, 
        IBUS_CONTROL_MASK | IBUS_MOD1_MASK
    );
    g_assert_true(handled);

    // 2. Mock punctuation window as visible (simulating what g_idle_add would do)
    test_set_punctuation_window(punctuation_window_mock);
    punctuation_window_mock_visible = TRUE;

    // 3. Press 'c' while punctuation window is visible
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'm', 0, 0);

    // Verify commit
    g_assert_cmpstr(committed_text, ==, "。");

    // Reset mock
    test_set_punctuation_window(NULL);
    punctuation_window_mock_visible = FALSE;

    g_object_unref(engine);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    ibus_init();

    g_test_add_func("/engine/h_9_space", test_h_9_space);
    g_test_add_func("/engine/w_8_7", test_w_8_7);
    g_test_add_func("/engine/punctuation_window_m", test_punctuation_window_m);

    return g_test_run();
}
