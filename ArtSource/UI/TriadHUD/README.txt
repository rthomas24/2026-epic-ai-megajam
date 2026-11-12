TRIAD DRIFT — HUD STUDY 01

Open index.html in a modern desktop browser. No server, download or dependency
is required. Everything shown is explicitly MOCK DATA, including target speed,
range, health, control glyphs and signature readiness.

Click ship cards or press 1 / 2 / 3. Tab cycles only when the scene itself has
focus; normal control focus remains keyboard navigable. H previews a hit and
S previews the Director's Cut slate. Controls below the scene also preview
target visibility, low hull, and reduced motion. No bright full-screen flash.

Deliverables: responsive 16:9 interactive visual concept; standalone 64x64 SVG
icons in icons/; tokens.json. SVG icons use currentColor, stroke width 2 and
transparent backgrounds. Tint in the UI. They are source assets, not UE
uassets. Rasterize icons at 128 or 256px after the binary-claim gate if needed.

Presentation API: window.TriadHUDPreview.setMockState({active:0,
ships:[{hull:100,shield:85,boost:80,heat:25}],targetVisible:true,
lowHull:false,take:2}). Values are display percentages, clamped 0–100.
selectShip(0..2), hit(), scene(), getState() are also available.

Integration TODO: port layout to WBP_SpaceHUD and connect a separate adapter
only after Ryan finalizes his read-only presentation snapshot. No gameplay
class, map, config or interface is modified or assumed to exist by this concept.
Fonts are platform fonts (Arial/Helvetica/monospace), not bundled font assets.
The simple procedural background is illustrative context, not environment art.

Preview target: desktop 1920x1080; scales at 16:9. Small-screen text is for
composition inspection, not evidence of console accessibility compliance.

Original art and code created for Nicholas with OpenAI Codex, 2026-11-12.

A standalone source overview is in hud-overview.svg. Regenerate the overview
and 12 SVG icons with: node export_sources.js. This writes text SVG only.
Validation: node syntax checks; in-memory display-state smoke checks; all
13 SVG documents parsed as XML. Browser visual QA remains for the Lead.
