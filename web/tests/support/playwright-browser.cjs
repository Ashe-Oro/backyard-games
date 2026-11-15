// Preloaded by run-manifest.mjs (NODE_OPTIONS=--require) into every check process.
// The checks call chromium.launch() with no executablePath; this applies the runner's browser
// selection (WEB_TESTS_CHROME) so no out-of-repo shim is needed. Unset = Playwright's own Chromium.
const executablePath=process.env.WEB_TESTS_CHROME;
if(executablePath){
 let playwright;
 try{playwright=require('playwright-core');}catch{playwright=null;}
 if(playwright){
  const launch=playwright.chromium.launch.bind(playwright.chromium);
  playwright.chromium.launch=(options={})=>launch({executablePath,...options});
 }
}
