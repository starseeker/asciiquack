/// @file html5.hpp
/// @brief HTML 5 converter for asciiquack.
///
/// Walks the Document AST and produces an HTML 5 string that is compatible
/// with the default output of the Ruby Asciidoctor html5 backend.
///
/// The converter is a pure function: it takes a const Document& and returns
/// a std::string.  All state is held on the call stack.

#pragma once

#include "document.hpp"
#include "substitutors.hpp"

#include <sstream>
#include <string>
#include <unordered_map>

namespace asciiquack {

// ─────────────────────────────────────────────────────────────────────────────
// Html5Converter
// ─────────────────────────────────────────────────────────────────────────────

class Html5Converter {
public:
    Html5Converter() = default;

    /// Convert a parsed Document to an HTML 5 string.
    [[nodiscard]] std::string convert(const Document& doc) const {
        std::ostringstream out;
        convert_document(doc, out);
        return out.str();
    }

private:
    // ── Shorthand helpers ─────────────────────────────────────────────────────

    /// Apply normal substitutions to inline text.
    [[nodiscard]] static std::string subs(const std::string& text,
                                          const Document&    doc) {
        return apply_normal_subs(text, doc.attributes());
    }

    /// Escape HTML characters only (for code / literal content).
    [[nodiscard]] static std::string verbatim(const std::string& text) {
        return apply_verbatim_subs(text);
    }

    // ── Document structure ────────────────────────────────────────────────────

    void convert_document(const Document& doc, std::ostringstream& out) const {
        bool embedded = doc.has_attr("embedded");

        if (!embedded) {
            const std::string& encoding = doc.attr("encoding", "UTF-8");
            const std::string& lang     = doc.attr("lang",     "en");

            out << "<!DOCTYPE html>\n";
            out << "<html lang=\"" << lang << "\">\n";
            out << "<head>\n";
            out << "<meta charset=\"" << encoding << "\">\n";
            out << "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n";
            out << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";

            // Generator meta (omit when reproducible is set)
            if (!doc.has_attr("reproducible")) {
                out << "<meta name=\"generator\" content=\"asciiquack "
                    << doc.attr("asciidoctor-version", "0.1.0") << "\">\n";
            }

            // Optional meta tags
            if (doc.has_attr("description")) {
                out << "<meta name=\"description\" content=\""
                    << doc.attr("description") << "\">\n";
            }
            if (doc.has_attr("keywords")) {
                out << "<meta name=\"keywords\" content=\""
                    << doc.attr("keywords") << "\">\n";
            }
            if (doc.has_attr("authors")) {
                out << "<meta name=\"author\" content=\""
                    << doc.attr("authors") << "\">\n";
            }

            // Title
            std::string page_title = doc.doctitle();
            if (page_title.empty()) { page_title = doc.attr("untitled-label", "Untitled"); }
            out << "<title>" << sub_specialchars(page_title) << "</title>\n";

            // Inline default stylesheet (always included unless linkcss is set)
            if (!doc.has_attr("linkcss")) {
                out << "<style>\n"
                    << default_css()
                    << "</style>\n";
            }

            out << "</head>\n";

            const std::string& doctype = doc.doctype();
            out << "<body class=\"" << doctype << "\">\n";

            // Header div
            out << "<div id=\"header\">\n";
            if (!doc.doctitle().empty()) {
                out << "<h1>" << subs(doc.doctitle(), doc) << "</h1>\n";
            }

            const auto& authors = doc.authors();
            if (!authors.empty()) {
                out << "<div class=\"details\">\n";
                for (std::size_t i = 0; i < authors.size(); ++i) {
                    const auto& a    = authors[i];
                    std::string sfx  = (i == 0) ? "" : std::to_string(i + 1);
                    out << "<span id=\"author"  << sfx << "\" class=\"author\">"
                        << sub_specialchars(a.fullname()) << "</span>";
                    if (!a.email.empty()) {
                        out << "<br>\n"
                            << "<span id=\"email" << sfx << "\" class=\"email\">"
                            << "<a href=\"mailto:" << a.email << "\">"
                            << sub_specialchars(a.email) << "</a></span>";
                    }
                    out << "<br>\n";
                }

                const auto& rev = doc.revision();
                if (!rev.number.empty()) {
                    out << "<span id=\"revnumber\">version " << sub_specialchars(rev.number);
                    if (!rev.date.empty()) { out << ","; }
                    out << "</span>\n";
                }
                if (!rev.date.empty()) {
                    out << "<span id=\"revdate\">" << sub_specialchars(rev.date) << "</span>\n";
                }
                if (!rev.remark.empty()) {
                    out << "<br>\n"
                        << "<span id=\"revremark\">" << sub_specialchars(rev.remark) << "</span>\n";
                }
                out << "</div>\n";
            }
            out << "</div>\n";  // #header

            // TOC placement: auto (in header) or preamble (after preamble)
            if (doc.has_attr("toc")) {
                const std::string& placement =
                    doc.attr("toc-placement", "auto");
                if (placement == "auto" || placement.empty()) {
                    convert_toc(doc, out);
                }
            }

            out << "<div id=\"content\">\n";
        }

        // ── Preamble ────────────────────────────────────────────────────────────
        // Blocks that appear before the first section are the preamble.
        std::vector<const Block*> preamble_blocks;
        std::vector<const Block*> section_blocks;

        for (const auto& child : doc.blocks()) {
            if (child->context() == BlockContext::Section) {
                section_blocks.push_back(child.get());
            } else if (section_blocks.empty()) {
                preamble_blocks.push_back(child.get());
            } else {
                section_blocks.push_back(child.get());
            }
        }

        if (!preamble_blocks.empty()) {
            out << "<div id=\"preamble\">\n";
            out << "<div class=\"sectionbody\">\n";
            for (const Block* b : preamble_blocks) {
                convert_block(*b, doc, out);
            }
            out << "</div>\n";  // sectionbody
            out << "</div>\n";  // #preamble

            // TOC after preamble (toc-placement: preamble)
            if (!embedded && doc.has_attr("toc")) {
                const std::string& placement = doc.attr("toc-placement", "auto");
                if (placement == "preamble") {
                    convert_toc(doc, out);
                }
            }
        }

        for (const Block* b : section_blocks) {
            convert_block(*b, doc, out);
        }

        if (!embedded) {
            out << "</div>\n";  // #content

            out << "<div id=\"footer\">\n";
            out << "<div id=\"footer-text\">\n";
            const auto& rev = doc.revision();
            if (!rev.number.empty()) {
                out << "Version " << sub_specialchars(rev.number) << "<br>\n";
            }
            out << "</div>\n";  // #footer-text
            out << "</div>\n";  // #footer
            out << "</body>\n";
            out << "</html>\n";
        }
    }

