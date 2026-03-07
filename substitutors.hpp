/// @file substitutors.hpp
/// @brief Text substitution engine for asciiquack.
///
/// Mirrors the Ruby Asciidoctor Substitutors module.
///
/// Substitution pipeline (applied in order for "normal" subs):
///   1. specialcharacters  – escape &, <, >
///   2. quotes             – bold, italic, monospace, …
///   3. attributes         – {name} references
///   4. replacements       – (C), (R), (TM), --, …, '
///   5. macros             – link:, image:, <<xref>>, etc.
///   6. post_replacements  – hard line-break marker (" +")
///
/// Each function is a pure transformation: it takes a string and returns a
/// new string with the substitution applied.

#pragma once

#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace asciiquack {

// ─────────────────────────────────────────────────────────────────────────────
// 1. Special characters
// ─────────────────────────────────────────────────────────────────────────────

/// Escape HTML special characters: & < >
[[nodiscard]] inline std::string sub_specialchars(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 16);
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            default:  out += c;        break;
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Inline quote formatting
// ─────────────────────────────────────────────────────────────────────────────
//
// Constrained  (*word*, _word_, `word`, #word#, ^word^, ~word~)
//   – may not be immediately preceded / followed by a word character
// Unconstrained (**text**, __text__, ``text``, ##text##, ^^text^^, ~~text~~)
//   – no boundary restriction

namespace detail {

/// Replace the first capture group of a regex with open+text+close.
inline std::string apply_quote_rx(
        const std::string& text,
        const std::regex&  rx,
        const std::string& open_tag,
        const std::string& close_tag)
{
    return std::regex_replace(text, rx,
        open_tag + "$1" + close_tag);
}

} // namespace detail

