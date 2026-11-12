"""Rebuild V1 from Nick's local, already owned packs; no acquisition or source edits."""
from pathlib import Path
import argparse, datetime, hashlib, importlib.util, json, os, subprocess, sys
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
PRIVATE=ROOT/'Saved/AudioMusic/PrototypeV1'
AUDIT=Path(r'C:\Users\nicho\OneDrive\Desktop\GameJam\Worktrees\audio-music\ArtSource\Audio\EpicLibraryAudit01')
SKILL_COMMIT='4096f47'
VROOT='/Game/TriadDrift/Audio/PrototypeV1'
PACK50='50FreeSo92fbc86158f1V1'
PACKTERM='Terminal898ca54d9692V1'
PACKMUSIC='Atmoshphe294217f9506V1'
CHOICES=[
 ('Engine',PACKTERM,'CMPTMisc_ComputerAmbienceLoop_HA_TerminalUI_06_Wav','loop',1,-14),
 ('Acceleration',PACKTERM,'MACHAppl_PowerUp_HA_TerminalUI_02_Wav','sfx',3,-10),
 ('Primary',PACK50,'Sci-Fi_Gun_1-1','sfx',3,-6),
 ('Missile',PACK50,'Fire_Whoosh_2-15','sfx',3,-5),
 ('Explosion',PACK50,'Explosion_Medium_2-1','sfx',3,-7),
 ('Shield',PACK50,'Shield_Metal_Impact_1-3','sfx',3,-11),
 ('Hull',PACK50,'Hit_Generic_2-1','sfx',3,-9),
 ('Rift',PACK50,'Whoosh_4-1','sting',1,-10),
 ('Lancer',PACKTERM,'MACHAppl_PowerUp_HA_TerminalUI_02_Wav','sting',1,-10),
 ('Bastion',PACKTERM,'CMPTMisc_DiskTrayClose_HA_TerminalUI_02_Wav','sting',1,-10),
 ('Target',PACKTERM,'MACHAppl_VCRButton_HA_TerminalUI_04_Wav','ui',1,-16),
 ('Ability',PACKTERM,'CMPTMisc_PowerButton_HA_TerminalUI_02_Wav','ui',1,-12),
 ('Explore',PACKMUSIC,'Nordic_Exploration__The_Ancient_Moon','music',1,0),
 ('Combat',PACKMUSIC,'Trailer_Solaric','music',1,0),
]
def run(args):
 p=subprocess.run([str(x) for x in args],text=True,capture_output=True)
 if p.returncode: raise RuntimeError(f'{args}\n{p.stdout[-2500:]}\n{p.stderr[-2500:]}')
 return p.stdout
