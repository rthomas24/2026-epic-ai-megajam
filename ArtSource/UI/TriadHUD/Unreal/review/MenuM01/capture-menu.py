import sys,time,json,base64,traceback,subprocess,argparse
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'unrealGameJam.uproject').is_file())
sys.path.insert(0,str(ROOT/'Tools'))
from td_mcp import McpClient,McpError
parser=argparse.ArgumentParser();parser.add_argument('--attempt',type=int,required=True)
attempt=parser.parse_args().attempt
assert 1<=attempt<=9
OUT=ROOT/('Saved/HUDMenuM01/captures-%02d'%attempt)
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
    record('scope',{'real_menu_host':True,'development_opt_in':'TDUIMenuReview','default_activation':False,'shipping_validation':False})
    until=time.monotonic()+90
    while time.monotonic()<until:
        try:
            client=McpClient('http://127.0.0.1:8026/mcp',timeout=15);client.connect();break
        except McpError:time.sleep(1)
    else:raise RuntimeError('MCP startup deadline exceeded')
    log=ROOT/('Saved/HUDMenuM01/game-%02d/engine.log'%attempt)
    if 'widget=TDMainMenuWidget' not in log.read_text(errors='replace'):
        raise RuntimeError('New native menu not assigned to actual host; refuse blind navigation')
    cmd('t.MaxFPS 60',1)
    shot('01-menu-1080p')
    cmd('r.SetRes 1280x720w',1);shot('02-menu-720p')
    cmd('r.SetRes 2560x1080w',1);shot('03-menu-ultrawide')
    cmd('r.SetRes 1920x1080w',1)
    # Fresh isolated user directory has no save: Down skips disabled Continue to Settings.
    cmd('TD.UI.Key Down');cmd('TD.UI.Key Enter');shot('04-menu-settings')
    cmd('TD.UI.Key Enter');shot('05-menu-reduced-motion')
    cmd('TD.UI.Key Escape');cmd('TD.UI.Key Enter',3)
    status();shot('06-new-game-transition')
    record('timings',decoded(call('PerfToolset.StatsToolset','GetFrameTimings',{})))
    cmd('quit',0)
    print('COMPLETE: inspect all6 pixels, focus log and actual world transition before acceptance.',flush=True)
except Exception:
    record('driver-error',traceback.format_exc());print(traceback.format_exc(),flush=True)
    if client:
        try:cmd('quit',0)
        except Exception:pass
    raise