    // ── Generic block dispatcher ───────────────────────────────────────────────

    void convert_block(const Block& block, const Document& doc,
                       std::ostringstream& out) const {
        switch (block.context()) {
            case BlockContext::Section:
                convert_section(dynamic_cast<const Section&>(block), doc, out);
                break;
            case BlockContext::Paragraph:
                convert_paragraph(block, doc, out);
                break;
            case BlockContext::Listing:
                convert_listing(block, doc, out);
                break;
            case BlockContext::Literal:
                convert_literal(block, doc, out);
                break;
            case BlockContext::Example:
                convert_example(block, doc, out);
                break;
            case BlockContext::Quote:
                convert_quote(block, doc, out, false);
                break;
            case BlockContext::Verse:
                convert_quote(block, doc, out, true);
                break;
            case BlockContext::Sidebar:
                convert_sidebar(block, doc, out);
                break;
            case BlockContext::Admonition:
                convert_admonition(block, doc, out);
                break;
            case BlockContext::Ulist:
                convert_ulist(dynamic_cast<const List&>(block), doc, out);
                break;
            case BlockContext::Olist:
                convert_olist(dynamic_cast<const List&>(block), doc, out);
                break;
            case BlockContext::Dlist:
                convert_dlist(dynamic_cast<const List&>(block), doc, out);
                break;
            case BlockContext::Colist:
                convert_colist(dynamic_cast<const List&>(block), doc, out);
                break;
            case BlockContext::Table:
                convert_table(dynamic_cast<const Table&>(block), doc, out);
                break;
            case BlockContext::Image:
                convert_image(block, doc, out);
                break;
            case BlockContext::Video:
                convert_video(block, doc, out);
                break;
            case BlockContext::Audio:
                convert_audio(block, doc, out);
                break;
            case BlockContext::FloatingTitle:
                convert_floating_title(block, doc, out);
                break;
            case BlockContext::Toc:
                convert_toc(doc, out);
                break;
            case BlockContext::Pass:
                // Raw passthrough: emit content without any substitutions
                out << block.source() << '\n';
                break;
            case BlockContext::ThematicBreak:
                out << "<hr>\n";
                break;
            case BlockContext::PageBreak:
                out << "<div style=\"page-break-after: always;\"></div>\n";
                break;
            default:
                // Recurse into compound children
                for (const auto& child : block.blocks()) {
                    convert_block(*child, doc, out);
                }
                break;
        }
    }

