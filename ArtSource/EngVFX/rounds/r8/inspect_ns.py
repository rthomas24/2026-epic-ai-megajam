# 10g: why do the small sprite emitters built this round not render? Compare user variables and the first renderer of a
# working system with failing ones (headless is enough: data only).
import json
T = 'NiagaraToolsets.NiagaraToolset_System.'
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
out = []
for name, em in (('NS_TD_Dominion_LightRow', 'Light00'), ('NS_TD_Pickup_Hull', None), ('NS_TD_Debug_GlowOffset', 'Beacon'), ('NS_TD_Cloak_Shimmer', 'Glints'), ('NS_TD_Region_Beacon', 'Beacon')):
    s = unreal.load_asset(SYS + name)
    summ = await call_tool(T + 'GetSystemSummary', system=s)
    out.append('== %s: %s' % (name, json.dumps({k: v for k, v in summ.items() if k != 'emitters'})[:900]))
    ems = [e['emitterName'] for e in summ['emitters']]
    e = em or ems[0]
    for tool, arg in (('GetEmitterData', {'emitterRef': {'system': s, 'emitterName': e}}), ('GetRendererData', {'rendererRef': {'system': s, 'emitterName': e, 'rendererIndex': 0}})):
        try:
            r = await call_tool(T + tool, **arg)
            out.append('  %s %s: %s' % (e, tool, json.dumps(r)[:1400]))
        except Exception as ex:
            out.append('  %s %s FAILED %s' % (e, tool, str(ex)[:200]))
"\n".join(out)
