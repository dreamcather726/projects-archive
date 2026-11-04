// index.js
// 导入IoT API
const iotapi = require('../../utils/iotapi.js');
// 导入API配置
const { API } = require('../../utils/apiConfig');

Page({
  data: {
    // 设备1状态
    device1Pump: false,
    device1Fan: false,
    autoMode1: false,
    
    // 设备2状态
    device2Pump: false,
    device2Fan: false,
    autoMode2: false,
    
    // 更新时间
    lastUpdateTime: '',
    
    // 设备数据数组
    deviceData: [
      {
        id: 1,
        temperature: 23,
        humidity: 65,
        light: 50,
        soilHumidity: 45,
        waterPump: false,
        fan: false,
        autoMode: false
      },
      {
        id: 2,
        temperature: 25,
        humidity: 60,
        light: 55,
        soilHumidity: 40,
        waterPump: false,
        fan: false,
        autoMode: false
      }
    ],
    
    // 设备ID列表
    deviceIds: ['68ee03050a9ab42ae5847f9e_test', '68ee03050a9ab42ae5847f9e_text2'],
    
    // 华为云IoT平台配置
    endpoint: 'cn-east-3.iotda-app.myhuaweicloud.com',
    projectId: 'e703e7c456ff41379cb07e5416c765f4',
    
    // 加载状态
    isLoading: true
  },
  
  onLoad() {
    // 页面加载时的初始化逻辑
    // console.log('智能灌溉系统页面加载')
    
    // 显示加载框
    wx.showLoading({
      title: '加载中...',
    });
    
    // 获取用户信息和用户类型
    this.getUserInfo();
    
    // 设置初始时间
    this.updateTime()
    
    // 测试获取token功能
    this.testToken();
    
    // 获取设备影子数据
    this.fetchDeviceData();
    // 设置定时器，每隔5秒刷新一次数据
    this.dataRefreshTimer = setInterval(() => {
      this.fetchDeviceData();
    }, 5000);

    // 设置定时器，2秒后隐藏加载框
    setTimeout(() => {
      wx.hideLoading();
      this.setData({
        isLoading: false
      });
    }, 5000);
    
    
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
  
  onUnload() {
    // console.log('页面卸载，清理资源')
  },
  
  /**
   * 测试获取token
   */
  async testToken() {

    try {
    
      // 直接调用getToken函数进行测试
      const token = await iotapi.getToken();
    
    } catch (error) {
      console.error('Token测试失败:', error);
    }
  },
  
  onUnload() {
    // 页面卸载时清理资源
    // console.log('页面卸载，清理资源')
    // 清除定时器
    if (this.dataRefreshTimer) {
      clearInterval(this.dataRefreshTimer);
    }
  },
  
  /**
   * 获取设备数据
   */
  async fetchDeviceData() {
    // 检查登录状态，如果未登录则不获取数据
    const app = getApp();
    if (app && typeof app.isLoggedIn === 'function' && !app.isLoggedIn()) {
      console.log('用户未登录，不获取设备数据');
      return;
    }
    
    try {
      // console.log('开始获取设备影子数据...');
      // 调用getAllDevicesData获取所有设备数据
      const devicesData = await iotapi.getAllDevicesData(this.data.deviceIds);
      
      // 如果返回的数据为空数组，说明是因为未登录而返回的
      if (!devicesData || devicesData.length === 0) {
        console.log('未获取到设备数据');
        return;
      }
        
      // console.log('设备影子数据获取成功:', devicesData);
      
      // 更新UI显示
      if (Array.isArray(devicesData)) {
        // 更新设备1数据
        if (devicesData[0]) {
          // 确保只更新有实际值的字段，不使用默认数据
          const device1Updates = {};
          
          // 只有当waterPump不是null或undefined时才更新
          if (devicesData[0].waterPump !== undefined) {
            device1Updates.device1Pump = devicesData[0].waterPump;
          }
          
          if (devicesData[0].fan !== undefined) {
            device1Updates.device1Fan = devicesData[0].fan;
          }
          
          if (devicesData[0].autoMode !== undefined) {
            device1Updates.autoMode1 = devicesData[0].autoMode;
          }
          
          // 更新deviceData数组中的设备1数据
          const device1DataUpdate = {
            ...this.data.deviceData[0],
            deviceId: devicesData[0].deviceId
          };
          
          // 只更新有实际值的传感器数据
          if (devicesData[0].temperature !== undefined) {
            device1DataUpdate.temperature = devicesData[0].temperature;
          }
          
          if (devicesData[0].humidity !== undefined) {
            device1DataUpdate.humidity = devicesData[0].humidity;
          }
          
          if (devicesData[0].light !== undefined) {
            device1DataUpdate.light = devicesData[0].light;
          }
          
          if (devicesData[0].soilHumidity !== undefined) {
            device1DataUpdate.soilHumidity = devicesData[0].soilHumidity;
          }
          
          if (devicesData[0].waterPump !== undefined) {
            device1DataUpdate.waterPump = devicesData[0].waterPump;
          }
          
          if (devicesData[0].fan !== undefined) {
            device1DataUpdate.fan = devicesData[0].fan;
          }
          
          if (devicesData[0].autoMode !== undefined) {
            device1DataUpdate.autoMode = devicesData[0].autoMode;
          }
          
          // 添加错误状态
          device1DataUpdate.error = devicesData[0].error || false;
          if (devicesData[0].errorMessage) {
            device1DataUpdate.errorMessage = devicesData[0].errorMessage;
          }
          
          device1Updates['deviceData[0]'] = device1DataUpdate;
          
          // 只有当有更新时才调用setData
          if (Object.keys(device1Updates).length > 0) {
            this.setData(device1Updates);
          }
          
          // 如果设备1有错误，显示提示
          if (devicesData[0].error) {
            console.warn('设备1数据获取异常:', devicesData[0].errorMessage);
          }
        }
        
        // 更新设备2数据，逻辑与设备1相同
        if (devicesData[1]) {
          const device2Updates = {};
          
          if (devicesData[1].waterPump !== undefined) {
            device2Updates.device2Pump = devicesData[1].waterPump;
          }
          
          if (devicesData[1].fan !== undefined) {
            device2Updates.device2Fan = devicesData[1].fan;
          }
          
          if (devicesData[1].autoMode !== undefined) {
            device2Updates.autoMode2 = devicesData[1].autoMode;
          }
          
          const device2DataUpdate = {
            ...this.data.deviceData[1],
            deviceId: devicesData[1].deviceId
          };
          
          if (devicesData[1].temperature !== undefined) {
            device2DataUpdate.temperature = devicesData[1].temperature;
          }
          
          if (devicesData[1].humidity !== undefined) {
            device2DataUpdate.humidity = devicesData[1].humidity;
          }
          
          if (devicesData[1].light !== undefined) {
            device2DataUpdate.light = devicesData[1].light;
          }
          
          if (devicesData[1].soilHumidity !== undefined) {
            device2DataUpdate.soilHumidity = devicesData[1].soilHumidity;
          }
          
          if (devicesData[1].waterPump !== undefined) {
            device2DataUpdate.waterPump = devicesData[1].waterPump;
          }
          
          if (devicesData[1].fan !== undefined) {
            device2DataUpdate.fan = devicesData[1].fan;
          }
          
          if (devicesData[1].autoMode !== undefined) {
            device2DataUpdate.autoMode = devicesData[1].autoMode;
          }
          
          device2DataUpdate.error = devicesData[1].error || false;
          if (devicesData[1].errorMessage) {
            device2DataUpdate.errorMessage = devicesData[1].errorMessage;
          }
          
          device2Updates['deviceData[1]'] = device2DataUpdate;
          
          if (Object.keys(device2Updates).length > 0) {
            this.setData(device2Updates);
          }
          
          if (devicesData[1].error) {
            console.warn('设备2数据获取异常:', devicesData[1].errorMessage);
          }
        }
      }
      
      this.uploadSensorDataToDatabase(devicesData);

      // 更新时间显示
      this.updateTime();
      
    } catch (error) {
      console.error('获取设备影子数据失败:', error);
      wx.showToast({
        title: '数据获取异常',
        icon: 'none'
      });
    } finally {
      // 无论成功失败，都在300ms后隐藏加载框，确保用户能看到加载状态
      setTimeout(() => {
        wx.hideLoading();
      }, 300);
    }
  },

  uploadSensorDataToDatabase(devicesData) {
    if (!Array.isArray(devicesData) || devicesData.length === 0) return;

    const app = getApp();
    if (app && typeof app.isLoggedIn === 'function' && !app.isLoggedIn()) {
      return;
    }

    if (!this._lastSensorUploadAt) {
      this._lastSensorUploadAt = {};
    }

    const userInfo = wx.getStorageSync('userInfo') || {};
    const userId = userInfo.userid || null;
    const now = Date.now();
    const uploadIntervalMs = 10000;

    devicesData.forEach((d, idx) => {
      if (!d || d.error) return;
      if (!d.deviceId) return;

      const hasAnySensor =
        d.temperature !== null && d.temperature !== undefined ||
        d.humidity !== null && d.humidity !== undefined ||
        d.light !== null && d.light !== undefined ||
        d.soilHumidity !== null && d.soilHumidity !== undefined;
      if (!hasAnySensor) return;

      const key = String(d.deviceId);
      const lastAt = this._lastSensorUploadAt[key] || 0;
      if (now - lastAt < uploadIntervalMs) return;
      this._lastSensorUploadAt[key] = now;

      const payload = {
        device_id: d.id || (idx + 1),
        temperature: d.temperature,
        humidity: d.humidity,
        light: d.light,
        soil_humidity: d.soilHumidity,
        water_pump: d.waterPump,
        fan: d.fan,
        auto_mode: d.autoMode,
        user_id: userId
      };

      wx.request({
        url: API.SENSOR_DATA,
        method: 'POST',
        data: payload,
        success: () => {},
        fail: (err) => {
          console.error('上传传感器数据失败:', err);
        }
      });
    });
  },
  
  // 更新时间显示
  updateTime() {
    const now = new Date()
    const timeString = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`
    this.setData({
      lastUpdateTime: timeString
    })
  },
  
  // 切换自动模式
  toggleAutoMode(e) {
    // 检查用户权限
    if (!this.checkAdminPermission()) {
      return;
    }
    
    // 确保deviceId是数字类型，从字符串转换
    const deviceId = parseInt(e.currentTarget.dataset.deviceId)
    // 确保isChecked始终是布尔类型，避免undefined值
    let isChecked = e.detail.value
    
    // 类型检查和转换
    if (isChecked === undefined) {
      // 如果是undefined，使用当前UI状态的相反值
      isChecked = deviceId === 1 ? !this.data.autoMode1 : !this.data.autoMode2
    } else {
      // 确保是布尔类型
      isChecked = !!isChecked
    }
    
    console.log(`设备${deviceId}自动模式切换为: ${isChecked} (${typeof isChecked})`)
    
    // 更新UI状态
    if (deviceId === 1) {
      this.setData({
        autoMode1: isChecked
      })
    } else if (deviceId === 2) {
      this.setData({
        autoMode2: isChecked
      })
    }
    
    // 记录管理员操作日志
    this.chang_status_logs(
      null, // 无错误
      'toggle_auto_mode', // 操作类型
      deviceId.toString(), // 设备ID
      `设备${deviceId}`, // 设备名称
      `将设备${deviceId}自动模式切换为${isChecked ? '开启' : '关闭'}` // 操作详情
    );
    
    // 发送控制命令到设备，确保auto_mode_change是布尔值
  this.sendControlCommand(deviceId, 'DataGet', {
    auto_mode: Boolean(isChecked), // 确保是布尔类型
    device_id: parseInt(deviceId)
  });
  },
  
  // 发送设备控制命令
  async sendControlCommand(deviceId, commandType, params) {
    try {
      // 显示加载框
      wx.showLoading({
        title: '发送命令中...',
      });
      
      const actualDeviceId = this.data.deviceIds[deviceId - 1];
      
      console.log(`向设备${actualDeviceId}发送${commandType}命令:`, params);
      
      // 调用IoT API发送命令
      const result = await iotapi.sendDeviceCommand(
        actualDeviceId,
        commandType,
        params
      );
      
      // 命令发送成功，记录到日志
      const paramsDesc = this.getParamsDescription(params);
      console.log(`参数描述:`, paramsDesc);
      this.chang_status_logs(
        paramsDesc, // 参数描述
        'send_command', // 操作类型
        deviceId.toString(), // 设备ID
        `设备${deviceId}`, // 设备名称
        `向设备${deviceId}发送${commandType}命令，参数: ${paramsDesc}` // 操作详情
      );
      
      return result;
    } catch (error) {
      console.error(`命令发送失败:`, error);
      wx.showToast({
        title: '命令发送失败',
        icon: 'none'
      });
      console.log(`命令发送失败:`, error);
      // 命令发送失败，记录错误日志
      const errorMessage = error.message || '未知错误';
      this.chang_status_logs(
        errorMessage, // 错误信息
        'send_command_failed', // 操作类型
        deviceId.toString(), // 设备ID
        `设备${deviceId}`, // 设备名称
        `向设备${deviceId}发送${commandType}命令失败: ${errorMessage}` // 操作详情
      );
      
      throw error;
    } finally {
      setTimeout(() => {
        wx.hideLoading();
      }, 300);
    }
  },
  
  // 获取参数描述文本
  getParamsDescription(params) {
    if (!params) return '';
    
    const descriptions = [];
    if (params.auto_mode !== undefined) {
      descriptions.push(`自动模式=${params.auto_mode ? '开启' : '关闭'}`);
    }
    if (params.pump_status !== undefined) {
      descriptions.push(`水泵=${params.pump_status ? '开启' : '关闭'}`);
    }
    if (params.fan_status !== undefined) {
      descriptions.push(`风扇=${params.fan_status ? '开启' : '关闭'}`);
    }
    
    return descriptions.join(', ');
  },
  
  // 切换水泵状态
  togglePump(e) {
    // 检查用户权限
    if (!this.checkAdminPermission()) {
      return;
    }
    
    // 确保deviceId是数字类型，从字符串转换
    const deviceId = parseInt(e.currentTarget.dataset.deviceId)
    // 确保isChecked始终是布尔类型，避免undefined值
    let isChecked = e.detail.value
    
    // 类型检查和转换
    if (isChecked === undefined) {
      // 如果是undefined，使用当前UI状态的相反值
      isChecked = deviceId === 1 ? !this.data.device1Pump : !this.data.device2Pump
    } else {
      // 确保是布尔类型
      isChecked = !!isChecked
    }
    
    console.log(`设备${deviceId}水泵状态切换为: ${isChecked} (${typeof isChecked})`)
    
    // 更新UI状态
    if (deviceId === 1) {
      this.setData({
        device1Pump: isChecked
      })
    } else if (deviceId === 2) {
      this.setData({
        device2Pump: isChecked
      })
    }
    
    // 记录管理员操作日志
    this.chang_status_logs(
      null, // 无错误
      'toggle_pump', // 操作类型
      deviceId.toString(), // 设备ID
      `设备${deviceId}`, // 设备名称
      `将设备${deviceId}水泵状态切换为${isChecked ? '开启' : '关闭'}` // 操作详情
    );
    
    // 发送控制命令到设备，确保pump_status是布尔值
    this.sendControlCommand(deviceId, 'DataGet', {
      pump_status: Boolean(isChecked), // 确保是布尔类型
      device_id: parseInt(deviceId)
    });
  },
  
  // 切换风扇状态
  toggleFan(e) {
    // 检查用户权限
    if (!this.checkAdminPermission()) {
      return;
    }
    
    // 确保deviceId是数字类型，从字符串转换
    const deviceId = parseInt(e.currentTarget.dataset.deviceId)
    // 确保isChecked始终是布尔类型，避免undefined值
    let isChecked = e.detail.value
    
    // 类型检查和转换
    if (isChecked === undefined) {
      // 如果是undefined，使用当前UI状态的相反值
      isChecked = deviceId === 1 ? !this.data.device1Fan : !this.data.device2Fan
    } else {
      // 确保是布尔类型
      isChecked = !!isChecked
    }
    
    console.log(`设备${deviceId}风扇状态切换为: ${isChecked} (${typeof isChecked})`)
    
    // 更新UI状态
    if (deviceId === 1) {
      this.setData({
        device1Fan: isChecked
      })
    } else if (deviceId === 2) {
      this.setData({
        device2Fan: isChecked
      })
    }
    
    // 记录管理员操作日志
    this.chang_status_logs(
      null, // 无错误
      'toggle_fan', // 操作类型
      deviceId.toString(), // 设备ID
      `设备${deviceId}`, // 设备名称
      `将设备${deviceId}风扇状态切换为${isChecked ? '开启' : '关闭'}` // 操作详情
    );
    
    // 发送控制命令到设备，确保fan_status是布尔值
    this.sendControlCommand(deviceId, 'DataGet', {
      fan_status: Boolean(isChecked), // 确保是布尔类型
      device_id: parseInt(deviceId)
    });
  },
  
  /**
   * 设备命令下发示例函数
   * @param {number} deviceNum - 设备号，可选，默认为1，支持1和2
   */
  setCommand(deviceNum = 1) {
    // 确保deviceNum是数字类型
    deviceNum = parseInt(deviceNum)
    console.log(`开始下发命令到设备${deviceNum}。。。`);//打印完整消息
    var that = this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
    var token = wx.getStorageSync('token');//读缓存中保存的token
    
    // 验证设备号
    if (deviceNum < 1 || deviceNum > 2) {
      console.error("无效的设备号，仅支持1和2");
      return;
    }
    
    // 根据设备号获取对应的设备ID
    const deviceId = this.data.deviceIds[deviceNum - 1];
    
    wx.request({
      // 使用正确的华为云IoT平台命令下发URL格式
      // POST https://endpoint/v5/iot/project_id/devices/device_id/commands
      url: `https://cn-east-3.iotda-app.myhuaweicloud.com/v5/iot/e703e7c456ff41379cb07e5416c765f4/devices/${deviceId}/commands`,
      data: '{"service_id": "Dev_data","command_name": "Control","paras": { "led": 1}}',
      method: 'POST', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
      header: {'content-type': 'application/json','X-Auth-Token': token }, //请求的header  
      success: function(res){
        // success
        console.log("下发命令成功");//打印完整消息
        console.log(res);//打印完整消息
      },
      fail: function(){
        // fail
        console.log("命令下发失败");//打印完整消息
        console.log("请先获取token");//打印完整消息
      },
      complete: function() {
        // complete
        console.log("命令下发完成");//打印完整消息
        that.setData({result: '设备命令下发完成'});
      }
    });
  },
  
  // 检查管理员权限
  checkAdminPermission() {
    // 获取最新的用户信息
    this.getUserInfo();
    
    // 检查用户类型是否为管理员
    if (this.data.userType !== 'admin') {
      wx.showToast({
        title: '普通用户无操作权限',
        icon: 'none',
        duration: 2000
      });
      return false;
    }
    return true;
  },
  
  // 导航到设置页面
  navigateToSettings() {
    console.log('导航到系统设置页面')
    this.check_threshold()
    // 普通用户也可以跳转，但在设置页面会有只读限制
    wx.navigateTo({
      url: '../settings/settings'
    })
  },
  
  // 导航到操作日志页面
  navigateToLogs() {
    // 检查是否为管理员
    if (!this.checkAdminPermission()) {
      return;
    }
    
    console.log('导航到操作日志页面')
    
    wx.navigateTo({
      url: '../operation_logs/operation_logs'
    })
  },
  
  // 创建日志文件夹

  
    // 导航到登录页面 - 退出登录
  navigateToLogin() {
    console.log('执行退出登录操作')
    // 清除定时器，停止获取华为云数据
    if (this.dataRefreshTimer) {
      clearInterval(this.dataRefreshTimer);
      console.log('已停止数据刷新定时器');
    }
    // 清除token信息
    wx.removeStorageSync('token');
    // 清除内存中的token信息
    if (typeof iotapi !== 'undefined') {
      iotapi.tokenInfo = null;
    }
    // 设置全局登录状态为未登录
    const app = getApp();
    app.setLoginStatus(false);
    
    // 获取用户信息用于日志记录
    const userInfo = wx.getStorageSync('userInfo');
    
    // 立即清除本地用户信息存储，避免页面跳转时序问题
    wx.removeStorageSync('userInfo');
    
    // 调用退出登录日志记录（使用已保存的用户信息）
    this.login_out(null, userInfo)
    
    // 使用redirectTo替换navigateTo，避免页面栈中保留index页面
    wx.redirectTo({ url: '/pages/logs/logs' });
  },
  // 添加退出登录操作日志到数据库
  login_out(errorMessage = null, userInfo = null) {
    // console.log('=== 开始记录退出登录操作日志 ===');
    
    // 如果没有传入userInfo，尝试从缓存获取（兼容旧调用方式）
    const userData = userInfo || wx.getStorageSync('userInfo');
    console.log('使用的用户信息:', userData);
    
    const id = userData ? (userData.userid || this.data.userid) : null;
    console.log('退出登录用户ID:', id);
    
    // 即使没有用户ID，也要尝试记录日志
    if (!id) {
      console.error('无法获取用户ID，但仍尝试记录日志');
    }
    
    // 准备发送到后端的数据
    const requestData = {
      userid: id || null,
      operation_type: 'logout', // 退出登录操作类型
      device_id: 'null', // 默认设备ID
      operation_result: errorMessage ? 'failure' : 'success', // 使用数据库enum类型支持的值
      error_message: errorMessage
    };
    // console.log('准备发送到后端的退出登录日志数据:', requestData);
    
    wx.request({
      url: API.LOGIN_RECORD, // 使用配置的API地址
      method: 'POST',
      data: requestData,
      success: (res) => {
        // console.log('退出登录日志记录成功，后端返回:', res.data);
      },
      fail: (err) => {
        console.error('记录退出登录日志请求失败:', err);
        console.error('失败详情:', JSON.stringify(err));
      },
      complete: () => {
        console.log('=== 退出登录日志记录完成 ===');
        // 注意：用户信息已在navigateToLogin中清除，这里不再重复操作
      }
    });
  },
  // 添加检查阈值操作日志到数据库
  check_threshold(errorMessage = null) {
    console.log('=== 开始记录检查阈值操作日志 ===');
    
    // 获取用户信息
    const userInfo = wx.getStorageSync('userInfo');
    console.log('从缓存中获取的用户信息:', userInfo);
    
    const id = userInfo.userid || this.data.userid;
    console.log('检查阈值用户ID:', id);
    
    // 即使没有用户ID，也要尝试记录日志
    if (!id) {
      console.error('无法获取用户ID，但仍尝试记录日志');
    }
    
    // 准备发送到后端的数据
    const requestData = {
      userid: id || null,
      operation_type: 'check_threshold', // 检查阈值操作类型
      device_id: 'null', // 默认设备ID
      operation_result: errorMessage ? 'failure' : 'success', // 使用数据库enum类型支持的值
      error_message: errorMessage
    };
    console.log('准备发送到后端的检查阈值日志数据:', requestData);
    
    wx.request({
      url: API.LOGIN_RECORD, // 使用配置的API地址
      method: 'POST',
      data: requestData,
      success: (res) => {
        console.log('检查阈值日志记录成功，后端返回:', res.data);
      },
      fail: (err) => {
        console.error('记录检查阈值日志请求失败:', err);  
        console.error('失败详情:', JSON.stringify(err));
      },
      complete: () => {
        console.log('=== 检查阈值日志记录完成 ===');
        // 注意：这里不需要清除用户信息，因为这只是检查操作，不是退出登录
      }
    });
  },

  // 记录管理员操作到日志中
  chang_status_logs(errorMessage = null, operation_type = 'chang_status', device_id = null, device_name = null, action_details = '') {
    console.log('=== 开始记录管理员操作日志 ===');
    
    // 获取用户信息
    const userInfo = wx.getStorageSync('userInfo');
    console.log('从缓存中获取的用户信息:', userInfo);
    
    const id = userInfo.userid || this.data.userid;
    const username = userInfo.username || '未知用户';
    const role = userInfo.role || 'user'; // 添加角色信息
    
    // 获取实际操作的设备ID，如果传入的是null，则尝试从页面数据中获取
    const actualDeviceId = device_id || this.data.deviceId || this.data.currentDeviceId || 'unknown';
    const actualDeviceName = device_name || this.data.deviceName || '未知设备';
    
    // 准备发送到后端的数据，包含更详细的管理员操作信息
    const requestData = {
      userid: id || null,
      username: username,
      role: role,
      operation_type: operation_type, // 操作类型
      device_id: actualDeviceId,
      device_name: actualDeviceName,
      action_details: action_details, // 详细操作内容
      operation_result: errorMessage ? 'failure' : 'success', // 操作结果
      error_message: errorMessage,
      operation_time: new Date().toISOString() // 添加操作时间
    };
    
    console.log('准备发送到后端的管理员操作日志数据:', requestData);
    
    wx.request({
      url: API.LOGIN_RECORD, // 使用配置的API地址
      method: 'POST',
      data: requestData,
      success: (res) => {
        console.log('管理员操作日志记录成功，后端返回:', res.data);
      },
      fail: (err) => {
        console.error('记录管理员操作日志请求失败:', err);
        console.error('失败详情:', JSON.stringify(err));
      },
      complete: () => {
        console.log('=== 管理员操作日志记录完成 ===');
      }
    });
  },

  // 手动刷新数据
  onRefresh() {
    console.log('手动刷新数据')
    this.fetchDeviceData();
  }
})
