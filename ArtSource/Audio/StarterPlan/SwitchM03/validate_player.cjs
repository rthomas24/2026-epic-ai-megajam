// Static artifact / deterministic scheduler verification, not a browser hearing test.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),crypto=require('node:crypto');
const root=__dirname,html=fs.readFileSync(path.join(root,'audition/index.html'),'utf8');
const code=html.match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(code);
const elements=new Map(),events=[];
function element(id){if(!elements.has(id))elements.set(id,{value:id==='#volume'?'.501187':id==='#order'?'Rift,Lancer,Bastion':'',checked:true,textContent:'',disabled:false,addEventListener(){},getContext(){return{beginPath(){},moveTo(){},lineTo(){},stroke(){}}},width:600});return elements.get(id)}
const document={querySelector:element,querySelectorAll:()=>[]};
const gain=()=>({value:1,setValueAtTime(){},linearRampToValueAtTime(){},cancelAndHoldAtTime(){},setTargetAtTime(){}});
class Context{
 constructor(){this.currentTime=0;this.destination={};this.state='running'}
 async resume(){}
 createGain(){return{gain:gain(),connect(){},disconnect(){}}}
 createBufferSource(){let started=false;return{buffer:null,connect(){},disconnect(){},start(t){started=true;events.push({type:'start',t})},stop(t){if(!started)throw Error('stop called before start');events.push({type:'stop',t})}}}
 async decodeAudioData(buffer){const b=Buffer.from(buffer);if(b.toString('ascii',0,4)!=='RIFF'||b.toString('ascii',8,12)!=='WAVE')throw Error('Invalid WAV');return{duration:b.readUInt32LE(40)/b.readUInt32LE(28),getChannelData(){return new Float32Array(32)}}}
}
const sandbox={document,AudioContext:Context,Uint8Array,Float32Array,Math,atob:s=>Buffer.from(s,'base64').toString('binary'),setTimeout,clearTimeout,window:{addEventListener(){}}};
vm.createContext(sandbox);vm.runInContext(code,sandbox);
async function main(){
 const payload=vm.runInContext('PAYLOAD',sandbox),embedded=[];
 for(const ship of Object.keys(payload))for(const rev of ['M02','M03']){
  const p=path.resolve(root,rev==='M02'?'../SwitchM02':'','audition',`SW_TD_${ship}_Switch_${rev}.wav`);
  const b=Buffer.from(payload[ship][rev].wav,'base64'),actual=fs.readFileSync(p);
  if(!b.equals(actual))throw Error('Embedded WAV differs from original: '+p);
  const hash=crypto.createHash('sha256').update(b).digest('hex');
  if(hash!==payload[ship][rev].sha256)throw Error('Embedded hash mismatch');
  embedded.push({ship,rev,sha256:hash});
 }
 await vm.runInContext('initialize()',sandbox);
 for(const ship of Object.keys(payload))await vm.runInContext(`play('${ship}','M03')`,sandbox);
 for(const ship of Object.keys(payload))await vm.runInContext(`ab('${ship}')`,sandbox);
 const sequences={};
 for(const rev of ['M02','M03']){
  events.length=0;await vm.runInContext(`rapid('${rev}')`,sandbox);
  const starts=events.filter(e=>e.type==='start');if(starts.length!==12)throw Error('Expected 12 scheduled starts');
  for(let i=1;i<starts.length;i++)if(Math.abs(starts[i].t-starts[i-1].t-.4)>1e-8)throw Error('Wrong sequence interval');
  sequences[rev]={events:starts.length,interval_s:.4,older_tail_fade_s:.06};
 }
 vm.runInContext('stop()',sandbox);
 const links=[...html.matchAll(/href="([^"]+)"/g)].map(m=>m[1]);
 for(const href of links)if(!fs.existsSync(path.resolve(root,'audition',href)))throw Error('Broken local link '+href);
 const report={passed:true,method:'Node VM with mocked Web Audio nodes; syntax, six exact WAV payload hashes, local links, individual/A-B/rapid scheduling and start-before-stop API order. Not browser decoding, device playback or auditory quality approval.',embedded,sequences,local_links:links.length};
 fs.writeFileSync(path.join(root,'player-qa.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));
}
main().catch(e=>{console.error(e);process.exitCode=1});
