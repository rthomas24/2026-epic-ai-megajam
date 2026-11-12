// Original Triad Drift source prototype. Node.js, no dependencies or binary output.
import {writeFileSync, readFileSync, mkdirSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
const dir=path.dirname(fileURLToPath(import.meta.url));
const write=(name,data)=>writeFileSync(path.join(dir,name),typeof data==='string'?data:JSON.stringify(data,null,2)+'\n');
const add=(a,b)=>a.map((v,i)=>v+b[i]), sub=(a,b)=>a.map((v,i)=>v-b[i]);
const cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];
const dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0), norm=a=>Math.hypot(...a);
const materials=[
 {name:'M_Lancer_Ceramic',hex:'#E8F4FF',metallic:0.15,roughness:0.4},
 {name:'M_Lancer_CyanPaint',hex:'#12C8FF',metallic:0.2,roughness:0.37},
 {name:'M_Lancer_Composite',hex:'#22262C',metallic:0,roughness:0.72},
 {name:'M_Lancer_BareMetal',hex:'#8A939E',metallic:1,roughness:0.28},
 {name:'M_Lancer_Canopy',hex:'#123E54',metallic:0.45,roughness:0.16},
 {name:'M_Lancer_Emission',hex:'#3FE9FF',metallic:0,roughness:0.35,emissive_strength:2}
];
const assets=[];
function asset(name,status){const a={name,status,parts:[]};assets.push(a);return a;}
function part(a,name,vertices,triangles,material){
 // Orient each closed source solid consistently; centred volume avoids cancellation.
 const centre=vertices.reduce((s,v)=>add(s,v),[0,0,0]).map(v=>v/vertices.length);
 const volume=triangles.reduce((s,t)=>s+dot(sub(vertices[t[0]],centre),cross(sub(vertices[t[1]],centre),sub(vertices[t[2]],centre)))/6,0);
 if(volume<0)triangles=triangles.map(([x,y,z])=>[x,z,y]);
 a.parts.push({name,material,vertices:vertices.map(v=>v.map(n=>+n.toFixed(6))),triangles});
}
// Closed polygon rings along X; bevelled rectangular cross section, no primitives.
function loft(a,name,sections,mat,n=8,round=false){
 const ring=round?Array.from({length:n},(_,i)=>[Math.cos(i*2*Math.PI/n),Math.sin(i*2*Math.PI/n)]):[[1,.62],[.62,1],[-.62,1],[-1,.62],[-1,-.62],[-.62,-1],[.62,-1],[1,-.62]];
 n=ring.length; const vs=[]; const ts=[];
 for(const [x,w,h,y=0,z=0] of sections)for(const [ry,rz]of ring)vs.push([x,y+ry*w,z+rz*h]);
 for(let s=0;s<sections.length-1;s++)for(let k=0;k<n;k++){
  const x=s*n+k,y=s*n+(k+1)%n,z=(s+1)*n+(k+1)%n,w=(s+1)*n+k;ts.push([x,y,z],[x,z,w]);
 }
 for(const [s,reverse] of [[0,true],[sections.length-1,false]]){
  const c=vs.length,sec=sections[s];vs.push([sec[0],sec[3]||0,sec[4]||0]);
  for(let k=0;k<n;k++){const t=[c,s*n+k,s*n+(k+1)%n];ts.push(reverse?t.reverse():t);}
 }
 part(a,name,vs,ts,mat);
}
function prism(a,name,outline,z,thickness,mat){
 const centre=outline.reduce((s,v)=>[s[0]+v[0]/outline.length,s[1]+v[1]/outline.length],[0,0]);
 const vs=[],ts=[],n=outline.length;
 const bevel=Math.min(2.5,thickness*.2);
 for(const [height,inset]of [[z-thickness/2,.95],[z-thickness/2+bevel,1],[z+thickness/2-bevel,1],[z+thickness/2,.95]])
  for(const p of outline)vs.push([centre[0]+(p[0]-centre[0])*inset,centre[1]+(p[1]-centre[1])*inset,height]);
 for(let s=0;s<3;s++)for(let k=0;k<n;k++){const j=(k+1)%n;ts.push([s*n+k,s*n+j,(s+1)*n+j],[s*n+k,(s+1)*n+j,(s+1)*n+k]);}
 for(const s of [0,3])for(let k=1;k<n-1;k++)ts.push(s===0?[s*n,s*n+k+1,s*n+k]:[s*n,s*n+k,s*n+k+1]);
 part(a,name,vs,ts,mat);
}
// Closed radial profile: hollow tubes have actual inside surfaces and annular lips.
function lathe(a,name,profile,mat,y=0,z=0,n=24){
 const vs=[],ts=[];
 for(const[x,r]of profile)for(let i=0;i<n;i++){const t=i*2*Math.PI/n;vs.push([x,y+r*Math.cos(t),z+r*Math.sin(t)]);}
 for(let j=0;j<profile.length;j++)for(let i=0;i<n;i++){
  const k=(i+1)%n,p=(j+1)%profile.length;ts.push([j*n+i,j*n+k,p*n+k],[j*n+i,p*n+k,p*n+i]);
 }part(a,name,vs,ts,mat);
}
const hull=asset('SM_Ship_Lancer_Proxy','silhouette_review_proxy');
loft(hull,'Fuselage',[[-340,52,24],[-220,68,33],[-65,72,38],[115,52,31],[275,27,14],[390,4,5],[400,2,2]],0);
loft(hull,'VentralKeel',[[-285,28,11,0,-27],[-50,34,14,0,-36],[220,16,8,0,-20],[365,2,2,0,-5]],2);
loft(hull,'CanopyRim',[[-75,41,13,0,37],[10,45,24,0,45],[125,34,18,0,43],[155,19,5,0,34]],2);
loft(hull,'Canopy',[[-68,34,11,0,46],[10,37,18,0,54],[115,27,12,0,52],[143,15,3,0,39]],4);
// Continuous dorsal spine supports an adjustable overdrive emissive insert.
loft(hull,'DriveSpine',[[-305,19,12,0,30],[-115,23,19,0,38],[-52,18,10,0,41]],1);
loft(hull,'OverdriveInsert',[[-280,5,2,0,43],[-135,6,2,0,57],[-105,4,2,0,56]],5);
for(const side of [-1,1]){
 const tag=side<0?'L':'R',y=v=>v*side;
 prism(hull,'Wing_'+tag,[[-205,y(52)],[-142,y(170)],[120,y(340)],[182,y(320)],[-8,y(168)],[42,y(50)]],-8,20,0);
 prism(hull,'WingCyan_'+tag,[[-142,y(94)],[-107,y(175)],[127,y(323)],[157,y(313)],[-21,y(168)],[15,y(85)]],3.2,3,1);
 loft(hull,'WingRoot_'+tag,[[-215,30,15,y(72),-5],[-80,33,20,y(83),-5],[115,13,10,y(68),-2]],0);
 loft(hull,'Nacelle_'+tag,[[-369,33,30,y(111),-3],[-338,45,42,y(111),-3],[-215,44,42,y(111),-2],[-134,28,30,y(111),-1],[-102,20,22,y(111),0]],0,24,true);
 lathe(hull,'EngineCollar_'+tag,[[-392,27],[-379,35],[-346,34],[-346,28],[-379,25],[-392,23]],3,y(111),-3);
 lathe(hull,'EngineDark_'+tag,[[-398,22],[-359,25],[-350,22],[-350,16],[-390,18],[-398,18]],2,y(111),-3);
 lathe(hull,'EngineEmission_'+tag,[[-397,18],[-390,18],[-390,14],[-397,14]],5,y(111),-3);
 // Vertical canted stabilizer made as closed three-dimensional lofted form.
 loft(hull,'Stabilizer_'+tag,[[-329,6,26,y(111),37],[-248,7,61,y(116),55],[-208,5,50,y(118),50],[-186,3,14,y(118),25]],1);
 loft(hull,'GunPylon_'+tag,[[-12,15,13,y(100),-28],[38,19,17,y(100),-32],[64,12,13,y(100),-28]],2);
 loft(hull,'RocketPylon_'+tag,[[-130,13,22,y(220),-15],[-75,16,24,y(220),-19],[-47,10,17,y(220),-16]],2);
 // Vent blocks deliberately grouped at the drive root, not across calm panels.
 for(let i=0;i<4;i++)loft(hull,'Vent_'+tag+'_'+i,[[-195+i*18,5,4,y(78),24],[-187+i*18,5,4,y(78),24]],2);
}
const mount=asset('SM_Lancer_UniversalMount','modular_source_prototype');
loft(mount,'MountFoot',[[-36,22,5,0,1],[-31,27,8,0,1],[29,27,8,0,1],[36,22,5,0,1]],3);
loft(mount,'MountCradle',[[-21,15,7,0,-8],[21,15,7,0,-8]],2);
const gun=asset('SM_Lancer_MinigunBody','modular_source_prototype');
loft(gun,'Breech',[[-43,18,17,0,-15],[-35,24,22,0,-15],[22,24,22,0,-15],[35,18,17,0,-15]],2);
loft(gun,'ReceiverArmour',[[-35,21,6,0,3],[17,21,6,0,3],[28,14,3,0,1]],0);
lathe(gun,'DriveMotor',[[-34,10],[-30,17],[12,17],[15,10]],3,0,-15);
const rotor=asset('SM_Lancer_MinigunRotor','modular_source_prototype');
// Rotor coordinates are relative to axial pivot (35,0,-15) on the gun body.
for(let i=0;i<6;i++){
 const t=i*Math.PI/3,cy=12*Math.cos(t),cz=12*Math.sin(t);
 lathe(rotor,'Barrel_'+(i+1),[[-6,4.1],[132,4.1],[136,3.7],[136,2.5],[-6,2.5]],3,cy,cz,12);
}
lathe(rotor,'RearClamp',[[-5,19],[3,20],[8,19],[8,16.7],[-5,16.7]],2,0,0);
lathe(rotor,'FrontClamp',[[103,19],[112,20],[119,19],[119,16.7],[103,16.7]],2,0,0);
loft(rotor,'Axle',[[-8,4,4],[105,4,4]],3,12,true);
const pod=asset('SM_Lancer_RocketPod','modular_source_prototype');
loft(pod,'PodSpine',[[-65,9,27,0,-22],[-58,12,31,0,-22],[83,12,31,0,-22],[101,8,27,0,-22]],2);
loft(pod,'PodTop',[[-62,30,5,0,5],[-52,35,7,0,5],[75,35,7,0,5],[99,28,4,0,5]],0);
for(const y of [-16,16])for(const z of [-9,-39]){
 lathe(pod,'Tube_'+y+'_'+z,[[-57,13.8],[91,13.8],[105,12.8],[108,12],[108,10],[-57,10]],3,y,z,16);
 lathe(pod,'Recess_'+y+'_'+z,[[-56,9.8],[85,9.8],[85,8.6],[-56,8.6]],2,y,z,16);
 loft(pod,'LoadedRocketNose_'+y+'_'+z,[[58,7.3,7.3,y,z],[73,7,7,y,z],[88,1,1,y,z]],1,16,true);
}
loft(pod,'PodSideL',[[-50,4,17,-33,-23],[65,4,17,-33,-23],[88,2,13,-33,-23]],1);
loft(pod,'PodSideR',[[-50,4,17,33,-23],[65,4,17,33,-23],[88,2,13,33,-23]],1);
const rocket=asset('SM_Lancer_SpikeRocket','projectile_visual_source');
loft(rocket,'RocketBody',[[-50,5,5],[-42,7.8,7.8],[22,7.8,7.8],[39,5.5,5.5],[50,0.5,0.5]],0,16,true);
lathe(rocket,'Nozzle',[[-54,5.4],[-44,5.4],[-44,3],[-54,3]],2,0,0,12);
for(const s of [-1,1])prism(rocket,'Fin_'+s,[[-45,s*5],[-48,s*17],[-29,s*14],[-18,s*6]],0,3,1);
const instances=[{name:'Hull',asset:hull.name,position_cm:[0,0,0]}];
const sockets=[];
function socket(name,parent,position,forward=[1,0,0],purpose='visual_attachment'){
 sockets.push({name,parent,position_cm:position,forward,up:[0,0,1],scale:[1,1,1],purpose});
}
for(const side of [-1,1]){
 const t=side<0?'L':'R',mountP=[30,side*100,-42],podP=[-90,side*220,-44];
 instances.push({name:'MountPrimary_'+t,asset:mount.name,position_cm:mountP});
 instances.push({name:'Primary_'+t,asset:gun.name,position_cm:mountP});
 instances.push({name:'Rotor_'+t,asset:rotor.name,position_cm:add(mountP,[35,0,-15])});
 instances.push({name:'MountSecondary_'+t,asset:mount.name,position_cm:podP});
 instances.push({name:'Secondary_'+t,asset:pod.name,position_cm:podP});
 socket('Mount_Primary_'+t,'Hull',mountP,[1,0,0],'swappable_weapon_mount');
 socket('Mount_Secondary_'+t,'Hull',podP,[1,0,0],'swappable_weapon_mount');
 socket('Rotor_Pivot','Primary_'+t,[35,0,-15],[1,0,0],'spin_visual_only');
 socket('Muzzle_Primary_'+t,'Primary_'+t,[174,0,-15],[1,0,0],'group_muzzle_outside_barrel_plane');
 for(let i=0;i<6;i++){const q=i*Math.PI/3;socket('Barrel_'+String(i+1).padStart(2,'0'),'Rotor_'+t,[138,12*Math.cos(q),12*Math.sin(q)],[1,0,0],'optional_rotating_barrel_origin');}
 let i=0;for(const y of [-16,16])for(const z of [-9,-39])socket('Muzzle_Secondary_'+t+'_'+String(++i).padStart(2,'0'),'Secondary_'+t,[112,y,z],[1,0,0],'rocket_origin_outside_tube');
 socket('Thruster_'+t,'Hull',[-402,side*111,-3],[-1,0,0],'VFX_exhaust_outward');
}
socket('Ability_Overdrive','Hull',[-205,0,61],[1,0,0],'VFX_attachment_no_effect');
socket('Ability_EMP','Hull',[0,0,0],[1,0,0],'VFX_attachment_no_effect');
socket('Ram_Visual','Hull',[404,0,0],[1,0,0],'VFX_attachment_no_gameplay_collision');
const manifest={schema_version:1,ship:'Lancer',status:'source_only_not_imported',units:'centimetres',axes:{forward:'+X',right:'+Y',up:'+Z'},rotation_convention:'forward/up basis vectors, parent-relative; no Euler ambiguity',assets:assets.map(a=>({name:a.name,status:a.status})),instances,sockets,materials,production_gates:['human silhouette review','final hero hull route A/B','published binary claims and production authorization','Blender geometry/material/UV QA','FBX calibration and round trip','UE 5.9 preview and performance check'],gameplay_owner:'Ryan',vfx_owner:'Existing VFX Agent'};
write('attachments.json',manifest);
for(const a of assets)write(a.name+'.mesh.json',{schema_version:1,units:'centimetres',axes:manifest.axes,...a});

