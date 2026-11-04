// app.js
// 导入API配置
import { API } from './utils/apiConfig';

App({
  onLaunch() {
    // 展示本地存储能力 - 实现日志轮转以避免存储超限
    const logs = wx.getStorageSync('logs') || []
    logs.unshift(Date.now())
    // 限制日志数量为最近100条，避免存储超限
    if (logs.length > 100) {
      logs.splice(100)
    }
    wx.setStorageSync('logs', logs)

    // 测试数据库连接 - 获取用户表信息
    this.testDatabaseConnection();

    // 登录
    wx.login({
      success: res => {
        // 发送 res.code 到后台换取 openId, sessionKey, unionId
      }
    })
  },
  
  // 测试数据库连接函数 - 通过wx.request调用后端API接口
  testDatabaseConnection() {
    console.log('开始测试数据库连接...');
    wx.request({
      url: API.BASE, // 使用配置的API地址
      method: 'GET',
      success: (res) => {
        console.log('数据库连接成功！');
        console.log('数据库查询结果:', res.data);
      },
      fail: (err) => {
        console.error('数据库连接失败:', err);
        console.log(`请确保后端服务器已启动，并且运行在${API.BASE}`);
      }
    });
  },
  globalData: {
    userInfo: null,
    isLoggedIn: false, // 添加全局登录状态标志
    deviceThresholds: {} // 存储设备阈值，格式：{device_id: {temp_threshold, soil_humidity_threshold}}
  },
  
  // 设置登录状态
  setLoginStatus(status) {
    this.globalData.isLoggedIn = status;
  },
  
  // 获取登录状态
  isLoggedIn() {
    return this.globalData.isLoggedIn;
  }
})
