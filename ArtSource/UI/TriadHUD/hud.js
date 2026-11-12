'use strict';
// Display-only mock model. This is not a contract with Unreal gameplay code.
const shipDefinitions = [
  {id:'rift', name:'RIFT', pilot:'WREN', role:'TRICKSTER', color:'#f18cfa', rgb:'241,140,250', power:'PORTAL BLASTER', icon:'rift', symbol:'◉', hull:100,shield:86,boost:82,heat:24},
  {id:'lancer', name:'LANCER', pilot:'KATO', role:'INTERCEPTOR',color:'#79e5ff',rgb:'121,229,255',power:'AFTERBURNER RAM',icon:'lancer',symbol:'↟',hull:92,shield:68,boost:65,heat:41},
  {id:'bastion',name:'BASTION',pilot:'MOSS',role:'HEAVY',color:'#ffce77',rgb:'255,206,119',power:'SHIELD DOME',icon:'bastion',symbol:'⬡',hull:100,shield:100,boost:94,heat:8}
];
const paths={rift:'M32 7C13 6 4 20 7 36c3 15 17 23 31 20-15-3-23-14-21-26C19 18 25 12 32 7Zm5 10 18 15-18 15 6-15Z',lancer:'M32 5 40 34 56 49 36 42 32 59 28 42 8 49 24 34Z',bastion:'M16 15h32l10 23-9 12H15L6 38Zm9-7h14v14H25Zm-7 20h8v12h-8Zm20 0h8v12h-8Z',shield:'M32 5 53 14v18c0 12-10 21-21 27C21 53 11 44 11 32V14Z',hull:'m32 7 23 38-23 12L9 45Z',target:'M8 24V8h16m16 0h16v16m0 16v16H40m-16 0H8V40M24 32h16m-8-8v16',portal:'M25 8C10 8 5 19 5 32s5 24 20 24m14-48c15 0 20 11 20 24s-5 24-20 24M17 32h30m-9-9 9 9-9 9',ram:'M32 5 48 27H37v25H27V27H16Zm-22 27v19m44-19v19',dome:'M7 39a25 25 0 0 1 50 0M5 44h54M17 39a15 25 0 0 1 30 0M32 14v25',objective:'m32 5 27 27-27 27L5 32ZM32 20v15m0 8v1',switch:'M10 21h42L41 10M54 43H12l11 11',lock:'M18 28V18a14 14 0 0 1 28 0v10M13 28h38v29H13Zm19 10v9'};
const svg = name => `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round" aria-hidden="true"><path d="${paths[name]}"/></svg>`;
let state={active:0,ships:shipDefinitions.map(x=>({...x})),targetVisible:true,lowHull:false,take:1};
const el=id=>document.getElementById(id), clamp=(value)=>Math.min(100,Math.max(0,Number(value)||0));
function render(){
  const selected=state.ships[state.active];
  document.documentElement.style.setProperty('--active',selected.color);
  document.documentElement.style.setProperty('--active-rgb',selected.rgb);
  el('shipCards').innerHTML=state.ships.map((ship,index)=>{
    const hull=state.lowHull&&index===state.active?18:clamp(ship.hull);
    return `<button type="button" class="ship-card ${index===state.active?'active':''} ${hull<25?'low':''}" style="--ship:${ship.color}" data-card="${index}" aria-label="Switch preview to ${ship.name}, hull ${hull} percent, shield ${clamp(ship.shield)} percent" aria-pressed="${index===state.active}"><span class="icon">${svg(ship.icon)}</span><span><span class="card-top"><strong class="card-name">${ship.name}</strong><span class="pilot">${ship.pilot}</span></span><span class="bar-row">${svg('shield')}<span class="bar"><span class="fill" style="display:block;width:${clamp(ship.shield)}%"></span></span><span>${clamp(ship.shield)}</span></span><span class="bar-row hull">${svg('hull')}<span class="bar"><span class="fill" style="display:block;width:${hull}%"></span></span><span>${hull}</span></span></span><span class="key-status"><kbd>${index+1}</kbd><span class="active-label">${index===state.active?'IN CONTROL':'WINGMAN'}</span><span class="ready-ring" aria-label="Signature ready"></span></span></button>`;
  }).join('');
  for(const [id,value] of Object.entries({pilot:selected.pilot,shipName:selected.name,shipRole:selected.role,powerName:selected.power,abilityIcon:selected.symbol,boostValue:`${clamp(selected.boost)}%`,heatValue:`${clamp(selected.heat)}%`,take:String(state.take).padStart(2,'0'),slateTake:String(state.take).padStart(2,'0')})) el(id).textContent=value;
  el('boostFill').style.width=`${clamp(selected.boost)}%`;el('heatFill').style.width=`${clamp(selected.heat)}%`;
  el('target').hidden=!state.targetVisible;document.querySelector('.lead-point').hidden=!state.targetVisible;
  el('warning').hidden=!state.lowHull;
  el('targetButton').textContent=state.targetVisible?'Target on':'Target off';el('targetButton').setAttribute('aria-pressed',String(state.targetVisible));el('damageButton').setAttribute('aria-pressed',String(state.lowHull));
  document.querySelectorAll('[data-ship]').forEach(button=>button.setAttribute('aria-pressed',String(Number(button.dataset.ship)===state.active)));
}
const timers={};
function cue(id,duration){const item=el(id);clearTimeout(timers[id]);item.classList.remove('show');void item.offsetWidth;item.classList.add('show');timers[id]=setTimeout(()=>item.classList.remove('show'),duration);}
function selectShip(index){if(!Number.isInteger(index)||index<0||index>2)return;state.active=index;render();const selected=state.ships[index];el('cutShip').textContent=selected.name;el('cutPilot').textContent=`${selected.pilot} / ${selected.role}`;cue('cutCard',1500);}
function hit(){cue('hit',600);}
function scene(){cue('slate',2400);}
document.addEventListener('click',event=>{const button=event.target.closest('[data-ship],[data-card]');if(button)selectShip(Number(button.dataset.ship??button.dataset.card));});
el('hitButton').onclick=hit;el('targetButton').onclick=()=>{state.targetVisible=!state.targetVisible;render();};el('damageButton').onclick=()=>{state.lowHull=!state.lowHull;render();};el('slateButton').onclick=scene;el('resetButton').onclick=()=>{state={active:0,ships:shipDefinitions.map(x=>({...x})),targetVisible:true,lowHull:false,take:1};render();};el('reduceMotion').onchange=event=>document.body.classList.toggle('reduce-motion',event.target.checked);
document.addEventListener('keydown',event=>{if(event.target.closest('button,input,select,textarea')||event.ctrlKey||event.metaKey||event.altKey)return;if(['1','2','3'].includes(event.key))selectShip(Number(event.key)-1);if(event.key==='Tab'){event.preventDefault();selectShip((state.active+(event.shiftKey?2:1))%3);}if(event.key.toLowerCase()==='h')hit();if(event.key.toLowerCase()==='s')scene();});
// Adapter TODO: after Ryan freezes the presentation contract, map his read-only HUD
// snapshot to these normalized percentages. No gameplay method is called here.
window.TriadHUDPreview={selectShip,hit,scene,getState:()=>structuredClone(state),setMockState(update){
  if(typeof update!=='object'||update===null)return;
  if(Number.isInteger(update.active)&&update.active>=0&&update.active<3)state.active=update.active;
  for(const flag of ['targetVisible','lowHull'])if(typeof update[flag]==='boolean')state[flag]=update[flag];
  if(Number.isFinite(update.take))state.take=Math.max(1,Math.floor(update.take));
  if(Array.isArray(update.ships))update.ships.slice(0,3).forEach((ship,i)=>{for(const key of ['hull','shield','boost','heat'])if(ship&&Number.isFinite(ship[key]))state.ships[i][key]=clamp(ship[key]);});
  render();
}};
render();

