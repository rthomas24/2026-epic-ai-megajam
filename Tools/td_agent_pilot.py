"""
td_agent_pilot.py - a scripted "evader" pilot for a running Triad Drift game with Unreal's MCP server.

Start the game first (Tools\\td-play-mcp.ps1), then for example:
    python Tools\\td_agent_pilot.py --port 8012 --ship Rift --stage Wave3 --seconds 60

It reads TD.UI.Status (works with either HUD) every loop and:
  * aims: turns toward the current target, else toward the nearest hostile marker (markers reach 2 km),
    and holds fire when the target is within 20 degrees of the nose;
  * dodges: double-taps D / A (alternating) whenever a hit indicator younger than 0.8 s appears,
    and pulses Left Alt every ~5 loops as a weave;
  * uses powers: signature every ~6 loops, ability every ~9, Bastion's dome every ~11;
  * captures a frame on a hit (up to 4) and every ~15 loops, under Saved/TDShots/mcp_pilot_<ship>_<stage>/.
It prints START / END status (hull, shield, alive enemies, hits) and how many dodges it made. Read deaths from
the log: "TDSQUAD switch <from> -> <to> cause=...".

Steering uses TD.Input.TurnToward (real mouse input onto a world bearing from TD.UI.Status' wyaw / wpitch),
re-issued every loop. It's still a lower bound on a human, not a model of one.
"""
import argparse, json, os, re, subprocess, sys, time

