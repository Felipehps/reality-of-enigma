import ctypes,json,pathlib,tempfile,copy,sys
root=pathlib.Path(__file__).resolve().parents[2]
lib=ctypes.CDLL(str(root/'Content/Runtime/Win64/Enigma.Native.dll'))
lib.EnigmaOpen.restype=ctypes.c_void_p
lib.EnigmaExecute.argtypes=[ctypes.c_void_p,ctypes.c_char_p];lib.EnigmaExecute.restype=ctypes.c_void_p
lib.EnigmaFree.argtypes=[ctypes.c_void_p];lib.EnigmaClose.argtypes=[ctypes.c_void_p]
session=lib.EnigmaOpen();checks=0;failures=[]
scratch=root/'Saved'/'NativeQA';scratch.mkdir(parents=True,exist_ok=True)
save=scratch/('save-'+next(tempfile._get_candidate_names())+'.json')
def call(command='state',**kw):
    ptr=lib.EnigmaExecute(session,json.dumps(dict(command=command,**kw),ensure_ascii=False).encode('utf8'))
    result=json.loads(ctypes.string_at(ptr));lib.EnigmaFree(ptr);return result
def check(ok,label):
    global checks
    checks+=1
    if not ok:failures.append(label);print('FAIL',label)
def init():return call('init',data=str(root/'Content/GameData'),save=str(save))
s=init();check(s['ok'],'native_init');check(len(s['catalog']['Stages'])==70,'70 stages');check(len(s['regions'])==7,'7 regions');check(len(s['opening'])==8,'8 prologue shots')
check(len(s['save']['Roster'])==5,'five starters');check(s['save']['Team']==['kael','savor','aelia','brakk','lyra'],'original team')
s=call('level',id='kael');check(s['save']['Roster'][0]['Level']==2,'level increase');check(s['save']['Inventory']['Xp']==4975,'level cost')
s=call('promote',id='kael');check(s['save']['Roster'][0]['Stars']==3,'promotion gate')
s=call('path',id='martial');check(s['save']['Roster'][0]['Path']=='base','path level gate')
s=call('summon',number=10);check(s['save']['Inventory']['Gold']==1000,'ten summon cost');check(sum(x['Copies'] for x in s['save']['Roster'])+len(s['save']['Roster'])-5==10,'copies count');check(all(not x['Id'].startswith('enemy') for x in s['save']['Roster']),'summon pool')
s=call('summon',number=10);check(s['save']['Inventory']['Gold']==1000,'insufficient gold no spend');check(not call('summon',number=2)['ok'],'invalid summon count')
s=call('formation',cells=[-1,0,0,9,6]);check(s['save']['FormationCells']==[0,1,2,3,6],'normalize formation')
s=call('team',id='lyra',number=0);check(s['save']['Team'][0]=='lyra' and s['save']['Team'][4]=='kael','swap team without duplicates')
check(not call('team',id='akuroth',number=0)['ok'],'unowned team rejected');check(not call('start',id='campaign',number=70)['ok'],'locked campaign rejected')
check(not call('start',id='training',boss='varkhan')['ok'],'locked training rejected')
s=call('start',id='campaign',number=1);check(s['ok'],'campaign stage one');check(any(x['id']=='raizen' for x in s['battle']['allies']),'narrative raizen guest');check(not any(x['Id']=='raizen' for x in s['save']['Roster']),'guest not recruited')
uid=s['battle']['allies'][0]['key'];s=call('move',id=uid,number=8);check(s['battle']['allies'][0]['column']==2 and s['battle']['allies'][0]['layer']==2,'battle formation movement')
call('begin');s=call('move',id=uid,number=0);check(s['battle']['allies'][0]['column']==2,'formation locked')
check(not call('act',id='akuroth_cut',target=s['battle']['enemies'][0]['key'])['ok'],'foreign skill rejected')
for kind in ['demo','cast','varkas','sevrin','floral','eclipse','sandbox','akuroth']:
    s=call('start',id=kind);check(s['ok'],'start '+kind);before=copy.deepcopy(s['save']);call('begin')
    count=0;seen=set();forms=False;phases=False
    while count<1500 and not s['battle']['finished']:
        s=call('act');check(s['ok'],'action '+kind) if not s['ok'] else None
        if not s['ok']:break
        seen.add(s['battle']['lastSkill']);forms|=any('Espírito' in x['name'] for x in s['battle']['allies']);phases|=any(x['ascended'] for x in s['battle']['enemies']);count+=1
    check(s['battle']['finished'],'bounded battle '+kind);check(s['save']==before,'arena no rewards '+kind)
    if kind=='varkas':check(forms,'body spirit split')
    if kind=='sandbox':check('martial_extreme' in seen,'kael extreme technique')
# Use an isolated valid save to exercise unlocked campaign and training, never the user's save.
state=copy.deepcopy(s['save']);state['CompletedStage']=19;state['Team']=['kael','savor','aelia','brakk','lyra'];state['FormationCells']=[0,1,3,4,6]
for c in state['Roster']:c['Stars']=6;c['Level']=70
state['Inventory']['Gold']=20000;state['Training']=[];state['TrainingWins']=[];save.write_text(json.dumps(state),encoding='utf8');s=init()
s=call('start',id='campaign',number=20);call('begin')
for _ in range(1500):
    s=call('act')
    if s['battle']['finished']:break
check(s['battle']['won'],'campaign boss victory');check(s['save']['CompletedStage']==20,'campaign progression');check('varkhan' in s['save']['Training'],'boss training unlocked')
reward=s['save']['Inventory']['Gold'];s=call('act');check(s['save']['Inventory']['Gold']==reward,'campaign reward once')
s=call('start',id='training',boss='varkhan');call('begin')
for _ in range(2500):
    s=call('act')
    if s['battle']['wave']==3 and s['battle']['finished']:break
check(s['battle']['wave']==3 and s['battle']['won'],'three waves won');check(s['save']['Inventory']['Fragments']['varkhan']==1,'training fragment')
reward=s['save']['Inventory']['Gold'];s=call('act');check(s['save']['Inventory']['Gold']==reward,'training reward once')
s=call('simulate',id='varkhan');check(s['save']['Inventory']['Tickets']==2,'simulation ticket');check(s['save']['Inventory']['Fragments']['varkhan']==2,'simulation fragment')
s=call('save');saved=copy.deepcopy(s['save']);s=init();check(s['save']==saved,'save reload parity')
lib.EnigmaClose(session)
print(f'NATIVE_MIGRATION_QA_RESULT checks={checks} failures={len(failures)}')
sys.exit(1 if failures else 0)
