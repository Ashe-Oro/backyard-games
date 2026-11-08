// A second camera shares the existing scene and renderer: bags/lighting never drift.
export class BoardView {
 constructor(THREE,renderer,scene,boardZ,element){
  Object.assign(this,{renderer,scene,element});
  this.camera=new THREE.PerspectiveCamera(30,1,.05,30);
  this.camera.position.set(0,2.5,boardZ+.75);
  this.camera.lookAt(0,.16,boardZ-.55);
  this.size=new THREE.Vector2();this.viewport=new THREE.Vector4();this.scissor=new THREE.Vector4();
 }
 render(){
  const {renderer:r,element:e}=this;
  if(!e||e.closest('[hidden]'))return;
  const box=e.getBoundingClientRect(),canvas=r.domElement.getBoundingClientRect();
  if(!box.width||!box.height||!canvas.width||!canvas.height)return;
  r.getSize(this.size);r.getViewport(this.viewport);r.getScissor(this.scissor);
  const oldTest=r.getScissorTest(),oldAuto=r.autoClear;
  const x=Math.round((box.left-canvas.left)/canvas.width*this.size.x),y=Math.round((canvas.bottom-box.bottom)/canvas.height*this.size.y);
  const w=Math.max(1,Math.round(box.width/canvas.width*this.size.x)),h=Math.max(1,Math.round(box.height/canvas.height*this.size.y));
  this.camera.aspect=w/h;this.camera.updateProjectionMatrix();
  try{
   r.setViewport(x,y,w,h);r.setScissor(x,y,w,h);r.setScissorTest(true);r.autoClear=false;
   r.clear(true,true,false);r.render(this.scene,this.camera);
  }finally{
   r.autoClear=oldAuto;r.setViewport(this.viewport);r.setScissor(this.scissor);r.setScissorTest(oldTest);
  }
 }
}
