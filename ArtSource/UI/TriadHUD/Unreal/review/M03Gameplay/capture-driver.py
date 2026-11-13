import sys,time,json,base64,traceback
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'unrealGameJam.uproject').is_file())
sys.path.insert(0,str(ROOT/'Tools'))
from td_mcp import McpClient,McpError
OUT=ROOT/'Saved/M03Gameplay';OUT.mkdir(exist_ok=True)
records=[]
client=None
def record(label,result):
    records.append({'label':label,'result':result})
    (OUT/'mcp-records.json').write_text(json.dumps(records,indent=2))
def call(ts,tool,args):
    result=client.tool('call_tool',{'toolset_name':ts,'tool_name':tool,'arguments':args})
    if result.get('isError'):raise RuntimeError(str(result))
    return result
def cmd(text,pause=.15):
    result=call('ConsoleToolset.ConsoleToolset','RunConsoleCommand',{'command':text})
    record(text,result);print(text,flush=True)
    time.sleep(pause)
def decoded(result):
    if result.get('structuredContent') is not None:return result['structuredContent']
    for b in result.get('content',[]):
        if b.get('type')=='text':
            try:return json.loads(b['text'])
            except ValueError:pass
    return {}
def shot(name):
    worlds=decoded(call('SceneCaptureToolset.SceneCaptureToolset','ListWorlds',{})).get('returnValue',[])
    game=[w for w in worlds if w.get('worldType')=='Game']
    if len(game)!=1:raise RuntimeError('Expected one game world: '+str(worlds))
    result=call('SceneCaptureToolset.SceneCaptureToolset','CaptureGameViewport',{'world':game[0]['world'],'maxResolution':2560})
    images=[b for b in result.get('content',[]) if b.get('type')=='image']
    if len(images)!=1:raise RuntimeError('Expected one viewport image: '+str(result)[:1000])
    data=base64.b64decode(images[0]['data']);(OUT/(name+'.png')).write_bytes(data)
    images[0]['data']='<saved native viewport PNG: '+name+'.png>'
    record(name,result);print('CAPTURE '+name,flush=True)
def status():cmd('TD.UI.Status')
try:
    until=time.monotonic()+100
    while time.monotonic()<until:
        try:
            client=McpClient('http://127.0.0.1:8023/mcp',timeout=15);client.connect();break
        except McpError:time.sleep(1)
    else:raise RuntimeError('MCP startup deadline exceeded')
    cmd('t.MaxFPS 60')
    cmd('TD.Stage Waves1to2',2)
    cmd('TD.UI.NativeHUD 1',.5);status();shot('01-rift-live-1080p')
    cmd('TD.God')
    cmd('TD.Input.Key Two 0.1',.7);status();shot('02-lancer-m03-live-1080p')
    cmd('TD.Power.Targets 2 12000',.3)
    cmd('TD.Input.Key T 0.1',.5);status();shot('03-lancer-target-live-1080p')
    cmd('TD.Input.Key LeftMouseButton 2',.3)
    cmd('TD.Input.Key RightMouseButton 1',.4);status();shot('04-lancer-heat-ammo-live')
    cmd('TD.Input.ReleaseAll')
    cmd('TD.Input.Key Q 0.1',.5);status();shot('05-lancer-signature-live')
    cmd('TD.Unlock EMPPulse Lancer',.3)
    cmd('TD.Input.Key E 0.1',.4);status();shot('06-lancer-ability-live')
    cmd('TD.Input.Keys W+LeftShift 2',.5);status();shot('07-lancer-boost-live')
    cmd('TD.Input.ReleaseAll')
    cmd('TD.Input.Key Three 0.1',.7);status();shot('08-bastion-live')
    cmd('TD.Input.Key One 0.1',.7);status();shot('09-rift-switched-live')
    cmd('TD.Input.Key Two 0.1',.7)
    cmd('r.SetRes 1280x720w',.6);status();shot('10-lancer-live-720p')
    cmd('r.SetRes 2560x1080w',.6);status();shot('11-lancer-live-ultrawide')
    cmd('r.SetRes 1920x1080w',.5)
    cmd('TD.Stage Wave5',2)
    cmd('TD.Input.Key Two 0.1',.6);status();shot('12-wave5-live-1080p')
    cmd('TD.UI.NativeHUD 0',.4);status();shot('13-host-off')
    cmd('TD.UI.NativeHUD 1',.4);status();shot('14-host-restored')
    cmd('TD.Stage VerdanceOnFoot',2);status();shot('15-on-foot-spacehud-removed')
    cmd('TD.Stage Wave5',2)
    cmd('TD.Input.Key Two 0.1',.7);status();shot('16-return-to-space')
    samples=[]
    for _ in range(5):
        samples.append(decoded(call('PerfToolset.StatsToolset','GetFrameTimings',{})));time.sleep(.1)
    record('actual-game-timing-samples',samples)
    cmd('TD.Input.ReleaseAll')
    cmd('quit',0)
    print('DRIVER PASS:16 native actual-game captures; inspect screenshot values/logs before acceptance.',flush=True)
except Exception:
    record('driver-error',traceback.format_exc());print(traceback.format_exc(),flush=True)
    if client:
        try:cmd('quit',0)
        except Exception:pass
    raise
