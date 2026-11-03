const ts = require('typescript');
const { spawnSync } = require('node:child_process');
const examples = [
 ['typed declarations', 'let count: number = 2;\nlet total: number = count + 3;'],
 ['arithmetic and comparison', 'let ok = a === b && c >= 2;'],
 ['functions and arrows', 'function f(x: number): number { return x + 1; }\nlet f2 = (x: number) => x * 2;'],
 ['interfaces and type aliases', 'interface User { name: string; age?: number; }\ntype Id = string | number;'],
 ['generics', 'function identity<T>(x: T): T { return x; }'],
 ['classes', 'class Box { private value: number = 1; get(): number { return this.value; } }'],
 ['modules', 'import { x as y } from "./m.js";\nexport { y };'],
 ['arrays and object literals', 'let a: number[] = [1, 2];\nlet o = { x: 1, y: "ok" };'],
 ['comments', 'let a = 1; // explanation\n/* block */ let b = 2;'],
 ['template literals', 'let greeting = `hello ${name}`;'],
 ['numeric forms', 'let a = 0xff; let b = 1e3; let c = 1_000; let d = 10n;'],
 ['optional chaining', 'let x = obj?.field ?? "none";'],
 ['regular expressions', 'let pattern = /ab+c/gi;'],
 ['unicode identifiers', 'let имя = "текст";'],
 ['control flow', 'if (x > 1) { x++; } else { x--; }'],
 ['async and await', 'async function f() { return await Promise.resolve(1); }'],
];
let failed = 0, invalid = 0;
console.log(`TypeScript ${ts.version}: ${examples.length} syntax examples`);
for (const [name, source] of examples) {
 const parsed = ts.createSourceFile('example.ts', source, ts.ScriptTarget.Latest, true, ts.ScriptKind.TS);
 if (parsed.parseDiagnostics.length) { invalid++; console.log(`INVALID ${name}: ${parsed.parseDiagnostics.map(d => d.code).join(',')}`); continue; }
 const scanner = ts.createScanner(ts.ScriptTarget.Latest, true, ts.LanguageVariant.Standard, source);
 const ref = [];
 for (let kind = scanner.scan(); kind !== ts.SyntaxKind.EndOfFileToken; kind = scanner.scan()) ref.push(scanner.getTokenText());
 const run = spawnSync('./tests/typescript_lex_probe', { input: source, encoding: 'utf8' });
 if (run.status !== 0) { console.log(`ERROR ${name}: ${run.stderr}`); failed++; continue; }
 const ours = run.stdout.trim().split('\n').filter(Boolean).map(x => Buffer.from(x, 'hex').toString('utf8'));
 const same = JSON.stringify(ref) === JSON.stringify(ours);
 console.log(`${same ? 'PASS' : 'DIFF'} ${name}`);
 if (!same) { failed++; console.log(`  TypeScript: ${JSON.stringify(ref)}\n  RuCode:     ${JSON.stringify(ours)}`); }
}
console.log(`Result: ${examples.length - failed - invalid} matched, ${failed} differed, ${invalid} invalid examples`);
if (failed || invalid) process.exitCode = 1;
