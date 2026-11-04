# Libraries 功能库

本目录用于存放 IMU、OLED、编码器等可独立复用的功能库。

每个库使用单独的文件夹，并将头文件和源文件分开：

```text
Libraries/
├── IMU/
│   ├── Inc/
│   └── Src/
└── OLED/
    ├── Inc/
    └── Src/
```

新增库后，还需要在 Keil 工程中添加源文件，并把对应的 `Inc` 路径加入头文件搜索路径。
