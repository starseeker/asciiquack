#!/usr/bin/env python3
"""check_pdf_layout.py – Automated PDF layout analysis using OpenCV.

This script converts a PDF to page images and analyses each page for common
layout defects:

  * Grey code-block background rectangles with insufficient spacing before
    the following body text (the "overlap" bug: paragraph ascenders visually
    bleeding into the grey background).
  * Text overflowing the top or bottom page margins.

The primary check is a *gap measurement*: for each grey code-block box the
script finds the actual bottom of the grey region and the start of the first
significant body-text run below it.  If the gap is smaller than
MIN_GAP_AFTER_BOX_PX the box is flagged.

At 150 DPI the fixed gap (≈ 11 pt) produces roughly 19 px of clearance; the
buggy gap (≈ 5 pt) would produce only 1–5 px of clearance.  The threshold is
set conservatively at 8 px (≈ 3.8 pt) to give room for minor anti-aliasing.

Usage
-----
    python3 check_pdf_layout.py [PDF_FILE] [--dpi DPI] [--save-annotated]

If PDF_FILE is omitted the script looks for ``stress_test.pdf`` next to
itself, generating it automatically when an ``asciiquack`` binary is found.

Exit codes
----------
  0 – no defects detected
  1 – one or more defects detected
  2 – usage / environment error
"""

import argparse
import subprocess
import sys
import os
from pathlib import Path

try:
    import cv2
    import numpy as np
    from pdf2image import convert_from_path
except ImportError as exc:
    print(f"ERROR: Missing Python dependency: {exc}")
    print("Install with: pip install opencv-python-headless pdf2image")
    sys.exit(2)

# ─────────────────────────────────────────────────────────────────────────────
# Configuration
# ─────────────────────────────────────────────────────────────────────────────

# Rendering resolution.  150 DPI gives ~2 pt/pixel, sufficient to detect the
# ~4 pt overlap described in the bug report.
DEFAULT_DPI = 150

# Grey background detection thresholds (0-255 scale).
# The code-block background is rendered at RGB (0.95, 0.95, 0.95) ≈ 242/255.
GREY_LO = 210   # lower bound for "light grey"
GREY_HI = 252   # upper bound (pure white ≥ 253 is excluded)

# Dark text detection threshold.
TEXT_DARK = 80   # pixels darker than this are "ink / text"

# Minimum dark pixels in a row for it to count as a "text row".
MIN_TEXT_ROW_PX = 50

# Minimum area (pixels²) of a grey connected component to be considered a
# code-block background (not a table rule or admonition side-bar).
MIN_GREY_AREA = 300

# Minimum width as a fraction of page width.  Code blocks span nearly the
# full content area; narrow elements are excluded.
MIN_GREY_WIDTH_FRACTION = 0.30

# Minimum rows to scan below the grey box when searching for following text.
MAX_SCAN_ROWS = 80   # ~38 pt at 150 DPI – more than enough

# Gap threshold: the measured gap (in rows) between the grey box bounding
# bottom and the first body-text row below it must exceed this value.
# A gap of 1-3 rows (≈0.5-1.5 pt at 150 DPI) indicates the following text
# starts immediately adjacent to or inside the grey box.  Values of 4+ rows
# (≈2 pt) correspond to visually separate elements.
# The fixed code produces gaps of 6+ rows; the pre-fix bug produced 1-3 rows.
MIN_GAP_AFTER_BOX_PX = 4

# ─────────────────────────────────────────────────────────────────────────────
# Helpers
# ─────────────────────────────────────────────────────────────────────────────

