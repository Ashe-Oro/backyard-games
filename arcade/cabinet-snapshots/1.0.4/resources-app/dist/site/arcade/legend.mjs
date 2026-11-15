// Shared by the cabinet and the standalone screenshot fixture.
export const legendMarkup=`<aside id="arcade-spinner" aria-label="Spinner legend"><div>A ↻ spin</div><div>C ↺ spin</div><div>1 ← aim</div><div>3 → aim</div></aside><aside id="arcade-keys" aria-label="P1 button legend"><div class="keys-status"><span id="arcade-state">↑</span><span id="arcade-height">NORMAL</span><span id="arcade-surface">SLICK</span><button id="arcade-keys-toggle" aria-expanded="true" aria-controls="arcade-keys-map">KEYS</button></div><div id="arcade-keys-map"><div class="keys-buttons"><span class="keys-button"><i aria-hidden="true"></i><b>HEIGHT</b></span><span class="keys-button"><i aria-hidden="true"></i><b>SURFACE</b></span><span class="keys-button"><i aria-hidden="true"></i><b>RESET</b></span><span class="keys-button"><i aria-hidden="true"></i><b>CLEAR</b></span><span class="keys-button"><i aria-hidden="true"></i><b>LEFT</b></span><span class="keys-button"><i aria-hidden="true"></i><b>RIGHT</b></span><span class="keys-button"><i aria-hidden="true"></i><b>RESET</b></span><span class="keys-button"><i aria-hidden="true"></i><b>KEYS</b></span></div></div></aside>`;
const STORAGE_KEY='backyard-arcade-keys-tucked';
export function loadKeysTucked(storage){try{return storage.getItem(STORAGE_KEY)==='true';}catch{return false;}}
export function toggleKeys(legend,storage){
 const tucked=legend.dataset.tucked!=='true';
 renderKeys(legend,tucked);
 try{storage.setItem(STORAGE_KEY,String(tucked));}catch{}
 return tucked;
}
export function renderKeys(legend,tucked){
 legend.dataset.tucked=String(tucked);
 legend.querySelector('#arcade-keys-toggle').setAttribute('aria-expanded',String(!tucked));
 legend.querySelector('#arcade-keys-map').hidden=tucked;
}
