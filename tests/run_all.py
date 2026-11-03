"""Run every Playwright suite with bounded waits (serve the app on port 8000)."""
from pathlib import Path
import runpy
import subprocess
import sys

if len(sys.argv)>1 and sys.argv[1]=='--worker':
 from playwright.async_api import BrowserContext
 original=BrowserContext.new_page
 async def new_page(self,*args,**kwargs):
  page=await original(self,*args,**kwargs)
  page.set_default_timeout(8000)
  page.set_default_navigation_timeout(12000)
  return page
 BrowserContext.new_page=new_page
 runpy.run_path(sys.argv[2],run_name='__main__')
else:
 root=Path(__file__).resolve().parent.parent
 failed=[]
 for suite in sorted((root/'tests').glob('*_check.py')):
  print(f'Running {suite.name}',flush=True)
  try:
   result=subprocess.run([sys.executable,__file__,'--worker',str(suite)],cwd=root,timeout=150)
   if result.returncode:failed.append(suite.name)
  except subprocess.TimeoutExpired:
   print(f'{suite.name}: exceeded 150-second suite limit',flush=True)
   failed.append(suite.name)
 print('Failed: '+', '.join(failed) if failed else 'All Playwright suites passed',flush=True)
 sys.exit(bool(failed))
