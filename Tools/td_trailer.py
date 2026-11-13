"""
td_trailer.py - record reproducible gameplay clips from a running Triad Drift game and cut the jam video (AGENTS.md 5.2:
30-60 s, gameplay within the first seconds). Owner: Teammate 3.

  1. Start the game (Tools\\td-trailer.ps1 does all of this):  Tools\\td-play-mcp.ps1 -Offscreen -ResX 1920 -ResY 1080
  2. python Tools\\td_trailer.py record [--only ram,missile] [--port 8013]
       Each beat in BEATS is staged with the game's own dev commands (TD.Stage, TD.Director.*, TD.Power.Use, TD.Input.*,
       TD.Ground.*), recorded with the engine's SceneCaptureToolset viewport recorder (1080p, 30 fps MP4) and saved as
       Builds/trailer/raw/<beat>.mp4. Re-running a beat replaces its clip.
  3. python Tools\\td_trailer.py cut [--edl Tools/td_trailer_edl.json]
       Crops the recorder's frame border and its "Recording viewport" box, trims each clip to the edit list's in / out
       points, adds the title cards, joins everything with short crossfades and writes
       Builds/trailer/TriadDrift_<date>.mp4 plus a contact sheet (one frame a second) next to it.
  4. python Tools\\td_trailer.py sheet <clip.mp4>   a contact sheet of any clip (one frame a second).

Audio: the agent's game runs without the audio device, so the cut is silent for now (a music bed can be laid in with
--music <file> once Audio has one).
"""
import argparse, datetime, json, os, re, shutil, subprocess, sys, time

ROOT = os.environ.get("TD_ROOT") or os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
OUT = os.environ.get("TD_TRAILER_OUT") or os.path.join(ROOT, "Builds", "trailer")
RAW = os.path.join(OUT, "raw")
FPS = 30
# The recorder burns a 4 px red frame and a "Recording viewport / Stop / Cancel" box into the top-right corner:
# crop 46 px off the top and keep 16:9, then scale back to 1920 x 1080.
CROP = "crop=1831:1030:44:46,scale=1920:1080:flags=lanczos,setsar=1"
FONT = "C\\:/Windows/Fonts/segoeuib.ttf"

# Each beat: setup commands (with waits), then a recording of `seconds` while the timeline runs (t, command).
# "pilot": run Tools/td_agent_pilot.py alongside (aims, fires, dodges, uses powers) for the dogfight beats.
BEATS = {
    "dogfight": {"setup": [("TD.God", 0.5), ("TD.Stage Wave3", 6.0)], "pilot": ("Rift", 26), "lead": 4.0, "seconds": 18,
                 "timeline": [(4.5, "TD.Squad.Switch Lancer"), (11.5, "TD.Squad.Switch Bastion")]},
    "ram": {"setup": [("TD.Stage Wave2", 5.0)], "seconds": 7, "timeline": [(0.6, "TD.Director.RamKill")]},
    "missile": {"setup": [("TD.Stage Wave2", 5.0)], "seconds": 8, "timeline": [(0.6, "TD.Director.MissileReturn")]},
    "bastion": {"setup": [("TD.Stage Wave3", 5.0), ("TD.Squad.Switch Bastion", 1.5), ("TD.Power.Ship Bastion", 0.3)], "seconds": 8,
                "timeline": [(0.5, "TD.Power.Use Signature1"), (3.5, "TD.Power.Use Signature2"), (5.5, "TD.Power.Use Signature1")]},
    "multikill": {"setup": [("TD.Stage Wave3", 5.0)], "seconds": 7, "timeline": [(0.6, "TD.Director.MultiKill")]},
    "waveclear": {"setup": [("TD.Stage Wave1", 6.0)], "seconds": 10, "timeline": [(1.0, "TD.KillAll")]},
    "approach": {"setup": [("TD.Stage VerdanceArrival", 8.0)], "seconds": 10, "timeline": [(0.5, "TD.Input.Keys W+LeftShift 6")]},
    "onfoot": {"setup": [("TD.Stage VerdanceOnFoot", 25.0), ("TD.God", 0.5), ("TD.Ground.ThirdPerson 1", 1.0),
                         ("TD.Ground.Spawn Grunt 700", 1.0), ("TD.Ground.Spawn Trooper 1100", 1.5)], "map": "Verdance", "seconds": 11,
               "timeline": [(0.3, "TD.Ground.FaceNearest"), (0.6, "TD.Input.Keys W 0.8"), (1.4, "TD.Input.Key LeftMouseButton 0.15"),
                            (1.9, "TD.Input.Key LeftMouseButton 0.15"), (2.4, "TD.Input.Key LeftMouseButton 0.15"),
                            (3.6, "TD.Ground.FaceNearest"), (4.0, "TD.Input.Key RightMouseButton 1.2"), (5.6, "TD.Ground.FaceNearest"),
                            (5.9, "TD.Input.Keys W 0.8"), (6.8, "TD.Input.Key LeftMouseButton 0.15"), (7.3, "TD.Input.Key LeftMouseButton 0.15"),
                            (7.8, "TD.Input.Key LeftMouseButton 0.15")]},
    "dominion": {"setup": [("TD.Stage DominionAssault", 10.0), ("TD.God", 0.5)], "map": "Galaxy", "seconds": 10,
                 "timeline": [(0.8, "TD.Director.GeneratorKill"), (5.5, "TD.Director.GeneratorKill")]},
    # T4's framing: the stage puts the squad 200 m off the bow facing the reactor (~350 m); at cruise it passes it in ~6 s, so
    # aim at it and throttle down at once, then three kills ~1 s apart (one phase each; the third starts the breakup).
    # T4's framing: hold the ship still in front of the reactor, aim at it, then three kills ~1 s apart (one phase each;
    # the third starts the breakup, now with the placeholder breakup explosions and the round-4 effects).
    "reactor": {"setup": [("TD.God", 0.5), ("TD.Stage ReactorExposed", 2.5), ("TD.World.Hold 20", 0.3), ("TD.World.AimAt Reactor 1.5", 1.8)],
                "seconds": 14, "timeline": [(0.8, "TD.Director.ReactorKill"), (2.3, "TD.Director.ReactorKill"), (3.8, "TD.Director.ReactorKill"),
                                            (5.3, "TD.Director.ReactorKill")]},  # 1.5 s apart: each phase opens after a beat
    "victory": {"setup": [("TD.Stage Victory", 12.0)], "map": "MainMenu", "seconds": 12, "timeline": []},
}


