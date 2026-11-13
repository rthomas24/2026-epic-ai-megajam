import sys,time,json,base64,traceback,subprocess
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'unrealGameJam.uproject').is_file())
sys.path.insert(0,str(ROOT/'Tools'))
from td_mcp import McpClient,McpError
OUT=ROOT/'Saved/HUDUsabilityM04/focused-kill-captures-01'
assert not OUT.exists(), 'Preserve earlier capture attempt'
OUT.mkdir(parents=True)
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
    head=subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip()
    if head!='5a1382d39d2da5c564f6a2d53e65d58d66987025':raise RuntimeError('Wrong candidate')
    record('scope',{'head':head,'actual_game_world':True,'preview_mode':False,'one_focused_attempt':True,'fixtures':['TD.Stage Waves1to2','TD.God','TD.Squad.Passive 1','TD.Power.Targets 1 40000'],'no_manual_confirm_hit':True,'shipping_validation':False})
    until=time.monotonic()+90
    while time.monotonic()<until:
        try:
            client=McpClient('http://127.0.0.1:8026/mcp',timeout=15);client.connect();break
        except McpError:time.sleep(1)
    else:raise RuntimeError('MCP startup deadline exceeded')
    cmd('t.MaxFPS 60')
    cmd('TD.Stage Waves1to2',2)
    cmd('TD.UI.NativeHUD 1',.4)
    cmd('TD.God')
    cmd('TD.Squad.Passive 1')
    cmd('TD.Input.Key Two 0.1',.6)
    cmd('TD.Input.Keys S+SpaceBar 15',2)
    cmd('TD.Power.Targets 1 40000',.15)
    cmd('TD.Input.Key T 0.1',.15)
    status();shot('11-focused-before-fire-1080p')
    cmd('TD.Input.Key LeftMouseButton 8',.15)
    for index in range(8):
        time.sleep(.25)
        shot('focused-projectile-%02d-1080p'%index)
    cmd('TD.Input.ReleaseAll')
    status();shot('12-focused-after-fire-1080p')
    cmd('quit',0)
    print('ONE FOCUSED ATTEMPT COMPLETE; inspect native pixels for kill text, no automatic repeat.',flush=True)
except Exception:
    record('driver-error',traceback.format_exc());print(traceback.format_exc(),flush=True)
    if client:
        try:cmd('quit',0)
        except Exception:pass
    raise
