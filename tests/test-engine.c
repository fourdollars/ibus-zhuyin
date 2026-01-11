#include <glib.h>
#include <gtk/gtk.h>
#include <ibus.h>
#include "engine.h"
#include "zhuyin.h"

// Mocking IBus functions
static gchar *committed_text = NULL;
static gchar *current_preedit = NULL;

// Mocking GTK functions for punctuation window
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

// Include source directly to access static variables
#include "../src/engine.c"

static void test_h_9_space() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);
    
    // Simulate 'h'
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'h', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄘ"); // zhuyin for h
    
    // Simulate '9'
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '9', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄘㄞ"); // zhuyin for h9
    
    // Simulate ' ' (Space) - should commit '猜' (single candidate)
    // Clear committed text first
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    
    g_assert_cmpstr(committed_text, ==, "猜");
    
    g_object_unref(engine);
}

static void test_w_8_7() {

    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    

    // Reset state

    if (committed_text) { g_free(committed_text); committed_text = NULL; }

    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    

    // Simulate 'w'

    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'w', 0, 0);

    g_assert_cmpstr(current_preedit, ==, "ㄊ");

    

    // Simulate '8'

    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '8', 0, 0);

    g_assert_cmpstr(current_preedit, ==, "ㄊㄚ");

    

    // Simulate '7' - Tone 5 (Neutral)

    // Should commit '遢' (as it is the single candidate for ㄊㄚ˙)

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
    // Directly set the static variable
    punctuation_window = punctuation_window_mock;
    punctuation_window_mock_visible = TRUE;

    // 3. Press 'c' while punctuation window is visible
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'm', 0, 0);

    // Verify commit
    g_assert_cmpstr(committed_text, ==, "。");

    // Reset mock
    punctuation_window = NULL;
    punctuation_window_mock_visible = FALSE;

    g_object_unref(engine);
}

static void test_ctrl_grave_h_1() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // 1. Press Ctrl + `
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(
        engine,
        IBUS_grave,
        0,
        IBUS_CONTROL_MASK
    );

    // 2. Press 'h'
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'h', 0, 0);

    // 3. Press '1' to select the first candidate "☆"
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);

    // Verify commit
    g_assert_cmpstr(committed_text, ==, "☆");

    g_object_unref(engine);
}

static void test_shift_period() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // Simulate Shift + '.' (usually producing '>')
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(
        engine,
        '>',
        0,
        IBUS_SHIFT_MASK
    );

    // Verify commit
    g_assert_cmpstr(committed_text, ==, "。");

    g_object_unref(engine);
}

static void test_zhu_yin() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // --- Part 1: Submit '注' (5 j 4 4) ---
    // '5' (ㄓ), 'j' (ㄨ), '4' (ˋ)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '5', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'j', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '4', 0, 0);
    
    // Select candidate '4'
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '4', 0, 0);

    g_assert_cmpstr(committed_text, ==, "注");

    // --- Part 2: Submit '音' (u p space 2) ---
    // Clear committed text from previous part (optional, as engine usually overwrites or appends depending on implementation, 
    // but our mock helper just replaces committed_text. We should check if engine triggers commit signal again)
    // In our mock: ibus_engine_commit_text frees old committed_text and sets new.
    // So valid to check directly.
    
    // 'u' (ㄧ), 'p' (ㄣ)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'p', 0, 0);
    
    // Space (Tone 1 / Space) -> enter candidate mode
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);

    // Select candidate '2'
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '2', 0, 0);

    g_assert_cmpstr(committed_text, ==, "音");

    g_object_unref(engine);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    ibus_init();

    g_test_add_func("/engine/h_9_space", test_h_9_space);
    g_test_add_func("/engine/w_8_7", test_w_8_7);
    g_test_add_func("/engine/punctuation_window_m", test_punctuation_window_m);
    g_test_add_func("/engine/ctrl_grave_h_1", test_ctrl_grave_h_1);
    g_test_add_func("/engine/shift_period", test_shift_period);
    g_test_add_func("/engine/zhu_yin", test_zhu_yin);

    return g_test_run();
}
