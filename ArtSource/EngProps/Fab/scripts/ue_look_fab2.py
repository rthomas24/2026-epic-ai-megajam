# SPDX-License-Identifier: MIT
# Triad Drift - one in-engine look at the second Fab batch (PipeBeam, Mechanic Base parts) beside the pilot, run through
# the editor MCP Python tool in an offscreen editor on the template level. Unsaved actors only; logs "TDPY: LOOK2 ..." cameras.
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ue_mechbase_pick import PICK  # noqa: E402

MB = "/Game/TriadDrift/Gameplay/Props/Fab/MechanicBase/Mesh/SM/"
PILOT = "/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot"
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
Z = 210.0


def mesh(path, x, y, yaw=0.0):
    a = sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, Z), unreal.Rotator(0, 0, yaw))
    a.static_mesh_component.set_static_mesh(unreal.load_asset(path))
    return a


def pilot(x, y):
    p = sub.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x, y, Z), unreal.Rotator(0, 0, 180))
    p.skeletal_mesh_component.set_skinned_asset_and_update(unreal.load_asset(PILOT))


# PipeBeam: lying along Y, a second one standing on end as a pylon
mesh("/Game/TriadDrift/Gameplay/Props/Fab/PipeBeam/SM_Fab_PipeBeam", 600, 2500)
pilot(450, 2300)
unreal.log("TDPY: LOOK2 PipeBeam 600 2500 %.0f" % Z)
# Mechanic Base: props in a row of 16, frames behind
x0, y = 700.0, 4200.0
props = [p for p in PICK if "DOOR" not in p and "FRAME" not in p]
frames = [p for p in PICK if p not in props]
for i, rel in enumerate(props):
    mesh(MB + rel, x0, y + i * 260)
for i, rel in enumerate(frames):
    mesh(MB + rel, x0 + 700, y + i * 850)
pilot(x0 - 150, y + 600)
unreal.log("TDPY: LOOK2 MechanicBase %.0f %.0f %.0f" % (x0, y, Z))
unreal.log("TDPY: OK")
