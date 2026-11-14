# 10i (Lead, cook warnings): delete the dropped range-glow material and its instances, every debug/review system, the dropped
# region beacon and the dropped haze / beacon materials; refuse to delete anything still referenced by a kept asset.
EAL = unreal.EditorAssetLibrary
reg = unreal.AssetRegistryHelpers.get_asset_registry()
M = '/Game/TriadDrift/Gameplay/VFX/Materials/'; S = '/Game/TriadDrift/Gameplay/VFX/Systems/'
drop = [S + n for n in ('NS_TD_Debug_RangeA', 'NS_TD_Debug_BeaconLOD', 'NS_TD_Debug_BeaconFixed', 'NS_TD_Debug_BeaconFixedB', 'NS_TD_Debug_GlowOffset',
                        'NS_TD_Debug_HazeField', 'NS_TD_Debug_HazeBase', 'NS_TD_Debug_HazeSphere', 'NS_TD_Debug_Haze2', 'NS_TD_Region_Beacon')] + \
       [M + n for n in ('MI_TD_VFX_RangeGlow_Beacon', 'MI_TD_VFX_RangeGlow_Plume', 'M_TD_VFX_RangeGlow', 'MI_TD_RegionHaze_Review', 'M_TD_RegionHaze', 'M_TD_RegionHaze2', 'M_TD_VFX_Beacon')]
out = []
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
for p in drop:
    if not EAL.does_asset_exist(p):
        out.append('absent ' + p); continue
    refs = [str(r) for r in (reg.get_referencers(p, opts) or []) if str(r) not in drop]
    if refs:
        out.append('KEPT %s: referenced by %s' % (p, refs)); continue
    out.append(('deleted ' if EAL.delete_asset(p) else 'DELETE FAILED ') + p)
# what the remaining systems use
mats = {}
for d in reg.get_assets_by_path(S[:-1], recursive=True):
    if str(d.asset_class_path.asset_name) != 'NiagaraSystem':
        continue
    for dep in reg.get_dependencies(str(d.package_name), opts) or []:
        dep = str(dep)
        if dep.startswith('/Game/') and ('/Materials/' in dep or '/M_' in dep or '/MI_' in dep):
            mats.setdefault(dep, []).append(str(d.asset_name))
out.append('materials used by %d systems:' % len({s for v in mats.values() for s in v}))
for k in sorted(mats):
    out.append('  %s (%d systems)' % (k, len(mats[k])))
"\n".join(out)
