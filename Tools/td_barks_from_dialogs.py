"""
td_barks_from_dialogs.py - turn dialogs.md's bark tables into the engineering fallback bark table. Owner: Teammate 3.

  python Tools/td_barks_from_dialogs.py            # writes Tools/td_barks.csv (+ prints a summary)
  python Tools/td_barks_from_dialogs.py --check    # parse only; exit 1 on a malformed row

Then Tools/td-barks-import.ps1 builds /Game/TriadDrift/Gameplay/Characters/Barks/DT_BarksFallback from the CSV (row
struct FTDBarkRow). Nicholas's DT_Barks in the Presentation folder wins row by row (same row names).

Row name: "<BarkId>__<Speaker>" (e.g. "Bark_SquadSwitchIn__Moss"). Columns: Id, Speaker, Priority, Lines (an array of
strings; one is picked per play, with rotation in code).

dialogs.md conventions it understands:
- tables "| Id | When | Speaker | Priority | Wren | Kato | Moss |": a cell holds the pilot's lines as quoted strings;
  "-" = no line for that pilot.
- several ids in one cell ("`Bark_Ram_Go` / `_Kill`"): the second id is the first one's prefix + suffix; the pilot cell
  then holds one line per id in the same order, and the priority cell one priority per id ("Normal / High").
- a placeholder id ("`Bark_SquadEngage<Dir>`"): one row with {dir} left in the lines (code fills it).
- a typed id ("`Bark_NewEnemy_<Type>` ... : `Interceptor`, `Gunship`"): one id per type, lines in that order.
- the Command table "| Id | When | Line |": speaker Command.
- the Banter list: "- Wren: "..." / Kato: "..." / Moss: "..."": ids Bark_Banter<N>_<k>, one speaker per step.
- anything after " · " in a cell is a note, not a line.
"""
import argparse, csv, os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SRC = os.path.join(ROOT, "dialogs.md")
OUT = os.path.join(ROOT, "Tools", "td_barks.csv")
PILOTS = ["Wren", "Kato", "Moss"]
PRIORITIES = {"normal": "Normal", "high": "High", "critical": "Critical", "flavour": "Flavour", "flavor": "Flavour"}

QUOTE = re.compile(r'"([^"]+)"')


def split_cells(line):
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    return cells


def parse_ids(cell):
    """'`Bark_Ram_Go` / `_Kill` (new, T2)' -> ['Bark_Ram_Go', 'Bark_Ram_Kill']; types and placeholders handled."""
    ticks = re.findall(r"`([^`]+)`", cell)
    if not ticks:
        return []
    base = ticks[0]
    # A typed id: "`Bark_NewEnemy_<Type>` (new, T4): `Interceptor`, `Gunship`"
    if "<Type>" in base:
        return [base.replace("<Type>", t) for t in ticks[1:]]
    ids = [base]
    stem = base.rsplit("_", 1)[0] if "_" in base else base
    for t in ticks[1:]:
        if t.startswith("_"):
            ids.append(stem + t)
        elif t.startswith("Bark_"):
            ids.append(t)
    return ids


def lines_in(cell):
    cell = cell.split(" · ")[0]
    return [q.strip() for q in QUOTE.findall(cell)]


def priorities_for(cell, n):
    parts = [PRIORITIES.get(p.strip().lower(), "Normal") for p in cell.split("/")]
    if len(parts) < n:
        parts += [parts[-1]] * (n - len(parts))
    return parts[:n]


