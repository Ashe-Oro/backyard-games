const choices={6:[['triangle','Triangle']],4:[['diamond','Diamond'],['square','Square']],3:[['triangle','Triangle'],['stoplight','Stoplight']],2:[['line','Front-to-back']]};
export const layoutsFor=count=>(choices[count]||[]).map(([id,label])=>({id,label}));
// Coordinates are in cup-diameter units: depth 0 is the back edge of the rack.
export function layoutPoints(id,count){
 if(!layoutsFor(count).some(o=>o.id===id))return null;
 const h=Math.sqrt(3)/2;
 if(id==='triangle'){const rows=count===6?3:2,points=[];for(let row=0;row<rows;row++)for(let col=0;col<=row;col++)points.push({x:col-row/2,depth:(rows-1-row)*h});return points;}
 if(id==='diamond')return [{x:0,depth:2*h},{x:-.5,depth:h},{x:.5,depth:h},{x:0,depth:0}];
 if(id==='square')return [{x:-.5,depth:1},{x:.5,depth:1},{x:-.5,depth:0},{x:.5,depth:0}];
 return Array.from({length:count},(_,i)=>({x:0,depth:count-1-i}));
}
