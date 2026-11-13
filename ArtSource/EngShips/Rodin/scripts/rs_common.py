# SPDX-License-Identifier: MIT
# Triad Drift - shared table for the Rodin ship intake (Grok Bot's Hyper3D Rodin meshes -> game ships).
# Sources are read-only under ArtSource/EngAI/Ships; everything we write goes to ArtSource/EngShips/.
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(HERE))))
AI = os.path.join(REPO, "ArtSource", "EngAI", "Ships")
OUT = os.path.join(REPO, "ArtSource", "EngShips")


def _ship(folder, stem, heat=None, tex=2048):
    return dict(src=os.path.join(AI, *folder, "mesh_r1"), stem=stem, tex=tex,
                heat=os.path.join(AI, *heat) if heat else None)


# name -> source folder, source stem, heat mask, max texture size in Unreal
SHIPS = {
    "Rift": _ship(["Rift"], "Rift", ["Rift", "heatmask_r1", "T_Rift_HeatMask.png"]),
    "Lancer": _ship(["Lancer"], "Lancer", ["Lancer", "heatmask_r1", "T_Lancer_HeatMask.png"]),
    "Bastion": _ship(["Bastion"], "Bastion", ["Bastion", "heatmask_r1", "T_Bastion_HeatMask.png"]),
    "NullFighter": _ship(["NullFleet", "Fighter"], "NullFighter", tex=1024),
    "NullInterceptor": _ship(["NullFleet", "Interceptor"], "NullInterceptor", tex=1024),
    "NullGunship": _ship(["NullFleet", "Gunship"], "NullGunship", tex=1024),
}
