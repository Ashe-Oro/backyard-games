#!/usr/bin/env node
// Print (or --write) shared/cornhole/baseline.json from the baseline commit in git.
// Needs the baseline commit locally (a full clone, or `git fetch origin tag v1.0.3`).
//   node shared/cornhole/tools/baseline.mjs           # print to stdout
//   node shared/cornhole/tools/baseline.mjs --write   # overwrite shared/cornhole/baseline.json
// Only regenerate when a new field is recorded (its baseline value is read from git, not typed in)
// or when the baseline itself is deliberately moved (update SPEC in lib/baseline.mjs and clear the
// approvals it absorbs, in one reviewed PR).
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {SPEC,loadShared,baselineSources,buildBaseline,commitAvailable} from '../lib/baseline.mjs';

const shared=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const repo=path.resolve(shared,'..','..');
if(!commitAvailable(repo,SPEC.commit)){console.error(`Baseline commit ${SPEC.commit} (${SPEC.tag}) is not in this clone. Fetch full history first.`);process.exit(2);}
const doc=JSON.stringify(buildBaseline(loadShared(shared),baselineSources(repo)),null,1)+'\n';
if(process.argv.includes('--write')){fs.writeFileSync(path.join(shared,'baseline.json'),doc);console.error('wrote shared/cornhole/baseline.json');}
else process.stdout.write(doc);
