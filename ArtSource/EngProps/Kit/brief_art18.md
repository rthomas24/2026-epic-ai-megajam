# ART-18 story props from the kit (structures builder, Nov 13 night)

Route C (code-built in Blender, `scripts/kit_pieces_art18.py`), the planet kit's trim sheet and Glow slot, one convex
hull each (walk-clear: no thin part the capsule can catch). Built with `build_kit.py --set planet`, imported by
`ue_import_kit.py`, placed by `sites/SurveyLine_Verdance.manifest.json` and `sites/Artefact.manifest.json`.

## Survey instruments on one buried line (Verdance route, storyline Act 2)

The Triad's survey team left instruments along a line over something buried: the player follows the line.
They read as human field kit, small, practical, a little worn; teal status lights, nothing hostile.

| Mesh | Size (cm) | Brief |
|---|---|---|
| `SM_Kit_SurveyTripod` | 96 x 110 x 172 | Three splayed legs to a hub at 1.3 m, a sensor head with a lens facing +X and a teal status strip. The tallest; marks the line from a distance. |
| `SM_Kit_SurveyProbe` | 24 x 28 x 110 | A stake driven into the ground (collar), a readout box at hip height with a teal strip. |
| `SM_Kit_SeismicPod` | 60 x 60 x 90 | A squat drum pressed into the soil, a teal ring on the lid, a whip antenna. |

`BP_SurveyLine_Verdance` (Kit/Sites): six instruments 6 m apart along +X (30 m), probe, pod, tripod, pod, probe,
tripod. The gaps are free to walk through.

## The ancient artefact (storyline section 6 clue)

"The same ancient technology on both planets", and again in the reactor chamber. It must read as old and deliberate,
made by neither the Null Armada nor the Triad, and the same object every time the player sees it.

| Mesh | Size (cm) | Brief |
|---|---|---|
| `SM_Kit_Artefact` | 184 x 159 x 262 | A hexagonal stone plinth, a stone core, three leaning triangular blades locked around it with teal glyph channels down their outer edges, and a triangular capstone. Slots: `Stone` (`MI_Kit_Stone`, the relay's ancient stone) and `Glow` (teal). |

`BP_Kit_Artefact` (Kit/Sites): the artefact plus a soft teal light (250 cd, 7 m) so it reads at 20 m. One class for
Verdance, Cinder and the core chamber (T4 / T5 place it).
