# 一条消息表达同一次采样的完整语义，避免多个标量话题无法时间对齐。
std_msgs/Header header
string source
float32 distance_m
float32 temperature_c

# 用常量代替代码中的“魔法数字”，发布端和订阅端共享同一含义。
uint8 STATUS_OK=0
uint8 STATUS_WARNING=1
uint8 STATUS_DANGER=2
uint8 status
