# 测试临时工程清理

权限单元测试会在 `.tools/permissions-unit-<UUID>/` 中生成独立测试工程。
测试结束后可以清理这些可再生成目录：

```powershell
node scripts/clean-test-artifacts.cjs
node scripts/clean-test-artifacts.cjs --apply
```

第一条只显示数量，第二条执行清理。工具只处理仓库 `.tools/` 下名称严格匹配的普通目录，跳过符号链接。

保留 `test/` 回归测试、验收报告、历史成果备份、真实模型检查点、浏览器依赖及其他临时目录，避免删除尚未交付的任务成果。运行测试时不要同时执行清理。

本次作品集使用已提交的 0.8.3 源码快照；本地 0.8.4 改动和源码目录迁移尚未提交，未纳入快照。