const expanded=instances.flatMap(inst=>assets.find(a=>a.name===inst.asset).parts.map(p=>({...p,name:inst.name+'/'+p.name,vertices:p.vertices.map(v=>add(v,inst.position_cm))})));
const bounds=vs=>[0,1,2].map(k=>[Math.min(...vs.map(v=>v[k])),Math.max(...vs.map(v=>v[k]))]);
const qa=[];
for(const a of assets){
 let errors=[],tris=0,verts=0,volume=0;
 for(const p of a.parts){
  tris+=p.triangles.length;verts+=p.vertices.length;
  const edges=new Map();let vol=0;
  for(const t of p.triangles){
   if(t.some(i=>i<0||i>=p.vertices.length))throw Error('index outside mesh');
   const [a,b,c]=t.map(i=>p.vertices[i]);
   if(norm(cross(sub(b,a),sub(c,a)))<1e-6)errors.push(p.name+': degenerate triangle');
   vol+=dot(a,cross(b,c))/6;
   for(let i=0;i<3;i++){const x=t[i],y=t[(i+1)%3],key=[Math.min(x,y),Math.max(x,y)].join(':');const e=edges.get(key)||[];e.push(x<y?1:-1);edges.set(key,e);}
  }
  if([...edges.values()].some(v=>v.length!==2))errors.push(p.name+': nonmanifold/open edge');
  if([...edges.values()].some(v=>v.reduce((s,n)=>s+n,0)!==0))errors.push(p.name+': inconsistent face winding');
  if(vol<=0)errors.push(p.name+': nonpositive solid volume');volume+=vol;
  if(new Set(p.vertices.map(v=>v.join(','))).size!==p.vertices.length)errors.push(p.name+': duplicate vertices');
 }
 qa.push({asset:a.name,parts:a.parts.length,vertices:verts,triangles:tris,material_slots:[...new Set(a.parts.map(p=>p.material))].length,bounds_cm:bounds(a.parts.flatMap(p=>p.vertices)),volume_cm3:+volume.toFixed(2),errors});
}
const totalTris=expanded.reduce((s,p)=>s+p.triangles.length,0),bb=bounds(expanded.flatMap(p=>p.vertices));
// Resolve attachment parents explicitly; socket names are unique within each parent.
const socketKeys=sockets.map(s=>s.parent+'/'+s.name);
if(new Set(socketKeys).size!==socketKeys.length)throw Error('duplicate socket name within parent');
for(const s of sockets){if(!instances.some(i=>i.name===s.parent))throw Error('missing socket parent '+s.parent);if(Math.abs(norm(s.forward)-1)>1e-7||dot(s.forward,s.up)!==0)throw Error('socket basis invalid');}
const clearance=[];
for(const s of sockets.filter(s=>s.name.startsWith('Muzzle_'))){
 const inst=instances.find(i=>i.name===s.parent),a=assets.find(a=>a.name===inst.asset);
 const front=s.parent.startsWith('Primary')?35+Math.max(...rotor.parts.flatMap(p=>p.vertices.map(v=>v[0]))):Math.max(...a.parts.flatMap(p=>p.vertices.map(v=>v[0])));
 clearance.push({socket:s.name,parent:s.parent,clearance_cm:+(s.position_cm[0]-front).toFixed(3)});
}
const report={status:'source_geometry_checks_only',tool:'Node.js deterministic mesh validation; no Blender or Unreal run',assets:qa,assembled:{instances:instances.length,parts:expanded.length,triangles:totalTris,material_count:materials.length,bounds_cm:bb,dimensions_cm:bb.map(([a,b])=>+(b-a).toFixed(3)),socket_count:sockets.length,texture_memory_bytes:0},muzzle_clearance:clearance,pass:qa.every(a=>!a.errors.length)&&totalTris<20000&&clearance.every(c=>c.clearance_cm>0),not_checked:['Blender evaluated topology and shading','UVs (source flat-MI prototype has none)','triangle intersection/z-fighting','exact contact/floating islands','Unreal collision and imported sockets','draw calls/GPU memory/performance','hero art approval']};
write('verification.json',report);
if(!report.pass)throw Error(JSON.stringify(report));

