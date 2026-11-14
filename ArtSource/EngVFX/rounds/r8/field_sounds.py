# Lead (batch M): the four engineering fallback break sounds on DA_SpaceField_FX, Sound fields only.
EAL = unreal.EditorAssetLibrary
da = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/FX/DA_SpaceField_FX')
out = []
for field, snd in (('break_small', 'MSS_Field_Break_Small'), ('break_medium', 'MSS_Field_Break_Medium'), ('break_large', 'MSS_Field_Break_Large'), ('break_special', 'MSS_Field_Break_Special')):
    c = da.get_editor_property(field)
    s = unreal.load_asset('/Game/TriadDrift/Gameplay/Audio/Field/' + snd)
    if not s:
        out.append('NO SOUND ' + snd); continue
    c.set_editor_property('sound', s)
    da.set_editor_property(field, c)
    c2 = da.get_editor_property(field)
    out.append('%s sound=%s vfx=%s' % (field, snd, str(c2.get_editor_property('vfx')).split("'")[1].split('.')[-1]))
out.append('saved' if EAL.save_loaded_asset(da, False) else 'SAVE FAILED')
"\n".join(out)