/// Apply inline quote substitutions (*bold*, _italic_, etc.).
[[nodiscard]] inline std::string sub_quotes(const std::string& text) {
    std::string out = text;

    // --- Unconstrained (double markers, no boundary restriction) -------------

    // **strong**
    {
        static const std::regex rx(R"(\*\*(.+?)\*\*)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<strong>$1</strong>");
    }
    // __emphasis__
    {
        static const std::regex rx(R"(__(.+?)__)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<em>$1</em>");
    }
    // ``monospace``
    {
        static const std::regex rx(R"(``(.+?)``)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<code>$1</code>");
    }
    // ##highlight##
    {
        static const std::regex rx(R"(##(.+?)##)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<mark>$1</mark>");
    }
    // ^^superscript^^
    {
        static const std::regex rx(R"(\^\^(.+?)\^\^)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<sup>$1</sup>");
    }
    // ~~subscript~~
    {
        static const std::regex rx(R"(~~(.+?)~~)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<sub>$1</sub>");
    }

    // --- Constrained (single markers, requires non-word boundary) ------------
    //
    // std::regex (ECMAScript) does not support lookbehind, so we capture the
    // character before the opening marker in group $1 and re-emit it.
    // The lookahead (?=[^*\w]|$) for the closing boundary IS supported.

    // *strong*
    {
        static const std::regex rx(
            R"((^|[^*\w])\*(\S|\S.*?\S)\*(?=[^*\w]|$))",
            std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "$1<strong>$2</strong>");
    }
    // _emphasis_
    {
        static const std::regex rx(
            R"((^|[^_\w])_(\S|\S.*?\S)_(?=[^_\w]|$))",
            std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "$1<em>$2</em>");
    }
    // `monospace`
    {
        static const std::regex rx(
            R"((^|[^`\w])`(\S|\S.*?\S)`(?=[^`\w]|$))",
            std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "$1<code>$2</code>");
    }
    // +monospace+ (legacy)
    {
        static const std::regex rx(
            R"((^|[^+\w])\+(\S|\S.*?\S)\+(?=[^+\w]|$))",
            std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "$1<code>$2</code>");
    }
    // #highlight#
    {
        static const std::regex rx(
            R"((^|[^#\w])#(\S|\S.*?\S)#(?=[^#\w]|$))",
            std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "$1<mark>$2</mark>");
    }
    // ^superscript^
    {
        static const std::regex rx(R"(\^(\S+?)\^)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<sup>$1</sup>");
    }
    // ~subscript~
    {
        static const std::regex rx(R"(~(\S+?)~)",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "<sub>$1</sub>");
    }

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Attribute references
// ─────────────────────────────────────────────────────────────────────────────

/// Expand {attribute-name} references using the supplied attribute map.
/// Unknown attributes are left as-is.
[[nodiscard]] inline std::string sub_attributes(
        const std::string&                                    text,
        const std::unordered_map<std::string, std::string>&   attrs)
{
    // Fast path: no opening brace → nothing to do.
    if (text.find('{') == std::string::npos) { return text; }

    static const std::regex rx(R"(\{([\w][\w\-]*)\})",
                                std::regex::ECMAScript | std::regex::optimize);

    std::string out;
    out.reserve(text.size());

    auto begin = std::sregex_iterator(text.begin(), text.end(), rx);
    auto end   = std::sregex_iterator{};

    std::size_t last_pos = 0;
    for (auto it = begin; it != end; ++it) {
        const std::smatch& m = *it;
        // Append text before this match
        out.append(text, last_pos, static_cast<std::size_t>(m.position()) - last_pos);

        const std::string& name = m[1].str();
        auto ai = attrs.find(name);
        if (ai != attrs.end()) {
            out += ai->second;
        } else {
            out += m[0].str();  // leave unknown references intact
        }
        last_pos = static_cast<std::size_t>(m.position()) +
                   static_cast<std::size_t>(m.length());
    }
    out.append(text, last_pos);
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Typographic replacements
// ─────────────────────────────────────────────────────────────────────────────

/// Apply typographic replacements:
///   --          →  em-dash (&#8212;)
///   ...         →  ellipsis (&#8230;&#8203;)
///   (C)/(c)     →  © (&#169;)
///   (R)/(r)     →  ® (&#174;)
///   (TM)/(tm)   →  ™ (&#8482;)
///   '           →  right single quotation mark in certain contexts
[[nodiscard]] inline std::string sub_replacements(const std::string& text) {
    std::string out = text;

    // Em-dash: -- (but not --- or longer runs).
    // Capture one char before/after to avoid lookbehind; re-emit them.
    {
        // Match a non-dash, then --, then a non-dash (handles middle cases)
        static const std::regex rx_mid(R"(([^-])--([^-]))",
                                       std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx_mid, "$1&#8212;&#8203;$2");
        // Match -- at start of string followed by non-dash
        static const std::regex rx_start(R"(^--([^-]))",
                                          std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx_start, "&#8212;&#8203;$1");
        // Match non-dash followed by -- at end of string
        static const std::regex rx_end(R"(([^-])--$)",
                                        std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx_end, "$1&#8212;&#8203;");
    }
    // Ellipsis: ...
    {
        static const std::regex rx(R"(\.\.\.)");
        out = std::regex_replace(out, rx, "&#8230;&#8203;");
    }
    // Copyright
    {
        static const std::regex rx(R"(\([Cc]\))");
        out = std::regex_replace(out, rx, "&#169;");
    }
    // Registered
    {
        static const std::regex rx(R"(\([Rr]\))");
        out = std::regex_replace(out, rx, "&#174;");
    }
    // Trademark
    {
        static const std::regex rx(R"(\([Tt][Mm]\))");
        out = std::regex_replace(out, rx, "&#8482;");
    }
    // Smart apostrophe: word' or 'word
    {
        static const std::regex rx(R"((\w)'(\w))",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, "$1&#8217;$2");
    }

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. Inline macros
// ─────────────────────────────────────────────────────────────────────────────

/// Apply inline macro substitutions (links, images, xrefs, anchors).
[[nodiscard]] inline std::string sub_macros(const std::string& text) {
    std::string out = text;

    // Inline anchor: [[id]] or [[id, reftext]]
    {
        static const std::regex rx(R"(\[\[([A-Za-z_:][A-Za-z0-9_\-.:]*)(?:,\s*([^\]]+))?\]\])",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, R"(<a id="$1"></a>)");
    }

    // Xref: <<id>> or <<id,text>>
    {
        static const std::regex rx(R"(&lt;&lt;([A-Za-z0-9_\-#/.:{]+?)(?:,\s*(.*?))?\s*&gt;&gt;)",
                                   std::regex::ECMAScript | std::regex::optimize);
        // Replace with a link; text defaults to the id.
        std::string after;
        {
            auto begin = std::sregex_iterator(out.begin(), out.end(), rx);
            auto end   = std::sregex_iterator{};
            std::size_t last = 0;
            for (auto it = begin; it != end; ++it) {
                const std::smatch& m = *it;
                after.append(out, last, static_cast<std::size_t>(m.position()) - last);
                const std::string& id   = m[1].str();
                std::string        disp = m[2].matched ? m[2].str() : id;
                after += "<a href=\"#" + id + "\">" + disp + "</a>";
                last = static_cast<std::size_t>(m.position()) +
                       static_cast<std::size_t>(m.length());
            }
            after.append(out, last);
        }
        out = std::move(after);
    }

    // xref: macro form  xref:id[text]
    {
        static const std::regex rx(R"(xref:([A-Za-z0-9_\-#/.:{]+)\[([^\]]*)\])",
                                   std::regex::ECMAScript | std::regex::optimize);
        std::string after;
        {
            auto begin = std::sregex_iterator(out.begin(), out.end(), rx);
            auto end   = std::sregex_iterator{};
            std::size_t last = 0;
            for (auto it = begin; it != end; ++it) {
                const std::smatch& m = *it;
                after.append(out, last, static_cast<std::size_t>(m.position()) - last);
                const std::string& id   = m[1].str();
                std::string        disp = m[2].str().empty() ? id : m[2].str();
                after += "<a href=\"#" + id + "\">" + disp + "</a>";
                last = static_cast<std::size_t>(m.position()) +
                       static_cast<std::size_t>(m.length());
            }
            after.append(out, last);
        }
        out = std::move(after);
    }

    // Explicit link macro: link:url[text]
    // Matches any URL (absolute or relative), not just http/https.
    {
        static const std::regex rx(R"(link:([^\[]+)\[([^\]]*)\])",
                                   std::regex::ECMAScript | std::regex::optimize);
        std::string after;
        {
            auto begin = std::sregex_iterator(out.begin(), out.end(), rx);
            auto end   = std::sregex_iterator{};
            std::size_t last = 0;
            for (auto it = begin; it != end; ++it) {
                const std::smatch& m = *it;
                after.append(out, last, static_cast<std::size_t>(m.position()) - last);
                const std::string& url  = m[1].str();
                std::string        disp = m[2].str().empty() ? url : m[2].str();
                after += "<a href=\"" + url + "\">" + disp + "</a>";
                last = static_cast<std::size_t>(m.position()) +
                       static_cast<std::size_t>(m.length());
            }
            after.append(out, last);
        }
        out = std::move(after);
    }

    // Auto-link bare URLs: https://... or http://...
    // Avoid re-linking already-wrapped anchors by not matching inside href="..."
    // We use a simple approach: match URL not immediately inside a quote.
    {
        static const std::regex rx(R"((https?://[^\s<>\[\]"]+))",
                                   std::regex::ECMAScript | std::regex::optimize);
        // Only replace if not already inside an href attribute
        std::string after;
        auto begin = std::sregex_iterator(out.begin(), out.end(), rx);
        auto end   = std::sregex_iterator{};
        std::size_t last = 0;
        for (auto it = begin; it != end; ++it) {
            const std::smatch& m = *it;
            std::size_t match_pos = static_cast<std::size_t>(m.position());
            // Check that the character before this URL is not a double-quote
            // (which would mean we're inside an href="..." attribute)
            bool in_href = (match_pos > 0 && out[match_pos - 1] == '"');
            after.append(out, last, match_pos - last);
            if (in_href) {
                after += m[0].str();
            } else {
                after += "<a href=\"" + m[1].str() + "\">" + m[1].str() + "</a>";
            }
            last = match_pos + static_cast<std::size_t>(m.length());
        }
        after.append(out, last);
        out = std::move(after);
    }

    // Inline image: image:path[alt]
    {
        static const std::regex rx(R"(image:([^\[]+)\[([^\]]*)\])",
                                   std::regex::ECMAScript | std::regex::optimize);
        out = std::regex_replace(out, rx, R"(<span class="image"><img src="$1" alt="$2"></span>)");
    }

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Post-replacements  (hard line-break)
// ─────────────────────────────────────────────────────────────────────────────

/// Replace trailing " +" with <br>.
[[nodiscard]] inline std::string sub_post_replacements(const std::string& text) {
    static const std::regex rx(R"( \+$)",
                                std::regex::ECMAScript | std::regex::optimize);
    return std::regex_replace(text, rx, "<br>");
}

// ─────────────────────────────────────────────────────────────────────────────
// Combined pipelines
// ─────────────────────────────────────────────────────────────────────────────

/// Apply the "normal" substitution pipeline to inline text:
///   specialcharacters → quotes → attributes → replacements → macros → post
[[nodiscard]] inline std::string apply_normal_subs(
        const std::string&                                    text,
        const std::unordered_map<std::string, std::string>&   attrs)
{
    std::string s = sub_specialchars(text);
    s = sub_quotes(s);
    s = sub_attributes(s, attrs);
    s = sub_replacements(s);
    s = sub_macros(s);
    s = sub_post_replacements(s);
    return s;
}

/// Apply only special-character escaping (for verbatim / listing blocks).
[[nodiscard]] inline std::string apply_verbatim_subs(const std::string& text) {
    return sub_specialchars(text);
}

/// Apply header-level subs: specialcharacters + attributes.
[[nodiscard]] inline std::string apply_header_subs(
        const std::string&                                    text,
        const std::unordered_map<std::string, std::string>&   attrs)
{
    std::string s = sub_specialchars(text);
    return sub_attributes(s, attrs);
}

// ─────────────────────────────────────────────────────────────────────────────
// ID generation helpers  (mirrors Asciidoctor's Section#generate_id)
// ─────────────────────────────────────────────────────────────────────────────

/// Convert a section title to a valid HTML id attribute value.
///
/// Algorithm:
///   1. Lower-case the title.
///   2. Strip any HTML tags.
///   3. Replace sequences of non-word / non-hyphen characters with the
///      id-separator (default "_").
///   4. Prepend the id-prefix   (default "_").
///   5. Strip leading/trailing separators.
[[nodiscard]] inline std::string generate_id(
        const std::string& title,
        const std::string& prefix    = "_",
        const std::string& separator = "_")
{
    // 1. Lowercase
    std::string id;
    id.reserve(title.size());
    for (char c : title) {
        id += static_cast<char>(
            (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c);
    }

    // 2. Strip HTML tags
    {
        static const std::regex tag_rx(R"(<[^>]+>)",
                                       std::regex::ECMAScript | std::regex::optimize);
        id = std::regex_replace(id, tag_rx, "");
    }
    // 3. Strip HTML entities
    {
        static const std::regex ent_rx(R"(&[^;]+;)",
                                       std::regex::ECMAScript | std::regex::optimize);
        id = std::regex_replace(id, ent_rx, "");
    }

    // 4. Replace runs of unwanted characters with the separator
    std::string clean;
    clean.reserve(id.size());
    bool in_sep = false;
    for (char c : id) {
        bool ok = (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '-' || c == '_' || c == '.';
        if (ok) {
            clean += c;
            in_sep = false;
        } else {
            if (!in_sep && !clean.empty()) {
                clean += separator;
                in_sep = true;
            }
        }
    }
    // Trim trailing separator
    while (!clean.empty() && clean.back() == separator[0]) {
        clean.pop_back();
    }

    return prefix + clean;
}

} // namespace asciiquack