def mcp(port, *args, save=None, timeout=180):
    cmd = [sys.executable, os.path.join(ROOT, "Tools", "td_mcp.py"), "--port", str(port), "--timeout", "120"]
    if save:
        cmd += ["--save-images", save]
    r = subprocess.run(cmd + list(args), capture_output=True, text=True, cwd=ROOT, timeout=timeout)
    return r.stdout + r.stderr


def console(port, c):
    print("   >", c)
    return mcp(port, "call", "ConsoleToolset.ConsoleToolset", "RunConsoleCommand", json.dumps({"command": c}))


def game_world(port):
    """The current game world (it changes when a stage travels to a planet)."""
    out = mcp(port, "call", "SceneCaptureToolset.SceneCaptureToolset", "ListWorlds", "{}")
    paths = re.findall(r'"refPath":\s*"([^"]+)"', out)
    game = [p for p in paths if "/Maps/" in p and "Transition" not in p]
    return game[0] if game else (paths[0] if paths else None)


def wait_world(port, contains, seconds):
    t0 = time.time()
    while time.time() - t0 < seconds:
        w = game_world(port)
        if w and contains.lower() in w.lower():
            return w
        time.sleep(2)
    return game_world(port)


def record_beat(port, name, beat):
    print("== beat", name)
    console(port, "TD.UI.NativeHUD -1")  # the game's own HUD (an agent pilot may have left the dev HUD on)
    console(port, "TD.UI.SpaceHUD 2")     # the shipping default: engineering's flight HUD (wins over the pilot's legacy switch)
    for c, wait in beat.get("setup", []):
        console(port, c)
        time.sleep(wait)
    if beat.get("map"):
        wait_world(port, beat["map"], 60)
        time.sleep(3)
    world = game_world(port)
    pilot = None
    if beat.get("pilot"):
        ship, secs = beat["pilot"]
        pilot = subprocess.Popen([sys.executable, os.path.join(ROOT, "Tools", "td_agent_pilot.py"), "--port", str(port), "--ship", ship,
                                  "--stage", "Wave3", "--seconds", str(secs), "--god"], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(beat.get("lead", 3.0))
    opts = {"options": {"source": "GameViewport", "world": {"refPath": world}, "maxResolution": 1920, "frameRate": FPS,
                        "maxDurationSeconds": beat["seconds"] + 5}}
    out = mcp(port, "call", "SceneCaptureToolset.SceneCaptureToolset", "StartViewportRecording", json.dumps(opts))
    ids = re.findall(r"[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}", out)
    if not ids:
        print("   ! could not start recording:", out[-400:])
        return None
    t0 = time.time()
    for t, c in beat.get("timeline", []):
        time.sleep(max(0.0, t - (time.time() - t0)))
        console(port, c)
    time.sleep(max(0.0, beat["seconds"] - (time.time() - t0)))
    out = mcp(port, "call", "SceneCaptureToolset.SceneCaptureToolset", "StopViewportRecording", json.dumps({"recordingId": ids[0]}))
    if pilot:
        pilot.wait(timeout=120)
    path = re.findall(r'"filesystemPath":\s*"([^"]+)"', out)
    if not path or not os.path.exists(path[0]):
        print("   ! no file:", out[-400:])
        return None
    os.makedirs(RAW, exist_ok=True)
    dst = os.path.join(RAW, name + ".mp4")
    shutil.copyfile(path[0], dst)
    frames = re.findall(r'"frameCount":\s*(\d+)', out)
    print("   saved", dst, "frames", frames[0] if frames else "?")
    return dst


def sheet(clip, dst=None, cols=6, every=1.0, width=480):
    dst = dst or os.path.splitext(clip)[0] + "_sheet.png"
    dur = float(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", clip],
                               capture_output=True, text=True).stdout.strip() or 1)
    rows = max(1, -(-int(dur / every) // cols))
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", clip, "-vf",
                    "fps=1/%g,scale=%d:-1,tile=%dx%d:padding=4:color=black" % (every, width, cols, rows), "-frames:v", "1", dst], check=False)
    return dst


