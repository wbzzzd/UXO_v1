// Playwright 截图脚本：为指定页面或全部六页生成 1920x1080 权威整体图
// 用法：
//   node screenshot.js            # 为全部六页生成截图
//   node screenshot.js situation   # 仅生成态势页截图
//   node screenshot.js detection   # 仅生成探测页截图
//   ...（decision / devices / statistics / configuration）
//
// 依赖：本脚本位于 docs/ui/prototypes/ 根目录，node_modules 也在根目录。
// 截图输出到 docs/ui/images/<page>/overview-1920x1080.png。
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

// 校验页面名合法
if (target && !allPages.includes(target)) {
  console.error('未知页面: ' + target);
  console.error('可用页面: ' + allPages.join(', '));
  process.exit(1);
}

(async () => {
  const browser = await chromium.launch();
  const context = await browser.newContext({
    viewport: { width: 1920, height: 1080 },
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
    const outFile = path.join(outDir, 'overview-1920x1080.png');
    await page.screenshot({ path: outFile, fullPage: false, type: 'png' });
    console.log('Saved: ' + outFile);
  }

  await browser.close();
})();
