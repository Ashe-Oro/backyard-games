export function setupMusic(root){
 const $=id=>document.getElementById(id),audio=new Audio();audio.preload='metadata';
 let request=0;let tracks=[],index=0,enabled=localStorage.getItem('arcade-music-enabled')==='on',volume=Number(localStorage.getItem('arcade-music-volume')??.25);
 audio.volume=Math.max(0,Math.min(1,Number.isFinite(volume)?volume:.25));
 function render(){
  $('music-toggle').textContent=enabled?'PAUSE MUSIC':'PLAY MUSIC';$('music-toggle').disabled=!tracks.length;$('music-next').disabled=tracks.length<2;
  $('music-volume').textContent=Math.round(audio.volume*100)+'%';$('music-now').textContent=tracks[index]?.name||'Add MP3s to your music folder.';
 }
 async function play(){if(!tracks.length||!enabled)return;const ticket=++request,url=tracks[index].url;if(audio.src!==url)audio.src=url;try{await audio.play();}catch(error){if(error.name==='AbortError'||ticket!==request)return;enabled=false;audio.pause();localStorage.setItem('arcade-music-enabled','off');render();}}
 async function next(){if(!tracks.length)return;request++;index=(index+1)%tracks.length;audio.src=tracks[index].url;render();await play();}
 async function load(choose=false){
  request++;audio.pause();try{tracks=await (choose?window.arcadeMusic.choose():window.arcadeMusic.list());index=0;render();await play();}catch{$('music-now').textContent='Music folder unavailable.';}
 }
 $('music-toggle').onclick=()=>{enabled=!enabled;localStorage.setItem('arcade-music-enabled',enabled?'on':'off');if(enabled)play();else{request++;audio.pause();}render();};
 $('music-next').onclick=next;$('music-folder').onclick=()=>load(true);$('music-refresh').onclick=()=>load();
 for(const b of root.querySelectorAll('[data-music-volume]'))b.onclick=()=>{audio.volume=Math.max(0,Math.min(1,Number((audio.volume+Number(b.dataset.musicVolume)).toFixed(2))));localStorage.setItem('arcade-music-volume',audio.volume);render();};
 let failures=0;audio.onended=()=>{failures=0;next();};audio.onerror=()=>{if(++failures<tracks.length)next();else{enabled=false;render();$('music-now').textContent='Could not play these files.';}};
 document.addEventListener('pointerdown',()=>{if(enabled&&audio.paused)play();});
 document.addEventListener('keydown',()=>{if(enabled&&audio.paused)play();});
 window.addEventListener('blur',()=>audio.pause());window.addEventListener('focus',()=>{if(enabled)play();});
 render();if(window.arcadeMusic)load();else{$('music-folder').disabled=true;$('music-refresh').disabled=true;}
 return {audio,get tracks(){return [...tracks];},get enabled(){return enabled;}};
}
