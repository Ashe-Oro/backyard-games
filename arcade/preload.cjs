const {contextBridge,ipcRenderer}=require('electron');
let status=null;
const listeners=new Set();
ipcRenderer.on('cabinet-input',(_event,data)=>{if(data.kind==='status'||data.kind==='error')status=data;for(const listener of listeners)listener(data);});
contextBridge.exposeInMainWorld('cabinetInput',{subscribe(callback){listeners.add(callback);if(status)callback(status);return ()=>listeners.delete(callback);}});
