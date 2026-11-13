# SPDX-License-Identifier: MIT
# Triad Drift - shared table for the Rodin ship intake (Grok Bot's Hyper3D Rodin meshes -> game ships).
# Sources are read-only under ArtSource/EngAI/Ships; everything we write goes to ArtSource/EngShips/.
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(HERE))))
AI = os.path.join(REPO, "ArtSource", "EngAI", "Ships")
OUT = os.path.join(REPO, "ArtSource", "EngShips")


# Sources that are on main but not merged into this checkout yet can be read from a mirror of the repo tree
# (TD_RODIN_AI_ROOT = a folder that contains ArtSource/EngAI/...). Manifests always record repo-relative paths.
ALT_ROOT = os.environ.get("TD_RODIN_AI_ROOT", "")


def resolve(rel_or_abs):
    """Repo path if it exists, else the same relative path under TD_RODIN_AI_ROOT."""
    p = rel_or_abs if os.path.isabs(rel_or_abs) else os.path.join(REPO, rel_or_abs)
    if os.path.exists(p) or not ALT_ROOT:
        return p
    alt = os.path.join(ALT_ROOT, os.path.relpath(p, REPO))
    return alt if os.path.exists(alt) else p


def _ship(folder, stem, heat=None, tex=2048, rev="mesh_r1"):
    return dict(src=resolve(os.path.join(AI, *folder, rev)), rel=os.path.join(AI, *folder, rev), stem=stem, tex=tex,
                heat=os.path.join(AI, *heat) if heat else None)


# name -> source folder, source stem, heat mask, max texture size in Unreal
SHIPS = {
    "Rift": _ship(["Rift"], "Rift", ["Rift", "heatmask_r1", "T_Rift_HeatMask.png"]),
    "Lancer": _ship(["Lancer"], "Lancer", ["Lancer", "heatmask_r1", "T_Lancer_HeatMask.png"]),
    "Bastion": _ship(["Bastion"], "Bastion", ["Bastion", "heatmask_r1", "T_Bastion_HeatMask.png"]),
    "NullFighter": _ship(["NullFleet", "Fighter"], "NullFighter", tex=1024),
    "NullInterceptor": _ship(["NullFleet", "Interceptor"], "NullInterceptor", tex=1024, rev="mesh_r2"),
    "NullBomber": _ship(["NullFleet", "Bomber"], "NullBomber", tex=1024),
    "NullCarrier": _ship(["NullFleet", "Carrier"], "NullCarrier", tex=2048),     # 46 m long: 1K smears
    "NullTurret": _ship(["NullFleet", "Turret"], "NullTurret", tex=1024),
    "NullGunship": _ship(["NullFleet", "Gunship"], "NullGunship", tex=1024),
}