def parse(text):
    rows = []  # (id, speaker, priority, [lines])
    header = None
    lines = text.splitlines()
    for i, raw in enumerate(lines):
        line = raw.strip()
        if line.startswith("## "):
            header = None
        if line.startswith("| Id"):
            header = split_cells(line)
            continue
        if not header or not line.startswith("|") or line.startswith("|---"):
            if not line.startswith("|"):
                header = None if not line else header
            continue
        cells = split_cells(line)
        if len(cells) != len(header):
            raise ValueError("line %d: %d cells, header has %d" % (i + 1, len(cells), len(header)))
        col = {h: c for h, c in zip(header, cells)}
        ids = parse_ids(col["Id"])
        if not ids:
            continue
        if "Line" in col:  # the Command table
            for bid in ids:
                rows.append((bid, "Command", "High", lines_in(col["Line"])))
            continue
        prios = priorities_for(col.get("Priority", "Normal"), len(ids))
        for pilot in PILOTS:
            got = lines_in(col.get(pilot, ""))
            if not got:
                continue
            if len(ids) == 1:
                rows.append((ids[0], pilot, prios[0], got))
            elif all(t.startswith("Bark_") for t in re.findall(r"`([^`]+)`", col["Id"])[: len(ids)]) and len(got) < len(ids):
                # aliases for the same moment ("`Bark_SquadWipe` / `Bark_Wipe`"): every id gets the lines
                for bid in ids:
                    rows.append((bid, pilot, prios[0], got))
            else:
                # one line per id, in order (a pilot may have fewer: the ids without a line are skipped)
                for k, bid in enumerate(ids):
                    if k < len(got):
                        rows.append((bid, pilot, prios[k], [got[k]]))
    # Banter (section 9): "- Wren: "..." / Kato: "..." / Moss: "...""
    in_banter = False
    n = 0
    for raw in lines:
        if raw.startswith("## 9"):
            in_banter = True
            continue
        if in_banter and raw.startswith("## "):
            break
        if in_banter and raw.strip().startswith("- "):
            n += 1
            for k, step in enumerate(raw.strip()[2:].split(" / ")):
                m = re.match(r'\s*(Wren|Kato|Moss|Command):\s*"([^"]+)"', step)
                if m:
                    rows.append(("Bark_Banter%d_%d" % (n, k + 1), m.group(1), "Flavour", [m.group(2)]))
    return rows


# The code fires one id per downed ship (TDBarkIds::ShipDownFor): Bark_SquadShipDownRift / Lancer / Bastion.
# dialogs.md writes one "<Ship>" row whose lines each name a ship or its pilot; each line goes to that ship's id,
# so a squadmate never mourns the wrong pilot (T2 found Rift's id had no row).
SHIP_NAMES = {"Rift": ("Rift", "Wren"), "Lancer": ("Lancer", "Kato"), "Bastion": ("Bastion", "Moss")}


def expand_ship_down(rows):
    out = []
    for bid, spk, prio, ls in rows:
        if "<Ship>" not in bid:
            out.append((bid, spk, prio, ls))
            continue
        for ship, names in SHIP_NAMES.items():
            if spk == names[1]:
                continue  # the downed pilot does not call their own loss
            mine = [l for l in ls if any(n in l for n in names)]
            if mine:
                out.append((bid.replace("<Ship>", ship), spk, prio, mine))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--src", default=SRC)
    ap.add_argument("--out", default=OUT)
    a = ap.parse_args()
    text = open(a.src, encoding="utf-8").read()
    rows = expand_ship_down(parse(text))
    seen = set()
    for r in rows:
        key = "%s__%s" % (r[0], r[1])
        if key in seen:
            print("duplicate row %s" % key)
            sys.exit(1)
        seen.add(key)
    ids = sorted({r[0] for r in rows})
    print("%d rows, %d ids, %d lines" % (len(rows), len(ids), sum(len(r[3]) for r in rows)))
    if a.check:
        return
    with open(a.out, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f, quoting=csv.QUOTE_MINIMAL)
        w.writerow(["Name", "Id", "Speaker", "Priority", "Lines"])
        for bid, spk, prio, ls in rows:
            arr = "(" + ",".join('"%s"' % l.replace('"', "'") for l in ls) + ")"
            w.writerow(["%s__%s" % (bid, spk), bid, spk, prio, arr])
    print("wrote", a.out)


if __name__ == "__main__":
    main()