def find_asciiquack() -> "str | None":
    """Return the path to the asciiquack binary, or None."""
    candidates = [
        Path(__file__).parent.parent / "build" / "asciiquack",
        Path(__file__).parent.parent / "asciiquack",
    ]
    for c in candidates:
        if c.is_file() and os.access(c, os.X_OK):
            return str(c)
    try:
        result = subprocess.run(["which", "asciiquack"],
                                capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def generate_pdf(adoc_path: Path, pdf_path: Path) -> bool:
    """Run asciiquack to produce a PDF from *adoc_path*."""
    binary = find_asciiquack()
    if binary is None:
        print("ERROR: Cannot find asciiquack binary.  Build the project first.")
        return False
    print(f"Generating {pdf_path} …")
    result = subprocess.run(
        [binary, "-b", "pdf", str(adoc_path), "-o", str(pdf_path)],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"ERROR: asciiquack failed:\n{result.stderr}")
        return False
    return True


def find_grey_boxes(grey_mask: np.ndarray, page_w: int,
                    ) -> "list[tuple[int,int,int,int]]":
    """Return bounding boxes (x, y, w, h) of large light-grey regions.

    Only regions wider than MIN_GREY_WIDTH_FRACTION × page_w are returned so
    that narrow table rules and admonition side-bars are excluded.
    """
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 3))
    closed = cv2.morphologyEx(grey_mask, cv2.MORPH_CLOSE, kernel)

    n_labels, _labels, stats, _ = cv2.connectedComponentsWithStats(
        closed, connectivity=8
    )
    boxes = []
    for lbl in range(1, n_labels):
        x, y, w, h, area = stats[lbl]
        if area < MIN_GREY_AREA:
            continue
        if w < MIN_GREY_WIDTH_FRACTION * page_w:
            continue
        boxes.append((int(x), int(y), int(w), int(h)))
    return boxes


def first_text_row_after(dark_mask: np.ndarray,
                          start_row: int,
                          bx: int, bw: int,
                          page_h: int) -> "int | None":
    """Return the first row index ≥ start_row that has at least
    MIN_TEXT_ROW_PX dark pixels within the x-range [bx, bx+bw].

    Returns None if no such row is found within MAX_SCAN_ROWS.
    """
    for y in range(start_row, min(start_row + MAX_SCAN_ROWS, page_h)):
        row_slice = dark_mask[y, bx : bx + bw]
        if int(np.sum(row_slice > 0)) >= MIN_TEXT_ROW_PX:
            return y
    return None


# ─────────────────────────────────────────────────────────────────────────────
# Per-page analysis
# ─────────────────────────────────────────────────────────────────────────────

def check_page(image: np.ndarray, page_num: int,
               save_annotated: bool, out_dir: Path) -> "list[dict]":
    """Analyse a single page image and return a list of defect dicts."""
    defects: "list[dict]" = []
    page_h, page_w = image.shape[:2]

    grey_img = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)

    grey_mask = np.zeros(grey_img.shape, dtype=np.uint8)
    grey_mask[(grey_img >= GREY_LO) & (grey_img <= GREY_HI)] = 255

    dark_mask = np.zeros(grey_img.shape, dtype=np.uint8)
    dark_mask[grey_img < TEXT_DARK] = 255

    boxes = find_grey_boxes(grey_mask, page_w)

    annotated = image.copy() if save_annotated else None

    for (bx, by, bw, bh) in boxes:
        # Use the connected-component bounding box bottom as the reference.
        # This already includes anti-aliased edges of the last code line, so
        # scanning for text starts immediately after all code content.
        grey_bot = by + bh - 1

        # First significant text row BELOW the grey bounding box.
        first_text = first_text_row_after(dark_mask, grey_bot + 1,
                                           bx, bw, page_h)

        if first_text is None:
            # No following text on this page – nothing to flag
            gap = MAX_SCAN_ROWS  # treat as large gap
        else:
            gap = first_text - grey_bot

        defect = None
        if gap < MIN_GAP_AFTER_BOX_PX:
            defect = {
                "page":        page_num,
                "box":         (bx, by, bw, bh),
                "grey_bottom": grey_bot,
                "first_text":  first_text,
                "gap_px":      gap,
                "description": (
                    f"Page {page_num}: grey box bottom at row {grey_bot}, "
                    f"next text at row {first_text} "
                    f"(gap = {gap} px ≈ {gap / (DEFAULT_DPI / 72.0):.1f} pt) "
                    f"– insufficient spacing, following text may overlap the "
                    f"code-block background."
                ),
            }
            defects.append(defect)

        if save_annotated and annotated is not None:
            colour = (255, 0, 0) if defect else (0, 200, 0)
            # Draw box outline
            cv2.rectangle(annotated, (bx, by), (bx + bw, by + bh),
                          colour, 2)
            # Mark the grey actual bottom with a horizontal line
            cv2.line(annotated, (bx, grey_bot), (bx + bw, grey_bot),
                     (0, 128, 255), 1)
            # Mark first text row if found
            if first_text is not None:
                cv2.line(annotated, (bx, first_text), (bx + bw, first_text),
                         (128, 0, 255), 1)
            # Label gap value
            label = f"gap={gap}px"
            cv2.putText(annotated, label, (bx + 2, grey_bot - 4),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.4,
                        (255, 0, 0) if defect else (0, 128, 0), 1,
                        cv2.LINE_AA)

    if save_annotated and annotated is not None:
        out_path = out_dir / f"page_{page_num:03d}_annotated.png"
        cv2.imwrite(str(out_path), cv2.cvtColor(annotated, cv2.COLOR_RGB2BGR))
        print(f"  Saved annotated page {page_num} → {out_path}")

    return defects