// Deterministic orthographic painter for source review SVGs; no raster output.
function project(v,az,el){const a=az*Math.PI/180,e=el*Math.PI/180;const right=[-Math.sin(a),Math.cos(a),0],up=[-Math.cos(a)*Math.sin(e),-Math.sin(a)*Math.sin(e),Math.cos(e)],eye=[Math.cos(a)*Math.cos(e),Math.sin(a)*Math.cos(e),Math.sin(e)];return[dot(v,right),-dot(v,up),dot(v,eye)];}
function view(parts,az,el,x,y,w,h,wire=false){
 const all=parts.flatMap(p=>p.vertices.map(v=>project(v,az,el))),b=bounds(all),scale=Math.min((w-34)/(b[0][1]-b[0][0]),(h-34)/(b[1][1]-b[1][0]));
 const xy=p=>[(p[0]-(b[0][0]+b[0][1])/2)*scale+x+w/2,(p[1]-(b[1][0]+b[1][1])/2)*scale+y+h/2];
 const faces=[];for(const p of parts)for(const t of p.triangles){const vs=t.map(i=>p.vertices[i]),pts=vs.map(v=>project(v,az,el)),n=cross(sub(vs[1],vs[0]),sub(vs[2],vs[0]));const m=materials[p.material];const light=.43+.57*Math.max(0,dot(n,[.34,-.48,.81])/norm(n));let rgb=m.hex.slice(1).match(/../g).map(v=>parseInt(v,16));if(!m.emissive_strength)rgb=rgb.map(v=>Math.round(v*light));faces.push({d:pts.reduce((s,p)=>s+p[2]/3,0),polygon:pts.map(p=>xy(p).map(n=>n.toFixed(1)).join(',')).join(' '),fill:wire?'#163345':`rgb(${rgb})`,stroke:wire?'#5fa7bd':`rgb(${rgb})`});}
 return faces.sort((a,b)=>a.d-b.d).map(f=>`<polygon points="${f.polygon}" fill="${f.fill}" stroke="${f.stroke}" stroke-width="${wire?.45:.25}"/>`).join('');
}
const panels=[['FRONT / PORT',30,24],['TOP',90,90],['CHASE / STARBOARD',-150,22],['STARBOARD',90,0],['NOSE',0,0],['TOPOLOGY',30,24]];
let svg='<svg xmlns="http://www.w3.org/2000/svg" width="1500" height="1150" viewBox="0 0 1500 1150"><rect width="1500" height="1150" fill="#071723"/><style>text{font-family:Segoe UI,Arial,sans-serif;fill:#dceef5}.small{font-size:16px;fill:#86b4c8}.label{font-size:17px;letter-spacing:2px}</style><text x="40" y="48" font-size="30">LANCER / INTERCEPTOR</text><text x="40" y="78" class="small">M01 — source prototype · silhouette approval pending · 800 cm class · ice / cyan / graphite</text>';
panels.forEach(([title,az,el],i)=>{const x=24+(i%3)*492,y=106+Math.floor(i/3)*355;svg+=`<rect x="${x}" y="${y}" width="480" height="340" rx="12" fill="#102531" stroke="#22404d"/><text x="${x+18}" y="${y+30}" class="label">${title}</text>`+view(expanded,az,el,x+10,y+45,460,270,i===5);});
svg+=`<text x="40" y="850" class="label">MODULAR WEAPON SOURCES / COMMON MOUNT PIVOT</text>`;
for(const [i,a]of[gun,rotor,pod,rocket].entries()){svg+=view(a.parts,30,22,30+i*365,870,350,190)+`<text x="${40+i*365}" y="1080" class="small">${['MINIGUN BODY','SIX-BARREL ROTOR','FOUR-TUBE ROCKET POD','SPIKE ROCKET'][i]}</text>`;}
svg+=`<text x="40" y="1125" class="small">${totalTris.toLocaleString()} source triangles · ${materials.length} materials · ${sockets.length} attachment transforms · no gameplay logic / no VFX generated</text></svg>`;
write('Lancer_review.svg',svg);
// Preview loads embedded JSON without fetch/CDNs, so file:// works offline.
const template=readFileSync(path.join(dir,'preview_template.html'),'utf8');
write('Lancer_preview.html',template.replace('/*__DATA__*/',JSON.stringify({materials,assets,instances,sockets,report})));
console.log(JSON.stringify({pass:report.pass,assembled:report.assembled,files:assets.length+4}));
