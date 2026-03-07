/// @file test_asciiquack.cpp
/// @brief C++ unit tests for the asciiquack AsciiDoc processor.
///
/// Tests are self-contained: no external framework is required.
/// Each test is a function that asserts a condition; on failure it prints
/// a message and increments the failure counter.
///
/// Run with:   ./asciiquack_tests
/// CMake:      ctest  (via `add_test`)

#include "document.hpp"
#include "html5.hpp"
#include "manpage.hpp"
#include "parser.hpp"
#include "reader.hpp"
#include "substitutors.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Minimal test harness
// ─────────────────────────────────────────────────────────────────────────────

static int  g_total   = 0;
static int  g_failed  = 0;
static bool g_verbose = false;

#define EXPECT(cond)                                                        \
    do {                                                                    \
        ++g_total;                                                          \
        if (!(cond)) {                                                      \
            ++g_failed;                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__         \
                      << "  " << #cond << "\n";                             \
        } else if (g_verbose) {                                             \
            std::cout << "  PASS: " << #cond << "\n";                      \
        }                                                                   \
    } while (false)

#define EXPECT_EQ(a, b)                                                     \
    do {                                                                    \
        ++g_total;                                                          \
        if ((a) != (b)) {                                                   \
            ++g_failed;                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__         \
                      << "  expected\n    [" << (a)                         \
                      << "]\n  got\n    [" << (b) << "]\n";                 \
        } else if (g_verbose) {                                             \
            std::cout << "  PASS: " << #a << " == " << #b << "\n";         \
        }                                                                   \
    } while (false)

#define EXPECT_CONTAINS(haystack, needle)                                   \
    do {                                                                    \
        ++g_total;                                                          \
        if ((haystack).find(needle) == std::string::npos) {                 \
            ++g_failed;                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__         \
                      << "  expected to find [" << (needle)                 \
                      << "] in output\n";                                   \
        } else if (g_verbose) {                                             \
            std::cout << "  PASS: output contains [" << (needle) << "]\n"; \
        }                                                                   \
    } while (false)

#define EXPECT_NOT_CONTAINS(haystack, needle)                               \
    do {                                                                    \
        ++g_total;                                                          \
        if ((haystack).find(needle) != std::string::npos) {                 \
            ++g_failed;                                                     \
            std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__         \
                      << "  expected NOT to find [" << (needle)             \
                      << "] in output\n";                                   \
        } else if (g_verbose) {                                             \
            std::cout << "  PASS: output lacks [" << (needle) << "]\n";    \
        }                                                                   \
    } while (false)

static void begin_test(const char* name) {
    std::cout << "  " << name << " ... " << std::flush;
}
static void end_test() {
    std::cout << "ok\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: parse a string and return the HTML output
// ─────────────────────────────────────────────────────────────────────────────

static std::string html(const std::string& asciidoc,
                        asciiquack::ParseOptions opts = {}) {
    opts.safe_mode = asciiquack::SafeMode::Unsafe;
    auto doc = asciiquack::Parser::parse_string(asciidoc, opts);
    return asciiquack::convert_to_html5(*doc);
}

// ─────────────────────────────────────────────────────────────────────────────
// Reader tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_reader_basic() {
    begin_test("reader: basic line reading");

    asciiquack::Reader r("line1\nline2\nline3\n");
    EXPECT(r.has_more_lines());

    auto l1 = r.read_line();
    EXPECT(l1.has_value());
    EXPECT_EQ(*l1, "line1");

    EXPECT_EQ(r.lineno(), 2);

    auto l2 = r.peek_line();
    EXPECT(l2.has_value());
    EXPECT_EQ(std::string(*l2), "line2");

    // peek does not consume
    EXPECT_EQ(std::string(*r.peek_line()), "line2");

    auto l2r = r.read_line();
    EXPECT_EQ(*l2r, "line2");

    r.skip_line();  // line3
    EXPECT(!r.has_more_lines());
    EXPECT(!r.read_line().has_value());

    end_test();
}

static void test_reader_crlf() {
    begin_test("reader: CRLF line endings");

    asciiquack::Reader r("a\r\nb\r\nc\r\n");
    EXPECT_EQ(*r.read_line(), "a");
    EXPECT_EQ(*r.read_line(), "b");
    EXPECT_EQ(*r.read_line(), "c");

    end_test();
}

static void test_reader_unshift() {
    begin_test("reader: unshift_line");

    asciiquack::Reader r("b\n");
    r.unshift_line("a");
    EXPECT_EQ(*r.read_line(), "a");
    EXPECT_EQ(*r.read_line(), "b");

    end_test();
}

static void test_reader_skip_blank() {
    begin_test("reader: skip_blank_lines");

    asciiquack::Reader r("\n  \n\nfoo\n");
    int skipped = r.skip_blank_lines();
    EXPECT(skipped >= 2);
    EXPECT_EQ(*r.read_line(), "foo");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// Substitutors tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_sub_specialchars() {
    begin_test("substitutors: escape HTML special chars");

    EXPECT_EQ(asciiquack::sub_specialchars("a & b"), "a &amp; b");
    EXPECT_EQ(asciiquack::sub_specialchars("<tag>"),  "&lt;tag&gt;");
    EXPECT_EQ(asciiquack::sub_specialchars("a>b"),    "a&gt;b");
    EXPECT_EQ(asciiquack::sub_specialchars("plain"),  "plain");

    end_test();
}

static void test_sub_replacements() {
    begin_test("substitutors: typographic replacements");

    std::string em = asciiquack::sub_replacements("a -- b");
    EXPECT(em.find("&#8212;") != std::string::npos);

    std::string ellipsis = asciiquack::sub_replacements("...");
    EXPECT(ellipsis.find("&#8230;") != std::string::npos);

    std::string copyright = asciiquack::sub_replacements("(C)");
    EXPECT(copyright.find("&#169;") != std::string::npos);

    std::string tm = asciiquack::sub_replacements("(TM)");
    EXPECT(tm.find("&#8482;") != std::string::npos);

    end_test();
}

static void test_sub_attributes() {
    begin_test("substitutors: attribute expansion");

    std::unordered_map<std::string, std::string> attrs = {
        {"project", "asciiquack"},
        {"version", "1.0"},
    };

    EXPECT_EQ(asciiquack::sub_attributes("name: {project}", attrs), "name: asciiquack");
    EXPECT_EQ(asciiquack::sub_attributes("{version}", attrs), "1.0");
    // Unknown attribute is left as-is
    EXPECT_EQ(asciiquack::sub_attributes("{unknown}", attrs), "{unknown}");
    // No brace → fast path
    EXPECT_EQ(asciiquack::sub_attributes("hello", attrs), "hello");

    end_test();
}

static void test_generate_id() {
    begin_test("substitutors: generate_id");

    EXPECT_EQ(asciiquack::generate_id("Hello World"),    "_hello_world");
    EXPECT_EQ(asciiquack::generate_id("Section A"),      "_section_a");
    EXPECT_EQ(asciiquack::generate_id("C++ is great!"),  "_c_is_great");
    EXPECT_EQ(asciiquack::generate_id("simple"),         "_simple");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// Parser tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_parser_section_level() {
    begin_test("parser: section_level()");

    EXPECT_EQ(asciiquack::Parser::section_level("= Title"),      0);
    EXPECT_EQ(asciiquack::Parser::section_level("== Section"),   1);
    EXPECT_EQ(asciiquack::Parser::section_level("=== Sub"),      2);
    EXPECT_EQ(asciiquack::Parser::section_level("==== Sub2"),    3);
    EXPECT_EQ(asciiquack::Parser::section_level("===== Sub3"),   4);
    EXPECT_EQ(asciiquack::Parser::section_level("====== Sub4"),  5);
    EXPECT_EQ(asciiquack::Parser::section_level("======= Too many"), -1);
    EXPECT_EQ(asciiquack::Parser::section_level("not a title"),  -1);
    EXPECT_EQ(asciiquack::Parser::section_level(""),             -1);
    EXPECT_EQ(asciiquack::Parser::section_level("==no space"),   -1);

    end_test();
}

static void test_parser_section_title_text() {
    begin_test("parser: section_title_text()");

    EXPECT_EQ(asciiquack::Parser::section_title_text("= My Title"),    "My Title");
    EXPECT_EQ(asciiquack::Parser::section_title_text("== Section A"),  "Section A");
    // Trailing markers stripped
    EXPECT_EQ(asciiquack::Parser::section_title_text("== Foo =="),     "Foo");

    end_test();
}

