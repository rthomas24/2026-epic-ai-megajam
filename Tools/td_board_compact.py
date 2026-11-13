#!/usr/bin/env python3
"""Triad Drift - compact MESSAGEBOARD.md so open messages are easy to see. Lead-owned.

Every message whose status is COMPLETE is moved, word for word, to
docs/coordination/MESSAGEBOARD-ARCHIVE.md, and one table row (id, subject, outcome) is left in its
side's "Completed" table on the board. OPEN and ANSWERED messages are never touched and stay in
place, in full. Nothing is lost: the archive is append-only.

    git fetch origin && git merge origin/main        # compact only the latest board
    python Tools/td_board_compact.py summaries.json   # {"R-001": "one or two sentences: what was agreed", ...}
    git add MESSAGEBOARD.md docs/coordination/MESSAGEBOARD-ARCHIVE.md
    git commit -m "coordination: message board compacted" && push at once

A COMPLETE message with no summary in the JSON file is left alone (and reported), so a message is
never reduced to a row without someone having written what it decided. Exit code 0 = something was
compacted, 1 = nothing to do or a problem.
"""

import datetime
import io
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOARD = os.path.join(ROOT, "MESSAGEBOARD.md")
ARCHIVE = os.path.join(ROOT, "docs", "coordination", "MESSAGEBOARD-ARCHIVE.md")
ARCHIVE_REL = "docs/coordination/MESSAGEBOARD-ARCHIVE.md"

HEADING = re.compile(r"^### ((?:R|N)-\d+) [·-] ([A-Z]+) [·-] (.*)$")  # both sides' separators
TABLE_HEAD = "| Id | Subject | Outcome |\n|---|---|---|\n"


def marker(side):
    return "<!-- completed:%s -->" % side


def read(path):
    with io.open(path, "r", encoding="utf-8", newline="") as handle:
        text = handle.read()
    return text.replace("\r\n", "\n"), ("\r\n" in text)


def write(path, text, crlf):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if crlf:
        text = text.replace("\n", "\r\n")
    with io.open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(text)


def cell(text):
    return " ".join(text.replace("|", "/").split())


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    with io.open(sys.argv[1], "r", encoding="utf-8") as handle:
        summaries = json.load(handle)

    board, crlf = read(BOARD)
    lines = board.split("\n")

    # Only parse inside the two message sections (the header holds a template in a code fence).
    section_starts = [i for i, line in enumerate(lines) if line.startswith("## Messages from ")]
    if len(section_starts) != 2:
        print("td_board_compact: expected two '## Messages from' sections, found %d" % len(section_starts))
        return 1

    # Find message blocks: from a heading to the next heading, a '---' rule or a '## ' heading.
    blocks = []  # (start, end, id, status, subject)
    i = section_starts[0]
    while i < len(lines):
        match = HEADING.match(lines[i])
        if match:
            j = i + 1
            while j < len(lines) and not HEADING.match(lines[j]) and lines[j] != "---" and not lines[j].startswith("## "):
                j += 1
            blocks.append((i, j, match.group(1), match.group(2), match.group(3)))
            i = j
        else:
            i += 1

    moved, skipped = [], []
    for start, end, ident, status, subject in blocks:
        if status != "COMPLETE":
            continue
        if not summaries.get(ident):
            skipped.append(ident)
            continue
        moved.append((start, end, ident, subject, summaries[ident]))

    if skipped:
        print("td_board_compact: COMPLETE but no summary given, left in place: %s" % ", ".join(skipped))
    if not moved:
        print("td_board_compact: nothing to compact")
        return 1

    # Archive, word for word.
    stamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    if os.path.exists(ARCHIVE):
        archive, archive_crlf = read(ARCHIVE)
    else:
        archive, archive_crlf = (
            "# Triad Drift message board archive\n\n"
            "Completed messages moved here word for word from `/MESSAGEBOARD.md` by `Tools/td_board_compact.py`.\n"
            "Append-only: never edit or delete an entry. The board keeps a one-line outcome for each.\n",
            crlf,
        )
    archive = archive.rstrip("\n") + "\n\n## Archived %s (local time)\n\n" % stamp
    for start, end, ident, subject, outcome in moved:
        archive += "\n".join(lines[start:end]).rstrip("\n") + "\n\n"
    write(ARCHIVE, archive, archive_crlf)

    # Remove the moved blocks from the board (from the bottom up so indices stay valid).
    rows = {"R": [], "N": []}
    for start, end, ident, subject, outcome in moved:
        rows[ident[0]].append("| %s | %s | %s |" % (ident, cell(subject), cell(outcome)))
    for start, end, ident, subject, outcome in sorted(moved, reverse=True):
        del lines[start:end]

    text = "\n".join(lines)
    for side, heading_prefix in (("R", "## Messages from Ryan's side"), ("N", "## Messages from Nick's side")):
        if not rows[side]:
            continue
        new_rows = "\n".join(rows[side]) + "\n"
        mark = marker(side)
        if mark in text:
            # Append to the existing table: rows go right before the end marker.
            end_mark = "<!-- /completed:%s -->" % side
            idx = text.index(end_mark)
            text = text[:idx] + new_rows + text[idx:]
        else:
            idx = text.index(heading_prefix)
            line_end = text.index("\n", idx) + 1
            table = ("\n%s\n**Completed** (decisions kept here; full text, word for word, in `%s`):\n\n%s%s<!-- /completed:%s -->\n"
                     % (mark, ARCHIVE_REL, TABLE_HEAD, new_rows, side))
            text = text[:line_end] + table + text[line_end:]

    text = re.sub(r"\n{4,}", "\n\n\n", text)
    write(BOARD, text, crlf)

    remaining = [(m.group(1), m.group(2)) for m in (HEADING.match(line) for line in text.split("\n")) if m]
    print("td_board_compact: archived %d message(s): %s" % (len(moved), ", ".join(ident for _, _, ident, _, _ in sorted(moved))))
    print("td_board_compact: still on the board in full: %s" % (", ".join("%s (%s)" % pair for pair in remaining) or "none"))
    print("td_board_compact: board is now %d lines" % len(text.split("\n")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
