import {test} from 'node:test';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

test('exit cleanup releases only the virtual exit chord, including after a crash',()=>{
 execFileSync('python3',['-c',`
import runpy, sys, types
writes=[]
held=[56,62,42]
class Device:
 def __init__(self,p): self.path=p; self.name='batocera hotkeys' if p=='virtual' else 'physical keyboard'
 def active_keys(self): return held
 def write(self,*event): writes.append((self.path,event))
 def close(self): pass
sys.modules['evdev']=types.SimpleNamespace(list_devices=lambda:['physical','virtual'],InputDevice=Device,ecodes=types.SimpleNamespace(KEY_F4=62,KEY_LEFTALT=56,EV_KEY=1,EV_SYN=0,SYN_REPORT=0))
cleanup=runpy.run_path('release-exit-keys.py')['release_exit_keys']
cleanup()
assert writes==[('virtual',(1,62,0)),('virtual',(1,56,0)),('virtual',(0,0,0))],writes
writes.clear(); held=[]; cleanup(); assert writes==[]
held=[56]; cleanup(); assert writes==[('virtual',(1,56,0)),('virtual',(0,0,0))]
`],{cwd:fileURLToPath(new URL('..',import.meta.url))});
});