static void test_parser_empty_document() {
    begin_test("parser: empty document");

    auto doc = asciiquack::Parser::parse_string("");
    EXPECT(doc != nullptr);
    EXPECT(doc->doctitle().empty());
    EXPECT(doc->blocks().empty());

    end_test();
}

static void test_parser_document_title() {
    begin_test("parser: document title");

    auto doc = asciiquack::Parser::parse_string("= My Title\n");
    EXPECT_EQ(doc->doctitle(), "My Title");

    end_test();
}

static void test_parser_document_header_full() {
    begin_test("parser: full document header");

    const std::string src =
        "= Document Title\n"
        "John Doe <john@example.com>\n"
        "v1.2, 2024-06-01: Initial release\n"
        ":myattr: hello\n"
        "\n"
        "Body paragraph.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->doctitle(), "Document Title");
    EXPECT(!doc->authors().empty());
    EXPECT_EQ(doc->authors()[0].firstname, "John");
    EXPECT_EQ(doc->authors()[0].email,     "john@example.com");
    EXPECT_EQ(doc->revision().number,      "1.2");
    EXPECT_EQ(doc->revision().date,        "2024-06-01");
    EXPECT_EQ(doc->attr("myattr"),         "hello");
    EXPECT(!doc->blocks().empty());

    end_test();
}

static void test_parser_paragraph() {
    begin_test("parser: paragraph");

    const std::string src = "Hello, world.\n\nSecond paragraph.\n";
    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->blocks().size(), std::size_t{2});
    EXPECT(doc->blocks()[0]->context() == asciiquack::BlockContext::Paragraph);
    EXPECT_EQ(doc->blocks()[0]->source(), "Hello, world.");

    end_test();
}

static void test_parser_sections() {
    begin_test("parser: sections");

    const std::string src =
        "= Document\n\n"
        "Preamble text.\n\n"
        "== Section A\n\n"
        "Section A content.\n\n"
        "=== Subsection\n\n"
        "Subsection content.\n\n"
        "== Section B\n\n"
        "Section B content.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->doctitle(), "Document");

    // There should be a preamble paragraph and 2 top-level sections
    int sections = 0;
    int paragraphs = 0;
    for (const auto& b : doc->blocks()) {
        if (b->context() == asciiquack::BlockContext::Section) { ++sections; }
        if (b->context() == asciiquack::BlockContext::Paragraph) { ++paragraphs; }
    }
    EXPECT(sections >= 2);
    EXPECT(paragraphs >= 1);

    end_test();
}

static void test_parser_attribute_entry() {
    begin_test("parser: attribute entry in body");

    const std::string src =
        ":greeting: Hello\n\n"
        "Paragraph with {greeting}.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->attr("greeting"), "Hello");

    end_test();
}

static void test_parser_listing_block() {
    begin_test("parser: listing block");

    const std::string src =
        "[source,cpp]\n"
        "----\n"
        "int main() { return 0; }\n"
        "----\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(!doc->blocks().empty());
    auto& b = *doc->blocks()[0];
    EXPECT(b.context() == asciiquack::BlockContext::Listing);
    EXPECT(b.source().find("int main") != std::string::npos);

    end_test();
}

static void test_parser_unordered_list() {
    begin_test("parser: unordered list");

    const std::string src =
        "* Item 1\n"
        "* Item 2\n"
        "* Item 3\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(!doc->blocks().empty());
    auto& b = *doc->blocks()[0];
    EXPECT(b.context() == asciiquack::BlockContext::Ulist);

    const auto& lst = dynamic_cast<const asciiquack::List&>(b);
    EXPECT_EQ(lst.items().size(), std::size_t{3});
    EXPECT_EQ(lst.items()[0]->source(), "Item 1");

    end_test();
}

static void test_parser_ordered_list() {
    begin_test("parser: ordered list");

    const std::string src =
        ". First\n"
        ". Second\n"
        ". Third\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(!doc->blocks().empty());
    EXPECT(doc->blocks()[0]->context() == asciiquack::BlockContext::Olist);

    const auto& lst = dynamic_cast<const asciiquack::List&>(*doc->blocks()[0]);
    EXPECT_EQ(lst.items().size(), std::size_t{3});
    EXPECT_EQ(lst.items()[1]->source(), "Second");

    end_test();
}

static void test_parser_admonition_paragraph() {
    begin_test("parser: admonition paragraph");

    const std::string src = "NOTE: Pay attention.\n";
    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(!doc->blocks().empty());
    EXPECT(doc->blocks()[0]->context() == asciiquack::BlockContext::Admonition);
    EXPECT_EQ(doc->blocks()[0]->attr("name"), "note");

    end_test();
}

static void test_parser_block_title() {
    begin_test("parser: block title (.Title)");

    const std::string src =
        ".My Code Block\n"
        "----\n"
        "code here\n"
        "----\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(!doc->blocks().empty());
    EXPECT_EQ(doc->blocks()[0]->title(), "My Code Block");

    end_test();
}

static void test_parser_thematic_break() {
    begin_test("parser: thematic break (''')");

    const std::string src = "para\n\n'''\n\npara2\n";
    auto doc = asciiquack::Parser::parse_string(src);

    bool found = false;
    for (const auto& b : doc->blocks()) {
        if (b->context() == asciiquack::BlockContext::ThematicBreak) { found = true; }
    }
    EXPECT(found);

    end_test();
}

static void test_parser_comment_line() {
    begin_test("parser: comment line (//)");

    const std::string src =
        "// This should not appear in the output\n"
        "Visible paragraph.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    // Only the paragraph, not the comment
    EXPECT(!doc->blocks().empty());
    bool has_comment = false;
    for (const auto& b : doc->blocks()) {
        if (b->source().find("should not appear") != std::string::npos) {
            has_comment = true;
        }
    }
    EXPECT(!has_comment);

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// HTML5 converter tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_html5_doctype() {
    begin_test("html5: DOCTYPE present");
    EXPECT_CONTAINS(html("= Title\n"), "<!DOCTYPE html>");
    end_test();
}

static void test_html5_title() {
    begin_test("html5: document title in <title> and <h1>");
    std::string out = html("= My Document\n");
    EXPECT_CONTAINS(out, "<title>My Document</title>");
    EXPECT_CONTAINS(out, "<h1>My Document</h1>");
    end_test();
}

static void test_html5_author() {
    begin_test("html5: author in header");
    std::string out = html("= Title\nJane Smith <jane@example.com>\n");
    EXPECT_CONTAINS(out, "Jane Smith");
    EXPECT_CONTAINS(out, "jane@example.com");
    end_test();
}

static void test_html5_paragraph() {
    begin_test("html5: paragraph");
    std::string out = html("Hello world.\n");
    EXPECT_CONTAINS(out, "<div class=\"paragraph\">");
    EXPECT_CONTAINS(out, "<p>Hello world.</p>");
    end_test();
}

static void test_html5_section() {
    begin_test("html5: section headings");
    std::string out = html("= Doc\n\n== Section One\n\nText.\n");
    EXPECT_CONTAINS(out, "class=\"sect1\"");
    EXPECT_CONTAINS(out, "<h2");
    EXPECT_CONTAINS(out, "Section One");
    end_test();
}

static void test_html5_section_id() {
    begin_test("html5: section id generated from title");
    std::string out = html("= Doc\n\n== My Section\n\nText.\n");
    EXPECT_CONTAINS(out, "id=\"_my_section\"");
    end_test();
}

static void test_html5_listing_block() {
    begin_test("html5: listing block");
    std::string out = html(
        "[source,python]\n"
        "----\n"
        "print('hello')\n"
        "----\n");
    EXPECT_CONTAINS(out, "class=\"listingblock\"");
    EXPECT_CONTAINS(out, "language-python");
    EXPECT_CONTAINS(out, "print(");
    end_test();
}

static void test_html5_literal_block() {
    begin_test("html5: literal block");
    std::string out = html(
        "....\n"
        "literal text here\n"
        "....\n");
    EXPECT_CONTAINS(out, "class=\"literalblock\"");
    EXPECT_CONTAINS(out, "literal text here");
    end_test();
}