def main():
 PRIVATE.mkdir(parents=True,exist_ok=True)
 tools=PRIVATE/'skill4096f47';tools.mkdir(exist_ok=True)
 for name in ['audio_common.py','audio_prep.py','audio_layer.py','audio_report.py','audio_view.py','ue_run_python.py','ue_audio_build.py','ue_audio_check.py']:
  data=subprocess.check_output(['git','show',f'{SKILL_COMMIT}:.claude/skills/ue-game-audio/scripts/{name}'],cwd=ROOT)
  (tools/name).write_bytes(data)
 os.environ['PATH']=r'C:\Program Files\Shotcut'+os.pathsep+os.environ['PATH']
 m=importlib.util.spec_from_file_location('audit_helpers',AUDIT/'build_audit.py');a=importlib.util.module_from_spec(m);m.loader.exec_module(a)
 rawdir=PRIVATE/'source';out=PRIVATE/'export';rawdir.mkdir(exist_ok=True);out.mkdir(exist_ok=True)
 reports={}; sources=[]; asset_import=[]; shots=[];loops=[]
 # Optional identity information is not an acquisition: all inputs must already exist in VaultCache.
 for name,pid,original,cat,takes,level in CHOICES:
  matches=list((a.VAULT/pid/'data').rglob(original+'.uasset'))
  assert len(matches)==1,(name,matches)
  p=matches[0];before=a.sha(p.read_bytes());r,method=a.payload(p.read_bytes());wav,transform=a.decode_wave(r)
  raw=rawdir/(original+'.wav');raw.write_bytes(wav)
  if name=='Combat':
   selection=rawdir/(original+'_selection.wav')
   run(['ffmpeg','-hide_banner','-nostdin','-y','-v','error','-ss','0.75','-i',raw,'-t','48','-c:a','pcm_s16le',selection])
   raw=selection
  sources.append(dict(event=name,pack=a.PACKS[pid][0],listing='https://www.fab.com/listings/'+a.PACKS[pid][1],original_asset=original,original_path=str(p),source_sha256=before,extraction=method,restoration=transform))
  prepdir=PRIVATE/'prepared'/name
  args=[sys.executable,tools/'audio_prep.py',raw,'--category',cat,'--name','V1_'+name,'--out',prepdir]
  if cat=='loop':args+=['--loop-crossfade','0.6']
  elif cat=='music':args+=['--max-duration','48','--loop-crossfade','2','--fade-in-ms','0','--fade-out-ms','0','--target','-10' if name=='Explore' else '-3']
  elif name=='Primary':args+=['--max-duration','0.32','--fade-out-ms','30']
  elif name=='Shield':args+=['--max-duration','0.65']
  elif name=='Hull':args+=['--tail-db','-45','--fade-out-ms','20']
  logs=[run(args)]
  final=prepdir
  if takes>1:
   recipe=dict(name='V1_'+name,category=cat,takes=takes,seed=1947,layers=[dict(role='existing designed source',file=str(prepdir/('V1_'+name+'_01.wav')),fade_in_ms=1,fade_out_ms=30,vary=dict(pitch_semitones=.25,gain_db=.25))],master=dict(fade_out_ms=30))
   rp=PRIVATE/(name+'.recipe.json');rp.write_text(json.dumps(recipe,indent=2))
   final=out/name;logs.append(run([sys.executable,tools/'audio_layer.py',rp,'--out',final]))
  files=sorted(final.glob('*.wav'))
  report=HERE/('qa_'+name+'.json')
  args=[sys.executable,tools/'audio_report.py',*files,'--category',cat,'--json',report]
  if takes>1:args+=['--takes']
  logs.append(run(args));reports[name]='\n'.join(logs)
  wavepaths=[]
  for i,f in enumerate(files,1):
   dest=VROOT+'/Waves/SW_V1_'+name+f'_{i:02d}'
   wavepaths.append(dest)
   asset_import.append(dict(file=str(f),path=dest,properties=dict(loadingBehavior='RetainOnLoad' if cat!='music' else 'PrimeOnLoad',bLooping=cat in ('music','loop'),virtualizationMode='PlayWhenSilent' if cat in ('music','loop') else 'Restart')))
  cls='Music' if cat=='music' else 'UI' if cat=='ui' else 'Cine' if cat=='sting' else 'SFX'
  sound=dict(soundClassObject=VROOT+'/Mix/SC_V1_'+cls)
  if cat in ('music','loop'):sound['virtualizationMode']='PlayWhenSilent'
  if cat in ('sfx','loop'):
   sound['attenuationSettings']=VROOT+'/Mix/ATT_V1_'+('Near' if cat=='loop' else 'Medium')
   sound['concurrencySet']=[VROOT+'/Mix/SCON_V1_'+('Loop' if cat=='loop' else 'Weapon' if name in ('Primary','Missile','Acceleration') else 'Explosion' if name=='Explosion' else 'Impact')]
  entry=dict(path=VROOT+'/Sources/MSS_V1_'+name,format='Stereo' if cat in ('music','sting') else 'Mono',sound=sound)
  if cat in ('music','loop'):
   entry.update(wave=wavepaths[0],param='Speed01' if cat=='loop' else 'Intensity',pitch_range=[-2,3] if cat=='loop' else [0,0],gain_range_db=[-14,-9] if cat=='loop' else [0,0],smooth_seconds=.15 if cat=='loop' else 1.5);loops.append(entry)
  else:
   entry.update(sounds=wavepaths,volume_db=level,volume_spread_db=1.5 if takes>1 else 0,pitch_semitones=0,pitch_spread=.7 if takes>1 else 0);shots.append(entry)
  assert a.sha(p.read_bytes())==before
 (HERE/'sources.json').write_text(json.dumps(dict(checked_utc=datetime.datetime.now(datetime.UTC).isoformat(),skill_commit=SKILL_COMMIT,originals_unchanged=True,assets=sources),indent=2))
 (HERE/'preparation.log').write_text('\n\n'.join(k+'\n'+v for k,v in reports.items()))
 mix=[dict(type='SoundClass',path=VROOT+'/Mix/SC_V1_Master')]
 for bus in ['SFX','UI','Cine','Music']:
  props=dict(parentClass=VROOT+'/Mix/SC_V1_Master')
  if bus=='Music':props['properties.bIsMusic']=True;props['properties.volume']=1.0
  if bus=='UI':props['properties.bIsUISound']=True
  mix.append(dict(type='SoundClass',path=VROOT+'/Mix/SC_V1_'+bus,properties=props))
 for name,inner,outer in [('Near',2500,20000),('Medium',3000,60000)]:
  mix.append(dict(type='SoundAttenuation',path=VROOT+'/Mix/ATT_V1_'+name,properties={'attenuation.bAttenuate':True,'attenuation.bSpatialize':True,'attenuation.attenuationShapeExtents':dict(x=inner,y=0,z=0),'attenuation.falloffDistance':outer-inner,'attenuation.nonSpatializedRadiusStart':2500,'attenuation.bAttenuateWithLPF':True,'attenuation.lPFRadiusMin':inner,'attenuation.lPFRadiusMax':outer,'attenuation.lPFFrequencyAtMin':20000,'attenuation.lPFFrequencyAtMax':2500}))
 for name,count,rule,retrigger in [('Weapon',6,'StopQuietest',.03),('Impact',8,'StopQuietest',.02),('Explosion',4,'StopQuietest',.05),('Loop',12,'StopFarthestThenOldest',0)]:
  mix.append(dict(type='SoundConcurrency',path=VROOT+'/Mix/SCON_V1_'+name,properties={'concurrency.maxCount':count,'concurrency.resolutionRule':rule,'concurrency.retriggerTime':retrigger,'concurrency.voiceStealReleaseTime':.03}))
 mix.append(dict(type='SoundMix',path=VROOT+'/Mix/MIX_V1_Settings',properties=dict(duration=-1,fadeInTime=.15,fadeOutTime=.15)))
 spec=dict(mix=mix,**{'import':asset_import},oneshots=shots,loops=loops)
 (HERE/'build.spec.json').write_text(json.dumps(spec,indent=2))
 checks=dict(waves=[r['path'] for r in asset_import],render=[dict(sound=VROOT+'/Sources/MSS_V1_Primary',name='V1_Primary',plays=5,gap_seconds=.5,tail_seconds=1),*[dict(sound=VROOT+'/Sources/MSS_V1_Engine',name='V1_Engine_'+str(speed),plays=1,gap_seconds=10,tail_seconds=.5,params=dict(Speed01=speed)) for speed in [0,1]]])
 (HERE/'check.spec.json').write_text(json.dumps(checks,indent=2))
 print('Prepared',len(asset_import),'waves and',len(shots)+len(loops),'MetaSound definitions; originals unchanged.')
if __name__=='__main__':main()
