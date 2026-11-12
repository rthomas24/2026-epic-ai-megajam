TRIAD DRIFT / SPACE FOUNDATION 01

Source-only first kit. No uassets or textures have been built; UE 5.9 runtime
validation is pending the claims gate. Open preview.html for an interactive
art-direction preview (Canvas 2D; no external resources).

AFTER CLAIMS ARE PUBLISHED, LEAD ONLY:
1. Python: generate_sky.py --write-assets --width 1024
2. In Unreal 5.9 Python: exec(compile(open(r'<absolute path>/build_unreal.py',
   encoding='utf-8').read(), r'<absolute path>/build_unreal.py', 'exec'))
   Set __file__ to the script path when running through exec, or use the
   engine's -ExecutePythonScript=<absolute path> launch option.
3. Assign MI_TD_Space_Combat to an inward-viewed Engine BasicShapes Sphere.
   Disable collision, shadow casting and ray-tracing visibility on that sky
   mesh. It must enclose the play volume, with no position-dependent parallax.
   Follow the map owner's placement process. This kit never edits a map.

Material instances expose yaw, nebula tint/intensity, stars and global gain.
Two texture samples, no runtime cloud noise, volume, particles or translucency.
Keep the combat view below the brightest nebula bank; rotate YawTurns to frame
objectives. Hero is for menus/screenshots, Combat for target readability.

Lighting values in presets.json are art starting points, not automatic changes.
Actual physical lighting is exposure-dependent; the generated sky is LDR colour
art with no physical HDR radiance calibration. Do not use it to calibrate lux.
Fix the preview exposure first, then balance ship lighting and stars in-engine.

The builder refuses pre-existing names. Review or rename manually for a new
version instead of overwriting another artist's material instance edits.

Validation without file generation:
  python generate_sky.py
Checks colour range, longitude seam, pole continuity and star-buffer dimensions.
Browser preview is an art approximation, not a screenshot of Unreal output.
