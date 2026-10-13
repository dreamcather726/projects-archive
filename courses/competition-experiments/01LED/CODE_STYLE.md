# 课堂代码风格

本项目后续示例统一遵守以下规则：

1. 一个示例只讲一个主要知识点。
2. 只保留 `setup()`、`loop()` 和确有必要的简单函数。
3. 引脚统一写在代码最上方，用 `const int` 定义。
4. 使用容易口头解释的英文变量名。
5. 不为了缩短代码而把多条语句挤在一行。
6. 循环变化写完整，例如 `i = i + 5`，便于初学者理解。
7. 注释解释“为什么”，不重复代码表面含义。
8. 优先使用 Arduino 通用函数，如 `pinMode()`、`digitalWrite()`、
   `digitalRead()`、`analogWrite()` 和 `delay()`。
9. 暂不引入类、指针、宏技巧、状态机等进阶写法。
10. 代码应当能由学生在课堂上逐行手敲完成。

## LEDC 舵机模板

后续编写舵机控制代码时，默认采用以下写法：

1. 使用 `#define` 分别定义舵机引脚和 LEDC 通道。
2. 使用 `ledcAttachChannel()` 指定引脚、频率、精度和通道。
3. 使用 `ledcWriteChannel()` 按通道输出占空比。
4. 舵机频率使用 `50Hz`，PWM 精度使用 `14` 位。
5. 舵机角度限制在 `0～180` 度。
6. 使用 `map(angle, 0, 180, 410, 2048)` 把角度转换为占空比。
7. 舵机转到 90 度时，占空比使用 `1229`。
8. 多个舵机分别使用独立的 LEDC 通道。
9. 串口命令保持简短，例如 `b90`、`a90`、`g90`。

标准接口形式：

```cpp
ledcAttachChannel(PIN, 50, 14, CHANNEL);
ledcWriteChannel(CHANNEL, duty);
```

除非课程内容另有要求，不把这套写法替换成 `analogWrite()`、
`ledcAttach()` 或旧版的 `ledcSetup()`、`ledcAttachPin()`。
