"""Lightweight preparation from existing owned sources. No downloads, native import or shared edits."""
from pathlib import Path
import hashlib,json,os,subprocess,sys

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
PRIVATE=ROOT/'Saved/AudioMusic/PrototypeV1/Combat01'
SKILL=ROOT/'.claude/skills/ue-game-audio/scripts'
VROOT='/Game/TriadDrift/Audio/Combat01'
os.environ['PATH']='C:/Program Files/Shotcut'+os.pathsep+os.environ['PATH']

def run(name,*args):
    subprocess.run([sys.executable,str(SKILL/name),*map(str,args)],check=True)

def main():
    PRIVATE.mkdir(parents=True,exist_ok=True)
    original_manifest=json.loads((ROOT/'Saved/AudioMusic/PrototypeV1/LibraryOriginals01/sources.json').read_text())
    for row in original_manifest:
        assert hashlib.sha256(Path(row['preview']).read_bytes()).hexdigest()==row['preview_sha256']
    needle=PRIVATE/'prepared/NeedleCannonShort'
    run('audio_prep.py',*[row['preview'] for row in original_manifest], '--category','sfx','--name','SFX_NeedleCannon','--max-duration',.32,'--fade-in-ms',1,'--fade-out-ms',30,'--out',needle)
    source_manifest=json.loads((ROOT/'ArtSource/Audio/PrototypeV1/sources.json').read_text())['assets']
    sources=original_manifest[:]
    sets={'NeedleCannon':sorted(needle.glob('*.wav'))}
    for name,filename,cut in (('Hull','Hit_Generic_2-1.wav',.30),('Shield','Shield_Metal_Impact_1-3.wav',.55)):
        raw=ROOT/'Saved/AudioMusic/PrototypeV1/source'/filename
        prepared=PRIVATE/'prepared'/name
        run('audio_prep.py',raw,'--category','sfx','--name','SFX_Combat_'+name+'Impact','--max-duration',cut,'--fade-in-ms',1,'--fade-out-ms',35,'--out',prepared)
        recipe={'name':'SFX_Combat_'+name+'Impact','category':'sfx','takes':3,'seed':2701 if name=='Hull' else 2702,
                'layers':[{'role':'existing licensed impact','file':str(next(prepared.glob('*.wav'))),'vary':{'pitch_semitones':.25,'gain_db':.25}}],
                'master':{'fade_out_ms':30}}
        rp=HERE/(name.lower()+'.recipe.json');rp.write_text(json.dumps(recipe,indent=2)+'\n')
        output=PRIVATE/'export'/name;run('audio_layer.py',rp,'--out',output);sets[name]=sorted(output.glob('*.wav'))
        sources.extend(row for row in source_manifest if row['event']==name)
    for name,files in sets.items():
        run('audio_report.py',*files,'--category','sfx','--takes','--json',HERE/('qa_'+name+'.json'))
    old=json.loads((ROOT/'ArtSource/Audio/PrototypeV1/build.spec.json').read_text())
    mix=[]
    for item in old['mix']:
        last=item['path'].rsplit('/',1)[-1]
        if last not in ('SC_V1_SFX','ATT_V1_Medium','SCON_V1_Weapon','SCON_V1_Impact'):continue
        item=json.loads(json.dumps(item).replace('/Game/TriadDrift/Audio/PrototypeV1/Mix/',VROOT+'/Mix/').replace('_V1_','_Combat01_'))
        if item['type']=='SoundClass':item['properties']={'properties.volume':1.0}
        mix.append(item)
    spec={'mix':mix,'import':[],'oneshots':[]}
    levels={'NeedleCannon':-8,'Hull':-14,'Shield':-16}
    for name,files in sets.items():
        waves=[]
        for i,file in enumerate(files,1):
            path=VROOT+'/Waves/SW_'+name+'_%02d'%i
            spec['import'].append({'file':str(file),'path':path,'properties':{'bLooping':False}});waves.append(path)
        spec['oneshots'].append({'path':VROOT+'/MSS_'+name,'format':'Mono','sounds':waves,'volume_db':levels[name],
            'volume_spread_db':1.5,'pitch_semitones':0,'pitch_spread':.7,
            'sound':{'soundClassObject':VROOT+'/Mix/SC_Combat01_SFX','attenuationSettings':VROOT+'/Mix/ATT_Combat01_Medium',
                     'concurrencySet':[VROOT+'/Mix/SCON_Combat01_'+('Weapon' if name=='NeedleCannon' else 'Impact')]}})
    (HERE/'build.spec.json').write_text(json.dumps(spec,indent=2)+'\n')
    (HERE/'sources.json').write_text(json.dumps({'source_records':sources,'source_audio_git':False,'native_publication':'Private team game assets only, subject to Lead license review; no standalone source redistribution',
         'prepared':[{'path':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for files in sets.values() for f in files]},indent=2)+'\n')
    print('Prepared nine waves/three real-game cue specifications; NO native import/binding executed.')

if __name__=='__main__':main()