static void test_html5_ulist() {
    begin_test("html5: unordered list");
    std::string out = html("* One\n* Two\n* Three\n");
    EXPECT_CONTAINS(out, "<ul>");
    EXPECT_CONTAINS(out, "<li>");
    EXPECT_CONTAINS(out, "One");
    EXPECT_CONTAINS(out, "Two");
    end_test();
}

static void test_html5_olist() {
    begin_test("html5: ordered list");
    std::string out = html(". First\n. Second\n");
    EXPECT_CONTAINS(out, "<ol");
    EXPECT_CONTAINS(out, "First");
    EXPECT_CONTAINS(out, "Second");
    end_test();
}

static void test_html5_admonition() {
    begin_test("html5: admonition paragraph");
    std::string out = html("TIP: Use asciiquack!\n");
    EXPECT_CONTAINS(out, "admonitionblock tip");
    EXPECT_CONTAINS(out, "Use asciiquack");
    end_test();
}

static void test_html5_special_chars() {
    begin_test("html5: special characters escaped");
    std::string out = html("a < b & c > d\n");
    EXPECT_CONTAINS(out, "&lt;");
    EXPECT_CONTAINS(out, "&amp;");
    EXPECT_CONTAINS(out, "&gt;");
    EXPECT_NOT_CONTAINS(out, "a < b");
    end_test();
}

static void test_html5_inline_bold() {
    begin_test("html5: inline bold");
    std::string out = html("Some *bold* text.\n");
    EXPECT_CONTAINS(out, "<strong>bold</strong>");
    end_test();
}

static void test_html5_inline_italic() {
    begin_test("html5: inline italic");
    std::string out = html("Some _italic_ text.\n");
    EXPECT_CONTAINS(out, "<em>italic</em>");
    end_test();
}

static void test_html5_inline_monospace() {
    begin_test("html5: inline monospace");
    std::string out = html("Use `code` here.\n");
    EXPECT_CONTAINS(out, "<code>code</code>");
    end_test();
}

static void test_html5_embedded() {
    begin_test("html5: embedded (no header/footer)");
    asciiquack::ParseOptions opts;
    opts.safe_mode = asciiquack::SafeMode::Unsafe;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string("= Title\n\nParagraph.\n", opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_NOT_CONTAINS(out, "<!DOCTYPE html>");
    EXPECT_NOT_CONTAINS(out, "<html");
    EXPECT_CONTAINS(out, "<div class=\"paragraph\">");
    end_test();
}

static void test_html5_horizontal_rule() {
    begin_test("html5: horizontal rule");
    std::string out = html("'''\n");
    EXPECT_CONTAINS(out, "<hr>");
    end_test();
}

static void test_html5_attribute_ref() {
    begin_test("html5: attribute reference in paragraph");
    std::string out = html(":greeting: Hello\n\n{greeting}, world!\n");
    EXPECT_CONTAINS(out, "Hello, world!");
    end_test();
}

static void test_html5_image() {
    begin_test("html5: block image macro");
    std::string out = html("image::photo.png[A photo]\n");
    EXPECT_CONTAINS(out, "<img");
    EXPECT_CONTAINS(out, "photo.png");
    EXPECT_CONTAINS(out, "A photo");
    end_test();
}

static void test_html5_table() {
    begin_test("html5: basic table");
    std::string out = html("|===\n|Col1 |Col2\n|a |b\n|===\n");
    EXPECT_CONTAINS(out, "<table");
    EXPECT_CONTAINS(out, "<td");
    end_test();
}

static void test_html5_link_relative() {
    begin_test("html5: link macro with relative URL");
    std::string out = html("See link:/docs/guide[the guide].\n");
    EXPECT_CONTAINS(out, "<a href=\"/docs/guide\">");
    EXPECT_CONTAINS(out, "the guide");
    end_test();
}


static void test_parser_example_block_closed() {
    begin_test("parser: example block properly closed by delimiter");

    const std::string src =
        "====\n"
        "Inside example.\n"
        "====\n"
        "\n"
        "After example.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    // Should have two blocks: the example block and the paragraph after it
    EXPECT(doc->blocks().size() >= 2);
    EXPECT(doc->blocks()[0]->context() == asciiquack::BlockContext::Example);
    // The "After example." paragraph must be a sibling, not swallowed into the example
    bool found_after = false;
    for (const auto& b : doc->blocks()) {
        if (b->source().find("After example") != std::string::npos) { found_after = true; }
    }
    EXPECT(found_after);

    end_test();
}

static void test_parser_sidebar_block_closed() {
    begin_test("parser: sidebar block properly closed by delimiter");

    const std::string src =
        "****\n"
        "Sidebar content.\n"
        "****\n"
        "\n"
        "Normal paragraph.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(doc->blocks().size() >= 2);
    EXPECT(doc->blocks()[0]->context() == asciiquack::BlockContext::Sidebar);
    EXPECT(doc->blocks()[1]->context() == asciiquack::BlockContext::Paragraph);

    end_test();
}

static void test_parser_quote_block_closed() {
    begin_test("parser: quote block properly closed by delimiter");

    const std::string src =
        "[quote,Author Name]\n"
        "____\n"
        "Some quoted text.\n"
        "____\n"
        "\n"
        "After quote.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT(doc->blocks().size() >= 2);
    EXPECT(doc->blocks()[0]->context() == asciiquack::BlockContext::Quote);
    EXPECT(doc->blocks()[1]->context() == asciiquack::BlockContext::Paragraph);

    end_test();
}

static void test_html5_example_block() {
    begin_test("html5: example block");
    std::string out = html(
        "====\n"
        "Example content.\n"
        "====\n"
        "\n"
        "After block.\n");
    EXPECT_CONTAINS(out, "class=\"exampleblock\"");
    EXPECT_CONTAINS(out, "Example content.");
    EXPECT_CONTAINS(out, "After block.");

    end_test();
}

static void test_html5_sidebar_block() {
    begin_test("html5: sidebar block");
    std::string out = html(
        "****\n"
        "Sidebar text.\n"
        "****\n"
        "\n"
        "Regular text.\n");
    EXPECT_CONTAINS(out, "class=\"sidebarblock\"");
    EXPECT_CONTAINS(out, "Sidebar text.");
    EXPECT_CONTAINS(out, "Regular text.");

    end_test();
}

static void test_html5_admonition_block() {
    begin_test("html5: admonition block (====)");
    std::string out = html(
        "[NOTE]\n"
        "====\n"
        "Compound note content.\n"
        "====\n"
        "\n"
        "After admonition.\n");
    EXPECT_CONTAINS(out, "admonitionblock note");
    EXPECT_CONTAINS(out, "Compound note content.");
    EXPECT_CONTAINS(out, "After admonition.");

    end_test();
}


static void test_integration_sample() {
    begin_test("integration: sample.adoc");

    const std::string src =
        "Document Title\n"
        "==============\n"
        "Doc Writer <thedoc@asciidoctor.org>\n"
        ":idprefix: id_\n"
        "\n"
        "Preamble paragraph.\n"
        "\n"
        "NOTE: This is test, only a test.\n"
        "\n"
        "== Section A\n"
        "\n"
        "*Section A* paragraph.\n"
        "\n"
        "=== Section A Subsection\n"
        "\n"
        "*Section A* 'subsection' paragraph.\n"
        "\n"
        "== Section B\n"
        "\n"
        "*Section B* paragraph.\n"
        "\n"
        ".Section B list\n"
        "* Item 1\n"
        "* Item 2\n"
        "* Item 3\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->doctitle(), "Document Title");
    EXPECT(!doc->authors().empty());
    EXPECT_EQ(doc->authors()[0].firstname, "Doc");

    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "<!DOCTYPE html>");
    EXPECT_CONTAINS(out, "Document Title");
    EXPECT_CONTAINS(out, "Section A");
    EXPECT_CONTAINS(out, "Section B");
    EXPECT_CONTAINS(out, "Preamble paragraph");
    EXPECT_CONTAINS(out, "admonitionblock note");
    EXPECT_CONTAINS(out, "<ul>");
    EXPECT_CONTAINS(out, "Item 1");

    end_test();
}

