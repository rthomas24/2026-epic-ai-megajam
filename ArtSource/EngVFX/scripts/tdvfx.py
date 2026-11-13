# Triad Drift ENG-37: a small declarative builder for Niagara systems, driven through Unreal 5.9's
# Niagara toolset (NiagaraToolsets.NiagaraToolset_System) from inside the editor's Python.
#
# Use from a script run with run_editor_py.py:
#     import sys; sys.path.insert(0, <this folder>); import importlib, tdvfx; importlib.reload(tdvfx)
#     fx = tdvfx.Builder(call_tool)
#     await fx.system('NS_TD_Thing', user={'Color': (1, .2, .9, 1), 'Scale': 1.0}, emitters=[ {...}, ... ])
#
# Emitter spec keys:
#   name      emitter name
#   base      'burst' | 'rate' | 'ribbon' | 'single' | 'minimal'   (engine template emitters)
#   props     dict merged into the emitter data (bLocalSpace, SimTarget, CalculateBoundsMode, FixedBounds ...)
#   remove    [module names]                       add: [(script, module asset path)], appended in order
#   set       [(script, module, input, value), ...] applied in order (static switches first)
#   rend_pre  renderer properties set first (they unlock others)
#   mat       material asset path;  rend: dict merged into renderer 0;  rclass: replace renderer 0 by this class
# Values:  float/int/bool, (x,y) / (x,y,z) / (r,g,b,a) tuples, E('Display Name') enum by display name,
#          X('hlsl expression'), L('Particles.Position') linked parameter.
import asyncio
import json
import unreal

T = 'NiagaraToolsets.NiagaraToolset_System.'
ROOT = '/Game/TriadDrift/Gameplay/VFX'
MATS = ROOT + '/Materials/'
MOD = '/Niagara/Modules/'
TEMPLATES = {
    'burst': '/Niagara/DefaultAssets/Templates/Emitters/OmnidirectionalBurst',
    'rate': '/Niagara/DefaultAssets/Templates/Emitters/Fountain',
    'ribbon': '/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon',
    'single': '/Niagara/DefaultAssets/Templates/Emitters/SingleLoopingParticle',
    'minimal': '/Niagara/DefaultAssets/Templates/Emitters/Minimal',
}
ES, EU, PS, PU = 'EmitterSpawnScript', 'EmitterUpdateScript', 'ParticleSpawnScript', 'ParticleUpdateScript'


class E(object):
    def __init__(self, display):
        self.display = display


class X(object):
    def __init__(self, hlsl):
        self.hlsl = hlsl


class L(object):
    def __init__(self, name):
        self.name = name


def S(path):
    return {'refPath': path}


def encode(v):
    if isinstance(v, X):
        return {'struct': S('/Script/NiagaraEditor.NiagaraExt_StackInputData_HlslExpression'), 'value': {'hlslExpression': v.hlsl}}
    if isinstance(v, bool):
        return {'struct': S('/Script/Niagara.NiagaraBool'), 'value': {'value': -1 if v else 0}}
    if isinstance(v, int):
        return {'struct': S('/Script/Niagara.NiagaraInt32'), 'value': {'value': v}}
    if isinstance(v, float):
        return {'struct': S('/Script/Niagara.NiagaraFloat'), 'value': {'value': v}}
    if isinstance(v, (tuple, list)):
        if len(v) == 2:
            return {'struct': S('/Script/CoreUObject.Vector2f'), 'value': {'x': v[0], 'y': v[1]}}
        if len(v) == 3:
            return {'struct': S('/Script/CoreUObject.Vector3f'), 'value': {'x': v[0], 'y': v[1], 'z': v[2]}}
        if len(v) == 4:
            return {'struct': S('/Script/CoreUObject.LinearColor'), 'value': {'r': v[0], 'g': v[1], 'b': v[2], 'a': v[3]}}
    raise ValueError('cannot encode %r' % (v,))


