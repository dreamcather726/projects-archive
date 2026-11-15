# 近似项目精简

整理日期：2026-11-15。保留一份完整代表源码，旧版差异保存在 Git 历史中，不再占用独立的源码目录。

这次归并 13 个项目目录，另外删除 270 个字节完全一致的内部副本。仅共享开发模板、但业务实现不同的 PlatformIO 项目继续保留。

| 旧目录 | 保留版本 | 旧文件数 | 在保留版本中找到相同内容 |
| --- | --- | ---: | ---: |
| [embedded/xiaozhi-esp32-main](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/embedded/xiaozhi-esp32-main) | [embedded/xiaozhi-latest](../embedded/xiaozhi-latest/) | 1561 | 1170 |
| [embedded/xiaozhi-esp32-main---副本](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/embedded/xiaozhi-esp32-main---%E5%89%AF%E6%9C%AC) | [embedded/xiaozhi-latest](../embedded/xiaozhi-latest/) | 1561 | 1171 |
| [embedded/xiaozhi-esp32-main-d-drive](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/embedded/xiaozhi-esp32-main-d-drive) | [embedded/xiaozhi-latest](../embedded/xiaozhi-latest/) | 1454 | 1078 |
| [vision/HandTracking-4指-d-drive](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/vision/HandTracking-4%E6%8C%87-d-drive) | [vision/HandTracking-4指](../vision/HandTracking-4%E6%8C%87/) | 13 | 11 |
| [robotics/yahboomcar_ws](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/robotics/yahboomcar_ws) | [robotics/yahboom-rdk](../robotics/yahboom-rdk/) | 140 | 140 |
| [robotics/ROBO](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/robotics/ROBO) | [applications/robonexus](../applications/robonexus/) | 50 | 49 |
| [embedded/bugrtoscar](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/embedded/bugrtoscar) | [projects/01-AegisMotion-RTOS](../projects/01-AegisMotion-RTOS/) | 158 | 151 |
| [ai/voice-assistant-scripts](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/ai/voice-assistant-scripts) | [ai/voice_assistant](../ai/voice_assistant/) | 7 | 7 |
| [ai/spacemit_asr](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/ai/spacemit_asr) | [ai/voice_assistant](../ai/voice_assistant/) | 10 | 10 |
| [ai/spacemit_audio](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/ai/spacemit_audio) | [ai/voice_assistant](../ai/voice_assistant/) | 3 | 3 |
| [ai/spacemit_llm](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/ai/spacemit_llm) | [ai/voice_assistant](../ai/voice_assistant/) | 4 | 4 |
| [ai/spacemit_tts](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/ai/spacemit_tts) | [ai/voice_assistant](../ai/voice_assistant/) | 65 | 65 |
| [ai/3-3-课程作业资料](https://github.com/dreamcather726/projects-archive/tree/56e278b39dec2f11e247092db688c384f63b1314/ai/3-3-%E8%AF%BE%E7%A8%8B%E4%BD%9C%E4%B8%9A%E8%B5%84%E6%96%99) | [ai/NLP](../ai/NLP/) | 90 | 90 |

270 个内部副本包括：`ai/voice_assistant` 中 180 个带 `(1)`、`(2)` 的文件，以及 `ai/NLP/NLP/` 中 90 个嵌套文件。均按 Git blob 校验一致，保留未编号文件和外层完整模块。

## 差异与恢复

近似项目不是字节完全相同的项目。旧版的硬件配置、接口或实验实现可能与保留版本不同；本次选择代表版本，不将这些差异混入新版本。全部独有文件名、旧目录树 ID、原始来源和脱敏记录见 [机器可读记录](consolidation-history.json)。

精简前提交：`56e278b39dec2f11e247092db688c384f63b1314`。点击上表旧目录可浏览原始内容。需要旧版本时，可以导出到单独的本地目录：

```powershell
git archive --format=zip --output=old-project.zip 56e278b39dec2f11e247092db688c384f63b1314 embedded/xiaozhi-esp32-main
```

本次仅调整归档，不编译或运行硬件项目。其他未提交的课程删除和新增课程文件保留在工作区，未纳入本次提交。

## Snapshot metadata validation

All 5,090 retained manifest entries were checked against the committed Git blobs. 242 pre-existing AI Team Manager entries described imported source bytes rather than committed snapshot bytes; their hashes and sizes now describe the snapshot. Original values are retained in `consolidation-history.json`. No AI Team Manager source files were modified.