static void test_integration_basic() {
    begin_test("integration: basic.adoc");

    const std::string src =
        "= Document Title\n"
        "Doc Writer <doc.writer@asciidoc.org>\n"
        "v1.0, 2013-01-01\n"
        "\n"
        "Body content.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->doctitle(), "Document Title");
    EXPECT_EQ(doc->revision().number, "1.0");
    EXPECT_EQ(doc->revision().date,   "2013-01-01");

    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Document Title");
    EXPECT_CONTAINS(out, "Body content.");
    EXPECT_CONTAINS(out, "version 1.0");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// P1 feature tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_multiline_attribute_value() {
    begin_test("parser: multi-line attribute value (trailing \\)");

    const std::string src =
        ":long-val: first part \\\n"
        "second part\n"
        "\n"
        "{long-val}\n";

    auto doc = asciiquack::Parser::parse_string(src);
    EXPECT_EQ(doc->attr("long-val"), "first part second part");

    // Verify it's expanded in paragraph
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "first part second part");

    end_test();
}

static void test_section_numbering() {
    begin_test("parser: section numbering (:sectnums:)");

    const std::string src =
        "= Document\n"
        ":sectnums:\n"
        "\n"
        "== First Section\n"
        "\n"
        "Content.\n"
        "\n"
        "== Second Section\n"
        "\n"
        "Content.\n"
        "\n"
        "=== Subsection\n"
        "\n"
        "Content.\n";

    auto doc = asciiquack::Parser::parse_string(src);

    // Find numbered sections
    int numbered = 0;
    for (const auto& b : doc->blocks()) {
        if (b->context() == asciiquack::BlockContext::Section) {
            const auto& sect = dynamic_cast<const asciiquack::Section&>(*b);
            if (sect.numbered()) { ++numbered; }
        }
    }
    EXPECT(numbered >= 2);

    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "sectnum");
    EXPECT_CONTAINS(out, "1.");
    EXPECT_CONTAINS(out, "2.");

    end_test();
}

static void test_section_numbering_levels() {
    begin_test("parser: section numbering with :sectnumlevels: 1");

    const std::string src =
        "= Document\n"
        ":sectnums:\n"
        ":sectnumlevels: 1\n"
        "\n"
        "== Section A\n"
        "\n"
        "=== Subsection\n"
        "\n"
        "Text.\n";

    auto doc = asciiquack::Parser::parse_string(src);

    // Find level-2 section (===): should NOT be numbered since sectnumlevels is 1
    bool level2_not_numbered = true;
    std::function<void(const asciiquack::Block&)> walk;
    walk = [&](const asciiquack::Block& b) {
        if (b.context() == asciiquack::BlockContext::Section) {
            const auto& sect = dynamic_cast<const asciiquack::Section&>(b);
            if (sect.level() == 2 && sect.numbered()) { level2_not_numbered = false; }
        }
        for (const auto& child : b.blocks()) { walk(*child); }
    };
    walk(*doc);
    EXPECT(level2_not_numbered);

    end_test();
}

static void test_table_of_contents() {
    begin_test("html5: table of contents (:toc:)");

    const std::string src =
        "= Document\n"
        ":toc:\n"
        "\n"
        "== Introduction\n"
        "\n"
        "Text.\n"
        "\n"
        "== Conclusion\n"
        "\n"
        "Text.\n";

    std::string out = html(src);
    EXPECT_CONTAINS(out, "id=\"toc\"");
    EXPECT_CONTAINS(out, "Table of Contents");
    EXPECT_CONTAINS(out, "Introduction");
    EXPECT_CONTAINS(out, "Conclusion");
    // TOC links should reference section IDs
    EXPECT_CONTAINS(out, "href=\"#_introduction\"");

    end_test();
}

static void test_toc_custom_title() {
    begin_test("html5: TOC with custom :toc-title:");

    const std::string src =
        "= Doc\n"
        ":toc:\n"
        ":toc-title: Contents\n"
        "\n"
        "== Section\n"
        "\n"
        "Text.\n";

    std::string out = html(src);
    EXPECT_CONTAINS(out, "Contents");
    EXPECT_NOT_CONTAINS(out, "Table of Contents");

    end_test();
}

static void test_toc_with_sectnums() {
    begin_test("html5: TOC includes section numbers when :sectnums: set");

    const std::string src =
        "= Document\n"
        ":toc:\n"
        ":sectnums:\n"
        "\n"
        "== Alpha\n"
        "\n"
        "Text.\n"
        "\n"
        "== Beta\n"
        "\n"
        "Text.\n";

    std::string out = html(src);
    EXPECT_CONTAINS(out, "id=\"toc\"");
    // Section numbers should appear in the TOC
    EXPECT_CONTAINS(out, "1.");
    EXPECT_CONTAINS(out, "2.");

    end_test();
}

static void test_ifdef_single_line() {
    begin_test("parser: ifdef:: single-line form");

    // Attribute is set → content included
    const std::string src1 =
        ":myattr:\n"
        "\n"
        "ifdef::myattr[Included content.]\n";
    auto doc1 = asciiquack::Parser::parse_string(src1);
    bool found1 = false;
    for (const auto& b : doc1->blocks()) {
        if (b->source().find("Included content") != std::string::npos) { found1 = true; }
    }
    EXPECT(found1);

    // Attribute is NOT set → content excluded
    const std::string src2 =
        "ifdef::missing_attr[Should not appear.]\n"
        "Visible.\n";
    auto doc2 = asciiquack::Parser::parse_string(src2);
    bool found2 = false;
    for (const auto& b : doc2->blocks()) {
        if (b->source().find("Should not appear") != std::string::npos) { found2 = true; }
    }
    EXPECT(!found2);

    end_test();
}

static void test_ifdef_multiline() {
    begin_test("parser: ifdef:: multi-line form");

    const std::string src =
        ":myattr:\n"
        "\n"
        "Before.\n"
        "\n"
        "ifdef::myattr[]\n"
        "Conditional content.\n"
        "endif::myattr[]\n"
        "\n"
        "After.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Before.");
    EXPECT_CONTAINS(out, "Conditional content.");
    EXPECT_CONTAINS(out, "After.");

    end_test();
}

static void test_ifdef_multiline_false() {
    begin_test("parser: ifdef:: multi-line form (false branch skipped)");

    const std::string src =
        "Before.\n"
        "\n"
        "ifdef::missing_attr[]\n"
        "This should NOT appear.\n"
        "endif::missing_attr[]\n"
        "\n"
        "After.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Before.");
    EXPECT_NOT_CONTAINS(out, "This should NOT appear.");
    EXPECT_CONTAINS(out, "After.");

    end_test();
}

static void test_ifndef_single_line() {
    begin_test("parser: ifndef:: single-line form");

    // Attribute NOT set → content included
    const std::string src1 =
        "ifndef::missing[Included.]\n";
    auto doc1 = asciiquack::Parser::parse_string(src1);
    bool found1 = false;
    for (const auto& b : doc1->blocks()) {
        if (b->source().find("Included") != std::string::npos) { found1 = true; }
    }
    EXPECT(found1);

    // Attribute IS set → content excluded
    const std::string src2 =
        ":setattr:\n"
        "\n"
        "ifndef::setattr[Should not appear.]\n"
        "Visible.\n";
    auto doc2 = asciiquack::Parser::parse_string(src2);
    bool found2 = false;
    for (const auto& b : doc2->blocks()) {
        if (b->source().find("Should not appear") != std::string::npos) { found2 = true; }
    }
    EXPECT(!found2);

    end_test();
}

static void test_ifeval() {
    begin_test("parser: ifeval:: basic expression");

    // Version attribute set to "1.5"
    const std::string src =
        ":version: 1.5\n"
        "\n"
        "ifeval::[\"{version}\" >= \"1.0\"]\n"
        "Version is new enough.\n"
        "endif::[]\n"
        "\n"
        "ifeval::[\"{version}\" >= \"2.0\"]\n"
        "Should not appear.\n"
        "endif::[]\n"
        "\n"
        "Done.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Version is new enough.");
    EXPECT_NOT_CONTAINS(out, "Should not appear.");
    EXPECT_CONTAINS(out, "Done.");

    end_test();
}

