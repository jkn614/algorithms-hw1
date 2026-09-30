// report/REPORT.md를 제출용 PDF(A4)로 만든다.
//
//   node tools/pdf.mjs            → report/REPORT.pdf
//
// 제출 형식이 pdf라서 둔 도구다. marked(마크다운) · mermaid(다이어그램) ·
// playwright(크로미움으로 인쇄)를 쓴다. 저장소의 C 코드와 달리 외부 패키지가
// 필요하므로 make 대상에는 넣지 않았다: npm i -g marked playwright @mermaid-js/mermaid-cli
import { readFileSync, writeFileSync } from "node:fs";
import { createRequire } from "node:module";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { execSync } from "node:child_process";

const require = createRequire(import.meta.url);
const globalRoot = execSync("npm root -g").toString().trim();
const { marked } = require(`${globalRoot}/marked`);
const { chromium } = require(`${globalRoot}/playwright`);
const mermaidJs = `${globalRoot}/@mermaid-js/mermaid-cli/node_modules/mermaid/dist/mermaid.min.js`;

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const reportDir = resolve(root, "report");
const src = process.argv[2] ?? resolve(reportDir, "REPORT.md");
const out = process.argv[3] ?? resolve(reportDir, "REPORT.pdf");

// 제목에 붙는 GitHub식 앵커를 만든다 (차례의 링크가 PDF 안에서도 이어지게).
const slug = (t) =>
  t.toLowerCase().replace(/<[^>]+>/g, "").replace(/[^\p{L}\p{N}\s-]/gu, "").trim().replace(/\s/g, "-");

const renderer = new marked.Renderer();
renderer.code = ({ text, lang }) =>
  lang === "mermaid"
    ? `<pre class="mermaid">${text}</pre>`
    : `<pre><code>${text.replace(/&/g, "&amp;").replace(/</g, "&lt;")}</code></pre>`;
renderer.heading = function ({ tokens, depth }) {
  const html = this.parser.parseInline(tokens);
  return `<h${depth} id="${slug(html)}">${html}</h${depth}>`;
};

const body = marked.parse(readFileSync(src, "utf8"), { renderer, gfm: true });

const html = `<!doctype html><html lang="ko"><head><meta charset="utf-8">
<base href="${pathToFileURL(reportDir)}/">
<style>
  @page { size: A4; margin: 15mm 14mm 16mm; }
  body { font-family: 'Noto Sans CJK KR','Noto Sans KR','Apple SD Gothic Neo',sans-serif;
         font-size: 9.3pt; line-height: 1.45; color: #1f2328; }
  h1 { font-size: 17pt; border-bottom: 2px solid #1f2328; padding-bottom: 4px; margin: 0 0 8px; }
  h2 { font-size: 13pt; border-bottom: 1px solid #d0d7de; padding-bottom: 2px; margin: 16px 0 6px; break-after: avoid; }
  h3 { font-size: 11pt; margin: 12px 0 4px; break-after: avoid; }
  h4 { font-size: 10pt; margin: 10px 0 3px; break-after: avoid; }
  p, ul, ol { margin: 4px 0; } li { margin: 1px 0; }
  table { border-collapse: collapse; margin: 6px 0; font-size: 8.4pt; break-inside: avoid; width: 100%; }
  th, td { border: 1px solid #d0d7de; padding: 2px 5px; vertical-align: top; }
  th { background: #f6f8fa; }
  code { font-family: 'DejaVu Sans Mono',monospace; font-size: 8.2pt; background: #f3f4f6; padding: 0 2px; border-radius: 3px; }
  pre { background: #f6f8fa; padding: 6px 8px; border-radius: 4px; font-size: 8pt; break-inside: avoid; white-space: pre-wrap; margin: 5px 0; }
  pre code { background: none; padding: 0; }
  pre.mermaid { background: none; text-align: center; padding: 0; }
  pre.mermaid svg { max-height: 40mm; max-width: 100%; }
  img { width: 49.5%; break-inside: avoid; }
  p:has(> img:only-child) img, p img + img { vertical-align: top; }
  blockquote { border-left: 3px solid #d0d7de; margin: 6px 0; padding: 1px 10px; color: #57606a; }
  hr { border: none; border-top: 1px solid #d0d7de; margin: 10px 0; }
  a { color: #0969da; text-decoration: none; }
</style></head><body>${body}
<script src="${pathToFileURL(mermaidJs)}"></script>
<script>
  mermaid.initialize({ startOnLoad: false, theme: "neutral",
    fontFamily: "'Noto Sans CJK KR', sans-serif", flowchart: { htmlLabels: true } });
  mermaid.run().then(() => { document.body.dataset.ready = "1"; });
</script></body></html>`;

const tmp = resolve(reportDir, ".report-print.html");
writeFileSync(tmp, html);
const browser = await chromium.launch();
const page = await browser.newPage();
await page.goto(pathToFileURL(tmp).href);
await page.waitForSelector("body[data-ready='1']", { timeout: 30000 });
await page.pdf({
  path: out, format: "A4", printBackground: true, displayHeaderFooter: true,
  headerTemplate: "<span></span>",
  footerTemplate: `<div style="font-size:8px;width:100%;text-align:center;color:#888">
    <span class="pageNumber"></span> / <span class="totalPages"></span></div>`,
  margin: { top: "15mm", bottom: "16mm", left: "14mm", right: "14mm" },
});
await browser.close();
execSync(`rm -f "${tmp}"`);
console.log(`wrote ${out}`);
