"""Triad Drift - check audio assets INSIDE the editor: the engine's own wave analysis, and an offline
render of what a sound asset actually plays.

Sent to the editor's `execute_python` tool by ue_run_python.py, which defines SPEC before this text:

    python ue_run_python.py --port 8021 ue_audio_check.py --spec check.json

SPEC:
  "waves":  ["/Game/TriadDrift/Audio/SFX/Weapons/SW_X_01", ...]
            -> AudioAnalysisSubsystem.analyze_sound_waves: loudness (gated and ungated), true peak, leading
               and trailing silence, loop seam jump, per wave. Nothing is played.
  "render": [{"sound": "/Game/TriadDrift/Audio/SFX/Weapons/MSS_X", "name": "MSS_X", "plays": 5,
              "gap_seconds": 0.8, "tail_seconds": 1.0, "params": {"Speed01": 1.0}}]
            -> plays the sound on the editor preview voice while recording the main output, and writes
               <Project>/Saved/BouncedWavFiles/ue-game-audio/<name>.wav. Measure it with audio_report.py and
               look at it with audio_view.py: are the plays different (random take, pitch, level)? any
               clipping? does a loop run without a click? does a parameter change the sound?

The render needs an editor that can render audio: start it with -NonRealtimeAudio (offline, nothing
comes out of the speakers) instead of -nosound. The preview voice is not the game mix: sound class
volumes and mixes apply, distance attenuation and the in-game listener do not, so use the render for
"what does it play" and relative comparisons, never as the in-game level.
Checked on UE 5.9.0 CL 60351404, 2026-11-12.
"""
RESULT = {'waves': [], 'renders': [], 'errors': []}
KEYS = ('duration_seconds', 'num_channels', 'sample_rate', 'integrated_loudness_lufs', 'ungated_loudness_lufs',
        'loudness_range_lu', 'sample_peak_db', 'true_peak_db', 'rms_db', 'is_silent', 'leading_silence_seconds',
        'trailing_silence_seconds', 'loop_seam_jump_db')


def object_path(path):
    path = path.split('.')[0]
    return '%s.%s' % (path, path.rsplit('/', 1)[1])


wave_paths = SPEC.get('waves', [])
if wave_paths:
    try:
        waves = [unreal.load_asset(object_path(p)) for p in wave_paths]
        missing = [p for p, w in zip(wave_paths, waves) if not w]
        if missing:
            raise RuntimeError('cannot load: %s' % missing)
        pending = unreal.get_editor_subsystem(unreal.AudioAnalysisSubsystem).analyze_sound_waves(waves, unreal.AudioAnalysisSettings())
        for _ in range(600):
            done = pending.is_complete  # a property in some builds, a method in others
            if done() if callable(done) else done:
                break
            await asyncio.sleep(0.1)
        reports = pending.get_editor_property('value')
        if hasattr(reports, 'get_editor_property'):  # a struct wrapping the array in some builds
            reports = reports.get_editor_property('reports')
        for path, report in zip(wave_paths, reports):
            data = report.to_dict()
            row = {'wave': path}
            for key in KEYS:
                value = data.get(key)
                row[key] = round(value, 3) if isinstance(value, float) else value
            RESULT['waves'].append(row)
    except Exception as error:
        RESULT['errors'].append('waves: %s' % str(error)[:500])

renders = SPEC.get('render', [])
if renders:
    preview = unreal.get_editor_subsystem(unreal.AudioPreviewSubsystem)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not preview.is_preview_available():
        RESULT['errors'].append('render: editor audio is off (-nosound). Start the editor with -NonRealtimeAudio instead.')
        renders = []
    for item in renders:
        try:
            sound = unreal.load_asset(object_path(item['sound']))
            if not sound:
                raise RuntimeError('cannot load %s' % item['sound'])
            name = item.get('name') or item['sound'].rsplit('/', 1)[1]
            plays = int(item.get('plays', 4))
            gap = float(item.get('gap_seconds', 0.8))
            unreal.AudioMixerLibrary.start_recording_output(world, plays * gap + 5.0)
            for index in range(plays):
                preview.preview_sound(sound)
                for key, value in (item.get('params') or {}).items():
                    preview.set_preview_float_parameter(key, float(value))
                await asyncio.sleep(gap)
            preview.stop_preview()
            await asyncio.sleep(float(item.get('tail_seconds', 1.0)))
            unreal.AudioMixerLibrary.stop_recording_output(world, unreal.AudioRecordingExportType.WAV_FILE, name, 'ue-game-audio')
            await asyncio.sleep(0.5)
            saved = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
            RESULT['renders'].append('%sBouncedWavFiles/ue-game-audio/%s.wav' % (saved, name))
        except Exception as error:
            RESULT['errors'].append('render %s: %s' % (item.get('sound'), str(error)[:500]))

RESULT