static void test_floating_title() {
    begin_test("parser+html5: floating title ([discrete])");

    const std::string src =
        "Normal paragraph.\n"
        "\n"
        "[discrete]\n"
        "== Floating Heading\n"
        "\n"
        "Another paragraph.\n";

    auto doc = asciiquack::Parser::parse_string(src);

    // The [discrete] section should NOT be a Section node
    bool has_section = false;
    for (const auto& b : doc->blocks()) {
        if (b->context() == asciiquack::BlockContext::Section) { has_section = true; }
    }
    EXPECT(!has_section);

    std::string out = asciiquack::convert_to_html5(*doc);
    // Should have an <h2> with class="discrete" but no sect1 wrapper
    EXPECT_CONTAINS(out, "class=\"discrete\"");
    EXPECT_CONTAINS(out, "Floating Heading");
    EXPECT_NOT_CONTAINS(out, "class=\"sect1\"");

    end_test();
}

static void test_video_block() {
    begin_test("html5: video block macro");

    std::string out = html("video::demo.mp4[width=640,height=480]\n");
    EXPECT_CONTAINS(out, "<video");
    EXPECT_CONTAINS(out, "demo.mp4");
    EXPECT_CONTAINS(out, "controls");

    end_test();
}

static void test_audio_block() {
    begin_test("html5: audio block macro");

    std::string out = html("audio::demo.ogg[]\n");
    EXPECT_CONTAINS(out, "<audio");
    EXPECT_CONTAINS(out, "demo.ogg");
    EXPECT_CONTAINS(out, "controls");

    end_test();
}

static void test_include_directive() {
    begin_test("parser: include:: directive (unsafe mode)");

    // Write a temp file to include
    // Create a temp file with portable path
    namespace fs = std::filesystem;
    const std::string tmp_path =
        (fs::temp_directory_path() / "asciiquack_test_include.adoc").string();
    {
        std::ofstream f(tmp_path);
        f << "Included paragraph.\n";
    }

    const std::string src =
        "Before.\n"
        "\n"
        "include::" + tmp_path + "[]\n"
        "\n"
        "After.\n";

    asciiquack::ParseOptions opts;
    opts.safe_mode = asciiquack::SafeMode::Unsafe;
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Before.");
    EXPECT_CONTAINS(out, "Included paragraph.");
    EXPECT_CONTAINS(out, "After.");

    // Clean up
    std::remove(tmp_path.c_str());

    end_test();
}

static void test_include_directive_secure_mode() {
    begin_test("parser: include:: skipped in secure mode");

    const std::string src =
        "Before.\n"
        "include::/tmp/some_file.adoc[]\n"
        "After.\n";

    // Default mode is Secure – include should be silently ignored
    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Before.");
    EXPECT_CONTAINS(out, "After.");

    end_test();
}

static void test_bug7_description_list_not_table() {
    begin_test("bug #7: description list regex does not match | lines");

    // A line starting with | should not be mistaken for a description list
    const std::string src =
        "|===\n"
        "|Col1 |Col2\n"
        "|a |b\n"
        "|===\n"
        "\n"
        "After table.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    // Should be a table, not a description list
    bool has_table = false;
    bool has_dlist = false;
    for (const auto& b : doc->blocks()) {
        if (b->context() == asciiquack::BlockContext::Table)  { has_table = true; }
        if (b->context() == asciiquack::BlockContext::Dlist)  { has_dlist = true; }
    }
    EXPECT(has_table);
    EXPECT(!has_dlist);

    end_test();
}

static void test_ifeval_numeric() {
    begin_test("parser: ifeval:: numeric comparison");

    const std::string src =
        ":counter: 5\n"
        "\n"
        "ifeval::[{counter} > 3]\n"
        "Counter is large.\n"
        "endif::[]\n"
        "\n"
        "ifeval::[{counter} > 10]\n"
        "Counter is huge.\n"
        "endif::[]\n"
        "\n"
        "Done.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Counter is large.");
    EXPECT_NOT_CONTAINS(out, "Counter is huge.");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// P2 features tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_inline_passthrough() {
    begin_test("substitutors: pass:[] inline passthrough");

    // pass:[] with no subs – content is emitted raw (not HTML-escaped)
    const std::string src =
        "= Doc\n"
        "\n"
        "Raw pass:pass:[<b>bold</b>] here.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    // The raw HTML should be present (not escaped)
    EXPECT_CONTAINS(out, "<b>bold</b>");
    // The pass:[] macro text itself should not appear
    EXPECT_NOT_CONTAINS(out, "pass:[");

    end_test();
}

static void test_inline_passthrough_q() {
    begin_test("substitutors: pass:q[] quotes-only passthrough");

    // pass:q[] applies only quotes substitution inside
    const std::string src =
        "= Doc\n"
        "\n"
        "pass:q[*bold text* inside pass]\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "<strong>bold text</strong>");

    end_test();
}

static void test_kbd_macro() {
    begin_test("substitutors: kbd:[] macro");

    const std::string src =
        "= Doc\n"
        "\n"
        "Press kbd:[Ctrl+T] to open a new tab.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "<kbd>Ctrl</kbd>");
    EXPECT_CONTAINS(out, "<kbd>T</kbd>");
    EXPECT_CONTAINS(out, "keyseq");

    end_test();
}

static void test_btn_macro() {
    begin_test("substitutors: btn:[] macro");

    const std::string src =
        "= Doc\n"
        "\n"
        "Click btn:[OK] to continue.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "<b class=\"button\">OK</b>");

    end_test();
}

static void test_menu_macro() {
    begin_test("substitutors: menu:[] macro");

    const std::string src =
        "= Doc\n"
        "\n"
        "Use menu:File[Save] to save.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "<span class=\"menuseq\">");
    EXPECT_CONTAINS(out, "<span class=\"menu\">File</span>");
    EXPECT_CONTAINS(out, "<span class=\"menuitem\">Save</span>");

    end_test();
}

static void test_counter_macro() {
    begin_test("substitutors: counter: inline macro");

    const std::string src =
        "= Doc\n"
        "\n"
        "Item counter:item. Item counter:item. Item counter:item.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    // counter:item increments each time; should produce 1, 2, 3
    EXPECT_CONTAINS(out, "Item 1.");
    EXPECT_CONTAINS(out, "Item 2.");
    EXPECT_CONTAINS(out, "Item 3.");

    end_test();
}

static void test_counter2_macro() {
    begin_test("substitutors: counter2: does not emit value");

    // counter2: increments but produces no output
    const std::string src =
        "= Doc\n"
        "\n"
        "counter2:hidden. counter2:hidden. Value: counter:hidden.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    // counter2: should produce nothing; the final counter: should show 3
    EXPECT_CONTAINS(out, "Value: 3.");

    end_test();
}

static void test_footnote_macro() {
    begin_test("html5: footnote:[] macro rendered at end");

    const std::string src =
        "= Doc\n"
        "\n"
        "Some text.footnote:[This is a footnote.] More text.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    // Inline footnote reference marker
    EXPECT_CONTAINS(out, "class=\"footnote\"");
    EXPECT_CONTAINS(out, "_footnoteref_1");
    // Footnote body at end of document
    EXPECT_CONTAINS(out, "id=\"footnotes\"");
    EXPECT_CONTAINS(out, "This is a footnote.");
    EXPECT_CONTAINS(out, "_footnotedef_1");

    end_test();
}

static void test_footnote_multiple() {
    begin_test("html5: multiple footnotes numbered sequentially");

    const std::string src =
        "= Doc\n"
        "\n"
        "First.footnote:[Note one.] Second.footnote:[Note two.]\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "_footnoteref_1");
    EXPECT_CONTAINS(out, "_footnoteref_2");
    EXPECT_CONTAINS(out, "Note one.");
    EXPECT_CONTAINS(out, "Note two.");

    end_test();
}

static void test_ordered_list_style() {
    begin_test("html5: [loweralpha] ordered list style");

    const std::string src =
        "= Doc\n"
        "\n"
        "[loweralpha]\n"
        ". First\n"
        ". Second\n"
        ". Third\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "class=\"loweralpha\"");
    EXPECT_CONTAINS(out, "olist loweralpha");

    end_test();
}

static void test_ordered_list_start() {
    begin_test("html5: [start=3] ordered list start attribute");

    const std::string src =
        "= Doc\n"
        "\n"
        "[start=3]\n"
        ". Third item\n"
        ". Fourth item\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "start=\"3\"");

    end_test();
}

static void test_special_section_names() {
    begin_test("html5: special section names ([preface], [appendix])");

    const std::string src =
        "= Doc\n"
        "\n"
        "[preface]\n"
        "== Preface\n"
        "\n"
        "Preface text.\n"
        "\n"
        "[appendix]\n"
        "== Appendix A\n"
        "\n"
        "Appendix text.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "class=\"preface\"");
    EXPECT_CONTAINS(out, "class=\"appendix\"");

    end_test();
}

