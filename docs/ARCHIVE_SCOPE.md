# 整理范围

扫描了 C 盘用户的桌面、PlatformIO、Arduino、OpenMV 和已识别的开发目录，以及 D 盘 work、大学、下载、百度网盘下载与同步目录。系统、应用安装目录不作为用户项目收录。

收录源码、构建入口、依赖声明、文本说明、选定图片/音频资源、机器人描述和小型 CAD 文件。保持每个项目内的相对目录关系；存在差异的同名项目保留为独立版本。

不收录嵌套 Git 历史、虚拟环境、node_modules、SDK 安装目录、构建缓存、生成程序、压缩包、磁盘镜像、模型权重、视频和办公文档。单个文件大于 20 MiB 时也不收录。未纳入的本地资料仍在原位置，可按需求单独追加。

未修改的上游工具仓库仅记录来源，不重复导入：

- LeRobot：<https://github.com/huggingface/lerobot>
- ROS 2 Kortex：<https://github.com/Kinovarobotics/ros2_kortex>
- SO-ARM100：<https://github.com/TheRobotStudio/SO-ARM100>

桌面 LeLamp、microduck 工程按参考源码快照导入；下载目录中的厂家 SDK/教程包未整包导入。参考示例目录和项目内第三方文件保留原许可证；本仓库不声称所有收录代码均为个人原创。

## 检查记录

校验了入选文件的 SHA-256，除凭据占位符替换外均与来源文件一致。运行了 Gitleaks 和针对字面量连接配置的检查。未运行项目源码、下载依赖或编译硬件工程。本次提交为筛选后的当前文件快照，不导入原仓库历史。
