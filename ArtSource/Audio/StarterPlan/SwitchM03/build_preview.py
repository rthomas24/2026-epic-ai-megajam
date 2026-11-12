"""Build a self-contained offline A/B player from the six actual PCM WAV files."""
from pathlib import Path
import base64,json,hashlib
ROOT=Path(__file__).resolve().parent
def main():
    report=json.loads((ROOT/'verification.json').read_text())
    payload={}
    for ship in ['Rift','Lancer','Bastion']:
        payload[ship]={}
        for version in ['M02','M03']:
            p=(ROOT.parent/'SwitchM02' if version=='M02' else ROOT)/'audition'/f'SW_TD_{ship}_Switch_{version}.wav'
            payload[ship][version]={'wav':base64.b64encode(p.read_bytes()).decode(),
                'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
                'lufs':report[version.lower()][ship]['short_sfx_lufs']}
    html=TEMPLATE.replace('__PAYLOAD__',json.dumps(payload,separators=(',',':')))
    (ROOT/'audition'/'index.html').write_text(html,encoding='utf-8')
    print('Offline A/B page written; six embedded WAV byte copies, original files untouched.')
TEMPLATE=r'''<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Triad Drift · M02 / M03 audio review</title>
<style>
:root{color-scheme:dark;font:16px/1.55 system-ui,sans-serif;background:#090e16;color:#e5edf5}*{box-sizing:border-box}body{margin:0}main{max-width:1150px;margin:auto;padding:42px 24px 64px}h1{font-size:clamp(30px,5vw,50px);line-height:1.12;letter-spacing:-.04em;font-weight:550}.tag{letter-spacing:.16em;font-size:12px;color:#abc1d2}p{max-width:850px;color:#b0c2d1}.controls{display:flex;flex-wrap:wrap;align-items:center;gap:14px;padding:18px;background:#152130;border-radius:6px}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:18px;margin:26px 0}.card{border:1px solid #34475a;border-top:3px solid var(--accent);border-radius:6px;padding:22px;background:#111c29}h2{margin:0;color:var(--accent)}.card p{font-size:14px;min-height:88px}button,select{font:inherit;color:inherit;background:#203247;border:1px solid #5c748e;padding:9px 13px;border-radius:4px;cursor:pointer}button:hover{background:#304b67}button:focus-visible,input:focus-visible,select:focus-visible,a:focus-visible{outline:3px solid #91ceff;outline-offset:4px}button:disabled{opacity:.45;cursor:wait}.buttons{display:flex;flex-wrap:wrap;gap:8px;margin:14px 0}a{color:#aad7ff;text-underline-offset:4px}.note{font-size:13px}.badge{padding:5px 9px;background:#1b3443;border-radius:4px;font-size:12px}.status{min-height:55px;padding:15px 0;color:#b7d6eb}.test{padding:20px;border:1px solid #34475a;border-radius:6px}.test label{display:inline-flex;gap:9px;align-items:center;margin-right:14px}.wave{width:100%;height:65px;background:#0c1420;margin-top:14px}footer{margin-top:30px;color:#9bb1c5;font-size:13px}@media(max-width:790px){.grid{grid-template-columns:1fr}.card p{min-height:0}main{padding-top:25px}}
</style>
<main><div class="tag">TRIAD DRIFT / AUDIO &amp; MUSIC / REVISION M03</div>
<h1>A smoother cut into every cockpit.</h1>
<p>M02 is preserved for comparison. M03 replaces rough contacts and exposed grit with rounded mechanics, blended energy and controlled depth. These are original procedural designs, awaiting listening approval.</p>
<div class="controls"><button id="initialize">Load six WAVs</button><label>Preview level <input id="volume" type="range" min="0" max="1" step=".001" value=".501187"><output id="volume-label">−6 dB</output></label><label><input id="match" type="checkbox" checked> Match measured loudness</label><button id="stop">Fade / stop</button></div>
<div id="status" class="status" role="status" aria-live="polite">Click Load six WAVs to decode the actual exported files. No network or server is required.</div>
<div class="grid">
<section class="card" style="--accent:#ed9ae2"><h2>Rift</h2><span class="badge">Smooth dimensional energy</span><p>A rounded pressure bloom moves into a flowing spatial wake, with softer resonances and less high-frequency texture.</p><div class="buttons"><button disabled data-ship="Rift" data-version="M02">M02</button><button disabled data-ship="Rift" data-version="M03">M03</button><button disabled data-ab="Rift">A then B</button></div><a href="SW_TD_Rift_Switch_M03.wav">M03 WAV</a> · <a href="../../SwitchM02/audition/SW_TD_Rift_Switch_M02.wav">M02 WAV</a><canvas id="wave-Rift" class="wave" width="600" height="90" aria-label="Rift M03 waveform"></canvas></section>
<section class="card" style="--accent:#7bdcf4"><h2>Lancer</h2><span class="badge">Precise turbine engagement</span><p>Subdued armature contacts blend into a continuous turbine surge. Saturation and scratchy accents are removed.</p><div class="buttons"><button disabled data-ship="Lancer" data-version="M02">M02</button><button disabled data-ship="Lancer" data-version="M03">M03</button><button disabled data-ab="Lancer">A then B</button></div><a href="SW_TD_Lancer_Switch_M03.wav">M03 WAV</a> · <a href="../../SwitchM02/audition/SW_TD_Lancer_Switch_M02.wav">M02 WAV</a><canvas id="wave-Lancer" class="wave" width="600" height="90" aria-label="Lancer M03 waveform"></canvas></section>
<section class="card" style="--accent:#f0c27b"><h2>Bastion</h2><span class="badge">Controlled armored weight</span><p>A softened armored engagement anchors a deep pressure body. Resonances decay together into a quiet, complete tail.</p><div class="buttons"><button disabled data-ship="Bastion" data-version="M02">M02</button><button disabled data-ship="Bastion" data-version="M03">M03</button><button disabled data-ab="Bastion">A then B</button></div><a href="SW_TD_Bastion_Switch_M03.wav">M03 WAV</a> · <a href="../../SwitchM02/audition/SW_TD_Bastion_Switch_M02.wav">M02 WAV</a><canvas id="wave-Bastion" class="wave" width="600" height="90" aria-label="Bastion M03 waveform"></canvas></section>
</div>
<section class="test"><strong>Rapid-switch review</strong><p class="note">Twelve switches, 0.4 seconds apart. The latest two cues retain their tails; an older voice fades over 60 ms. Compare revisions using identical playback settings.</p><label>Order <select id="order"><option value="Rift,Lancer,Bastion">Rift → Lancer → Bastion</option><option value="Bastion,Lancer,Rift">Bastion → Lancer → Rift</option><option value="Lancer,Rift,Bastion">Lancer → Rift → Bastion</option></select></label><div class="buttons"><button disabled data-rapid="M02">Rapid M02</button><button disabled data-rapid="M03">Rapid M03</button></div></section>
<footer>48 kHz / 24-bit stereo WAV. Playback decodes embedded copies of the six real files; no oscillator generation. Loudness matching uses a short-SFX K-weighted reference and may need final adjustment by ear. Preview level defaults to −6 dB. Waveforms and technical checks do not establish perceived realism.<br><a href="../verification.json">Measurements and rapid-sequence checks</a> · <a href="../README.md">Design / provenance / limits</a></footer></main>
<script>
'use strict';
const PAYLOAD=__PAYLOAD__;
const ships=['Rift','Lancer','Bastion'];
let context=null,master=null,buffers={},voices=[],timers=[];
const status=document.querySelector('#status');
function say(s){status.textContent=s}
async function initialize(){
 if(context){await context.resume();return}
 const button=document.querySelector('#initialize');button.disabled=true;
 try{
  context=new AudioContext({sampleRate:48000});master=context.createGain();
  master.gain.value=Number(document.querySelector('#volume').value);master.connect(context.destination);
  await context.resume();
  for(const ship of ships){buffers[ship]={};for(const version of ['M02','M03']){
   const data=Uint8Array.from(atob(PAYLOAD[ship][version].wav),c=>c.charCodeAt(0));
   buffers[ship][version]=await context.decodeAudioData(data.buffer);
  }draw(ship)}
  document.querySelectorAll('[data-ship],[data-ab],[data-rapid]').forEach(b=>b.disabled=false);
  button.textContent='Six WAVs ready';say('Ready. Play one cue, compare A then B, or try rapid switching.');
 }catch(e){say('Playback unavailable: '+e.message+'. Open the WAV links directly.');button.disabled=false;context=null}
}
function draw(ship){
 const canvas=document.querySelector('#wave-'+ship),g=canvas.getContext('2d'),x=buffers[ship].M03.getChannelData(0);
 g.strokeStyle='#83b7ce';g.lineWidth=1;g.beginPath();
 for(let i=0;i<canvas.width;i++){const a=Math.floor(i*x.length/canvas.width),b=Math.floor((i+1)*x.length/canvas.width);let p=0;
  for(let k=a;k<b;k++)p=Math.max(p,Math.abs(x[k]));g.moveTo(i,45-p*42);g.lineTo(i,45+p*42)}
 g.stroke();
}
function levelFor(ship,version){
 if(!document.querySelector('#match').checked)return 1;
 const target=Math.min(...ships.flatMap(s=>['M02','M03'].map(v=>PAYLOAD[s][v].lufs)));
 return 10**((target-PAYLOAD[ship][version].lufs)/20);
}
function stop(){
 timers.forEach(clearTimeout);timers=[];if(!context)return;
 const t=context.currentTime;
 for(const voice of voices){try{
  if(voice.when>t){voice.source.stop();continue}
  voice.gain.gain.cancelAndHoldAtTime(t);voice.gain.gain.linearRampToValueAtTime(0,t+.06);voice.source.stop(t+.065);
 }catch{}}
 voices=[];say('Playback fading to silence.');
}
function schedule(ship,version,when,endAfter=null){
 const source=context.createBufferSource(),gain=context.createGain();source.buffer=buffers[ship][version];
 gain.gain.value=levelFor(ship,version);source.connect(gain);gain.connect(master);
 source.start(when);
 if(endAfter!==null&&endAfter<source.buffer.duration){
  gain.gain.setValueAtTime(levelFor(ship,version),when+endAfter);
  gain.gain.linearRampToValueAtTime(0,when+endAfter+.06);
  source.stop(when+endAfter+.065);
 }
 source.onended=()=>{source.disconnect();gain.disconnect();voices=voices.filter(v=>v.source!==source)};
 voices.push({source,gain,when});
}
async function play(ship,version){await context.resume();stop();schedule(ship,version,context.currentTime+.07);say('Playing '+ship+' '+version+' · '+(document.querySelector('#match').checked?'matched reference level':'original file level'))}
async function ab(ship){
 await context.resume();stop();let at=context.currentTime+.07;
 schedule(ship,'M02',at);at+=buffers[ship].M02.duration+.35;schedule(ship,'M03',at);
 say(ship+' comparison: A = M02, then B = M03 after a short gap.');
}
async function rapid(version){
 await context.resume();stop();
 const order=document.querySelector('#order').value.split(','),events=Array.from({length:12},(_,i)=>order[i%3]);
 const start=context.currentTime+.07;
 events.forEach((ship,i)=>schedule(ship,version,start+i*.4,i<events.length-2?.8:null));
 say('Rapid '+version+' · 12 switches · 400 ms spacing · 60 ms older-tail fade.');
}
document.querySelector('#initialize').addEventListener('click',initialize);
document.querySelector('#stop').addEventListener('click',stop);
document.querySelectorAll('[data-ship]').forEach(b=>b.addEventListener('click',()=>play(b.dataset.ship,b.dataset.version)));
document.querySelectorAll('[data-ab]').forEach(b=>b.addEventListener('click',()=>ab(b.dataset.ab)));
document.querySelectorAll('[data-rapid]').forEach(b=>b.addEventListener('click',()=>rapid(b.dataset.rapid)));
document.querySelector('#volume').addEventListener('input',e=>{
 const v=Number(e.target.value);document.querySelector('#volume-label').value=v?((20*Math.log10(v)).toFixed(1)+' dB'):'Muted';
 if(master)master.gain.setTargetAtTime(v,context.currentTime,.02);
});
window.addEventListener('pagehide',stop);
</script></html>
'''
if __name__=='__main__': main()

