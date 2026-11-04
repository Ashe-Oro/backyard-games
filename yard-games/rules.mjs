export const games={
 'ladder-golf':{name:'Ladder Golf',object:'bola',count:3,distance:4.572,tag:'Three rungs. One perfect wrap.',source:'https://fcrv.org/wp-content/uploads/2018/08/laddergolf.pdf',rules:'Throw all 3 bolas, then your opponent throws 3. Top rung: 3; middle: 2; bottom: 1. Only bolas still hanging count; knock-offs score zero. Each player adds their own points. Reach exactly 21; a round that takes you over scores nothing. If both reach 21 together, overtime continues until one leads by 2. Optional bonus: +1 for three on one rung or one on every rung.'},
 washers:{name:'Washers',object:'washer',count:3,distance:5.49,tag:'Find the cup. Hear the clink.',source:'https://www.costco.ca/wcsstore/CostcoCABCCatalogAssetStore/Attachment/324857-Washer-Toss-Rules.pdf',rules:'Box-and-cup rules: alternate 3 washers each. Cup: 3 points; box: 1; outside: 0. Points are added without cancellation after all 6 throws. Reach exactly 21. If your round takes you over 21, subtract that round’s points instead. Bounces into the target count.'},
 'lawn-darts':{name:'Lawn Darts',object:'dart',count:2,distance:10.668,tag:'A little arc. A lot of accuracy.',source:'https://images.thdstatic.com/catalog/pdfImages/3d/3da343e5-f648-487f-a5e0-8a7b4afadce2.pdf#page=33',rules:'Handly Cup rules: alternate 2 darts each. A tip inside the ring scores 3. Opposing ring hits cancel. If both teams have ring hits, outside darts cannot score. Otherwise, outside darts closer to the ring than every opposing dart score 1 each. First to 21 or more wins after the round.'}
};
export function roundPoints(kind,shots,bonus=false){
 const active=shots.filter(s=>s.alive!==false),points=[0,0];
 if(kind==='ladder-golf')for(let t=0;t<2;t++){const r=active.filter(s=>s.team===t&&s.rung>=0).map(s=>s.rung);points[t]=r.reduce((n,i)=>n+i+1,0);if(bonus&&r.length===3&&(new Set(r).size===1||new Set(r).size===3))points[t]++;}
 if(kind==='washers')for(const s of active)points[s.team]+=s.value||0;
 if(kind==='lawn-darts'){
  const inside=[0,1].map(t=>active.filter(s=>s.team===t&&s.inside).length),net=inside[0]-inside[1];
  if(net)points[net>0?0:1]=Math.abs(net)*3;
  if(!(inside[0]&&inside[1]))for(let t=0;t<2;t++){
   const other=active.filter(s=>s.team!==t),closest=Math.min(...other.map(s=>s.distance));
   points[t]+=active.filter(s=>s.team===t&&!s.inside&&s.distance<closest-1e-7).length;
  }
 }
 return points;
}
export function settleRound(kind,scores,shots,{bonus=false,overtime=false}={}){
 const points=roundPoints(kind,shots,bonus),next=[...scores],bust=[false,false];let winner=null,ot=overtime;
 for(let t=0;t<2;t++){
  const sum=scores[t]+points[t];
  if(kind==='ladder-golf'&&!ot&&sum>21)bust[t]=true;
  else if(kind==='washers'&&sum>21){next[t]=Math.max(0,scores[t]-points[t]);bust[t]=true;}
  else next[t]=sum;
 }
 if(kind==='ladder-golf'){
  if(ot){if(Math.abs(next[0]-next[1])>=2)winner=next[0]>next[1]?0:1;}
  else if(next[0]===21&&next[1]===21)ot=true;
  else if(next.includes(21))winner=next[0]===21?0:1;
 }else if(kind==='washers'){
  if(next[0]===21&&next[1]===21){winner=null;}
  else if(next.includes(21))winner=next[0]===21?0:1;
 }else if(Math.max(...next)>=21&&next[0]!==next[1])winner=next[0]>next[1]?0:1;
 const first=points[0]===points[1]?null:points[0]>points[1]?0:1;
 return {points,scores:next,bust,winner,overtime:ot,first};
}
