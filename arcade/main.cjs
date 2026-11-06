const {app,BrowserWindow,Menu,protocol,session}=require('electron');
const fs=require('node:fs'),path=require('node:path');
const root=path.join(__dirname,'dist','site');
protocol.registerSchemesAsPrivileged([{scheme:'backyard',privileges:{standard:true,secure:true,supportFetchAPI:true,corsEnabled:true}}]);
// Explicit troubleshooting launchers opt into Chromium's software renderer.
if(process.argv.includes('--software'))app.commandLine.appendSwitch('use-gl','angle'),app.commandLine.appendSwitch('use-angle','swiftshader');
const smoke=process.argv.includes('--smoke-test');
if(process.env.ARCADE_TEST_DATA)app.setPath('userData',process.env.ARCADE_TEST_DATA);
app.setName('Backyard Cornhole Arcade');
app.whenReady().then(async()=>{
 protocol.handle('backyard',async request=>{
  const url=new URL(request.url);if(url.host!=='game')return new Response('Not found',{status:404});
  let pathname;try{pathname=decodeURIComponent(url.pathname);}catch{return new Response('Bad path',{status:400});}
  const file=path.resolve(root,'.'+pathname);
  if(!file.startsWith(root+path.sep)||!fs.existsSync(file)||!fs.statSync(file).isFile())return new Response('Not found',{status:404});
  const type={'.html':'text/html','.js':'application/javascript','.mjs':'application/javascript','.css':'text/css','.woff2':'font/woff2'}[path.extname(file)]||'application/octet-stream';
  return new Response(await fs.promises.readFile(file),{headers:{'content-type':type}});
 });
 session.defaultSession.setPermissionRequestHandler((wc,permission,callback)=>callback(wc.getURL().startsWith('backyard://game/')&&['pointerLock','fullscreen'].includes(permission)));
 session.defaultSession.setPermissionCheckHandler((wc,permission)=>!!wc&&wc.getURL().startsWith('backyard://game/')&&['pointerLock','fullscreen'].includes(permission));
 // The cabinet build has no remote resources, update checks, login, or external navigation.
 session.defaultSession.webRequest.onBeforeRequest((details,callback)=>callback({cancel:!['backyard:','devtools:','data:','blob:'].some(s=>details.url.startsWith(s))}));
 Menu.setApplicationMenu(null);
 const win=new BrowserWindow({width:1280,height:800,fullscreen:!smoke&&!process.argv.includes('--windowed'),show:!smoke,backgroundColor:'#173f3d',autoHideMenuBar:true,webPreferences:{nodeIntegration:false,contextIsolation:true,sandbox:true}});
 win.webContents.setWindowOpenHandler(()=>({action:'deny'}));
 win.webContents.on('will-navigate',(e,url)=>{if(!url.startsWith('backyard://game/'))e.preventDefault();});
 win.webContents.on('before-input-event',(e,input)=>{
  if(input.type==='keyDown'&&input.alt&&input.key==='F4'){e.preventDefault();app.quit();}
 });
 win.webContents.on('render-process-gone',(_e,details)=>{console.error('Game renderer stopped:',details.reason);app.exit(1);});
 await win.loadURL('backyard://game/cornhole/index.html');
 if(smoke){
  let result;
  for(let i=0;i<100;i++){result=await win.webContents.executeJavaScript('Boolean(window.__arcade)');if(result)break;await new Promise(r=>setTimeout(r,100));}
  if(!result)throw new Error('Arcade controller failed to initialize');
  const report=await win.webContents.executeJavaScript(`(async()=>{__arcade.start('practice',false);await new Promise(r=>setTimeout(r,1200));return {title:document.title,phase:__game.state.phase,canvas:!!document.querySelector('canvas'),offline:performance.getEntriesByType('resource').every(r=>!r.name.startsWith('http')),settings:__arcade.config};})()`);
  console.log('ARCADE_SMOKE '+JSON.stringify(report));if(report.phase!=='aim'||!report.offline)throw new Error('Offline startup failed');app.quit();
 }
}).catch(error=>{console.error(error);app.exit(1);});
app.on('window-all-closed',()=>app.quit());