class Builder(object):
    def __init__(self, call_tool):
        self.call = call_tool
        self.enums = {}
        self.log = []

    async def enum_name(self, enum_path, display):
        if enum_path not in self.enums:
            info = await self.call('NiagaraToolsets.NiagaraToolset_Info.UEnum_Info', enum=unreal.load_object(None, enum_path))
            if isinstance(info, str):
                info = json.loads(info)
            self.enums[enum_path] = {v['DisplayName']: v['Name'] for v in info['Values']}
        table = self.enums[enum_path]
        if display in table:
            return table[display]
        for k, n in table.items():
            if k.lower().startswith(display.lower()):
                return n
        raise ValueError('enum %s has no value "%s"; valid: %s' % (enum_path.split('.')[-1], display, list(table.keys())))

    async def set_input(self, system, emitter, script, module, name, value):
        ref = {'system': system, 'emitterName': emitter, 'scriptName': script, 'moduleName': module,
               'inputNameStack': name if isinstance(name, list) else [name]}
        try:
            if isinstance(value, E):
                cur = await self.call(T + 'GetStackInputData', stackInputRef=ref)
                enum_path = cur['value']['enum']['refPath']
                en = await self.enum_name(enum_path, value.display)
                data = {'struct': S('/Script/NiagaraEditor.NiagaraExt_StackInputData_Enum'),
                        'value': {'enum': S(enum_path), 'enumName': enum_path.split('.')[-1] + '::' + en}}
            elif isinstance(value, L):
                cur = await self.call(T + 'GetStackInputTopology', stackInputRef=ref)
                data = {'struct': S('/Script/NiagaraEditor.NiagaraExt_StackInputData_Linked'),
                        'value': {'linkedVariable': {'name': value.name, 'type': cur['type']}}}
            else:
                data = encode(value)
            await self.call(T + 'SetStackInputData', stackInputRef=ref, inputData=data)
        except Exception as ex:
            self.log.append('SET FAIL %s/%s/%s/%s: %s' % (emitter, script, module, name, str(ex)[:500]))

    async def emitter(self, system, spec):
        tmpl = unreal.load_asset(TEMPLATES[spec['base']])
        topo = await self.call(T + 'AddEmitter', system=system, templateEmitter=tmpl, emitterName=spec['name'])
        en = topo['emitterName']
        eref = {'system': system, 'emitterName': en}
        if spec.get('props'):
            await self.call(T + 'SetEmitterData', emitterRef=eref, emitterData={'propertyValues': json.dumps(spec['props'])})
        for script, module in spec.get('remove', []):
            try:
                await self.call(T + 'RemoveModule', moduleRef={'system': system, 'emitterName': en, 'scriptName': script, 'moduleName': module})
            except Exception as ex:
                self.log.append('REMOVE FAIL %s/%s: %s' % (en, module, str(ex)[:300]))
        adds = list(spec.get('add', []))
        if any('Forces/' in a[1] or 'Velocity/' in a[1] for a in adds if a[0] == PU) and spec['base'] != 'minimal' and not spec.get('keep_solver_order'):
            # force modules must run before the solver: take the solver out and put it back after them
            try:
                await self.call(T + 'RemoveModule', moduleRef={'system': system, 'emitterName': en, 'scriptName': PU, 'moduleName': 'SolveForcesAndVelocity'})
            except Exception:
                pass
            last_force = max(i for i, a in enumerate(adds) if a[0] == PU and ('Forces/' in a[1] or 'Velocity/' in a[1]))
            adds.insert(last_force + 1, (PU, 'Solvers/SolveForcesAndVelocity'))
        for item in adds:
            script, path = item[0], item[1]
            loc = {'system': system, 'emitterName': en, 'scriptName': script}
            if len(item) > 2:
                loc['moduleName'] = item[2]
            try:
                m = await self.call(T + 'AddModule', moduleLocationRef=loc, moduleAsset=unreal.load_asset(path if path.startswith('/') else MOD + path))
                if spec.get('verbose'):
                    self.log.append('added %s as %s' % (path, m['moduleName']))
            except Exception as ex:
                self.log.append('ADD FAIL %s/%s: %s' % (en, path, str(ex)[:300]))
        for script, module, name, value in spec.get('set', []):
            await self.set_input(system, en, script, module, name, value)
        rref = {'system': system, 'emitterName': en, 'rendererIndex': 0}
        if spec.get('rclass'):
            try:
                await self.call(T + 'RemoveRenderer', rendererRef=rref)
            except Exception as ex:
                self.log.append('RREMOVE FAIL %s: %s' % (en, str(ex)[:200]))
            await self.call(T + 'AddRenderer', newRendererLocation=eref, rendererClass=unreal.load_class(None, spec['rclass']))
        rend = dict(spec.get('rend', {}))
        if spec.get('mat'):
            m = spec['mat']
            m = m if m.startswith('/') else MATS + m
            rend['Material'] = S(m + '.' + m.split('/')[-1])
        if spec.get('rend_pre'):   # properties that unlock others (edit conditions), e.g. bOverrideMaterials
            try:
                await self.call(T + 'SetRendererData', rendererRef=rref, rendererData={'propertyValues': json.dumps(spec['rend_pre'])})
            except Exception as ex:
                self.log.append('REND PRE FAIL %s: %s' % (en, str(ex)[:400]))
        if rend:
            try:
                await self.call(T + 'SetRendererData', rendererRef=rref, rendererData={'propertyValues': json.dumps(rend)})
            except Exception as ex:
                self.log.append('REND FAIL %s: %s' % (en, str(ex)[:400]))
        return en

    async def user_vars(self, system, user):
        add = []
        for k, v in user.items():
            enc = encode(v)
            add.append({'name': k, 'type': {'classStructOrEnum': enc['struct']}, 'defaultValue': enc, 'description': ''})
        if add:
            await self.call(T + 'AddUserVariables', system=system, variablesToAdd=add)

    async def system(self, name, emitters, user=None, folder=None, props=None):
        folder = folder or ROOT + '/Systems'
        path = folder + '/' + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            system = unreal.load_asset(path)
        else:
            base = unreal.load_asset('/Niagara/DefaultAssets/DefaultSystem')
            await self.call(T + 'CreateNiagaraSystem', assetName=name, assetPath=folder, templateSystem=base)
            system = unreal.load_asset(path)
        summ = await self.call(T + 'GetSystemSummary', system=system)
        for e in summ['emitters']:
            await self.call(T + 'RemoveEmitter', emitterRef={'system': system, 'emitterName': e['emitterName']})
        await self.user_vars(system, user or {})
        for spec in emitters:
            try:
                await self.emitter(system, spec)
            except Exception as ex:
                self.log.append('EMITTER FAIL %s/%s: %s' % (name, spec.get('name'), str(ex)[:500]))
        if props:
            await self.call(T + 'SetSystemData', system=system, systemData={'propertyValues': json.dumps(props)})
        cs = await self.call(T + 'GetSystemCompileState', system=system)
        for _ in range(120):
            if not cs.get('bIsCompiling'):
                break
            await asyncio.sleep(0.5)
            cs = await self.call(T + 'GetSystemCompileState', system=system)
        bad = [s['emitterName'] + ':' + s['scriptName'].split('::')[-1] + ' ' + s['errorSummary'][:300] for s in cs['scripts'] if s['lastCompileStatus'] not in ('UpToDate', 'UpToDateWithWarnings') or s['errorSummary']]
        try:
            issues = await self.call(T + 'GetStackIssues', system=system)
            for i in issues.get('issues', []):
                if i.get('severity') == 'Error':
                    loc = i.get('location', {})
                    bad.append('ERR %s/%s/%s: %s' % (loc.get('emitterName'), loc.get('moduleName'), loc.get('inputNameStack'), (i.get('shortDescription', '') + ' - ' + i.get('longDescription', ''))[:260]))
        except Exception as ex:
            bad.append('issues? ' + str(ex)[:100])
        unreal.EditorAssetLibrary.save_loaded_asset(system)
        self.log.append('%s: %s%s' % (name, cs['aggregateStatus'], (' | ' + ' || '.join(bad)) if bad else ''))
        return system

    async def dump(self, system, emitter, modules=None):
        """Compact list of a built emitter's modules and visible input names (for discovering input names)."""
        topo = await self.call(T + 'GetEmitterTopology', emitterRef={'system': system, 'emitterName': emitter})
        out = []
        for sk in ('emitterUpdateScript', 'particleSpawnScript', 'particleUpdateScript'):
            for m in topo[sk]['modules']:
                if modules and m['moduleName'] not in modules:
                    continue
                out.append('%s.%s: %s' % (topo[sk]['scriptName'], m['moduleName'], ' | '.join(i['name'] + ('$' if i['bIsStaticSwitch'] else '') for i in m['inputs'] if i['bIsVisible'])))
        return '\n'.join(out)

    def report(self):
        return '\n'.join(self.log)
