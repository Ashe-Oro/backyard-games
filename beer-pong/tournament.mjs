export const TOURNAMENT_KEY='backyard-beer-pong-tournament-v1';
export const CIRCUITS=['Backyard','Block Party','County Fair','Cup Finals'];
export const RIVALS=[
 {name:'Patio Pete',profile:'rookie',color:'#e6a45c',taunt:'Pull up a chair.',shoot:{precise:.10,jitter:.40,bounce:.18}},
 {name:'Bounce Bea',profile:'rookie',color:'#b393db',taunt:'One bounce. One cup.',shoot:{precise:.18,jitter:.36,bounce:.40}},
 {name:'Lucky Lou',profile:'regular',color:'#86b65d',taunt:'Luck has good aim.',shoot:{precise:.26,jitter:.32,bounce:.24}},
 {name:'Bankshot Barb',profile:'regular',color:'#e98291',taunt:'Mind the bounce.',shoot:{precise:.36,jitter:.28,bounce:.50}},
 {name:'Captain Cups',profile:'ace',color:'#79b8d4',taunt:'Steady as she goes.',shoot:{precise:.47,jitter:.22,bounce:.28}},
 {name:'Steady Sam',profile:'ace',color:'#db9458',taunt:'No wasted shots.',shoot:{precise:.59,jitter:.17,bounce:.40}},
 {name:'Ace Avery',profile:'boss',color:'#8eb5d2',taunt:'Your move.',shoot:{precise:.70,jitter:.12,bounce:.30}},
 {name:'Cup Queen',profile:'boss',color:'#d49cc5',taunt:'Earn the crown.',shoot:{precise:.82,jitter:.085,bounce:.40}}
].map((r,index)=>Object.freeze({...r,index,circuit:CIRCUITS[Math.floor(index/2)],shoot:Object.freeze(r.shoot)}));
Object.freeze(RIVALS);
function parsedProgress(raw){try{const data=JSON.parse(raw);return data?.version===1&&Number.isInteger(data.cleared)&&data.cleared>=0&&data.cleared<=RIVALS.length?data.cleared:0;}catch{return 0;}}
// Only completed rivals are saved; interrupted matches restart the pending rival.
export class TournamentProgress{
 constructor(storage){this.storage=storage;this.cleared=0;this.canSave=!!storage;try{this.cleared=parsedProgress(storage?.getItem(TOURNAMENT_KEY));}catch{this.canSave=false;}}
 get complete(){return this.cleared===RIVALS.length;}
 recordWin(index){if(index!==this.cleared||this.complete)return false;this.cleared++;this.save();return true;}
 reset(){this.cleared=0;this.save(true);}
 save(reset=false){try{if(!this.storage)throw Error('No storage');if(!reset)this.cleared=Math.max(this.cleared,parsedProgress(this.storage.getItem(TOURNAMENT_KEY)));this.storage.setItem(TOURNAMENT_KEY,JSON.stringify({version:1,cleared:this.cleared}));this.canSave=true;}catch{this.canSave=false;}}
}
export function portraitSvg(rival){
 const looks=[
 ['#e9b78e','#65442f','M4 3h8v3H4z','<path fill="#e6a45c" d="M3 3h10v2H3zM9 5h5v1H9z"/>'],
 ['#aa795a','#403045','M3 4h10v3H3zM2 6h3v6H2zM11 6h3v6h-3z','<path fill="#b393db" d="M3 3h10v2H3z"/>'],
 ['#dca67d','#4f3d29','M4 3h8v4H4z','<path fill="#86b65d" d="M4 2h8v3H4zM3 5h10v1H3z"/>'],
 ['#815239','#272939','M3 3h10v4H3zM3 6h2v6H3zM11 6h2v6h-2z','<path fill="#e98291" d="M5 2h2v2H5zM8 2h2v2H8z"/>'],
 ['#ecc29b','#b3a388','M4 4h8v3H4z','<path fill="#fff1cf" d="M3 2h10v3H3z"/><path fill="#448fc4" d="M3 5h10v1H3z"/>'],
 ['#c28d67','#a35e31','M4 3h8v3H4zM4 6h1v2H4zM11 6h1v2h-1z','<path fill="#a35e31" d="M5 10h6v2H5z"/>'],
 ['#79513d','#252633','M4 3h8v4H4z','<path fill="#8eb5d2" d="M3 5h10v1H3z"/><path fill="#252633" d="M5 8h3v2H5zM9 8h3v2H9zM8 8h1v1H8z"/>'],
 ['#ecc19b','#6e3e46','M3 4h10v9h-2V6H5v7H3z','<path fill="#ffe16b" d="M4 1h2v2h1V0h2v3h1V1h2v4H4z"/>']
 ][rival.index];
 const [skin,hair,hairPath,accessory]=looks;
 return `<svg class="rival-portrait" viewBox="0 0 16 16" role="img" aria-label="${rival.name}" shape-rendering="crispEdges"><rect width="16" height="16" fill="#29483f"/><path fill="${rival.color}" d="M2 13h12v3H2z"/><path fill="${skin}" d="M4 5h8v7H4zM3 7h1v3H3zM12 7h1v3h-1zM6 12h4v1H6z"/><path fill="${hair}" d="${hairPath}"/><path fill="#25312d" d="M5 8h2v1H5zM9 8h2v1H9zM7 10h3v1H7z"/>${accessory}</svg>`;
}
