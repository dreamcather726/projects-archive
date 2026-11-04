-- IoT育苗控制系统数据库表结构
-- 可直接导入MySQL数据库

-- 创建用户表
DROP TABLE IF EXISTS users;
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID',
    name VARCHAR(100) NOT NULL COMMENT '用户姓名',
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    password VARCHAR(255) NOT NULL COMMENT '密码（建议加密存储）',
    user_type ENUM('admin', 'user') DEFAULT 'user' COMMENT '用户类型：admin管理员，user普通用户',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'
) COMMENT='用户信息表';

-- 创建土壤数据表
DROP TABLE IF EXISTS soil_data;
CREATE TABLE IF NOT EXISTS soil_data (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '记录ID',
    device_id VARCHAR(50) NOT NULL COMMENT '设备ID',
    temperature DECIMAL(5,2) NOT NULL COMMENT '温度（℃）',
    humidity DECIMAL(5,2) NOT NULL COMMENT '湿度（%）',
    light DECIMAL(5,2) NOT NULL COMMENT '光照强度',
    soil_humidity DECIMAL(5,2) NOT NULL COMMENT '土壤湿度（%）',
    water_pump BOOLEAN DEFAULT FALSE COMMENT '水泵状态',
    fan BOOLEAN DEFAULT FALSE COMMENT '风扇状态',
    auto_mode BOOLEAN DEFAULT FALSE COMMENT '自动模式状态',
    collected_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '数据采集时间',
    INDEX idx_device_id (device_id),
    INDEX idx_collected_at (collected_at)
) COMMENT='土壤环境数据表';

-- 创建操作记录表
DROP TABLE IF EXISTS operation_logs;
CREATE TABLE IF NOT EXISTS operation_logs (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '记录ID',
    user_id INT NULL COMMENT '操作用户ID，NULL表示系统自动操作',
    device_id VARCHAR(50) NOT NULL COMMENT '设备ID',
    operation_type VARCHAR(50) NOT NULL COMMENT '操作类型：login, logout, toggle_pump, toggle_fan, toggle_auto, set_threshold',
    operation_result ENUM('success', 'failure') DEFAULT 'success' COMMENT '操作结果',
    error_message VARCHAR(255) NULL COMMENT '错误信息，仅在失败时填写',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '操作时间',
    INDEX idx_user_id (user_id),
    INDEX idx_device_id (device_id),
    INDEX idx_operation_time (created_at),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL
) COMMENT='用户操作记录表';

-- 创建阈值设置表
DROP TABLE IF EXISTS threshold_settings;
CREATE TABLE IF NOT EXISTS threshold_settings (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '设置ID',
    device_id VARCHAR(50) NOT NULL UNIQUE COMMENT '设备ID',
    temp_threshold DECIMAL(5,2) NOT NULL DEFAULT 30.00 COMMENT '温度上限阈值',
    soil_humidity_threshold DECIMAL(5,2) NOT NULL DEFAULT 50.00 COMMENT '土壤湿度下限阈值',
    updated_by INT COMMENT '最后更新用户ID',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_device_id (device_id),
    FOREIGN KEY (updated_by) REFERENCES users(id) ON DELETE SET NULL
) COMMENT='设备阈值设置表';

-- 插入示例用户（使用明文密码）
INSERT INTO users (name, username, password, user_type) VALUES
('系统管理员', 'admin1', '123', 'admin'),
('张经理', 'admin2', '123', 'admin'),
('李技术员', 'user1', '123', 'user'),
('王观察', 'user2', '123', 'user'),
('刘助理', 'user3', '123', 'user');

-- 插入默认阈值设置（确保device_id唯一）
INSERT INTO threshold_settings (device_id, temp_threshold, soil_humidity_threshold) VALUES
('1', 30.00, 50.00),
('2', 30.00, 50.00);


-- 插入示例土壤数据
INSERT INTO soil_data (device_id, temperature, humidity, light, soil_humidity, water_pump, fan, auto_mode, collected_at) VALUES
-- 设备1的数据
('1', 23.5, 65.0, 120, 45.0, FALSE, FALSE, TRUE, '2024-01-15 08:00:00'),
('1', 24.1, 63.5, 135, 43.0, FALSE, TRUE, TRUE, '2024-01-15 08:10:00'),
('1', 25.3, 62.0, 150, 41.0, TRUE, TRUE, TRUE, '2024-01-15 08:20:00'),
('1', 26.0, 60.5, 165, 46.0, FALSE, TRUE, TRUE, '2024-01-15 08:30:00'),
('1', 26.5, 59.0, 180, 44.0, FALSE, TRUE, TRUE, '2024-01-15 08:40:00'),
-- 设备2的数据
('2', 22.8, 68.0, 110, 48.0, FALSE, FALSE, FALSE, '2024-01-15 08:00:00'),
('2', 23.4, 66.5, 125, 46.0, FALSE, FALSE, FALSE, '2024-01-15 08:10:00'),
('2', 24.6, 65.0, 140, 43.0, TRUE, FALSE, FALSE, '2024-01-15 08:20:00'),
('2', 25.2, 63.5, 155, 47.0, FALSE, FALSE, FALSE, '2024-01-15 08:30:00'),
('2', 25.8, 62.0, 170, 45.0, FALSE, TRUE, FALSE, '2024-01-15 08:40:00'),
-- 最近的数据
('1', 27.0, 58.0, 200, 42.0, TRUE, TRUE, TRUE, NOW() - INTERVAL 5 MINUTE),
('2', 26.3, 61.0, 185, 44.0, FALSE, TRUE, FALSE, NOW() - INTERVAL 5 MINUTE);

-- 插入示例操作记录
INSERT INTO operation_logs (user_id, device_id, operation_type, operation_result, created_at) VALUES
-- 用户手动操作
(1, '1', 'toggle_auto', 'success', '2024-01-15 07:30:00'),
(1, '1', 'set_threshold', 'success', '2024-01-15 07:35:00'),
(3, '2', 'toggle_fan', 'success', '2024-01-15 08:15:00'),
(1, '1', 'login', 'success', '2024-01-15 07:25:00'),
(3, '2', 'login', 'success', '2024-01-15 08:00:00'),
-- 系统自动操作（user_id为NULL表示系统自动执行）
(NULL, '1', 'toggle_pump', 'success', '2024-01-15 08:20:00'),
(NULL, '1', 'toggle_fan', 'success', '2024-01-15 08:10:00'),
(NULL, '2', 'toggle_pump', 'success', '2024-01-15 08:20:00'),
-- 最近的操作记录
(1, '1', 'toggle_pump', 'success', NOW() - INTERVAL 10 MINUTE),
(3, '2', 'toggle_auto', 'success', NOW() - INTERVAL 15 MINUTE);

-- 创建操作记录用户信息视图
CREATE VIEW operation_user_info_view AS
SELECT 
    u.name  ,
    ol.device_id  ,
    ol.operation_type   ,
    ol.operation_result  ,
    ol.created_at
FROM operation_logs ol
LEFT JOIN users u ON ol.user_id = u.id
ORDER BY ol.created_at DESC;