ROOT = os.environ.get("TD_ROOT") or os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
WORLD = {"refPath": "/Game/TriadDrift/Maps/L_Galaxy.L_Galaxy"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8019)
    ap.add_argument("--ship", default="Rift", choices=["Rift", "Lancer", "Bastion"])
    ap.add_argument("--stage", default="Wave3")
    ap.add_argument("--seconds", type=float, default=60.0)
    ap.add_argument("--god", action="store_true", help="keep the player alive (for captures)")
    ap.add_argument("--native-hud", action="store_true", help="Nick's HUD instead of the dev HUD")
    ap.add_argument("--trace", action="store_true", help="print every loop's status")
    a = ap.parse_args()
    log = os.path.join(ROOT, "Saved", "Logs", "td-play-mcp_%d.log" % a.port)
    out = os.path.join(ROOT, "Saved", "TDShots", "mcp_pilot_%s_%s" % (a.ship, a.stage))

    def mcp(toolset, tool, args, save=None):
        cmd = [sys.executable, os.path.join(ROOT, "Tools", "td_mcp.py"), "--port", str(a.port)]
        if save:
            cmd += ["--save-images", save]
        cmd += ["call", toolset, tool, json.dumps(args)]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT, timeout=120)
        return (r.stdout + r.stderr).strip()

    def console(c):
        mcp("ConsoleToolset.ConsoleToolset", "RunConsoleCommand", {"command": c})

    def shot(name):
        d = os.path.join(out, name)
        os.makedirs(d, exist_ok=True)
        before = set(os.listdir(d))
        mcp("SceneCaptureToolset.SceneCaptureToolset", "CaptureGameViewport", {"world": WORLD, "maxResolution": 1280}, save=d)
        new = sorted(set(os.listdir(d)) - before)
        return os.path.join(d, new[0]) if new else None

    def status():
        size = os.path.getsize(log) if os.path.exists(log) else 0
        console("TD.UI.Status")
        time.sleep(0.1)
        with open(log, "rb") as f:
            f.seek(size)
            txt = f.read().decode("utf-8", "ignore")
        st = {}
        m = re.search(r"target=(\S+) onscreen=(\d) at \((-?\d+), (-?\d+)\)", txt)
        if m:
            st.update(target=m.group(1), on=m.group(2) == "1", x=float(m.group(3)), y=float(m.group(4)))
        m = re.search(r"viewport=(\d+)x(\d+)", txt)
        if m:
            st.update(w=int(m.group(1)), h=int(m.group(2)))
        m = re.search(r"nearest=(\S+) dist=(\d+) onscreen=(\d) at \((-?\d+), (-?\d+)\) yaw=(-?[\d.]+) pitch=(-?[\d.]+) wyaw=(-?[\d.]+) wpitch=(-?[\d.]+)", txt)
        if m:
            st.update(near=m.group(1), ndist=float(m.group(2)), nyaw=float(m.group(6)), npitch=float(m.group(7)),
                      nwyaw=float(m.group(8)), nwpitch=float(m.group(9)))
        m = re.search(r"target wyaw=(-?[\d.]+) wpitch=(-?[\d.]+) dist=(\d+)", txt)
        if m:
            st.update(twyaw=float(m.group(1)), twpitch=float(m.group(2)))
        m = re.search(r"hits=(\d+) newest=(-?[\d.]+)s", txt)
        if m:
            st.update(hits=int(m.group(1)), age=float(m.group(2)))
        m = re.search(r"wave=(\d+) alive=(\d+)", txt)
        if m:
            st.update(wave=int(m.group(1)), alive=int(m.group(2)))
        for c in re.finditer(r"card (\d) active=1 alive=(\d) unlocked=\d hull=([\d.]+) shield=([\d.]+)", txt):
            st.update(active=int(c.group(1)), hull=float(c.group(3)), shield=float(c.group(4)))
        return st

    if a.god:
        console("TD.God")
    console("TD.UI.NativeHUD %d" % (1 if a.native_hud else 0))
    for unlock in ("Cloak Rift", "EMPPulse Lancer", "GravityWell Bastion"):
        console("TD.Unlock " + unlock)
    console("TD.Stage " + a.stage)
    time.sleep(4)
    console("TD.Power.Ship " + a.ship)
    time.sleep(1.5)

    t0, n, dodges, side, shots = time.time(), 0, 0, "D", []
    print("START", status())
    while time.time() - t0 < a.seconds:
        n += 1
        st = status()
        if a.trace:
            print("LOOP", n, st)
        # 1. Dodge a fresh hit.
        if st.get("hits", 0) > 0 and st.get("age", 9) < 0.8:
            console("TD.Input.Tap %s 2" % side)
            side = "A" if side == "D" else "D"
            dodges += 1
            if len(shots) < 4:
                shots.append(shot("hit_%02d" % n))
        # 2. Aim (real mouse steering onto a world bearing, T1's TD.Input.TurnToward) and fire.
        if "twyaw" in st:
            console("TD.Input.TurnToward %.1f %.1f 1.2" % (st["twyaw"], st["twpitch"]))
            # Fire on the bearing: the target's screen projection can read off-screen during the chase-camera lag.
            aligned = st.get("near") == st.get("target") and abs(st.get("nyaw", 99)) < 20 and abs(st.get("npitch", 99)) < 15
            if st.get("on") or aligned:
                console("TD.Input.Key LeftMouseButton 0.9")
        elif "nwyaw" in st:
            console("TD.Input.TurnToward %.1f %.1f 1.2" % (st["nwyaw"], st["nwpitch"]))
            if abs(st["nyaw"]) < 20 and abs(st["npitch"]) < 15:
                console("TD.Input.Key LeftMouseButton 0.9")
        elif n % 2 == 0:
            console("TD.Power.Turn 40 0")
        # 3. Powers and weave.
        if n % 6 == 0:
            console("TD.Power.Use Signature1")
        if n % 9 == 0:
            console("TD.Power.Use Ability")
        if a.ship == "Bastion" and n % 11 == 0:
            console("TD.Power.Use Signature2")
        if n % 5 == 0:
            console("TD.Input.Key LeftAlt 0.1")
        if n % 15 == 0 and len(shots) < 6:
            shots.append(shot("t%02d" % int(time.time() - t0)))
        time.sleep(0.3)
    print("END", status(), "dodges", dodges, "loops", n)
    print("SHOTS", [s for s in shots if s])


if __name__ == "__main__":
    main()
