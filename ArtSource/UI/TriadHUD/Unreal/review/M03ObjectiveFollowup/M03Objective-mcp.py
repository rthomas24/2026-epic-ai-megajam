"""Serial private8023 verification helper. Requires a separately cleared running game."""
import argparse,base64,json,sys,time
from pathlib import Path

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'unrealGameJam.uproject').is_file())
sys.path.insert(0,str(ROOT/'Tools'))
from td_mcp import McpClient

def decoded(result):
    if result.get('structuredContent') is not None: return result['structuredContent']
    for block in result.get('content',[]):
        if block.get('type')=='text':
            try: return json.loads(block['text'])
            except ValueError: pass
    return result

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('mode',choices=['cmd','shot','describe','call'])
    parser.add_argument('args',nargs='+')
    args=parser.parse_args()
    out=ROOT/'Saved/M03Objective';out.mkdir(exist_ok=True)
    client=McpClient('http://127.0.0.1:8023/mcp',timeout=20);client.connect()
    def call(toolset,tool,arguments):
        result=client.tool('call_tool',{'toolset_name':toolset,'tool_name':tool,'arguments':arguments})
        if result.get('isError'): raise RuntimeError(str(result)[:2500])
        return result
    label=' '.join(args.args)
    if args.mode=='cmd':
        result=call('ConsoleToolset.ConsoleToolset','RunConsoleCommand',{'command':label})
    elif args.mode=='describe':
        result=client.tool('describe_toolsets',{'toolset_names':args.args,'include_schemas':True})
    elif args.mode=='call':
        data=json.loads(Path(args.args[2]).read_text()) if len(args.args)>2 else {}
        result=call(args.args[0],args.args[1],data)
    else:
        worlds=decoded(call('SceneCaptureToolset.SceneCaptureToolset','ListWorlds',{})).get('returnValue',[])
        games=[w for w in worlds if w.get('worldType')=='Game']
        if len(games)!=1: raise RuntimeError('Expected one actual game world: '+str(worlds))
        result=call('SceneCaptureToolset.SceneCaptureToolset','CaptureGameViewport',{'world':games[0]['world'],'maxResolution':2560})
        images=[b for b in result.get('content',[]) if b.get('type')=='image']
        if len(images)!=1: raise RuntimeError('Expected one native image')
        path=out/(args.args[0]+'.png');path.write_bytes(base64.b64decode(images[0]['data']))
        images[0]['data']='<native PNG saved to '+str(path)+'>'
        print('CAPTURE',path)
    record={'time':time.time(),'mode':args.mode,'label':label,'result':result}
    with (out/'mcp-records.jsonl').open('a',encoding='utf-8') as handle: handle.write(json.dumps(record)+'\n')
    print(json.dumps(decoded(result),ensure_ascii=True)[:6500])

if __name__=='__main__': main()
