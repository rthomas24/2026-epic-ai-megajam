# Read-only: what the round-6 leftover slots hold today (VFX, VFXScale, and whether a Sound is set).
CH = '/Game/TriadDrift/Gameplay/Characters/'
WANT = {'FX/DA_Travel_FX': ['portal_open', 'portal_close', 'portal_idle', 'portal_transit', 'hyperdrive_tunnel', 'hyperdrive_spool', 'hyperdrive_drop_out'],
        'FX/DA_Ability_EMPPulse_FX': ['activate', 'loop', 'end', 'affected_target'],
        'FX/DA_Ability_Cloak_FX': ['activate', 'loop', 'end'],
        'FX/DA_Ability_GravityWell_FX': ['activate', 'loop', 'end', 'affected_target'],
        'Dominion/DA_Dominion_Visuals': ['shield_idle', 'shield_collapse', 'distortion_field_idle', 'distortion_field_contact', 'distortion_field_collapse', 'distortion_field_material'],
        'NullTrooper/DA_Crew_Trooper_Visuals': ['hull_hit', 'shield_hit', 'death']}
out = []
for rel, fields in WANT.items():
    da = unreal.load_asset(CH + rel)
    if not da:
        out.append('MISSING ' + rel); continue
    for f in fields:
        try:
            v = da.get_editor_property(f)
        except Exception as ex:
            out.append('%s.%s: no such field' % (rel, f)); continue
        if isinstance(v, unreal.TDFXCue):
            out.append('%s.%s: vfx=%s scale=%s sound=%s' % (rel, f, str(v.get_editor_property('vfx')).split('.')[-1][:50], v.get_editor_property('vfx_scale').x,
                                                            'set' if str(v.get_editor_property('sound')) not in ('None', '') else 'none'))
        else:
            out.append('%s.%s: %s' % (rel, f, str(v)[:80]))
"\n".join(out)
