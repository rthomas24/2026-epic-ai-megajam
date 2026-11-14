import json
G="A stocky armored brawler standing in a fighting stance with both fists raised"
H="A huge, heavily armored soldier in thick, heavy plate armor, standing braced with a big shield held on his left forearm in front of him,"
W="A giant, massive armored warrior, twice the size of a man and extremely heavy, standing in a wide planted stance,"
END_STILL="then lies completely still, the legs relaxed and not moving."
P={}
P['grunt_front']=G+" is hit hard in the chest from the front. His upper body snaps backward, his knees buckle, he takes one short step back and falls heavily onto his back on the ground close to where he stood, the arms dropping to the sides, "+END_STILL
P['grunt_back']=G+" is hit hard in the back from behind. His back arches and he lurches forward, his knees give way, he drops to his knees and falls forward face down onto the ground close to where he stood, the arms falling forward, "+END_STILL
P['grunt_left']=G+" is hit hard on the left side of his body. His body twists and is knocked sideways to his right, his knees give way and he collapses onto his right side on the ground close to where he stood, "+END_STILL
P['grunt_right']=G+" is hit hard on the right side of his body. His body twists and is knocked sideways to his left, his knees give way and he collapses onto his left side on the ground close to where he stood, "+END_STILL
P['heavy_front']=H+" is hit in the chest from the front. He rocks back slowly, his knees bend under the weight, and he falls backward like a toppling wall, landing heavily flat on his back on the ground where he stood, the arms dropping to the sides, "+END_STILL
P['heavy_back']=H+" is hit in the back from behind. He staggers, his heavy body sags forward, he drops slowly onto both knees and then falls forward face down onto the ground with a heavy thud close to where he stood, "+END_STILL
P['heavy_left']=H+" is hit on the left side of his body. His heavy body is pushed slowly sideways to his right, the knees give way under the weight and he collapses heavily onto his right side on the ground where he stood, "+END_STILL
P['heavy_right']=H+" is hit on the right side of his body. His heavy body is pushed slowly sideways to his left, the knees give way under the weight and he collapses heavily onto his left side on the ground where he stood, "+END_STILL
P['warden_front']=W+" is struck in the chest from the front. He reels back slowly, his huge body sways, his knees buckle under the enormous weight and he crashes slowly backward onto his back on the ground where he stood, the arms falling to the sides, "+END_STILL
P['warden_back']=W+" is struck in the back from behind. He arches and slumps forward, slowly sinks down onto both knees under the enormous weight, then topples forward and crashes face down onto the ground where he stood, "+END_STILL
P['warden_left']=W+" is struck on the left side of his body. His huge body is driven slowly sideways to his right, his knees give way under the enormous weight and he crashes down onto his right side on the ground where he stood, "+END_STILL
P['warden_right']=W+" is struck on the right side of his body. His huge body is driven slowly sideways to his left, his knees give way under the enormous weight and he crashes down onto his left side on the ground where he stood, "+END_STILL
DUR={'grunt':2.6,'heavy':3.0,'warden':3.4}
base={'grunt':4000,'heavy':5000,'warden':6000}
dirs=['front','back','left','right']
reqs=[]
idx=0
for ch in ('grunt','heavy','warden'):
  for di,d in enumerate(dirs):
    for v,g in ((1,6),(2,7)):
      k=f'{ch}_{d}'; seed=base[ch]+100*(di+1)+v
      reqs.append(dict(index=idx,submission_key=f"edeath-{k}-s{seed}",endpoint_id="fal-ai/hunyuan-motion",input=dict(prompt=P[k],seed=seed,duration=DUR[ch],guidance_scale=g,output_format="fbx"),name=f"{k}-s{seed}"))
      idx+=1
json.dump(reqs,open('requests.json','w'),indent=1)
print(len(reqs))