    // ── Section ───────────────────────────────────────────────────────────────

    void convert_section(const Section& sect, const Document& doc,
                         std::ostringstream& out) const {
        int level = sect.level();  // 0 = document title level, 1 = h2, …
        // level 1 (==) → class="sect1", <h2>
        // level 2 (===) → class="sect2", <h3>
        // etc.
        int depth = level;
        std::string tag = "h" + std::to_string(std::min(level + 1, 6));
        std::string css = "sect" + std::to_string(depth);

        out << "<div class=\"" << css << "\">\n";

        const std::string& id = sect.id();

        // Build title text with optional section number prefix
        std::string title_html;
        if (sect.numbered() && !sect.sectnum_string().empty()) {
            title_html = "<span class=\"sectnum\">"
                       + sect.sectnum_string()
                       + "</span> "
                       + subs(sect.title(), doc);
        } else {
            title_html = subs(sect.title(), doc);
        }

        if (!id.empty()) {
            out << "<" << tag << " id=\"" << id << "\">"
                << title_html
                << "</" << tag << ">\n";
        } else {
            out << "<" << tag << ">"
                << title_html
                << "</" << tag << ">\n";
        }

        out << "<div class=\"sectionbody\">\n";
        for (const auto& child : sect.blocks()) {
            convert_block(*child, doc, out);
        }
        out << "</div>\n";  // sectionbody
        out << "</div>\n";  // sect<N>
    }

    // ── Paragraph ─────────────────────────────────────────────────────────────

    void convert_paragraph(const Block& block, const Document& doc,
                           std::ostringstream& out) const {
        std::string role = block.role();
        out << "<div class=\"paragraph";
        if (!role.empty()) { out << " " << role; }
        out << "\">\n";

        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }

        out << "<p>" << subs(block.source(), doc) << "</p>\n";
        out << "</div>\n";
    }

    // ── Listing / source block ─────────────────────────────────────────────────

    void convert_listing(const Block& block, const Document& doc,
                         std::ostringstream& out) const {
        const std::string& style = block.style();
        // Language: explicit "language" attr takes precedence, then the second
        // positional attribute (set by [source,<lang>] block attribute lines).
        const std::string& lang_explicit = block.attr("language");
        const std::string& lang_pos2     = block.attr("2");
        const std::string& language = lang_explicit.empty() ? lang_pos2 : lang_explicit;
        bool is_source = (style == "source") || !language.empty();

        out << "<div class=\"listingblock\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }
        out << "<div class=\"content\">\n";

        if (is_source && !language.empty()) {
            out << "<pre class=\"highlight\"><code class=\"language-" << language
                << "\" data-lang=\"" << language << "\">"
                << verbatim(block.source())
                << "</code></pre>\n";
        } else {
            out << "<pre>" << verbatim(block.source()) << "</pre>\n";
        }

        out << "</div>\n";  // content
        out << "</div>\n";  // listingblock
    }

    // ── Literal block ──────────────────────────────────────────────────────────

    void convert_literal(const Block& block, const Document& doc,
                         std::ostringstream& out) const {
        out << "<div class=\"literalblock\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }
        out << "<div class=\"content\">\n"
            << "<pre>" << verbatim(block.source()) << "</pre>\n"
            << "</div>\n"
            << "</div>\n";
    }

    // ── Example block ──────────────────────────────────────────────────────────

    void convert_example(const Block& block, const Document& doc,
                         std::ostringstream& out) const {
        out << "<div class=\"exampleblock\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }
        out << "<div class=\"content\">\n";
        for (const auto& child : block.blocks()) {
            convert_block(*child, doc, out);
        }
        out << "</div>\n"
            << "</div>\n";
    }

    // ── Quote / verse ──────────────────────────────────────────────────────────

