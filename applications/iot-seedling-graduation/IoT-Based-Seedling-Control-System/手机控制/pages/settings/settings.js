// settings.js
// 导入iotapi模块以使用其中的连接信息
const iotapi = require('../../utils/iotapi');
// 导入API配置
const { API } = require('../../utils/apiConfig');

Page({
  data: {
    // 设备1阈值设置
    device1TempThreshold: 0,  // 设备1温度阈值
    device1HumThreshold: 0,   // 设备1土壤湿度阈值
    // 设备2阈值设置
    device2TempThreshold: 0,  // 设备2温度阈值
    device2HumThreshold: 0,   // 设备2土壤湿度阈值
    tokenStatus: '', // token获取状态
    token: '', // 获取到的token
    userType: 'user' // 默认用户类型
  },

  onLoad() {
    // 获取用户信息和用户类型
    this.getUserInfo();
    
    // 检查用户权限
    if (!this.checkAdminPermission()) {
      // 普通用户只能查看，禁用输入框
      this.setData({
        isReadOnly: true
      });
    }
    
    // 页面加载时从本地存储获取保存的阈值设置
    this.loadSavedSettings();
  },
  
  // 获取用户信息
  getUserInfo() {
    const userInfo = wx.getStorageSync('userInfo');
    if (userInfo) {
      this.setData({
        userInfo: userInfo,
        userType: userInfo.userType || 'user'
      });
    }
  },
  
  // 检查管理员权限
  checkAdminPermission() {
    return this.data.userType === 'admin';
  },

  // 从全局变量加载设备阈值
  loadSavedSettings() {
    const app = getApp();
    const deviceThresholds = app.globalData.deviceThresholds || {};
    
    // 从数据库中获取到阈值
    wx.request({
      url: API.THRESHOLDS, // 使用配置的API地址
      method: 'GET',
      success: (res) => {
        console.log('数据库连接成功！');
        console.log('数据库查询结果:', res.data);
        // 解析数据库返回的阈值数据
        const thresholds = res.data.thresholds || res.data || {};
       
        this.setData({
          device1TempThreshold: thresholds['1'].temp_threshold  ,// 设备1温度阈值
          device1HumThreshold: thresholds['1'].soil_humidity_threshold  ,// 设备1土壤湿度阈值
          device2TempThreshold: thresholds['2'].temp_threshold  ,// 设备2温度阈值
          device2HumThreshold: thresholds['2'].soil_humidity_threshold  // 设备2土壤湿度阈值
        });
        // 同时更新全局变量
        const app = getApp();
        app.globalData.deviceThresholds = thresholds;
      },
      fail: (err) => {
        console.error('数据库连接失败:', err);
        console.log(`请确保后端服务器已启动，并且运行在${API.BASE}`);
      }
    });
  },
  // 返回上一页
  goBack() {
    wx.navigateBack();
  },

  // 设备1温度阈值变化处理
  onDevice1TempChange(e) {
    const value = e.detail.value;
    // 确保输入是有效的数字
    if (value === '' || !isNaN(value)) {
      this.setData({
        device1TempThreshold: value === '' ? '' : Number(value)
      });
    }
  },

  // 设备1土壤湿度阈值变化处理
  onDevice1HumChange(e) {
    const value = e.detail.value;
    // 确保输入是有效的数字
    if (value === '' || !isNaN(value)) {
      this.setData({
        device1HumThreshold: value === '' ? '' : Number(value)
      });
    }
  },
  
  // 设备2温度阈值变化处理
  onDevice2TempChange(e) {
    const value = e.detail.value;
    // 确保输入是有效的数字
    if (value === '' || !isNaN(value)) {
      this.setData({
        device2TempThreshold: value === '' ? '' : Number(value)
      });
    }
  },

  // 设备2土壤湿度阈值变化处理
  onDevice2HumChange(e) {
    const value = e.detail.value;
    // 确保输入是有效的数字
    if (value === '' || !isNaN(value)) {
      this.setData({
        device2HumThreshold: value === '' ? '' : Number(value)
      });
    }
  },

  // 保存设置
  async saveSettings() {
    // 检查用户权限
    if (!this.checkAdminPermission()) {
      wx.showToast({
        title: '普通用户无权限修改设置',
        icon: 'none',
        duration: 2000
      });
      return;
    }
    
    const { device1TempThreshold, device1HumThreshold, device2TempThreshold, device2HumThreshold } = this.data;
    
    // 验证输入
    const allValues = [device1TempThreshold, device1HumThreshold, device2TempThreshold, device2HumThreshold];
    if (allValues.some(val => val === '')) {
      wx.showToast({
        title: '请填写所有阈值设置',
        icon: 'none'
      });
      return;
    }
    
    // 验证范围
    const allNumbers = allValues.map(val => Number(val));
    if (allNumbers.some(val => val < 0 || val > 100)) {
      wx.showToast({
        title: '阈值超出合理范围(0-100)',
        icon: 'none'
      });
      return;
    }
    
    // 不再需要保存到本地存储，现在从数据库获取阈值

    // 尝试发送阈值设置到设备
    try {
      // 显示加载提示
      wx.showLoading({
        title: '发送阈值设置中...',
      });
      
      // 设备ID映射
      const deviceIds = {
        device1: '68ee03050a9ab42ae5847f9e_text2',  // 设备1 ID
        device2: '68ee03050a9ab42ae5847f9e_test'   // 设备2 ID
      };
      const serviceId = 'DataGet'; // 服务ID
      const command = {}; // 空命令对象
      
      // 先发送设备1的阈值设置
      await iotapi.sendthreshold(
        deviceIds.device1, 
        serviceId, 
        command, 
        Number(device1TempThreshold), 
        Number(device1HumThreshold)
      );
      // 加入 500ms 延时，避免并发请求导致设备响应异常
      await new Promise(resolve => setTimeout(resolve, 500));
      
      // 再发送设备2的阈值设置
      await iotapi.sendthreshold(
        deviceIds.device2, 
        serviceId, 
        command, 
        Number(device2TempThreshold), 
        Number(device2HumThreshold)
      );
      
      // 记录设置阈值操作日志
      this.setting_threshold_logs();
      
      // 将阈值保存到数据库中
      this.setting_threshold();
      
      wx.hideLoading();
      wx.showToast({
        title: '设置已保存并发送',
        icon: 'success'
      });
    } catch (error) {
      wx.hideLoading();
      // 发送失败，错误信息已在catch处理
      // 记录设置阈值操作日志（失败情况）
      this.setting_threshold_logs('本地设置已保存，但发送到设备失败');
      
      // 即使发送失败，也将本地保存的阈值保存到数据库中
      this.setting_threshold();
      
      wx.showToast({
        title: '本地设置已保存，但发送到设备失败',
        icon: 'none',
        duration: 2000
      });
    }
    
    // 保存成功后返回上一页
    setTimeout(() => {
      this.goBack();
    }, 1500);
  },
  //设置阈值
  setting_threshold_logs(errorMessage = null) {
    // 获取用户信息
    const userInfo = wx.getStorageSync('userInfo');
    const id = userInfo ? userInfo.userid : null;
    
    // 获取阈值设置信息
    const { device1TempThreshold, device1HumThreshold, device2TempThreshold, device2HumThreshold } = this.data;
    
    // 构建阈值信息字符串，放到operation_result字段中
    const thresholdInfo = `device1:temp_threshold=${device1TempThreshold},hum_threshold=${device1HumThreshold};device2:temp_threshold=${device2TempThreshold},hum_threshold=${device2HumThreshold}`;
    
    // 准备发送到后端的数据
    const requestData = {
      userid: id,
      operation_type: 'set_threshold', // 设置阈值操作类型
      device_id: 'null', // 默认设备ID，因为这里同时设置了多个设备的阈值
      operation_result: errorMessage ? 'failure' : thresholdInfo, // 成功时存储阈值信息，失败时为failure
      error_message: errorMessage || null
    };
    
    wx.request({
      url: API.LOGIN_RECORD, // 使用配置的API地址
      method: 'POST',
      data: requestData,
      success: (res) => {
        // 只保留成功信息，移除详细日志
      },
      fail: (err) => {
        // 只保留失败信息，移除详细错误日志
      }
    });
  },

  // 将阈值设置保存到数据库中并更新全局变量
  setting_threshold(){
    // 获取用户信息和用户ID
    const userInfo = wx.getStorageSync('userInfo');
    const userId = userInfo ? userInfo.userid : null;
    
    if (!userId) {
      wx.showToast({
        title: '无法获取用户信息',
        icon: 'none',
        duration: 2000
      });
      return;
    }
    
    // 获取当前设置的阈值数据
    const { 
      device1TempThreshold, 
      device1HumThreshold, 
      device2TempThreshold, 
      device2HumThreshold 
    } = this.data;
    
    // 准备要保存的数据数组（包含设备1和设备2）
    const thresholdData = [
      {
        device_id: '1',
        temp_threshold: Number(device1TempThreshold),
        soil_humidity_threshold: Number(device1HumThreshold),
        updated_by: userId
      },
      {
        device_id: '2', 
        temp_threshold: Number(device2TempThreshold),
        soil_humidity_threshold: Number(device2HumThreshold),
        updated_by: userId
      }
    ];
    
    // 保存设备1的阈值数据
    this.saveThresholdToDatabase(thresholdData[0]);
    
    // 保存设备2的阈值数据
    this.saveThresholdToDatabase(thresholdData[1]);
  },
  
  // 向数据库保存单个设备的阈值数据并更新全局变量
  saveThresholdToDatabase(data) {
    wx.request({
      url: API.UPDATE_THRESHOLD, // 使用配置的API地址
      method: 'POST',
      data: data,
      success: (res) => {
        // 可以在这里记录成功信息，但不显示给用户
        console.log(res.data);
        console.log(`设备${data.device_id}阈值保存成功`);
        
        // 更新全局变量中的阈值信息
        const app = getApp();
        if (!app.globalData.deviceThresholds) {
          app.globalData.deviceThresholds = {};
        }
        app.globalData.deviceThresholds[data.device_id] = {
          temp_threshold: data.temp_threshold,
          soil_humidity_threshold: data.soil_humidity_threshold
        };
        console.log('全局变量中的阈值已更新:', app.globalData.deviceThresholds);
      },
      fail: (err) => {
        console.error(`设备${data.device_id}阈值保存失败:`, err);
        // 这里可以选择是否显示错误信息给用户
      }
    });
  },

  // 获取token的方法
  async getToken() {
    try {
      this.setData({
        tokenStatus: '获取中...'
      });
      
      // 调用iotapi中的getToken方法
      const token = await iotapi.getToken();
      
      this.setData({
        tokenStatus: '获取成功',
        token: token.substring(0, 20) + '...' // 只显示部分token，保护安全
      });
      
      wx.showToast({
        title: 'Token获取成功',
        icon: 'success'
      });
    } catch (error) {
      // Token获取失败，已在UI中显示错误信息
      this.setData({
        tokenStatus: '获取失败',
        token: ''
      });
      
      wx.showToast({
        title: 'Token获取失败',
        icon: 'none'
      });
    }
  }
})