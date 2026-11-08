const fs=require('node:fs/promises'),path=require('node:path'),{randomBytes}=require('node:crypto');
const TYPES={'.mp3':'audio/mpeg','.ogg':'audio/ogg','.wav':'audio/wav','.m4a':'audio/mp4'};
class MusicLibrary{
 constructor(profile,defaultFolder){this.profile=profile;this.folder=defaultFolder;this.files=[];this.token=randomBytes(12).toString('hex');}
 async load(){try{const saved=JSON.parse(await fs.readFile(path.join(this.profile,'music-folder.json'),'utf8'));if(typeof saved.folder==='string'&&path.isAbsolute(saved.folder))this.folder=saved.folder;}catch{}return this.scan();}
 async select(folder){this.folder=await fs.realpath(folder);await fs.mkdir(this.profile,{recursive:true});await fs.writeFile(path.join(this.profile,'music-folder.json'),JSON.stringify({folder:this.folder}));return this.scan();}
 async scan(){
  this.token=randomBytes(12).toString('hex');this.files=[];try{const root=await fs.realpath(this.folder),entries=await fs.readdir(root,{withFileTypes:true});for(const e of entries.sort((a,b)=>a.name.localeCompare(b.name,undefined,{numeric:true}))){if(!e.isFile()||!TYPES[path.extname(e.name).toLowerCase()])continue;const file=path.join(root,e.name),stat=await fs.stat(file);if(stat.size<=100*1024*1024)this.files.push({file,name:e.name,type:TYPES[path.extname(e.name).toLowerCase()]});}}catch{}
  return this.files.map((f,i)=>({name:f.name,url:`backyard://game/local-music/${this.token}/${i}`}));
 }
 async response(pathname,range){
  const m=pathname.match(/^\/local-music\/([a-f0-9]{24})\/(\d+)$/);if(!m||m[1]!==this.token||!this.files[Number(m[2])])return new Response('Not found',{status:404});
  const f=this.files[Number(m[2])];try{
   const resolved=await fs.realpath(f.file),root=await fs.realpath(this.folder);if(path.dirname(resolved)!==root)return new Response('Not found',{status:404});
   const data=await fs.readFile(resolved);const headers={'Content-Type':f.type,'Accept-Ranges':'bytes','Cache-Control':'no-store'};
   if(range){const match=range.match(/^bytes=(\d+)-(\d*)$/);if(!match)return new Response(null,{status:416,headers:{'Content-Range':`bytes */${data.length}`}});const start=Number(match[1]),end=match[2]?Math.min(Number(match[2]),data.length-1):data.length-1;if(start>end||start>=data.length)return new Response(null,{status:416,headers:{'Content-Range':`bytes */${data.length}`}});headers['Content-Range']=`bytes ${start}-${end}/${data.length}`;headers['Content-Length']=String(end-start+1);return new Response(data.subarray(start,end+1),{status:206,headers});}
   headers['Content-Length']=String(data.length);return new Response(data,{headers});
  }catch{return new Response('Not found',{status:404});}
 }
}
module.exports={MusicLibrary};