def card(path, lines, seconds):
    """A title card: black, white text stacked about the centre, the squad's three colours under the last line."""
    draws, y = [], -sum(int(sz * 1.25) for _, sz in lines) // 2
    for text, size in lines:
        draws.append("drawtext=fontfile='%s':text='%s':fontcolor=white:fontsize=%d:x=(w-text_w)/2:y=(h/2)+(%d):alpha='min(1,t/0.4)*min(1,(%g-t)/0.4)'"
                     % (FONT, text.replace("'", "").replace(":", r"\:"), size, y, seconds))
        y += int(size * 1.25)
    bars = ["drawbox=x=(iw/2)-330+%d:y=(ih/2)+(%d):w=200:h=8:color=%s:t=fill" % (k * 230, y + 18, c)
            for k, c in enumerate(["0xFF33CC", "0x19E6FF", "0xFFA619"])]
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "lavfi", "-i", "color=c=black:s=1920x1080:r=%d:d=%g" % (FPS, seconds),
                    "-vf", ",".join(draws + bars), "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", str(FPS), path], check=True)


def cut(edl_path, music=None):
    edl = json.load(open(edl_path, encoding="utf-8"))
    work = os.path.join(OUT, "work")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    shots = edl["shots"]
    parts = []
    for i, e in enumerate(shots):
        p = os.path.join(work, "%02d.mp4" % i)
        # A shot with "fade" dips through black into it; the shot before it dips out the same way (default: hard cuts,
        # which is how a director's cut reads).
        fade_in = e.get("fade", 0.0)
        fade_out = shots[i + 1].get("fade", 0.0) if i + 1 < len(shots) else 0.6
        if e.get("card"):
            dur = e["seconds"]
            raw = p + ".card.mp4"
            card(raw, [(l["text"], l.get("size", 64)) for l in e["card"]], dur)
            src_args, vf = ["-i", raw], "null"
        else:
            dur = e["out"] - e["in"]
            src_args = ["-ss", str(e["in"]), "-to", str(e["out"]), "-i", os.path.join(RAW, e["clip"] + ".mp4")]
            vf = CROP + (",setpts=PTS/%g" % e["speed"] if e.get("speed") else "")
            dur = dur / e.get("speed", 1.0)
        if fade_in > 0:
            vf += ",fade=t=in:st=0:d=%g" % fade_in
        if fade_out > 0:
            vf += ",fade=t=out:st=%g:d=%g" % (max(0.0, dur - fade_out), fade_out)
        # Identical encoding for every part, so the concat demuxer joins them frame-exact without a re-encode.
        subprocess.run(["ffmpeg", "-v", "error", "-y"] + src_args + ["-vf", vf + ",fps=%d,format=yuv420p" % FPS, "-an", "-c:v", "libx264",
                        "-crf", "16", "-preset", "medium", "-video_track_timescale", "30000", p], check=True)
        parts.append(p)
    listfile = os.path.join(work, "parts.txt")
    with open(listfile, "w", encoding="utf-8") as f:
        for p in parts:
            f.write("file '%s'\n" % p.replace("\\", "/"))
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M")
    final = os.path.join(OUT, "TriadDrift_%s.mp4" % stamp)
    cmd = ["ffmpeg", "-v", "error", "-y", "-f", "concat", "-safe", "0", "-i", listfile]
    if music:
        cmd += ["-i", music, "-map", "0:v", "-map", "1:a", "-shortest", "-c:a", "aac", "-b:a", "192k"]
    cmd += ["-c:v", "copy", "-movflags", "+faststart", final]
    subprocess.run(cmd, check=True)
    total = float(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", final],
                                 capture_output=True, text=True).stdout.strip())
    print("CUT", final, "%.1f s" % total)
    print("SHEET", sheet(final))
    return final


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["record", "cut", "sheet"])
    ap.add_argument("clip", nargs="?")
    ap.add_argument("--port", type=int, default=8013)
    ap.add_argument("--only", default="")
    ap.add_argument("--edl", default=os.path.join(ROOT, "Tools", "td_trailer_edl.json"))
    ap.add_argument("--music", default=None)
    a = ap.parse_args()
    if a.mode == "record":
        names = [n for n in BEATS if not a.only or n in a.only.split(",")]
        for n in names:
            dst = record_beat(a.port, n, BEATS[n])
            if dst:
                print("   sheet", sheet(dst))
    elif a.mode == "cut":
        cut(a.edl, a.music)
    else:
        print(sheet(a.clip))


if __name__ == "__main__":
    main()
