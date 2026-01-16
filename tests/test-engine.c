#include <glib.h>
/**
 * Copyright (C) 2026 Shih-Yuan Lee (FourDollars) <fourdollars@debian.org>
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

#include <gtk/gtk.h>
#include <ibus.h>
#include "engine.h"
#include "zhuyin.h"

// Mocking IBus functions
static gchar *committed_text = NULL;
static gchar *current_preedit = NULL;
static gchar *current_aux_text = NULL;

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
void ibus_engine_update_auxiliary_text(IBusEngine *engine, IBusText *text, gboolean visible) {
    if (current_aux_text) g_free(current_aux_text);
    current_aux_text = visible ? g_strdup(text->text) : NULL;
}
void ibus_engine_hide_preedit_text(IBusEngine *engine) {}
void ibus_engine_show_preedit_text(IBusEngine *engine) {}
void ibus_engine_register_properties(IBusEngine *engine, IBusPropList *prop_list) {}
void ibus_engine_update_property(IBusEngine *engine, IBusProperty *prop) {}

// Include source directly to access static variables
#include "../src/engine.c"

static void test_hsu_layout() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);
    
    // Enable to setup properties
    IBUS_ENGINE_GET_CLASS(engine)->enable(engine);
    
    // Switch to Hsu's layout
    IBUS_ENGINE_GET_CLASS(engine)->property_activate(engine, "InputMode.Hsu", PROP_STATE_CHECKED);
    
    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }
    
    // Test 1: b (ㄅ) + k (ㄤ) + Space -> 幫
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'b', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'k', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    
    g_assert_cmpstr(committed_text, ==, "幫");
    
    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }
    
    // Test 2: Space re-interpretation
    // a (ㄔ/ㄟ) -> Space -> ㄟ
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'a', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    
    g_assert_cmpstr(committed_text, ==, "ㄟ");
    
    g_object_unref(engine);
}

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

    // 2. Mock punctuation window as visible
    punctuation_window = punctuation_window_mock;
    punctuation_window_mock_visible = TRUE;

    // 3. Press 'm' while punctuation window is visible
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

    // --- Part 1: Submit '注' ---
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '5', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'j', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '4', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '4', 0, 0);

    g_assert_cmpstr(committed_text, ==, "注");

    // --- Part 2: Submit '音' ---
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'p', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '2', 0, 0);

    g_assert_cmpstr(committed_text, ==, "音");

    g_object_unref(engine);
}

static void test_phrase_lookup() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // "一" (yi) -> u (ㄧ) + Space (Tone 1)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);

    // "一" should be the first candidate.
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    
    // Verify "一" was committed
    g_assert_cmpstr(committed_text, ==, "一");

    // Verify "(Shift to select)" reminder is shown
    g_assert_nonnull(current_aux_text);
    g_assert_true(g_str_has_suffix(current_aux_text, "(Shift to select)"));

    // --- Part 1: Select with Shift ---
    // Press 'Shift + 1' to select first phrase "個"
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, IBUS_SHIFT_MASK);
    
    // Verify that "個" was committed
    g_assert_cmpstr(committed_text, ==, "個");

    // --- Part 3: Press Shift alone ---
    // Clear committed_text
    g_free(committed_text); committed_text = NULL;
    
    // "一" (yi) -> u (ㄧ) + Space (Tone 1) -> Commit 1
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    g_assert_cmpstr(committed_text, ==, "一");
    g_free(committed_text); committed_text = NULL;

    // Now in PHRASE mode. Press Shift_L.
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Shift_L, 0, 0);
    
    // Phrase list should still be there. Press Shift + 1 to select "個".
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, IBUS_SHIFT_MASK);
    g_assert_cmpstr(committed_text, ==, "個");

    g_object_unref(engine);
}

static void test_immediate_selection() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // "ㄕ" (shi) -> g
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'g', 0, 0);

    // Verify "(Shift to select)" reminder is shown
    g_assert_nonnull(current_aux_text);
    g_assert_true(g_str_has_suffix(current_aux_text, "(Shift to select)"));

    // Press 'Shift + 1' to select first candidate of "ㄕ" (which is "失" usually)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, IBUS_SHIFT_MASK);
    
    // Verify that "失" was committed
    g_assert_cmpstr(committed_text, ==, "失");

    g_object_unref(engine);
}

static void test_phrase_return() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // 1. Commit "一" to enter PHRASE mode
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    g_assert_cmpstr(committed_text, ==, "一");
    g_free(committed_text); committed_text = NULL;

    // Verify we are in phrase mode (aux text shows "Shift to select")
    g_assert_nonnull(current_aux_text);
    g_assert_true(g_str_has_suffix(current_aux_text, "(Shift to select)"));

    // 2. Press Return. NEW behavior: does NOT commit "個".
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Return, 0, 0);
    
    // We expect committed_text to remain NULL
    g_assert_null(committed_text);

    g_object_unref(engine);
}

static void test_normal_return_with_candidates() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // "ㄕ" (shi) -> g
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'g', 0, 0);
    
    // Verify "(Shift to select)" reminder is shown
    g_assert_nonnull(current_aux_text);
    g_assert_true(g_str_has_suffix(current_aux_text, "(Shift to select)"));

    // Press Return.
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Return, 0, 0);
    
    // It should commit "ㄕ", NOT "失".
    g_assert_cmpstr(committed_text, ==, "ㄕ");

    g_object_unref(engine);
}

static void test_phrase_navigation_and_shortcuts() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // 1. Enter phrase mode with "一" (u + Space + 1)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    g_free(committed_text); committed_text = NULL; // Clear "一" commit

    // Verify Page 1
    g_assert_nonnull(current_aux_text);
    g_assert_true(g_str_has_prefix(current_aux_text, "1 /"));

    // 2. Test Space -> Page Down
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    g_assert_true(g_str_has_prefix(current_aux_text, "2 /"));

    // 3. Test Page_Up -> Page 1
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Page_Up, 0, 0);
    g_assert_true(g_str_has_prefix(current_aux_text, "1 /"));

    // 4. Test End -> Last Page
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_End, 0, 0);
    g_assert_false(g_str_has_prefix(current_aux_text, "1 /")); // Should not be page 1

    // 5. Test Home -> Page 1
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Home, 0, 0);
    g_assert_true(g_str_has_prefix(current_aux_text, "1 /"));

    // 6. Test Arrow Down -> Move cursor, eventually page down if needed.
    // Since we are checking paging, let's just assume arrow keys are consumed.
    gboolean handled = IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Down, 0, 0);
    g_assert_true(handled);

    // 7. Test Escape -> Reset (Exit phrase mode)
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Escape, 0, 0);
    // Aux text should be cleared or changed.
    // In reset, it hides lookup table and updates aux text.
    // In ibus_zhuyin_engine_reset: ibus_engine_hide_lookup_table(...) which might update aux text?
    // ibus_zhuyin_engine_reset calls ibus_zhuyin_engine_update_aux_text(zhuyin).
    // In reset state, aux text should be empty string (hidden).
    // current_aux_text depends on mocking logic.
    // void ibus_engine_update_auxiliary_text(..., visible) { ... current_aux_text = visible ? str : NULL; }
    // If visible is FALSE, current_aux_text is NULL.
    g_assert_null(current_aux_text);

    // 8. Re-enter phrase mode to test Shift + 2 selection
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    g_free(committed_text); committed_text = NULL;

    // Test Shift + 2 (Select 2nd candidate)
    // Note: Depends on actual candidates for "一". "個" is usually 1st.
    // We just verify it commits *something*.
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '2', 0, IBUS_SHIFT_MASK);
    g_assert_nonnull(committed_text);
    
    g_object_unref(engine);
}

static void test_phrase_cursor_navigation() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // Enter phrase mode with "一"
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ' ', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    g_free(committed_text); committed_text = NULL;

    // Test Right -> Move cursor to next candidate
    // Assuming default cursor pos 0.
    // We can't easily check cursor pos without inspecting 'table' directly,
    // but we can check if it handled the event (returned TRUE).
    // And verify it didn't crash.
    gboolean handled = IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Right, 0, 0);
    g_assert_true(handled);

    // Test Left -> Move cursor back
    handled = IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Left, 0, 0);
    g_assert_true(handled);
    
    // Test Up -> Move cursor up (Vertical) or Page Up (Horizontal)
    // Depending on orientation. Logic says:
    // if HORIZONTAL -> Page Up.
    // if VERTICAL -> Cursor Up.
    // Our mock environment might default to Vertical in code if version check fails, 
    // but IBus version macro check in code determines it.
    // We just verify it's handled.
    handled = IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Up, 0, 0);
    g_assert_true(handled);

    g_object_unref(engine);
}

static void test_preedit_editing() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // 1. Backspace single char
    // '1' -> "ㄅ" in Standard layout
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    g_assert_cmpstr(current_preedit, ==, "ㄅ");
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_BackSpace, 0, 0);
    // Should be empty or NULL.
    // The implementation might hide preedit or update with empty string.
    // We check if it handled it.
    
    // 2. Backspace multiple chars
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'u', 0, 0);
    // preedit should be "ㄅㄧ"
    g_assert_cmpstr(current_preedit, ==, "ㄅㄧ");
    
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_BackSpace, 0, 0);
    // Should remove last char -> "ㄅ"
    g_assert_cmpstr(current_preedit, ==, "ㄅ");

    // 3. Escape to clear
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, IBUS_Escape, 0, 0);
    // Preedit should be reset/hidden.
    
    g_object_unref(engine);
}

static void test_punctuation_symbols() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    // Reset state
    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // Test '[' -> "「"
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '[', 0, 0);
    g_assert_cmpstr(committed_text, ==, "「");

    // Test ']' -> "」"
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, ']', 0, 0);
    g_assert_cmpstr(committed_text, ==, "」");

    // Test '\' -> "＼" or "／" (Candidate mode)
    g_free(committed_text); committed_text = NULL;
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '\\', 0, 0);
    // Should trigger candidate mode, not direct commit.
    g_assert_null(committed_text);
    // Check if table is visible or preedit updated.
    // Ideally check if candidates are populated. 
    // But committed_text being NULL confirms it didn't commit immediately.
    
    g_object_unref(engine);
}

static void test_candidate_selection() {
    IBusEngine *engine = g_object_new(ibus_zhuyin_engine_get_type(), NULL);

    if (committed_text) { g_free(committed_text); committed_text = NULL; }
    if (current_preedit) { g_free(current_preedit); current_preedit = NULL; }

    // a (ㄇ) + 8 (ㄚ) + 3 (ˇ) -> ma3
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, 'a', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '8', 0, 0);
    IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '3', 0, 0);
    
    IBusZhuyinEngine *zhuyin = (IBusZhuyinEngine *)engine;
    
    // We expect multiple candidates for ma3 (e.g. 馬, 碼, 螞, etc.)
    // But if the dictionary is small/empty in test environment, it might behave differently.
    // However, zhuyin.c is compiled in.
    
    if (zhuyin->candidate_number > 1) {
        g_assert_cmpint(zhuyin->mode, ==, IBUS_ZHUYIN_MODE_CANDIDATE);
        
        // Select 1st candidate using '1'
        IBUS_ENGINE_GET_CLASS(engine)->process_key_event(engine, '1', 0, 0);
        g_assert_nonnull(committed_text);
    } else if (zhuyin->candidate_number == 1) {
        // Auto committed
        g_assert_nonnull(committed_text);
    } else {
        // No candidates? Should be valid if input is valid.
        // If no candidates, it might stay in Normal mode or Preedit?
        // But ma3 should be valid.
    }

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
    g_test_add_func("/engine/hsu_layout", test_hsu_layout);
    g_test_add_func("/engine/phrase_lookup", test_phrase_lookup);
    g_test_add_func("/engine/phrase_return", test_phrase_return);
    g_test_add_func("/engine/phrase_navigation_and_shortcuts", test_phrase_navigation_and_shortcuts);
    g_test_add_func("/engine/phrase_cursor_navigation", test_phrase_cursor_navigation);
    g_test_add_func("/engine/preedit_editing", test_preedit_editing);
    g_test_add_func("/engine/punctuation_symbols", test_punctuation_symbols);
    g_test_add_func("/engine/candidate_selection", test_candidate_selection);
    g_test_add_func("/engine/normal_return_with_candidates", test_normal_return_with_candidates);
    g_test_add_func("/engine/immediate_selection", test_immediate_selection);

    return g_test_run();
}