    void convert_quote(const Block& block, const Document& doc,
                       std::ostringstream& out, bool verse) const {
        const std::string& attribution = block.attr("attribution");
        const std::string& citetitle   = block.attr("citetitle");

        out << "<div class=\"" << (verse ? "verseblock" : "quoteblock") << "\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }

        out << "<blockquote>\n";

        if (verse) {
            out << "<pre class=\"content\">" << subs(block.source(), doc) << "</pre>\n";
        } else {
            for (const auto& child : block.blocks()) {
                convert_block(*child, doc, out);
            }
        }

        out << "</blockquote>\n";

        if (!attribution.empty() || !citetitle.empty()) {
            out << "<div class=\"attribution\">\n";
            if (!attribution.empty()) {
                out << "&#8212; " << subs(attribution, doc);
            }
            if (!citetitle.empty()) {
                if (!attribution.empty()) { out << "<br>\n"; }
                out << "<cite>" << subs(citetitle, doc) << "</cite>";
            }
            out << "\n</div>\n";
        }
        out << "</div>\n";
    }

    // ── Sidebar ────────────────────────────────────────────────────────────────

    void convert_sidebar(const Block& block, const Document& doc,
                         std::ostringstream& out) const {
        out << "<div class=\"sidebarblock\">\n"
            << "<div class=\"content\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }
        for (const auto& child : block.blocks()) {
            convert_block(*child, doc, out);
        }
        out << "</div>\n"
            << "</div>\n";
    }

    // ── Admonition ────────────────────────────────────────────────────────────

    void convert_admonition(const Block& block, const Document& doc,
                            std::ostringstream& out) const {
        const std::string& name    = block.attr("name",    "note");
        const std::string& caption = block.attr("caption", "Note");

        out << "<div class=\"admonitionblock " << name << "\">\n"
            << "<table><tr>\n"
            << "<td class=\"icon\">\n"
            << "<div class=\"title\">" << caption << "</div>\n"
            << "</td>\n"
            << "<td class=\"content\">\n";

        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }

        if (block.content_model() == ContentModel::Simple) {
            out << subs(block.source(), doc) << "\n";
        } else {
            for (const auto& child : block.blocks()) {
                convert_block(*child, doc, out);
            }
        }

