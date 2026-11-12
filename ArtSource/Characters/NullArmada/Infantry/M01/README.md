# Null Armada Infantry M01

**Agent:** Characters & Enemies Agent\
**Role:** Humanoid character and enemy asset production\
**Task:** Standard infantry source preparation

Status: **offline preparation only**. No character geometry, Blender source,
FBX, renders, rigging, animation or native integration exists in this checkpoint.

- [Design brief](brief.md)
- [Approved four-view reference](refs/NullArmada_Infantry_Approved_FourView.png)
- [Component and budget plan](production-plan.json)
- [Source provenance](source/SOURCE.md)
- [Geometry and review verification plan](qa-plan.md)
- [Current checkpoint](checkpoint.json)
- [Preparation file hashes](preparation-manifest.json)

Lead activated this one infantry milestone after verified HUD ca579ab handoff,
publishing exact ownership at da9a0a4. Work is isolated on
`nick/characters-infantry` in
`C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/characters-infantry`.
Only this M01 folder is writable. HUD and all unrelated work remain frozen.

N011 confirms the existing UE5 mannequin/Manny A-pose foundation at about180cm,
feet at origin and Unreal +X forward. The new worktree intentionally contains
LFS pointer packages. The frozen HUD worktree has actual mannequin packages,
available read-only for a separately scheduled export; they have not been exported.
Reference/provenance hashes record that distinction.

Next operation requires a **fresh Lead heavy-slot grant**: read-only base export
and inspection, then original armor modeling and staged QA in Blender. No launch
is authorized by this document or by available memory. Later M01 delivery requires
a complete full-body editable model, four orthographic views, one beauty view,
measured geometry/material/scale checks and Lead-owned commit/push. Nick must
approve the model before rigging, animation, native integration or another character.

Lead's Nick-delegated priority is infantry before the deferred Lancer pilot.
The complete character/enemy campaign is preserved; heavy, sensor and interceptor
work remains inactive. No model-completion, AAA acceptance or gameplay-ready claim.
