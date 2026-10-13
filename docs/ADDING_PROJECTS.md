# 后续添加项目

1. 将新项目源码放入 `<分类>/<项目名>/`；例如 `embedded/new-esp32-project/` 或 `robotics/new-arm/`。
2. 保留项目 README、依赖清单、配置示例和许可证，写清用途、运行环境和入口。
3. 排除 `.git`、虚拟环境、依赖包、缓存、构建输出、安装包和模型权重。将凭据放入本地 `.env` 或其他被忽略的配置文件。
4. 更新 `docs/PROJECTS.md` 和根 README 中的目录统计。`docs/manifest.json` 记录本次归档快照；后续可以按同样字段补充文件信息。
5. 检查提交内容，再推送到仓库。

```powershell
git status --short
git add <分类>/<项目名> docs/PROJECTS.md README.md
git diff --cached --stat
git commit -m "Add new project"
git push origin main
```

GitHub 网页也可通过 **Add file → Upload files** 上传新项目目录；上传前同样检查本地凭据与生成文件。