static void test_compound_list_items() {
    begin_test("parser+html5: compound list items (+ continuation)");

    const std::string src =
        "= Doc\n"
        "\n"
        "* First item\n"
        "+\n"
        "Attached paragraph in first item.\n"
        "\n"
        "* Second item\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "First item");
    EXPECT_CONTAINS(out, "Attached paragraph in first item.");
    EXPECT_CONTAINS(out, "Second item");
    // The attached paragraph should be inside the list item
    auto li_pos  = out.find("<li>");
    auto para_pos = out.find("Attached paragraph");
    auto li2_pos = out.find("<li>", li_pos + 1);
    EXPECT(li_pos  != std::string::npos);
    EXPECT(para_pos != std::string::npos);
    EXPECT(li2_pos  != std::string::npos);
    EXPECT(para_pos < li2_pos);  // attached para comes before the 2nd <li>

    end_test();
}

static void test_dlist_compound_body() {
    begin_test("html5: description list with compound body blocks");

    const std::string src =
        "= Doc\n"
        "\n"
        "term1::\n"
        "Body paragraph.\n"
        "+\n"
        "Second paragraph.\n"
        "\n"
        "term2:: Simple body.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "term1");
    EXPECT_CONTAINS(out, "Body paragraph.");
    EXPECT_CONTAINS(out, "term2");
    EXPECT_CONTAINS(out, "Simple body.");

    end_test();
}

static void test_idprefix_empty() {
    begin_test("html5: idprefix empty string (id without leading underscore)");

    const std::string src =
        "= Doc\n"
        ":idprefix:\n"
        ":idseparator: -\n"
        "\n"
        "== My Section\n"
        "\n"
        "Body.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    // ID should be "my-section" not "_my_section"
    EXPECT_CONTAINS(out, "id=\"my-section\"");

    end_test();
}

static void test_bug4_inline_bold_url() {
    begin_test("bug #4: constrained bold does not match inside URLs");

    // A URL containing a '*' character should not trigger constrained bold
    const std::string src =
        "= Doc\n"
        "\n"
        "See https://example.com/path*query for details.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    // The URL should not be broken by bold substitution
    EXPECT_CONTAINS(out, "example.com/path");
    EXPECT_NOT_CONTAINS(out, "<strong>query");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// P3 features tests
// ─────────────────────────────────────────────────────────────────────────────

static void test_source_callouts() {
    begin_test("html5: source callout markers <N> rendered as badges");

    const std::string src =
        "= Doc\n"
        "\n"
        "[source,ruby]\n"
        "----\n"
        "require 'sinatra' <1>\n"
        "get '/hi' do <2>\n"
        "  \"Hello!\"\n"
        "end\n"
        "----\n"
        "\n"
        "<1> Load the library.\n"
        "<2> Define a route.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);

    // Callout markers should be replaced with badge elements
    EXPECT_CONTAINS(out, "class=\"conum\"");
    EXPECT_CONTAINS(out, "(1)");
    EXPECT_CONTAINS(out, "(2)");
    // The callout list should be present
    EXPECT_CONTAINS(out, "colist");
    EXPECT_CONTAINS(out, "Load the library.");
    EXPECT_CONTAINS(out, "Define a route.");
    // Raw &lt;1&gt; should NOT appear in source code
    EXPECT_NOT_CONTAINS(out, "&lt;1&gt;");

    end_test();
}

static void test_admonition_caption_attr() {
    begin_test("html5: admonition caption from locale attribute");

    const std::string src =
        "= Doc\n"
        ":note-caption: Nota\n"
        ":tip-caption: Consejo\n"
        "\n"
        "NOTE: This is a note.\n"
        "\n"
        "TIP: This is a tip.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Nota");
    EXPECT_CONTAINS(out, "Consejo");
    EXPECT_NOT_CONTAINS(out, ">Note<");  // default should not appear

    end_test();
}

static void test_admonition_default_captions() {
    begin_test("html5: admonition default captions (Tip, Warning, Important, Caution)");

    const std::string src =
        "= Doc\n"
        "\n"
        "TIP: tip\n"
        "\n"
        "WARNING: warn\n"
        "\n"
        "IMPORTANT: important\n"
        "\n"
        "CAUTION: caution\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "Tip");
    EXPECT_CONTAINS(out, "Warning");
    EXPECT_CONTAINS(out, "Important");
    EXPECT_CONTAINS(out, "Caution");

    end_test();
}

static void test_stem_inline_macro() {
    begin_test("substitutors: stem:[] inline math macro");

    const std::string src =
        "= Doc\n"
        "\n"
        "The formula stem:[E = mc^2] is famous.\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    // Should render as MathJax inline math
    EXPECT_CONTAINS(out, "\\(E = mc^2\\)");
    EXPECT_NOT_CONTAINS(out, "stem:[");

    end_test();
}

static void test_latexmath_inline_macro() {
    begin_test("substitutors: latexmath:[] inline macro");

    const std::string src =
        "= Doc\n"
        "\n"
        "Inline: latexmath:[\\sum_{i=1}^{n} i].\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "\\(");
    EXPECT_NOT_CONTAINS(out, "latexmath:[");

    end_test();
}

static void test_stem_block() {
    begin_test("html5: [stem] block renders display math");

    const std::string src =
        "= Doc\n"
        "\n"
        "[stem]\n"
        "++++\n"
        "\\sum_{i=1}^{n} i = \\frac{n(n+1)}{2}\n"
        "++++\n";

    asciiquack::ParseOptions opts;
    opts.attributes["embedded"] = "";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_html5(*doc);
    // Should wrap in display math delimiters
    EXPECT_CONTAINS(out, "\\[");
    EXPECT_CONTAINS(out, "\\]");
    EXPECT_CONTAINS(out, "stemblock");

    end_test();
}

static void test_preamble_no_sections() {
    begin_test("html5: preamble div NOT emitted when no sections");

    // A document with only body content and no sections should not
    // wrap content in <div id="preamble">
    const std::string src =
        "= Doc\n"
        "\n"
        "Just a paragraph. No sections.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_NOT_CONTAINS(out, "id=\"preamble\"");
    EXPECT_CONTAINS(out, "Just a paragraph.");

    end_test();
}

static void test_preamble_with_sections() {
    begin_test("html5: preamble div emitted when sections follow");

    const std::string src =
        "= Doc\n"
        "\n"
        "This is the preamble.\n"
        "\n"
        "== Section One\n"
        "\n"
        "Section content.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "id=\"preamble\"");
    EXPECT_CONTAINS(out, "This is the preamble.");
    EXPECT_CONTAINS(out, "Section One");

    end_test();
}

static void test_linkcss_attribute() {
    begin_test("html5: :linkcss: emits <link> tag instead of inline style");

    const std::string src =
        "= Doc\n"
        ":linkcss:\n"
        "\n"
        "Body.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    // Should have a <link> tag, not inline <style>
    EXPECT_CONTAINS(out, "<link rel=\"stylesheet\"");
    EXPECT_NOT_CONTAINS(out, "<style>\n/* asciiquack");

    end_test();
}

static void test_stylesheet_attribute() {
    begin_test("html5: :stylesheet: path used in <link> tag");

    const std::string src =
        "= Doc\n"
        ":linkcss:\n"
        ":stylesheet: /custom/style.css\n"
        "\n"
        "Body.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "href=\"/custom/style.css\"");

    end_test();
}

static void test_stem_mathjax_script() {
    begin_test("html5: :stem: attribute adds MathJax script to head");

    const std::string src =
        "= Doc\n"
        ":stem: latexmath\n"
        "\n"
        "Body.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);
    EXPECT_CONTAINS(out, "MathJax");
    EXPECT_CONTAINS(out, "mathjax");

    end_test();
}

static void test_doctype_manpage() {
    begin_test("parser+html5: doctype manpage title parsing");

    const std::string src =
        "= git-commit(1)\n"
        "Git Author\n"
        "\n"
        "== Name\n"
        "\n"
        "git-commit - Record changes to the repository\n";

    asciiquack::ParseOptions opts;
    opts.doctype = "manpage";
    auto doc = asciiquack::Parser::parse_string(src, opts);

    // manname and manvolnum should be extracted
    EXPECT(doc->attr("manname") == "git-commit");
    EXPECT(doc->attr("manvolnum") == "1");

    std::string out = asciiquack::convert_to_html5(*doc);
    // Should show volume number in title heading
    EXPECT_CONTAINS(out, "git-commit(1)");
    EXPECT_CONTAINS(out, "Manual Page");

    end_test();
}

