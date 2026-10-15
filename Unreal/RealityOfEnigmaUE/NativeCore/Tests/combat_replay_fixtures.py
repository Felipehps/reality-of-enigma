import ctypes,json,pathlib,tempfile,random,sys,uuid
project=pathlib.Path(__file__).resolve().parents[2]; folder=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else project/'Saved/CombatReplay';folder.mkdir(parents=True,exist_ok=True)
lib=ctypes.CDLL(str(project/'Content/Runtime/Win64/Enigma.Native.dll'));lib.EnigmaOpen.restype=ctypes.c_void_p;lib.EnigmaExecute.argtypes=[ctypes.c_void_p,ctypes.c_char_p];lib.EnigmaExecute.restype=ctypes.c_void_p;lib.EnigmaFree.argtypes=[ctypes.c_void_p];lib.EnigmaClose.argtypes=[ctypes.c_void_p]
session=lib.EnigmaOpen()
def call(command,**kw):
    pointer=lib.EnigmaExecute(session,json.dumps(dict(command=command,**kw)).encode());result=json.loads(ctypes.string_at(pointer));lib.EnigmaFree(pointer)
    assert result['ok'],result.get('error');return result
save=folder/('isolated-'+uuid.uuid4().hex+'.json');call('init',data=str(project/'Content/GameData'),save=str(save))
found={}; wanted=['akuroth_cut','akuroth_waves','akuroth_prison','akuroth_meteor','akuroth_rift','transform','akuroth_chaos','akuroth_collapse','akuroth_rain','akuroth_rewrite','annihilation_charge','akuroth_annihilation']
call("formation",cells=[0,1,2,3,4])
for trial in range(100):
    generator=random.Random(trial);call("formation",cells=generator.sample(range(9),5))
    before=call('start',id='akuroth');before=call('begin')
    for turn in range(500):
        if before['battle']['finished']:break
        options=[s for s in before['battle']['skills'] if not s['reason']]
        if before['battle']['allyTurn'] and options:
            choice=generator.choice(options);side='allies' if choice['operator']=='heal' else 'enemies';targets=[u for u in before['battle'][side] if u['alive']]
            after=call('act',id=choice['id'],target=min(targets,key=lambda u:u['hp']/u['maxHp'])['key'])
        else:after=call('act')
        battle=after['battle'];skill=battle['lastSkill'];key=skill
        if skill=='akuroth_annihilation' and any(u['key']==battle['lastActor'] and u['charging'] for side in ('allies','enemies') for u in battle[side]):key='annihilation_charge'
        if key in wanted and key not in found:found[key]=(before,after)
        prior=next(u for u in before['battle']['enemies'] if u['id']=='akuroth');now=next(u for u in battle['enemies'] if u['id']=='akuroth')
        if now['ascended'] and not prior['ascended'] and 'transform' not in found:found['transform']=(before,after)
        before=after
    if len(found)==len(wanted):break
assert len(found)==len(wanted),'Nao observados: '+str(set(wanted)-set(found))
manifest=[]
for index,key in enumerate(wanted):
    before,after=found[key];skill=after['battle']['lastSkill'];hits=after['battle']['impacts']
    def at(hit):
        if hit['kind'] in ('poison','bleed'):return .25
        if skill=='akuroth_annihilation':return 3.0 if hit['hit']%2 else 1.1
        return (1.3 if 'meteor' in skill or 'rain' in skill else .65)+min(max(hit['hit'],0),10)*.08
    audio=sorted(set(at(h) for h in hits)) if skill else []
    times=[.5,1.45] if key not in ('akuroth_annihilation','transform') else ([1.2,3.1] if key=='akuroth_annihilation' else [1.0,3.5])
    if hits and key not in ('transform',):times=([1.0,1.2,3.1] if key=='akuroth_annihilation' else [max(.05,min(at(h) for h in hits)-.1),max(at(h) for h in hits)+.1])
    old={u['key']:u for side in ('allies','enemies') for u in before['battle'][side]}; samples=[]
    for time in times:
        expected={}
        for target in set(h['target'] for h in hits):
            if target not in old:continue
            amount=old[target]['hp']+sum(h['amount']*(1 if h['kind']=='heal' else -1) for h in hits if h['target']==target and at(h)<=time)
            if time>=(4.6 if key=='akuroth_annihilation' else 2.2)-.1:continue
            expected[target]=min(max(amount,0),old[target]['maxHp'])
        samples.append({'at':time,'health':expected})
    name=f'{index:02}-{key}';payload={'name':name,'before':before,'after':after,'audioTimes':audio,'samples':samples,'duration':4.6 if key=='akuroth_annihilation' else 2.2}
    (folder/(name+'.json')).write_text(json.dumps(payload,ensure_ascii=False),encoding='utf-8');manifest.append(name)
(folder/'manifest.json').write_text(json.dumps(manifest),encoding='utf-8');lib.EnigmaClose(session)
print('REAL_COMBAT_FIXTURES skills=10 transformation=1 charge=1 trials='+str(trial+1)+' source=NativeDLL')
