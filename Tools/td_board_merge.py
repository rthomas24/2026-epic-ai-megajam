#!/usr/bin/env python3
"""Triad Drift - resolve a merge conflict in MESSAGEBOARD.md by keeping BOTH sides. Lead-owned.

Both Leads append to the same log, so `git merge origin/main` often conflicts there. The rule in
AGENTS.md section 6.10 is "keep both edits". This applies it: in every conflict block the incoming
side's text (theirs: what is already on origin/main) goes first, then ours (not yet pushed), and
then it checks that no message id appears twice.

    git merge origin/main            # reports a conflict in MESSAGEBOARD.md
    python Tools/td_board_merge.py   # rewrites MESSAGEBOARD.md without markers
    git add MESSAGEBOARD.md && git commit --no-edit

Exit code 0 = resolved and ids unique. 1 = nothing to do or a duplicate id that needs a human
decision (renumber the message that is not on origin/main yet).
"""

import io
import os
import re
import sys


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "MESSAGEBOARD.md")
    with io.open(path, "r", encoding="utf-8", newline="") as handle:
        text = handle.read()
    crlf = "\r\n" in text
    text = text.replace("\r\n", "\n")

    pattern = re.compile(r"<<<<<<< [^\n]*\n(.*?)(?:\|\|\|\|\|\|\| [^\n]*\n.*?)?=======\n(.*?)>>>>>>> [^\n]*\n", re.S)
    blocks = pattern.findall(text)
    if not blocks:
        print("td_board_merge: no conflict markers in %s" % path)
        return 1

    def keep_both(match):
        ours, theirs = match.group(1), match.group(2)
        if ours.strip() == theirs.strip():
            return theirs
        return theirs.rstrip("\n") + "\n\n" + ours.lstrip("\n")

    text = pattern.sub(keep_both, text)
    if "<<<<<<<" in text or ">>>>>>>" in text:
        print("td_board_merge: markers remain; resolve by hand")
        return 1

    ids = re.findall(r"^### ((?:R|N)-\d+) ", text, flags=re.M)
    duplicates = sorted({i for i in ids if ids.count(i) > 1})

    if crlf:
        text = text.replace("\n", "\r\n")
    with io.open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(text)

    print("td_board_merge: kept both sides in %d block(s); ids: %s" % (len(blocks), ", ".join(ids)))
    if duplicates:
        print("td_board_merge: DUPLICATE ids %s - renumber the one that is not on origin/main yet" % ", ".join(duplicates))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
