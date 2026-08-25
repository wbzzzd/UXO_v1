// Playwright 截图脚本：为指定页面或全部六页生成指定视口的权威整体图
// 用法：
//   node screenshot.js                       # 全部六页，默认视口 1920x1080
//   node screenshot.js situation             # 仅态势页，1920x1080
//   node screenshot.js situation 1280x720    # 仅态势页，1280x720
//   node screenshot.js situation 3840x2160   # 仅态势页，3840x2160
//   ...（detection / decision / devices / statistics / configuration）
//
// 依赖：本脚本位于 docs/ui/prototypes/ 根目录，node_modules 也在根目录。
// 截图输出到 docs/ui/images/<page>/overview-<宽x高>.png（如 overview-1920x1080.png）。
const { chromium } = require('playwright');
const path = require('path');
const fs = require('fs');

// 六页固定顺序，与导航栏顺序一致
const allPages = [
  'situation',    // 态势
  'detection',    // 探测
  'decision',     // 决策
  'devices',      // 设备
  'statistics',   // 统计
  'configuration' // 配置
];

const target = process.argv[2];
const pages = target ? [target] : allPages;

// 视口参数（可选，默认 1920x1080），格式：宽x高，如 1280x720
const viewportArg = process.argv[3] || '1920x1080';

// 校验页面名合法
if (target && !allPages.includes(target)) {
  console.error('未知页面: ' + target);
  console.error('可用页面: ' + allPages.join(', '));
  process.exit(1);
}

// 校验视口格式合法（纯数字 宽x高）
const viewportMatch = /^(\d+)x(\d+)$/.exec(viewportArg);
if (!viewportMatch) {
  console.error('未知视口: ' + viewportArg);
  console.error('格式应为 宽x高，例如 1280x720、1920x1080、3840x2160');
  process.exit(1);
}
const viewport = { width: parseInt(viewportMatch[1], 10), height: parseInt(viewportMatch[2], 10) };

(async () => {
  const browser = await chromium.launch();
  const context = await browser.newContext({
    viewport,
    deviceScaleFactor: 1,
  });
  const page = await context.newPage();

  for (const p of pages) {
    const htmlPath = path.resolve(__dirname, p, 'index.html');
    if (!fs.existsSync(htmlPath)) {
      console.error('SKIP (missing): ' + htmlPath);
      continue;
    }
    await page.goto('file://' + htmlPath, { waitUntil: 'networkidle' });
    // 等待字体和渲染稳定
    await page.waitForTimeout(500);
    const outDir = path.resolve(__dirname, '..', 'images', p);
    fs.mkdirSync(outDir, { recursive: true });
    const outFile = path.join(outDir, 'overview-' + viewportArg + '.png');
    await page.screenshot({ path: outFile, fullPage: false, type: 'png' });
    console.log('Saved: ' + outFile);
  }

  await browser.close();
})();
