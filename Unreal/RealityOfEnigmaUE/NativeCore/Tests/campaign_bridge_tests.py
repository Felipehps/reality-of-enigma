import ctypes,json,pathlib,tempfile,copy,sys
root=pathlib.Path(__file__).resolve().parents[2]
lib=ctypes.CDLL(str(root/'Content/Runtime/Win64/Enigma.Native.dll'));lib.EnigmaOpen.restype=ctypes.c_void_p;lib.EnigmaExecute.argtypes=[ctypes.c_void_p,ctypes.c_char_p];lib.EnigmaExecute.restype=ctypes.c_void_p;lib.EnigmaFree.argtypes=[ctypes.c_void_p];lib.EnigmaClose.argtypes=[ctypes.c_void_p]
session=lib.EnigmaOpen();folder=root/'Saved/CampaignQA';folder.mkdir(parents=True,exist_ok=True);save=folder/('campaign-'+next(tempfile._get_candidate_names())+'.json')
def call(command='state',**kw):
 p=lib.EnigmaExecute(session,json.dumps(dict(command=command,**kw),ensure_ascii=False).encode());r=json.loads(ctypes.string_at(p));lib.EnigmaFree(p);return r
def init():return call('init',data=str(root/'Content/GameData'),save=str(save))
s=init();baseline=copy.deepcopy(s['save']);checks=0;errors=[];wins=0
s=call('start',id='story',number=1)
if not s['ok']:raise RuntimeError(s)
call('begin')
for _ in range(1000):
 s=call('act')
 if s['battle']['finished']:break
checks+=1
if s['save']!=baseline:errors.append('story replay changed save')
for stage in range(1,71):
 state=copy.deepcopy(baseline);state['CompletedStage']=stage-1
 for member in state['Roster']:member['Level']=70;member['Stars']=6
 save.write_text(json.dumps(state),encoding='utf8');s=init();s=call('start',id='campaign',number=stage)
 if not s['ok']:errors.append(f'stage {stage}: '+s.get('error',''));continue
 call('begin')
 for count in range(1000):
  s=call('act')
  if not s['ok']:errors.append(f'action {stage}: '+s.get('error',''));break
  if s['battle']['finished']:break
 checks+=1
 if not s['ok'] or not s['battle']['finished']:errors.append(f'not finished {stage}');continue
 if s['battle']['won']:wins+=1
 expected=stage if s['battle']['won'] else stage-1
 checks+=1
 if s['save']['CompletedStage']!=expected:errors.append(f'wrong progression {stage}')
 if stage%10==0:print(f'Campaign {stage}/70 checked')
lib.EnigmaClose(session)
for error in errors:print('FAIL',error)
print(f'CAMPAIGN_NATIVE_QA_RESULT checks={checks} stages=70 completedBattles={checks//2} referenceWins={wins} failures={len(errors)}')
sys.exit(1 if errors else 0)
