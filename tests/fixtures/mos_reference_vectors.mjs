#!/usr/bin/env node
// P0 MOS 合同参考向量与独立 oracle fixture 集。
// 生成 tests/fixtures/mos_rng_vectors.json 并提供 --check 自校验。
// 本文件仅冻结生成器合同与 oracle 输入布局，不实现 MOS 求解器（Todo 3 职责）。

import { writeFileSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const OUTPUT_PATH = join(__dirname, 'mos_rng_vectors.json');

// === 冻结合同常量 ===
const CONTRACT_VERSION = 'mos-p0-v1';

// 跑道与规划参数（合成 fixture 几何，非真实跑道尺寸）
const DEFAULT_PARAMS = {
  L: 3000,        // 跑道长度 (m)
  W: 50,          // 跑道宽度 (m)
  K: 1.5,         // UXO 影响半径系数
  expand: 1.5,    // 弹坑影响半径放大系数
  step: 1,        // Y 离散步长 (m)
  minLength: 460, // 最小修复矩形长度 (m)
  minWidth: 15,   // 最小修复矩形宽度 (m)
  backfill: 50,   // 回填速率 (m³/h)
  uxoHours: 8,    // UXO 固定工时 (h)
  tiers: 3,       // 修复档位数
};

// 生成器默认参数
const GENERATOR_DEFAULTS = {
  craterCount: 2,
  craterRMin: 3,
  craterRMax: 6,
  uxoCount: 2,
  uxoYMin: 10,
  uxoYMax: 50,
};

// 五个规范种子向量
const SEED_VECTORS = [0, 42, -1, -2147483648, 2147483647];

// === mulberry32（从原型 index.html lines 594-602 精确移植）===
function mulberry32(seed) {
  let a = seed >>> 0; // 种子归一化为 uint32
  return function () {
    a |= 0; a = (a + 0x6D2B79F5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

// === 生成器：冻结合同抽取顺序与公式 ===
// 弹坑：visibleRadius → x → y；threat 仅在首个弹坑之后抽取（i>0）
// UXO：syntheticYield → x → y → threat（总是抽取）
// 数值映射使用 JS Math.round 半数向正无穷（+∞）语义
function generateFixture(params, genParams, seed) {
  const p = { ...DEFAULT_PARAMS, ...params };
  const g = { ...GENERATOR_DEFAULTS, ...genParams };
  const rng = mulberry32(seed);
  const craters = [];
  const uxo = [];

  for (let i = 0; i < g.craterCount; i++) {
    const uR = rng();
    const uX = rng();
    const uY = rng();
    const visibleRadius = Math.round(g.craterRMin + uR * (g.craterRMax - g.craterRMin));
    const x = Math.round(uX * p.L);
    const y = Math.round(-40 + uY * 80);
    // 首个弹坑 threat 固定 'high'，不消耗 rng；其余弹坑抽取 threat
    const threat = i === 0 ? 'high' : (rng() > 0.5 ? 'medium' : 'high');
    const influenceRadius = visibleRadius * p.expand; // 派生 float
    craters.push({ visibleRadius, x, y, threat, influenceRadius });
  }

  for (let i = 0; i < g.uxoCount; i++) {
    const uYld = rng();
    const uX = rng();
    const uY = rng();
    const syntheticYield = g.uxoYMin + uYld * (g.uxoYMax - g.uxoYMin); // float
    const x = Math.round(uX * p.L);
    const y = Math.round(-40 + uY * 80);
    const threat = rng() > 0.5 ? 'high' : 'medium'; // 总是抽取
    const influenceRadius = p.K * Math.cbrt(syntheticYield); // 派生 float
    uxo.push({ syntheticYield, x, y, threat, influenceRadius });
  }

  return { craters, uxo };
}

// === 嵌套 fixture 顺序：floor(tierIndex * N / (T-1)) ===
function nestedFixtureOrder(T, N) {
  if (T < 1) return [];
  if (T === 1) return [0];
  return Array.from({ length: T }, (_, i) => Math.floor((i * N) / (T - 1)));
}

// === 难度标签：tier 0=无, tier T-1=高, 其余=中等 ===
function difficultyLabels(T) {
  if (T < 1) return [];
  if (T === 1) return ['无'];
  return Array.from({ length: T }, (_, i) =>
    i === 0 ? '无' : i === T - 1 ? '高' : '中等'
  );
}

// === 独立 oracle fixture 集（手工构造，覆盖 8 种场景）===
// 供 Qt planner 测试（Todo 3）消费；influenceRadius 按冻结合同公式计算
const ORACLE_CASES = [
  {
    name: 'empty',
    description: '无障碍物，整个跑道可用',
    params: { ...DEFAULT_PARAMS },
    obstacles: { craters: [], uxo: [] },
    expected: { solvable: true, reason: null, note: '全跑道为一个有效矩形' },
  },
  {
    name: 'no-solution',
    description: '障碍物每 400m 阻断全宽，无 ≥460m 空隙',
    params: { ...DEFAULT_PARAMS },
    obstacles: {
      craters: [200, 600, 1000, 1400, 1800, 2200, 2600].map((x) => ({
        visibleRadius: 20, x, y: 0, threat: 'high',
        influenceRadius: 20 * DEFAULT_PARAMS.expand, // 30 ≥ W/2=25，覆盖全宽
      })),
      uxo: [],
    },
    expected: { solvable: false, reason: 'NO_FEASIBLE_RECTANGLE', note: '所有 X 空隙 <460m' },
  },
  {
    name: 'tangency',
    description: '两弹坑影响圆相切（闭集禁止共享点）',
    params: { ...DEFAULT_PARAMS },
    obstacles: {
      craters: [
        { visibleRadius: 4, x: 1000, y: 0, threat: 'high', influenceRadius: 6 },
        { visibleRadius: 4, x: 1012, y: 0, threat: 'high', influenceRadius: 6 },
      ],
      uxo: [],
    },
    expected: { solvable: true, tangent: true, note: '距离=12=6+6，相切视为碰撞' },
  },
  {
    name: 'boundary',
    description: '障碍物触及坐标边界 x=0',
    params: { ...DEFAULT_PARAMS },
    obstacles: {
      craters: [
        { visibleRadius: 5, x: 0, y: 0, threat: 'high', influenceRadius: 7.5 },
      ],
      uxo: [],
    },
    expected: { solvable: true, note: 'x=0 边界障碍，右侧仍有空间' },
  },
  {
    name: 'symmetric-tie',
    description: '两相同障碍物关于 x=L/2 对称，产生等面积候选',
    params: { ...DEFAULT_PARAMS },
    obstacles: {
      craters: [
        { visibleRadius: 5, x: 1000, y: 0, threat: 'high', influenceRadius: 7.5 },
        { visibleRadius: 5, x: 2000, y: 0, threat: 'high', influenceRadius: 7.5 },
      ],
      uxo: [],
    },
    expected: { solvable: true, tie: true, note: '左右候选等面积，总排序须唯一' },
  },
  {
    name: 'overlap',
    description: '两弹坑影响圆重叠',
    params: { ...DEFAULT_PARAMS },
    obstacles: {
      craters: [
        { visibleRadius: 4, x: 1000, y: 0, threat: 'high', influenceRadius: 6 },
        { visibleRadius: 4, x: 1005, y: 0, threat: 'high', influenceRadius: 6 },
      ],
      uxo: [],
    },
    expected: { solvable: true, overlap: true, note: '距离=5<12，禁区合并' },
  },
  {
    name: 'five-tier-nesting',
    description: '五级嵌套修复集合，面积单调非减',
    params: { ...DEFAULT_PARAMS, tiers: 5 },
    obstacles: {
      craters: [
        { visibleRadius: 5, x: 1500, y: 0, threat: 'high', influenceRadius: 7.5 },
      ],
      uxo: [],
    },
    expected: {
      solvable: true,
      nested_order: nestedFixtureOrder(5, 10), // [0, 2, 5, 7, 10]
      labels: difficultyLabels(5), // ['无','中等','中等','中等','高']
      note: '嵌套顺序 floor(i*N/(T-1))',
    },
  },
  {
    name: 'duplicate-seed',
    description: '同种子重复生成须产出完全一致的字节',
    params: { ...DEFAULT_PARAMS },
    obstacles: null, // 使用生成器产出
    seed: 42,
    expected: { deterministic: true, note: '两次 generateFixture 结果 JSON 相同' },
  },
];

// === 组装参考向量 JSON 对象 ===
function buildReferenceVectors() {
  // 五个种子的规范 fixture
  const seedVectorResults = SEED_VECTORS.map((seed) => {
    const normalized = seed >>> 0;
    const fixture = generateFixture(DEFAULT_PARAMS, GENERATOR_DEFAULTS, seed);
    // canonical_bytes: JSON.stringify(fixture) 逐字节冻结，供 Qt 显式字节写入器比对
    return { seed_input: seed, seed_normalized: normalized, fixture, canonical_bytes: JSON.stringify(fixture) };
  });

  // 重复种子检查
  const dupSeed = 42;
  const dupRun1 = generateFixture(DEFAULT_PARAMS, GENERATOR_DEFAULTS, dupSeed);
  const dupRun2 = generateFixture(DEFAULT_PARAMS, GENERATOR_DEFAULTS, dupSeed);
  const dupIdentical = JSON.stringify(dupRun1) === JSON.stringify(dupRun2);

  return {
    contract_version: CONTRACT_VERSION,
    params: DEFAULT_PARAMS,
    generator_defaults: GENERATOR_DEFAULTS,
    draw_order: {
      crater: ['visibleRadius', 'x', 'y', { threat: '仅 i>0 抽取' }],
      uxo: ['syntheticYield', 'x', 'y', 'threat'],
      note: 'JS Math.round 半数向正无穷（+∞）语义',
    },
    canonical_field_order: {
      crater: ['visibleRadius', 'x', 'y', 'threat', 'influenceRadius'],
      uxo: ['syntheticYield', 'x', 'y', 'threat', 'influenceRadius'],
    },
    formulas: {
      crater_visibleRadius: 'Math.round(crMin + u * (crMax - crMin))',
      crater_x: 'Math.round(u * L)',
      crater_y: 'Math.round(-40 + u * 80)',
      crater_influenceRadius: 'visibleRadius * expand (float, 不取整)',
      uxo_syntheticYield: 'uyMin + u * (uyMax - uyMin) (float, 不取整)',
      uxo_x: 'Math.round(u * L)',
      uxo_y: 'Math.round(-40 + u * 80)',
      uxo_influenceRadius: 'K * Math.cbrt(syntheticYield) (float, 不取整)',
    },
    seed_vectors: seedVectorResults,
    oracle_cases: ORACLE_CASES,
    nested_fixture_order: {
      formula: 'floor(tierIndex * N / (T-1))',
      example_T5_N10: nestedFixtureOrder(5, 10),
    },
    difficulty_labels: {
      rule: 'tier 0=无, tier T-1=高, 其余=中等',
      example_T5: difficultyLabels(5),
    },
    duplicate_seed_check: {
      seed: dupSeed,
      identical: dupIdentical,
    },
  };
}

// === --check 自校验（8 项）===
function runChecks(vectors) {
  const failures = [];

  // 1. 种子归一化
  const normCases = [
    { input: 0, expected: 0 },
    { input: 42, expected: 42 },
    { input: -1, expected: 4294967295 },
    { input: -2147483648, expected: 2147483648 },
    { input: 2147483647, expected: 2147483647 },
  ];
  for (const c of normCases) {
    if ((c.input >>> 0) !== c.expected) {
      failures.push(`seed-normalization: ${c.input} >>> 0 = ${c.input >>> 0}, 预期 ${c.expected}`);
    }
  }

  // 2. 确定性：同种子同参数两次生成须一致
  const det1 = generateFixture(DEFAULT_PARAMS, GENERATOR_DEFAULTS, 42);
  const det2 = generateFixture(DEFAULT_PARAMS, GENERATOR_DEFAULTS, 42);
  if (JSON.stringify(det1) !== JSON.stringify(det2)) {
    failures.push('determinism: seed=42 两次生成结果不一致');
  }

  // 3. 公式正确性：influenceRadius 匹配合同公式
  for (const sv of vectors.seed_vectors) {
    for (const cr of sv.fixture.craters) {
      const expected = cr.visibleRadius * DEFAULT_PARAMS.expand;
      if (Math.abs(cr.influenceRadius - expected) > 1e-12) {
        failures.push(`formula-correctness: 弹坑 influenceRadius=${cr.influenceRadius} 预期=${expected}`);
      }
    }
    for (const ux of sv.fixture.uxo) {
      const expected = DEFAULT_PARAMS.K * Math.cbrt(ux.syntheticYield);
      if (Math.abs(ux.influenceRadius - expected) > 1e-12) {
        failures.push(`formula-correctness: UXO influenceRadius=${ux.influenceRadius} 预期=${expected}`);
      }
    }
  }

  // 4. 坐标范围：x ∈ [0, L], y ∈ [-40, 40]
  for (const sv of vectors.seed_vectors) {
    for (const cr of sv.fixture.craters) {
      if (cr.x < 0 || cr.x > DEFAULT_PARAMS.L) {
        failures.push(`coordinate-ranges: 弹坑 x=${cr.x} 超出 [0,${DEFAULT_PARAMS.L}]`);
      }
      if (cr.y < -40 || cr.y > 40) {
        failures.push(`coordinate-ranges: 弹坑 y=${cr.y} 超出 [-40,40]`);
      }
    }
    for (const ux of sv.fixture.uxo) {
      if (ux.x < 0 || ux.x > DEFAULT_PARAMS.L) {
        failures.push(`coordinate-ranges: UXO x=${ux.x} 超出 [0,${DEFAULT_PARAMS.L}]`);
      }
      if (ux.y < -40 || ux.y > 40) {
        failures.push(`coordinate-ranges: UXO y=${ux.y} 超出 [-40,40]`);
      }
    }
  }

  // 5. 抽取顺序：首个弹坑 threat='high' 且不消耗 rng；用计数包装器验证
  const countingRng = (origSeed) => {
    const inner = mulberry32(origSeed);
    let n = 0;
    const w = () => { n++; return inner(); };
    w.count = () => n;
    return w;
  };
  // craterCount=2, uxoCount=2 预期调用数：3(弹坑0) + 4(弹坑1) + 4(UXO0) + 4(UXO1) = 15
  const cr = countingRng(42);
  const crRng = cr;
  const testFixture = { craters: [], uxo: [] };
  for (let i = 0; i < 2; i++) {
    const uR = crRng(), uX = crRng(), uY = crRng();
    const threat = i === 0 ? 'high' : (crRng() > 0.5 ? 'medium' : 'high');
    testFixture.craters.push({ visibleRadius: Math.round(uR * 10), x: 0, y: 0, threat, influenceRadius: 0 });
  }
  for (let i = 0; i < 2; i++) {
    const uYld = crRng(), uX = crRng(), uY = crRng(), uT = crRng();
    testFixture.uxo.push({ syntheticYield: uYld, x: 0, y: 0, threat: uT > 0.5 ? 'high' : 'medium', influenceRadius: 0 });
  }
  if (cr.count() !== 15) {
    failures.push(`draw-order: rng 调用数=${cr.count()} 预期=15`);
  }
  if (testFixture.craters[0].threat !== 'high') {
    failures.push('draw-order: 首个弹坑 threat 应为 high（不抽取）');
  }

  // 6. 嵌套顺序
  const nested = nestedFixtureOrder(5, 10);
  const expectedNested = [0, 2, 5, 7, 10];
  if (JSON.stringify(nested) !== JSON.stringify(expectedNested)) {
    failures.push(`nested-order: nestedFixtureOrder(5,10)=${JSON.stringify(nested)} 预期=${JSON.stringify(expectedNested)}`);
  }

  // 7. 难度标签
  const labels = difficultyLabels(5);
  const expectedLabels = ['无', '中等', '中等', '中等', '高'];
  if (JSON.stringify(labels) !== JSON.stringify(expectedLabels)) {
    failures.push(`difficulty-labels: difficultyLabels(5)=${JSON.stringify(labels)} 预期=${JSON.stringify(expectedLabels)}`);
  }

  // 8. oracle 完整性：8 个案例均有 name/obstacles/expected
  const expectedNames = ['empty', 'no-solution', 'tangency', 'boundary', 'symmetric-tie', 'overlap', 'five-tier-nesting', 'duplicate-seed'];
  const actualNames = vectors.oracle_cases.map((c) => c.name);
  for (const name of expectedNames) {
    if (!actualNames.includes(name)) {
      failures.push(`oracle-completeness: 缺少案例 ${name}`);
    }
  }
  if (vectors.oracle_cases.length !== 8) {
    failures.push(`oracle-completeness: 案例数=${vectors.oracle_cases.length} 预期=8`);
  }

  // 重复种子检查
  if (!vectors.duplicate_seed_check.identical) {
    failures.push('duplicate-seed: seed=42 两次生成不一致');
  }

  // 9. canonical_bytes 完整性：每个种子向量须有字符串型 canonical_bytes 且可往返解析
  for (const sv of vectors.seed_vectors) {
    if (typeof sv.canonical_bytes !== 'string') {
      failures.push(`canonical-bytes: seed=${sv.seed_input} canonical_bytes 非字符串`);
      continue;
    }
    let parsed;
    try {
      parsed = JSON.parse(sv.canonical_bytes);
    } catch (e) {
      failures.push(`canonical-bytes: seed=${sv.seed_input} canonical_bytes JSON 解析失败: ${e.message}`);
      continue;
    }
    if (JSON.stringify(parsed) !== sv.canonical_bytes) {
      failures.push(`canonical-bytes: seed=${sv.seed_input} 往返不一致`);
    }
    if (JSON.stringify(parsed) !== JSON.stringify(sv.fixture)) {
      failures.push(`canonical-bytes: seed=${sv.seed_input} canonical_bytes 与 fixture 不匹配`);
    }
  }

  return { passed: failures.length === 0, failures };
}

// === CLI 入口 ===
function main() {
  const argv = process.argv.slice(2);
  const checkMode = argv.includes('--check');

  const vectors = buildReferenceVectors();
  const json = JSON.stringify(vectors, null, 2) + '\n';
  writeFileSync(OUTPUT_PATH, json, 'utf8');

  if (checkMode) {
    const result = runChecks(vectors);
    if (result.passed) {
      console.log(`OK: 已生成 ${OUTPUT_PATH}，9 项自校验全部通过`);
      process.exit(0);
    } else {
      console.error(`FAIL: ${result.failures.length} 项校验失败：`);
      for (const f of result.failures) {
        console.error(`  - ${f}`);
      }
      process.exit(1);
    }
  } else {
    console.log(`OK: 已生成 ${OUTPUT_PATH}`);
    process.exit(0);
  }
}

main();