static void test_manpage_backend_basic() {
    begin_test("manpage: basic document converts to troff");

    const std::string src =
        "= ls(1)\n"
        ":manvolnum: 1\n"
        "\n"
        "== Name\n"
        "\n"
        "ls - list directory contents\n"
        "\n"
        "== Synopsis\n"
        "\n"
        "ls [options] [file...]\n"
        "\n"
        "== Description\n"
        "\n"
        "List information about the FILEs.\n";

    asciiquack::ParseOptions opts;
    opts.doctype = "manpage";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_manpage(*doc);

    // .TH header must be present
    EXPECT_CONTAINS(out, ".TH");
    EXPECT_CONTAINS(out, "LS");
    // Section headings
    EXPECT_CONTAINS(out, ".SH NAME");
    EXPECT_CONTAINS(out, ".SH SYNOPSIS");
    EXPECT_CONTAINS(out, ".SH DESCRIPTION");
    // Paragraph content
    EXPECT_CONTAINS(out, "list directory contents");

    end_test();
}

static void test_manpage_backend_bold_italic() {
    begin_test("manpage: inline bold and italic rendered as troff markup");

    const std::string src =
        "= test(1)\n"
        "\n"
        "== Description\n"
        "\n"
        "Use *bold* text and _italic_ text here.\n";

    asciiquack::ParseOptions opts;
    opts.doctype = "manpage";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_manpage(*doc);

    EXPECT_CONTAINS(out, "\\fB");
    EXPECT_CONTAINS(out, "\\fR");
    EXPECT_CONTAINS(out, "\\fI");

    end_test();
}

static void test_manpage_backend_listing() {
    begin_test("manpage: listing block uses .nf/.fi");

    const std::string src =
        "= test(1)\n"
        "\n"
        "== Synopsis\n"
        "\n"
        "----\n"
        "command --flag value\n"
        "----\n";

    asciiquack::ParseOptions opts;
    opts.doctype = "manpage";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    std::string out = asciiquack::convert_to_manpage(*doc);

    EXPECT_CONTAINS(out, ".nf");
    EXPECT_CONTAINS(out, ".fi");
    EXPECT_CONTAINS(out, "command --flag value");

    end_test();
}

static void test_table_col_alignment() {
    begin_test("html5: table cols alignment prefix (< ^ >)");

    const std::string src =
        "= Doc\n"
        "\n"
        "[cols=\"<1,^1,>1\",options=\"header\"]\n"
        "|===\n"
        "| Left | Center | Right\n"
        "| a | b | c\n"
        "|===\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);

    EXPECT_CONTAINS(out, "halign-left");
    EXPECT_CONTAINS(out, "halign-center");
    EXPECT_CONTAINS(out, "halign-right");

    end_test();
}

static void test_table_col_repeat() {
    begin_test("html5: table cols repeat notation (3*)");

    const std::string src =
        "= Doc\n"
        "\n"
        "[cols=\"3*\"]\n"
        "|===\n"
        "| a | b | c\n"
        "|===\n";

    auto doc = asciiquack::Parser::parse_string(src);
    // Should parse 3 equal-width columns from the repeat shorthand "3*"
    bool found_table = false;
    for (const auto& blk : doc->blocks()) {
        if (blk->context() == asciiquack::BlockContext::Table) {
            auto& tbl = dynamic_cast<const asciiquack::Table&>(*blk);
            EXPECT(tbl.column_specs().size() == 3);
            found_table = true;
        }
    }
    EXPECT(found_table);

    end_test();
}

static void test_table_col_style_h() {
    begin_test("html5: table cols style 'h' renders first column as header");

    const std::string src =
        "= Doc\n"
        "\n"
        "[cols=\"1h,2\"]\n"
        "|===\n"
        "| Key | Value\n"
        "| name | Alice\n"
        "|===\n";

    auto doc = asciiquack::Parser::parse_string(src);
    std::string out = asciiquack::convert_to_html5(*doc);

    // The 'h' style column should render body cells as <th> not <td>
    EXPECT_CONTAINS(out, "<th");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// DocBook 5 backend tests
// ─────────────────────────────────────────────────────────────────────────────

#include "docbook5.hpp"

static void test_docbook5_basic_document() {
    begin_test("docbook5: basic document structure");

    const std::string src =
        "= My Article\n"
        "Jane Doe <jane@example.com>\n"
        "v1.0, 2024-01-01\n"
        "\n"
        "Introduction paragraph.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    EXPECT_CONTAINS(out, "xmlns=\"http://docbook.org/ns/docbook\"");
    EXPECT_CONTAINS(out, "version=\"5.0\"");
    EXPECT_CONTAINS(out, "<article");
    EXPECT_CONTAINS(out, "</article>");
    EXPECT_CONTAINS(out, "<info>");
    EXPECT_CONTAINS(out, "<title>My Article</title>");
    EXPECT_CONTAINS(out, "<author>");
    EXPECT_CONTAINS(out, "Jane");
    EXPECT_CONTAINS(out, "2024-01-01");
    EXPECT_CONTAINS(out, "<para>Introduction paragraph.</para>");

    end_test();
}

static void test_docbook5_sections() {
    begin_test("docbook5: sections become <section> elements with titles");

    const std::string src =
        "= Doc\n"
        "\n"
        "== First Section\n"
        "\n"
        "Body text.\n"
        "\n"
        "=== Subsection\n"
        "\n"
        "Sub content.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<section");
    EXPECT_CONTAINS(out, "<title>First Section</title>");
    EXPECT_CONTAINS(out, "<title>Subsection</title>");
    EXPECT_CONTAINS(out, "<para>Body text.</para>");

    end_test();
}

static void test_docbook5_listing_block() {
    begin_test("docbook5: listing block becomes <programlisting>");

    const std::string src =
        "= Doc\n"
        "\n"
        "[source,python]\n"
        "----\n"
        "print(\"hello\")\n"
        "----\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<programlisting language=\"python\">");
    EXPECT_CONTAINS(out, "print(");
    EXPECT_CONTAINS(out, "</programlisting>");

    end_test();
}

static void test_docbook5_admonition() {
    begin_test("docbook5: admonition paragraph becomes DocBook element");

    const std::string src =
        "= Doc\n"
        "\n"
        "NOTE: This is important.\n"
        "\n"
        "WARNING: Be careful.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<note>");
    EXPECT_CONTAINS(out, "</note>");
    EXPECT_CONTAINS(out, "<warning>");
    EXPECT_CONTAINS(out, "</warning>");
    EXPECT_CONTAINS(out, "This is important.");

    end_test();
}

static void test_docbook5_ulist() {
    begin_test("docbook5: unordered list becomes <itemizedlist>");

    const std::string src =
        "= Doc\n"
        "\n"
        "* Apple\n"
        "* Banana\n"
        "* Cherry\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<itemizedlist>");
    EXPECT_CONTAINS(out, "<listitem>");
    EXPECT_CONTAINS(out, "Apple");
    EXPECT_CONTAINS(out, "Banana");
    EXPECT_CONTAINS(out, "</itemizedlist>");

    end_test();
}

static void test_docbook5_olist() {
    begin_test("docbook5: ordered list becomes <orderedlist>");

    const std::string src =
        "= Doc\n"
        "\n"
        ". Step one\n"
        ". Step two\n"
        ". Step three\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<orderedlist");
    EXPECT_CONTAINS(out, "numeration=\"arabic\"");
    EXPECT_CONTAINS(out, "<listitem>");
    EXPECT_CONTAINS(out, "Step one");
    EXPECT_CONTAINS(out, "</orderedlist>");

    end_test();
}

static void test_docbook5_dlist() {
    begin_test("docbook5: description list becomes <variablelist>");

    const std::string src =
        "= Doc\n"
        "\n"
        "term one:: description one\n"
        "term two:: description two\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<variablelist>");
    EXPECT_CONTAINS(out, "<varlistentry>");
    EXPECT_CONTAINS(out, "<term>");
    EXPECT_CONTAINS(out, "term one");
    EXPECT_CONTAINS(out, "description one");
    EXPECT_CONTAINS(out, "</variablelist>");

    end_test();
}

