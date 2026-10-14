import ctypes,json,pathlib,tempfile,random,sys
project=pathlib.Path(__file__).resolve().parents[2];folder=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else project/'Saved/NewCharacterReplays';folder.mkdir(parents=True,exist_ok=True);lib=ctypes.CDLL(str(project/'Content/Runtime/Win64/Enigma.Native.dll'));lib.EnigmaOpen.restype=ctypes.c_void_p;lib.EnigmaExecute.argtypes=[ctypes.c_void_p,ctypes.c_char_p];lib.EnigmaExecute.restype=ctypes.c_void_p;lib.EnigmaFree.argtypes=[ctypes.c_void_p];lib.EnigmaClose.argtypes=[ctypes.c_void_p];handle=lib.EnigmaOpen()
def call(command,**kw):
 p=lib.EnigmaExecute(handle,json.dumps(dict(command=command,**kw)).encode());r=json.loads(ctypes.string_at(p));lib.EnigmaFree(p);assert r['ok'],r.get('error');return r
call('init',data=str(project/'Content/GameData'),save=str(folder/(next(tempfile._get_candidate_names())+'.json')))
found={}
for mode in ['mazakin','hajun','zhujin','tenrin','zevalon']:
 for trial in range(15):
  generator=random.Random(trial);call('formation',cells=generator.sample(range(9),5));before=call('start',id=mode);before=call('begin')
  for turn in range(500):
   if before['battle']['finished']:break
   options=[s for s in before['battle']['skills'] if not s['reason']]
   if before['battle']['allyTurn'] and options:
    choice=generator.choice(options);side='allies' if choice['operator']=='heal' else 'enemies';targets=[u for u in before['battle'][side] if u['alive']];after=call('act',id=choice['id'],target=min(targets,key=lambda u:u['hp']/u['maxHp'])['key'])
   else:after=call('act')
   battle=after['battle'];skill=battle['lastSkill'] or ''
   if skill.startswith(mode+'_') and mode+'-action' not in found:found[mode+'-action']=(before,after)
   if mode in ['mazakin','hajun','zhujin']:
    old=next(u for u in before['battle']['enemies'] if u['id']==mode);now=next(u for u in battle['enemies'] if u['id']==mode)
    form=lambda u:u['crimson'] if mode=='zhujin' else u['ascended']
    if form(now) and not form(old) and mode+'-transform' not in found:found[mode+'-transform']=(before,after)
    if form(now) and skill.startswith(mode+'_') and mode+'-phase2' not in found:found[mode+'-phase2']=(before,after)
    if mode=='mazakin' and skill=='mazakin_orb' and 'mazakin-orbs' not in found:found['mazakin-orbs']=(before,after)
    if mode=='zhujin' and now['blooming'] and not old['blooming'] and 'zhujin-bloom' not in found:found['zhujin-bloom']=(before,after)
   elif skill in ['tenrin_roar','zevalon_wrath'] and mode+'-ultimate' not in found:found[mode+'-ultimate']=(before,after)
   before=after
  wanted=([mode+'-action',mode+'-transform',mode+'-phase2'] if mode in ['mazakin','hajun','zhujin'] else [mode+'-action',mode+'-ultimate'])
  if all(key in found for key in wanted):break
manifest=[]
for index,(name,(before,after)) in enumerate(found.items()):
 skill=after['battle']['lastSkill'] or '';hits=after['battle']['impacts']
 def at(hit):
  if hit['kind'] in ['poison','bleed','hazard']:return .25
  return (1.3 if 'meteor' in skill or 'rain' in skill else .65)+min(max(hit['hit'],0),10)*.08
 times=[1.0,3.5] if name.endswith('transform') else ([max(.05,min(at(h) for h in hits)-.1),max(at(h) for h in hits)+.1] if hits else [.5,1.45]);old={u['key']:u for side in ['allies','enemies'] for u in before['battle'][side]};samples=[]
 for time in times:
  expected={}
  for target in set(h['target'] for h in hits):
   if target not in old or time>=2.1:continue
   hp=old[target]['hp']+sum(h['amount']*(1 if h['kind']=='heal' else -1) for h in hits if h['target']==target and at(h)<=time);expected[target]=min(max(hp,0),old[target]['maxHp'])
  samples.append(dict(at=time,health=expected))
 label=f'{index:02}-'+name;(folder/(label+'.json')).write_text(json.dumps(dict(name=label,before=before,after=after,audioTimes=sorted(set(at(h) for h in hits)) if skill else [],samples=samples,duration=2.2),ensure_ascii=False),encoding='utf-8');manifest.append(label)
(folder/'manifest.json').write_text(json.dumps(manifest),encoding='utf-8');lib.EnigmaClose(handle);print('NEW_CHARACTER_NATIVE_REPLAYS',len(manifest),manifest)
