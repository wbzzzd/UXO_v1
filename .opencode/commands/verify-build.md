---
description: 运行当前 Qt/CMake 构建验证
agent: build
---

请验证 UXO_v1 当前客户端构建。

执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target UXOMissionControl -j2
```

输出：

- 配置是否成功。
- 构建是否成功。
- 若失败，列出首个关键错误和可能关联文件。

注意：

- 不要运行交互式 `scripts/build.sh` 作为首选验证。
- 不要修改文件，除非用户明确要求修复。
