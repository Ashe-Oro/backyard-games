export const games={
 'ladder-golf':{name:'Ladder Golf',object:'bola',count:3,distance:4.572,tag:'Wrap the rungs.',source:'https://fcrv.org/wp-content/uploads/2018/08/laddergolf.pdf',rules:'Throw all 3 bolas, then pass. Top / middle / bottom: 3 / 2 / 1 points. Only hanging bolas score; no cancellation. Exactly 21 wins. Go over: no points that round. Both reach 21: win by 2 in overtime. Optional +1: three on one rung or one on each.'},
 washers:{name:'Washers',object:'washer',count:3,distance:5.49,tag:'Find the cup.',source:'https://www.costco.ca/wcsstore/CostcoCABCCatalogAssetStore/Attachment/324857-Washer-Toss-Rules.pdf',rules:'Alternate 3 washers each. Cup: 3; box: 1; outside: 0. Score after all 6 throws; no cancellation. Exactly 21 wins. Go over: subtract your round’s points instead. Bounces count.'},
 'lawn-darts':{name:'Lawn Darts',object:'dart',count:2,distance:10.668,tag:'Hit the ring.',source:'https://images.thdstatic.com/catalog/pdfImages/3d/3da343e5-f648-487f-a5e0-8a7b4afadce2.pdf#page=33',rules:'Alternate 2 darts each. Tip in the ring: 3 points; opposing ring hits cancel. If both hit the ring, outside darts score 0. Otherwise, each outside dart closer than every opposing dart scores 1. First to 21+ wins after the round.'}
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