        out << "</td>\n"
            << "</tr></table>\n"
            << "</div>\n";
    }

    // ── Unordered list ────────────────────────────────────────────────────────

    void convert_ulist(const List& list, const Document& doc,
                       std::ostringstream& out) const {
        const std::string& role  = list.role();
        const std::string& style = list.style();  // checklist, etc.

        out << "<div class=\"ulist";
        if (!style.empty()) { out << " " << style; }
        if (!role.empty())  { out << " " << role; }
        out << "\">\n";

        if (list.has_title()) {
            out << "<div class=\"title\">" << subs(list.title(), doc) << "</div>\n";
        }

        out << "<ul>\n";
        for (const auto& item : list.items()) {
            out << "<li>\n<p>" << subs(item->source(), doc) << "</p>\n";
            // Sub-list
            for (const auto& child : item->blocks()) {
                convert_block(*child, doc, out);
            }
            out << "</li>\n";
        }
        out << "</ul>\n"
            << "</div>\n";
    }

    // ── Ordered list ──────────────────────────────────────────────────────────

    void convert_olist(const List& list, const Document& doc,
                       std::ostringstream& out) const {
        // Determine CSS class from ordered_style
        std::string style_class;
        switch (list.ordered_style()) {
            case OrderedListStyle::Arabic:     style_class = "arabic";     break;
            case OrderedListStyle::LowerAlpha: style_class = "loweralpha"; break;
            case OrderedListStyle::UpperAlpha: style_class = "upperalpha"; break;
            case OrderedListStyle::LowerRoman: style_class = "lowerroman"; break;
            case OrderedListStyle::UpperRoman: style_class = "upperroman"; break;
        }

        const std::string& role = list.role();
        out << "<div class=\"olist " << style_class;
        if (!role.empty()) { out << " " << role; }
        out << "\">\n";

        if (list.has_title()) {
            out << "<div class=\"title\">" << subs(list.title(), doc) << "</div>\n";
        }

        out << "<ol class=\"" << style_class << "\">\n";
        for (const auto& item : list.items()) {
            out << "<li>\n<p>" << subs(item->source(), doc) << "</p>\n";
            for (const auto& child : item->blocks()) {
                convert_block(*child, doc, out);
            }
            out << "</li>\n";
        }
        out << "</ol>\n"
            << "</div>\n";
    }

    // ── Description list ──────────────────────────────────────────────────────

    void convert_dlist(const List& list, const Document& doc,
                       std::ostringstream& out) const {
        out << "<div class=\"dlist\">\n";
        if (list.has_title()) {
            out << "<div class=\"title\">" << subs(list.title(), doc) << "</div>\n";
        }
        out << "<dl>\n";
        for (const auto& item : list.items()) {
            out << "<dt class=\"hdlist1\">" << subs(item->term(), doc) << "</dt>\n";
            if (!item->source().empty()) {
                out << "<dd>\n<p>" << subs(item->source(), doc) << "</p>\n</dd>\n";
            } else {
                out << "<dd></dd>\n";
            }
        }
        out << "</dl>\n"
            << "</div>\n";
    }

    // ── Callout list ──────────────────────────────────────────────────────────

    void convert_colist(const List& list, const Document& doc,
                        std::ostringstream& out) const {
        out << "<div class=\"colist arabic\">\n";
        out << "<ol>\n";
        for (const auto& item : list.items()) {
            out << "<li>\n<p>" << subs(item->source(), doc) << "</p>\n</li>\n";
        }
        out << "</ol>\n"
            << "</div>\n";
    }

    // ── Table ──────────────────────────────────────────────────────────────────

    void convert_table(const Table& table, const Document& doc,
                       std::ostringstream& out) const {
        const std::string& role = table.role();

        out << "<table class=\"tableblock frame-all grid-all stretch";
        if (!role.empty()) { out << " " << role; }
        out << "\">\n";

        if (table.has_title()) {
            out << "<caption class=\"title\">" << subs(table.title(), doc) << "</caption>\n";
        }

        // Colgroup
        const auto& col_specs = table.column_specs();
        if (!col_specs.empty()) {
            out << "<colgroup>\n";
            for (const auto& cs : col_specs) {
                out << "<col style=\"width: " << cs.width << "%\">\n";
            }
            out << "</colgroup>\n";
        }

        // thead
        if (table.has_header()) {
            out << "<thead>\n";
            for (const auto& row : table.head_rows()) {
                out << "<tr>\n";
                for (const auto& cell : row.cells()) {
                    out << "<th class=\"tableblock halign-left valign-top\">"
                        << subs(cell->source(), doc) << "</th>\n";
                }
                out << "</tr>\n";
            }
            out << "</thead>\n";
        }

        // tbody
        if (!table.body_rows().empty()) {
            out << "<tbody>\n";
            for (const auto& row : table.body_rows()) {
                out << "<tr>\n";
                for (const auto& cell : row.cells()) {
                    out << "<td class=\"tableblock halign-left valign-top\">"
                        << "<p class=\"tableblock\">"
                        << subs(cell->source(), doc)
                        << "</p></td>\n";
                }
                out << "</tr>\n";
            }
            out << "</tbody>\n";
        }

        // tfoot
        if (table.has_footer()) {
            out << "<tfoot>\n";
            for (const auto& row : table.foot_rows()) {
                out << "<tr>\n";
                for (const auto& cell : row.cells()) {
                    out << "<td class=\"tableblock halign-left valign-top\">"
                        << "<p class=\"tableblock\">"
                        << subs(cell->source(), doc)
                        << "</p></td>\n";
                }
                out << "</tr>\n";
            }
            out << "</tfoot>\n";
        }

        out << "</table>\n";
    }

    // ── Image block ────────────────────────────────────────────────────────────

    void convert_image(const Block& block, const Document& doc,
                       std::ostringstream& out) const {
        const std::string& target = block.attr("target");
        const std::string& alt    = block.attr("alt");
        const std::string& width  = block.attr("width");
        const std::string& height = block.attr("height");
        const std::string& role   = block.role();

        out << "<div class=\"imageblock";
        if (!role.empty()) { out << " " << role; }
        out << "\">\n"
            << "<div class=\"content\">\n"
            << "<img src=\"" << target << "\" alt=\"" << sub_specialchars(alt) << "\"";

        if (!width.empty())  { out << " width=\""  << width  << "\""; }
        if (!height.empty()) { out << " height=\"" << height << "\""; }

        out << ">\n"
            << "</div>\n";

        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }

        out << "</div>\n";
    }

    // ── Video block ────────────────────────────────────────────────────────────

    void convert_video(const Block& block, const Document& doc,
                       std::ostringstream& out) const {
        const std::string& target = block.attr("target");
        const std::string& width  = block.attr("width");
        const std::string& height = block.attr("height");
        const std::string& role   = block.role();

        out << "<div class=\"videoblock";
        if (!role.empty()) { out << " " << role; }
        out << "\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }
        out << "<div class=\"content\">\n"
            << "<video src=\"" << sub_specialchars(target) << "\"";
        if (!width.empty())  { out << " width=\""  << width  << "\""; }
        if (!height.empty()) { out << " height=\"" << height << "\""; }
        if (block.has_attr("autoplay")) { out << " autoplay"; }
        if (block.has_attr("loop"))     { out << " loop"; }
        if (block.has_attr("nocontrols")) {
            // nocontrols: suppress default controls
        } else {
            out << " controls";
        }
        out << ">Your browser does not support the video tag.</video>\n"
            << "</div>\n"
            << "</div>\n";
    }

    // ── Audio block ────────────────────────────────────────────────────────────

    void convert_audio(const Block& block, const Document& doc,
                       std::ostringstream& out) const {
        const std::string& target = block.attr("target");
        const std::string& role   = block.role();

        out << "<div class=\"audioblock";
        if (!role.empty()) { out << " " << role; }
        out << "\">\n";
        if (block.has_title()) {
            out << "<div class=\"title\">" << subs(block.title(), doc) << "</div>\n";
        }
        out << "<div class=\"content\">\n"
            << "<audio src=\"" << sub_specialchars(target) << "\"";
        if (block.has_attr("autoplay")) { out << " autoplay"; }
        if (block.has_attr("loop"))     { out << " loop"; }
        if (!block.has_attr("nocontrols")) { out << " controls"; }
        out << ">Your browser does not support the audio tag.</audio>\n"
            << "</div>\n"
            << "</div>\n";
    }

    // ── Floating title ─────────────────────────────────────────────────────────

    void convert_floating_title(const Block& block, const Document& doc,
                                std::ostringstream& out) const {
        int level = 1;
        const std::string& lv_str = block.attr("level");
        if (!lv_str.empty()) {
            try { level = std::stoi(lv_str); } catch (...) {}
        }
        std::string tag = "h" + std::to_string(std::min(level + 1, 6));
        const std::string& id = block.id();

        out << "<" << tag << " class=\"discrete\"";
        if (!id.empty()) { out << " id=\"" << id << "\""; }
        out << ">" << subs(block.source(), doc) << "</" << tag << ">\n";
    }

    // ── Table of Contents ──────────────────────────────────────────────────────

    void convert_toc(const Document& doc, std::ostringstream& out) const {
        const auto& entries = doc.toc_entries();
        if (entries.empty()) { return; }

        int toclevels = 2;
        if (doc.has_attr("toclevels")) {
            try { toclevels = std::stoi(doc.attr("toclevels")); } catch (...) {}
        }

        std::string toc_title = doc.attr("toc-title", "Table of Contents");

        out << "<div id=\"toc\" class=\"toc\">\n"
            << "<div id=\"toctitle\">" << sub_specialchars(toc_title) << "</div>\n";

        // We maintain a stack of open list levels.
        // open_li[lv] = true means a <li> at that level has been opened but not yet
        // closed with </li>.  This lets us insert a nested <ul> inside it.
        static constexpr int MAX_LEVEL = 7;
        std::array<bool, MAX_LEVEL> open_li{};
        int list_depth = 0;  // deepest open <ul> level

        for (const auto& entry : entries) {
            if (entry.level < 1 || entry.level > toclevels) { continue; }
            int lv = entry.level;

            // ── Close deeper levels first ────────────────────────────────────
            while (list_depth > lv) {
                // Close the innermost open <li>, then the <ul>
                if (list_depth < MAX_LEVEL && open_li[list_depth]) {
                    out << "</li>\n";
                    open_li[list_depth] = false;
                }
                out << "</ul>\n";
                --list_depth;
            }

            // ── Open lists up to the required level ──────────────────────────
            while (list_depth < lv) {
                ++list_depth;
                out << "<ul class=\"sectlevel" << list_depth << "\">\n";
            }

            // ── Close the previous item at this level (if any) ───────────────
            if (lv < MAX_LEVEL && open_li[lv]) {
                out << "</li>\n";
                open_li[lv] = false;
            }

            // ── Emit the entry ────────────────────────────────────────────────
            std::string title_text = entry.title;
            if (!entry.sectnum.empty()) {
                title_text = entry.sectnum + " " + title_text;
            }
            out << "<li><a href=\"#" << entry.id << "\">"
                << sub_specialchars(title_text)
                << "</a>";
            if (lv < MAX_LEVEL) { open_li[lv] = true; }
        }

        // ── Close everything that's still open ────────────────────────────────
        while (list_depth >= 1) {
            if (list_depth < MAX_LEVEL && open_li[list_depth]) {
                out << "</li>\n";
                open_li[list_depth] = false;
            }
            out << "</ul>\n";
            --list_depth;
        }

        out << "</div>\n";
    }

    // ── Minimal default CSS ────────────────────────────────────────────────────

    /// Returns a minimal stylesheet sufficient for basic readability.
    /// A production build would use the full Asciidoctor stylesheet or link
    /// to an external file.
    [[nodiscard]] static std::string default_css() {
        return
            "/* asciiquack default stylesheet (minimal) */\n"
            "body{font-family:sans-serif;max-width:900px;margin:0 auto;padding:1em 2em;"
              "line-height:1.6;color:#333}\n"
            "h1,h2,h3,h4,h5,h6{color:#2c5282;line-height:1.2;margin-top:1.4em}\n"
            "h1{font-size:2em;border-bottom:2px solid #2c5282;padding-bottom:.25em}\n"
            "h2{font-size:1.6em;border-bottom:1px solid #ccc}\n"
            "code,pre{font-family:monospace;background:#f5f5f5;border-radius:3px}\n"
            "pre{padding:1em;overflow:auto;border-left:4px solid #ccc}\n"
            "code{padding:.1em .3em}\n"
            ".listingblock pre{background:#282c34;color:#abb2bf;border:none}\n"
            "table{border-collapse:collapse;width:100%}\n"
            "td,th{border:1px solid #ddd;padding:.5em .75em;text-align:left}\n"
            "th{background:#f0f0f0;font-weight:bold}\n"
            ".admonitionblock table{border:none;width:100%}\n"
            ".admonitionblock td.icon{width:80px;font-weight:bold;text-transform:uppercase}\n"
            ".admonitionblock td.content{padding:.5em 1em}\n"
            ".admonitionblock.note td.icon{color:#31708f}\n"
            ".admonitionblock.tip td.icon{color:#3c763d}\n"
            ".admonitionblock.warning td.icon{color:#8a6d3b}\n"
            ".admonitionblock.important td.icon{color:#a94442}\n"
            ".admonitionblock.caution td.icon{color:#a94442}\n"
            "blockquote{border-left:4px solid #ccc;padding:.5em 1em;margin:1em 0;"
              "color:#555;font-style:italic}\n"
            ".sidebarblock{background:#f5f5f5;border:1px solid #ddd;border-radius:4px;"
              "padding:1em;margin:1em 0}\n"
            "#header,#content,#footer{margin-bottom:1em}\n"
            "#footer{border-top:1px solid #ccc;padding-top:.5em;color:#666;font-size:.9em}\n"
            "#header .details{font-size:.9em;color:#666}\n"
            "mark{background:#ff9}\n"
            ".exampleblock{border:1px solid #ddd;border-radius:4px;padding:1em;margin:1em 0}\n"
            "img{max-width:100%;height:auto}\n"
            ".imageblock{text-align:center;margin:1em 0}\n"
            ".imageblock .title{font-style:italic;color:#666;margin-top:.5em}\n"
            "video,audio{max-width:100%}\n"
            ".videoblock,.audioblock{margin:1em 0}\n"
            ".sectnum{margin-right:.25em}\n"
            "#toc{background:#f8f8f8;border:1px solid #ddd;border-radius:4px;"
              "padding:1em 1.5em;margin:1em 0;display:inline-block;min-width:200px}\n"
            "#toctitle{font-weight:bold;margin-bottom:.5em}\n"
            "#toc ul{list-style:none;padding-left:0;margin:0}\n"
            "#toc ul ul{padding-left:1.5em}\n"
            "#toc a{text-decoration:none;color:#2c5282}\n"
            "#toc a:hover{text-decoration:underline}\n"
            "#toc li{margin:.25em 0}\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Free function convenience wrapper
// ─────────────────────────────────────────────────────────────────────────────

/// Convert a parsed Document to HTML 5.
[[nodiscard]] inline std::string convert_to_html5(const Document& doc) {
    return Html5Converter{}.convert(doc);
}

} // namespace asciiquack