static void test_docbook5_inline_markup() {
    begin_test("docbook5: inline bold and italic become <emphasis>");

    const std::string src =
        "= Doc\n"
        "\n"
        "Use *bold* and _italic_ and `mono` text.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<emphasis role=\"strong\">");
    EXPECT_CONTAINS(out, "<emphasis>");
    EXPECT_CONTAINS(out, "<literal>");
    EXPECT_CONTAINS(out, "bold");
    EXPECT_CONTAINS(out, "italic");
    EXPECT_CONTAINS(out, "mono");

    end_test();
}

static void test_docbook5_special_chars() {
    begin_test("docbook5: special characters are XML-escaped");

    const std::string src =
        "= Doc\n"
        "\n"
        "Use <angle> brackets & \"quotes\".\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "&lt;angle&gt;");
    EXPECT_CONTAINS(out, "&amp;");
    EXPECT_NOT_CONTAINS(out, "<angle>");

    end_test();
}

static void test_docbook5_book_doctype() {
    begin_test("docbook5: book doctype produces <book> root and <chapter>");

    const std::string src =
        "= My Book\n"
        ":doctype: book\n"
        "\n"
        "== Chapter One\n"
        "\n"
        "Content.\n";

    asciiquack::ParseOptions opts;
    opts.doctype = "book";
    auto doc = asciiquack::Parser::parse_string(src, opts);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<book");
    EXPECT_CONTAINS(out, "</book>");
    EXPECT_CONTAINS(out, "<chapter");
    EXPECT_CONTAINS(out, "<title>Chapter One</title>");

    end_test();
}

static void test_docbook5_table() {
    begin_test("docbook5: table becomes CALS <informaltable>");

    const std::string src =
        "= Doc\n"
        "\n"
        "|===\n"
        "| A | B\n"
        "| 1 | 2\n"
        "|===\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<informaltable");
    EXPECT_CONTAINS(out, "<tgroup");
    EXPECT_CONTAINS(out, "<tbody>");
    EXPECT_CONTAINS(out, "<row>");
    EXPECT_CONTAINS(out, "<entry>");

    end_test();
}

static void test_docbook5_image() {
    begin_test("docbook5: block image becomes <mediaobject>");

    const std::string src =
        "= Doc\n"
        "\n"
        "image::sunset.png[Beautiful sunset]\n";

    auto doc = asciiquack::Parser::parse_string(src);
    const std::string out = asciiquack::convert_to_docbook5(*doc);

    EXPECT_CONTAINS(out, "<mediaobject>");
    EXPECT_CONTAINS(out, "<imagedata fileref=\"sunset.png\"");
    EXPECT_CONTAINS(out, "Beautiful sunset");

    end_test();
}

static void test_section_nesting_warning() {
    begin_test("parser: section nesting skip emits warning to stderr");

    // Redirect stderr to a string buffer for inspection
    std::ostringstream err_buf;
    std::streambuf* old_err = std::cerr.rdbuf(err_buf.rdbuf());

    const std::string src =
        "= Doc\n"
        "\n"
        "== Section\n"
        "\n"
        "==== Deep nested (skips ===)\n"
        "\n"
        "Content.\n";

    auto doc = asciiquack::Parser::parse_string(src);
    (void)doc;

    std::cerr.rdbuf(old_err);  // restore

    std::string errmsg = err_buf.str();
    EXPECT_CONTAINS(errmsg, "WARNING");
    EXPECT_CONTAINS(errmsg, "nesting");

    end_test();
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // Check for -v flag
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose") {
            g_verbose = true;
        }
    }

    std::cout << "Running asciiquack C++ tests\n";
    std::cout << "============================\n\n";

    // Reader
    std::cout << "Reader tests:\n";
    test_reader_basic();
    test_reader_crlf();
    test_reader_unshift();
    test_reader_skip_blank();

    // Substitutors
    std::cout << "\nSubstitutor tests:\n";
    test_sub_specialchars();
    test_sub_replacements();
    test_sub_attributes();
    test_generate_id();

    // Parser
    std::cout << "\nParser tests:\n";
    test_parser_section_level();
    test_parser_section_title_text();
    test_parser_empty_document();
    test_parser_document_title();
    test_parser_document_header_full();
    test_parser_paragraph();
    test_parser_sections();
    test_parser_attribute_entry();
    test_parser_listing_block();
    test_parser_unordered_list();
    test_parser_ordered_list();
    test_parser_admonition_paragraph();
    test_parser_block_title();
    test_parser_thematic_break();
    test_parser_comment_line();

    // HTML5 converter
    std::cout << "\nHTML5 converter tests:\n";
    test_html5_doctype();
    test_html5_title();
    test_html5_author();
    test_html5_paragraph();
    test_html5_section();
    test_html5_section_id();
    test_html5_listing_block();
    test_html5_literal_block();
    test_html5_ulist();
    test_html5_olist();
    test_html5_admonition();
    test_html5_special_chars();
    test_html5_inline_bold();
    test_html5_inline_italic();
    test_html5_inline_monospace();
    test_html5_embedded();
    test_html5_horizontal_rule();
    test_html5_attribute_ref();
    test_html5_image();
    test_html5_table();
    test_html5_link_relative();

    // Compound delimited block tests (regression for FIXME fix)
    std::cout << "\nCompound delimited block tests:\n";
    test_parser_example_block_closed();
    test_parser_sidebar_block_closed();
    test_parser_quote_block_closed();
    test_html5_example_block();
    test_html5_sidebar_block();
    test_html5_admonition_block();

    // Integration
    std::cout << "\nIntegration tests:\n";
    test_integration_sample();
    test_integration_basic();

    // P1 features and bug fixes
    std::cout << "\nP1 features and bug fix tests:\n";
    test_multiline_attribute_value();
    test_section_numbering();
    test_section_numbering_levels();
    test_table_of_contents();
    test_toc_custom_title();
    test_toc_with_sectnums();
    test_ifdef_single_line();
    test_ifdef_multiline();
    test_ifdef_multiline_false();
    test_ifndef_single_line();
    test_ifeval();
    test_ifeval_numeric();
    test_floating_title();
    test_video_block();
    test_audio_block();
    test_include_directive();
    test_include_directive_secure_mode();
    test_bug7_description_list_not_table();

    // P2 features
    std::cout << "\nP2 features and bug fix tests:\n";
    test_inline_passthrough();
    test_inline_passthrough_q();
    test_kbd_macro();
    test_btn_macro();
    test_menu_macro();
    test_counter_macro();
    test_counter2_macro();
    test_footnote_macro();
    test_footnote_multiple();
    test_ordered_list_style();
    test_ordered_list_start();
    test_special_section_names();
    test_compound_list_items();
    test_dlist_compound_body();
    test_idprefix_empty();
    test_bug4_inline_bold_url();

    // P3 features
    std::cout << "\nP3 features tests:\n";
    test_source_callouts();
    test_admonition_caption_attr();
    test_admonition_default_captions();
    test_stem_inline_macro();
    test_latexmath_inline_macro();
    test_stem_block();
    test_preamble_no_sections();
    test_preamble_with_sections();
    test_linkcss_attribute();
    test_stylesheet_attribute();
    test_stem_mathjax_script();
    test_doctype_manpage();
    test_manpage_backend_basic();
    test_manpage_backend_bold_italic();
    test_manpage_backend_listing();
    test_table_col_alignment();
    test_table_col_repeat();
    test_table_col_style_h();
    test_section_nesting_warning();

    // DocBook 5 backend tests
    std::cout << "\nDocBook 5 backend tests:\n";
    test_docbook5_basic_document();
    test_docbook5_sections();
    test_docbook5_listing_block();
    test_docbook5_admonition();
    test_docbook5_ulist();
    test_docbook5_olist();
    test_docbook5_dlist();
    test_docbook5_inline_markup();
    test_docbook5_special_chars();
    test_docbook5_book_doctype();
    test_docbook5_table();
    test_docbook5_image();

    // Summary
    std::cout << "\n============================\n";
    if (g_failed == 0) {
        std::cout << "All " << g_total << " tests passed.\n";
        return 0;
    } else {
        std::cout << g_failed << " of " << g_total << " tests FAILED.\n";
        return 1;
    }
}
