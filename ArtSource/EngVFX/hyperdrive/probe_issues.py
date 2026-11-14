# Hyperdrive look (T7): list a system's compile state and every stack issue (all severities). NAME prepended or default.
import json
NAME = globals().get('NAME', 'NS_TD_Hyperdrive_Tunnel')
T = 'NiagaraToolsets.NiagaraToolset_System.'
s = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Hyperdrive/' + NAME)
out = []
cs = await call_tool(T + 'GetSystemCompileState', system=s)
out.append('aggregate %s' % cs.get('aggregateStatus'))
for sc in cs.get('scripts', []):
    out.append('  %s %s %s' % (sc.get('emitterName'), sc.get('scriptName', '').split('::')[-1], sc.get('lastCompileStatus')))
iss = await call_tool(T + 'GetStackIssues', system=s)
for i in iss.get('issues', []):
    loc = i.get('location', {})
    out.append('ISSUE %s %s/%s/%s: %s' % (i.get('severity'), loc.get('emitterName'), loc.get('moduleName'), loc.get('inputNameStack'), (i.get('shortDescription', '') + ' - ' + i.get('longDescription', ''))[:220]))
summ = await call_tool(T + 'GetSystemSummary', system=s)
out.append('emitters: ' + json.dumps(summ)[:900])
"\n".join(out)
