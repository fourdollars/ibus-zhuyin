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

#ifndef __PUNCTUATION_H__
#define __PUNCTUATION_H__

#include <ibus.h>

typedef struct {
    guint keyval;
    const gchar *candidates;
} PunctuationEntry;

static const PunctuationEntry leading_key_punctuation[] = {
    { IBUS_bracketleft, "【 〔 《 〈 ﹙ ﹛ ﹝ 「 『 ︻ ︹ ︷ ︿ ︽ ﹁ ﹃" },
    { IBUS_bracketright, "】 〕 》 〉 ﹚ ﹜ ﹞ 」 』 ︼ ︺ ︸ ︾ ︼ ﹂ ﹄" },
    { IBUS_minus, "— … ¯ ￣ ＿ ˍ ˗ ˜ ﹍ ﹎ ﹏" },
    { IBUS_equal, "＝ ≠ ≒ ≡ ≦ ≧ ≑ ≐ ≣ ∽ ∍ ≍ ≃ ≅" },
    { IBUS_apostrophe, "‘ ’ “ ” 〝 〞 ‵ ′ 〃" },
    { IBUS_comma, "， 、 ； ﹐ ¸" },
    { IBUS_period, "。 ． ‥ ﹒ ‧" },
    { IBUS_semicolon, "； ： ﹔ ﹕ ︰" },
    { IBUS_slash, "／ ？ ！ ﹖ ﹗ ⁄" },
    { IBUS_backslash, "＼ ﹨ ╲ ㇔" },
    { IBUS_a, "Ａ ａ Ｂ ｂ Ｃ ｃ Ｄ ｄ Ｅ ｅ Ｆ ｆ Ｇ ｇ Ｈ ｈ Ｉ ｉ Ｊ ｊ Ｋ ｋ Ｌ ｌ Ｍ ｍ Ｎ ｎ Ｏ ｏ Ｐ ｐ Ｑ ｑ Ｒ ｒ Ｓ ｓ Ｔ ｔ Ｕ ｕ Ｖ ｖ Ｗ ｗ Ｘ ｘ Ｙ ｙ Ｚ ｚ" },
    { IBUS_b, "┌ ┬ ┐ ├ ┼ ┤ └ ┴ ┘ ─ │ ═ ╞ ╪ ╡ ╔ ╦ ╗ ╠ ╬ ╣ ╚ ╩ ╝ ╒ ╤ ╕ ╘ ╧ ╛" },
    { IBUS_m, "∀ ∃ ∮ ∵ ∴ ♀ ♂ ⊕ ⊙ ↑ ↓ ← → ↖ ↗ ↙ ↘ ∥ ∣ ／ ＼ ∕ ﹨ √ ∞ ∟ ∠ ∩ ∪ ∫ ∬ ∭ ∮ ∯ ∰ ∱ ∲ ∳" },
    { IBUS_u, "℃ ℉ ％ ㎎ ㎏ ㎝ ㎜ ㎡ ㎥ ㏄ ㏕ ℡ ‰ ¢ £ ¤ ¥ ฿ ℓ ㏒ ㏑ ㏇ ㏕ ℡" },
    { IBUS_n, "⓪ ① ② ③ ④ ⑤ ⑥ ⑦ ⑧ ⑨ ⑩ ⑪ ⑫ ⑬ ⑭ ⑮ ⑯ ⑰ ⑱ ⑲ ⑳ ⓿ ❶ ❷ ❸ ❹ ❺ ❻ ❼ ❽ ❾ ❿ ⓫ ⓬ ⓭ ⓮ ⓯ ⓰ ⓱ ⓲ ⓳ ⓴ ⑴ ⑵ ⑶ ⑷ ⑸ ⑹ ⑺ ⑻ ⑼ ⑽ ⑾ ⑿ ⒀ ⒁ ⒂ ⒃ ⒄ ⒅ ⒆ ⒇ ⒈ ⒉ ⒊ ⒋ ⒌ ⒍ ⒎ ⒏ ⒐ ⒑ ⒒ ⒓ ⒔ ⒕ ⒖ ⒗ ⒘ ⒙ ⒚ ⒛ Ⅰ Ⅱ Ⅲ Ⅳ Ⅴ Ⅵ Ⅶ Ⅷ Ⅸ Ⅹ Ⅺ Ⅻ ⅰ ⅱ ⅲ ⅳ ⅴ ⅵ ⅶ ⅷ ⅸ ⅹ ⅺ ⅻ ㊀ ㊁ ㊂ ㊃ ㊄ ㊅ ㊆ ㊇ ㊈ ㊉ ㈠ ㈡ ㈢ ㈣ ㈤ ㈥ ㈦ ㈧ ㈨ ㈩" },
    { IBUS_s, "★ ▲ ● ◆ ■ ▼ ◀ ▶ ☻ ☎ ♣ ♥ ♠ ♦ ✦ ☀" },
    { IBUS_t, "㍘ ㏳ ㏠ ㍙ ㍚ ㍛ ㍜ ㍝ ㍞ ㍟ ㍠ ㍡ ㍢ ㍣ ㍤ ㍥ ㍦ ㍧ ㍨ ㍩ ㍪ ㍫ ㍬ ㍭ ㍮ ㍯ ㍰" },
    { IBUS_h, "☆ △ ○ ◇ □ ▽ ▷ ◁ ☼ ☺ ☏ ♧ ♡ ♤ ♢ ✧ ☁ ☂ ☃" },
    { IBUS_g, "Α α Β β Γ γ Δ δ Ε ε Ζ ζ Η η Θ θ Ι ι Κ κ Λ λ Μ μ Ν ν Ξ ξ Ο ο Π π Ρ ρ Σ σ ς Τ τ Υ υ Φ φ Χ χ Ψ ψ Ω ω" },
    { IBUS_p, "ㄅ ㄆ ㄇ ㄈ ㄉ ㄊ ㄋ ㄌ ㄍ ㄎ ㄏ ㄐ ㄑ ㄒ ㄓ ㄔ ㄕ ㄖ ㄗ ㄘ ㄙ ㄧ ㄨ ㄩ ㄚ ㄛ ㄜ ㄝ ㄞ ㄟ ㄠ ㄡ ㄢ ㄣ ㄤ ㄥ ㄦ" },
    { IBUS_1, "！ ﹗ １ 壹 ¹ ₁ 〡" },
    { IBUS_2, "＠ ２ 貳 ² ₂ 〢" },
    { IBUS_3, "＃ ３ 參 ³ ₃ 〣" },
    { IBUS_4, "￥ ＄ ４ 肆 ⁴ ₄ 〤" },
    { IBUS_5, "％ ５ 伍 ⁵ ₅ 〥" },
    { IBUS_6, "＾ ６ 陸 ⁶ ₆ 〦" },
    { IBUS_7, "＆ ７ 柒 ⁷ ₇ 〧" },
    { IBUS_8, "＊ ８ 捌 ⁸ ₈ 〨" },
    { IBUS_9, "（ ９ 玖 ⁹ ₉ 〩" },
    { IBUS_0, "） ０ 零 ⁰ ₀ 〸" },
    { 0, NULL }
};

#endif /* __PUNCTUATION_H__ */