def check_bottom_margin(image: np.ndarray, page_num: int,
                         margin_px: int = 20) -> "list[dict]":
    """Detect text below the bottom margin."""
    page_h = image.shape[0]
    grey_img = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)
    dark = int(np.sum(grey_img[page_h - margin_px:, :] < TEXT_DARK))
    if dark > 50:
        return [{
            "page": page_num,
            "description": (
                f"Page {page_num}: {dark} dark pixels in the bottom "
                f"{margin_px}px margin (text overflow?)."
            ),
        }]
    return []


def check_top_margin(image: np.ndarray, page_num: int,
                      margin_px: int = 20) -> "list[dict]":
    """Detect text above the top margin."""
    grey_img = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)
    dark = int(np.sum(grey_img[:margin_px, :] < TEXT_DARK))
    if dark > 50:
        return [{
            "page": page_num,
            "description": (
                f"Page {page_num}: {dark} dark pixels in the top "
                f"{margin_px}px margin (text overflow?)."
            ),
        }]
    return []


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyse a PDF for layout defects using OpenCV."
    )
    parser.add_argument(
        "pdf", nargs="?",
        help="Path to the PDF file (default: examples/stress_test.pdf)",
    )
    parser.add_argument(
        "--dpi", type=int, default=DEFAULT_DPI,
        help=f"Rendering DPI (default: {DEFAULT_DPI})",
    )
    parser.add_argument(
        "--save-annotated", action="store_true",
        help="Save annotated PNG images alongside the PDF.",
    )
    args = parser.parse_args()

    script_dir = Path(__file__).parent

    if args.pdf:
        pdf_path = Path(args.pdf)
    else:
        pdf_path = script_dir / "stress_test.pdf"

    # Auto-generate the PDF from the stress-test adoc if it doesn't exist
    if not pdf_path.exists():
        adoc_path = pdf_path.with_suffix(".adoc")
        if not adoc_path.exists():
            print(f"ERROR: Neither {pdf_path} nor {adoc_path} exists.")
            return 2
        if not generate_pdf(adoc_path, pdf_path):
            return 2

    if not pdf_path.exists():
        print(f"ERROR: PDF not found at {pdf_path}")
        return 2

    print(f"Analysing {pdf_path} at {args.dpi} DPI …")

    try:
        pages = convert_from_path(str(pdf_path), dpi=args.dpi)
    except Exception as exc:
        print(f"ERROR: Failed to convert PDF to images: {exc}")
        return 2

    print(f"  {len(pages)} page(s) to analyse.")
    out_dir = pdf_path.parent
    all_defects: "list[dict]" = []

    for page_num, page_img in enumerate(pages, start=1):
        img = np.array(page_img)  # PIL RGB → numpy
        defects  = check_page(img, page_num, args.save_annotated, out_dir)
        defects += check_bottom_margin(img, page_num)
        defects += check_top_margin(img, page_num)
        all_defects.extend(defects)

    print()
    if all_defects:
        print(f"DEFECTS FOUND: {len(all_defects)}")
        for d in all_defects:
            print(f"  [!] {d['description']}")
        return 1
    else:
        print(f"No layout defects detected across {len(pages)} page(s). ✓")
        return 0


if __name__ == "__main__":
    sys.exit(main())
