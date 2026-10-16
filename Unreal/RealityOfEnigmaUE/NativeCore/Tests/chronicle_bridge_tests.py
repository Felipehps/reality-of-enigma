"""Verify gating, real encounter routes, saves, loss handling and one-time rewards.

Reward tests use a temporary weakened catalog to force wins deterministically.
Production enemy definitions are also loaded and checked without altering balance.
"""
import ctypes,json,pathlib,tempfile,copy,sys,shutil
root=pathlib.Path(__file__).resolve().parents[2]
lib=ctypes.CDLL(str(root/'Content/Runtime/Win64/Enigma.Native.dll'))
lib.EnigmaOpen.restype=ctypes.c_void_p
lib.EnigmaExecute.argtypes=[ctypes.c_void_p,ctypes.c_char_p];lib.EnigmaExecute.restype=ctypes.c_void_p
lib.EnigmaFree.argtypes=[ctypes.c_void_p];lib.EnigmaClose.argtypes=[ctypes.c_void_p]
checks=0
def check(ok,label):
 global checks
 checks+=1
 if not ok:raise AssertionError(label)
class Session:
 def __init__(self,data,save):
  self.ptr=lib.EnigmaOpen();self.data=data;self.save=save
  self.state=self.call('init',data=str(data),save=str(save));check(self.state.get('ok'),'init')
 def call(self,command='state',**kw):
  p=lib.EnigmaExecute(self.ptr,json.dumps(dict(command=command,**kw),ensure_ascii=False).encode('utf-8'))
  result=json.loads(ctypes.string_at(p));lib.EnigmaFree(p);return result
 def close(self):lib.EnigmaClose(self.ptr)
with tempfile.TemporaryDirectory(prefix='enigma-chronicles-') as folder:
 temp=pathlib.Path(folder);save=temp/'save.json';data=root/'Content/GameData'
 s=Session(data,save);baseline=copy.deepcopy(s.state['save'])
 check(baseline['ChronicleWins']==[],'old/new saves default to no chronicle wins')
 routes=s.state['chronicles'];check(len(routes)==4,'four narrative routes')
 check([x['Id'] for x in routes]==['zhujin','hajun','mazakin','akuroth'],'narrative boss order')
 for i in range(1,5):check(not s.call('start',id='chronicle',number=i)['ok'],'first act gates route '+str(i))
 s.close()
 legacy=copy.deepcopy(baseline);legacy.pop('ChronicleWins',None)
 save.write_text(json.dumps(legacy),encoding='utf-8');s=Session(data,save)
 check(s.state['save']['ChronicleWins']==[],'legacy version-one save migrates missing collection')
 s.close()
 preview=copy.deepcopy(baseline);preview.update(CompletedStage=70,ActComplete=True,OpeningSeen=True,ForestEncounterSeen=True)
 for member in preview['Roster']:member.update(Level=70,Stars=6)
 # The preview is explicitly isolated by the Unreal launcher and never used as a normal campaign save.
 (data/'chronicle-preview-save.json').write_text(json.dumps(preview,ensure_ascii=False,indent=2),encoding='utf-8')
 save.write_text(json.dumps(preview),encoding='utf-8');s=Session(data,save)
 check(not s.call('start',id='chronicle',number=0)['ok'],'invalid route rejected')
 check(not s.call('start',id='chronicle',number=5)['ok'],'out-of-range route rejected')
 check(not s.call('start',id='chronicle',number=2)['ok'],'cannot skip previous boss')
 for i,route in enumerate(routes):
  seeded=copy.deepcopy(preview);seeded['ChronicleWins']=[r['Id'] for r in routes[:i]]
  save.write_text(json.dumps(seeded),encoding='utf-8');s.close();s=Session(data,save)
  result=s.call('start',id='chronicle',number=i+1)
  check(result['ok'],'production route starts')
  check(result['battle']['enemies'][0]['id']==route['Id'],'production boss matches map')
  check([u['id'] for u in result['battle']['allies']]==preview['Team'],'uses chosen campaign team')
  check(result['save']['CompletedStage']==70,'act one remains intact')
 s.close()
 # Weaken only the temporary enemy definitions, exercising Award through actual combat.
 testdata=temp/'data';testdata.mkdir()
 for name in ['catalog.json','regions.json','hero-stories.json','opening.json','chronicles.json']:shutil.copy2(data/name,testdata/name)
 catalog=json.loads((testdata/'catalog.json').read_text(encoding='utf-8-sig'))
 for hero in catalog['Characters']:
  if hero['Id'] in [r['Id'] for r in routes]:hero['Stats'].update(Hp=1,Strength=0,Essence=0,Defense=0,Speed=1)
 (testdata/'catalog.json').write_text(json.dumps(catalog,ensure_ascii=False),encoding='utf-8')
 save.write_text(json.dumps(preview),encoding='utf-8');s=Session(testdata,save)
 def win(index):
  state=s.call('start',id='chronicle',number=index);check(state['ok'],'reward route starts')
  s.call('begin')
  for _ in range(200):
   state=s.call('act')
   check(state.get('ok'),'combat command')
   if state['battle']['finished']:break
  check(state['battle']['finished'] and state['battle']['won'],'controlled fixture victory')
  return state
 for i,route in enumerate(routes):
  before=copy.deepcopy(s.call()['save']);state=win(i+1);after=state['save']
  check(route['Id'] in after['ChronicleWins'],'victory persists route')
  check(after['Inventory']['Gold']==before['Inventory']['Gold']+route['Gold'],'first win gold')
  check(after['Inventory']['Xp']==before['Inventory']['Xp']+route['Xp'],'first win XP')
  check(after['CompletedStage']==70,'reward does not overwrite act one')
  previous=copy.deepcopy(after);state=win(i+1)
  check(state['save']==previous,'replay does not duplicate rewards')
  s.close();s=Session(testdata,save)
  check(s.state['save']['ChronicleWins']==previous['ChronicleWins'],'wins survive reload')
 # Merely starting, leaving, or saving a new fight must never count as a win.
 state=s.call('start',id='chronicle',number=4);before=copy.deepcopy(state['save'])
 state=s.call('leave');check(state['save']==before,'leave preserves progress')
 s.close()
 # Force a loss in a separate temporary catalog; no progress or currency may be granted.
 for hero in catalog['Characters']:
  if hero['Id'] in preview['Team']:hero['Stats'].update(Hp=1,Defense=0,Speed=1)
  if hero['Id']=='zhujin':hero['Stats'].update(Hp=1000000,Strength=1000000,Essence=1000000,Speed=1000)
 (testdata/'catalog.json').write_text(json.dumps(catalog,ensure_ascii=False),encoding='utf-8')
 save.write_text(json.dumps(preview),encoding='utf-8');s=Session(testdata,save)
 state=s.call('start',id='chronicle',number=1);before=copy.deepcopy(state['save']);s.call('begin')
 for _ in range(200):
  state=s.call('act')
  if state['battle']['finished']:break
 check(state['battle']['finished'] and not state['battle']['won'],'controlled fixture loss')
 check(state['save']==before,'loss grants no reward or progression')
 s.close()
print(f'CHRONICLE_NATIVE_QA_RESULT checks={checks} routes=4 failures=0')
