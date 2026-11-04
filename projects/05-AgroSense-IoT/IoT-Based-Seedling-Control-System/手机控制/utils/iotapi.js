// iotapi.js - 华为云IoT平台API封装
// 华为云IoT平台基础URL
const BASE_URL = 'https://iotda.cn-east-3.myhuaweicloud.com';
const API_VERSION = 'v5';
// IAM token获取URL
const IAM_TOKEN_URL = 'https://iam.cn-east-3.myhuaweicloud.com/v3/auth/tokens';

// 存储token信息
let tokenInfo = null;

/**
 * 获取token
 */
const getToken = () => {
  return new Promise((resolve, reject) => {
    // console.log("开始获取。。。");//打印完整消息
    var that=this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
    // 构建认证信息，使用正确的domain.name
    const authData = {
      "auth": {
        "identity": {
          "methods": ["password"],
          "password": {
            "user": {
              "name": "master",
              "password": "REDACTED_CONFIGURE_LOCALLY",
              "domain": {
                "name": "GT-2501_91198025"
              }
            }
          }
        },
        "scope": {
          "project": {
            "name": "cn-east-3"
          }
        }
      }
    };
    
    wx.request({
      url: 'https://iam.cn-east-3.myhuaweicloud.com/v3/auth/tokens',
      data: authData,
      method: 'POST',
      header: {'content-type': 'application/json' },
      success: function(res){
      
        
        // 解析token的代码
        var token=''; 
        token=JSON.stringify(res.header['X-Subject-Token'] || res.header['x-subject-token']);
        token=token.replaceAll("\"", ""); 
        wx.setStorageSync('token',token);
        
        // 更新内存中的tokenInfo
        tokenInfo = token;
        
        resolve(token);
      },
      fail:function(err){
      
        console.error('获取token失败:', err);
        reject(err);
      },
      complete: function() {
      
      }
    });
  });
};

/**
 * 获取设备影子数据
 * 根据设备ID获取不同设备的影子数据
 * @param {string} deviceId - 设备ID，可选，默认为第一个设备
 * @param {Object} callbacks - 回调函数对象，包含success、fail、complete回调
 */
const getDeviceShadow = function(deviceId = '68ee03050a9ab42ae5847f9e_text2', callbacks = {}) {
  // console.log("开始获取影子" + (deviceId ? "，设备ID: " + deviceId : ""));//打印完整消息
  var token = wx.getStorageSync('token');//读缓存中保存的token

  
  // 设备URL映射表
  const deviceUrls = {
    '68ee03050a9ab42ae5847f9e_text2': 'https://6d5aef22d8.st1.iotda-app.cn-east-3.myhuaweicloud.com:443/v5/iot/e703e7c456ff41379cb07e5416c765f4/devices/68ee03050a9ab42ae5847f9e_text2/shadow',
    '68ee03050a9ab42ae5847f9e_test': 'https://6d5aef22d8.st1.iotda-app.cn-east-3.myhuaweicloud.com:443/v5/iot/e703e7c456ff41379cb07e5416c765f4/devices/68ee03050a9ab42ae5847f9e_test/shadow' // 假设第二个设备的URL
  };
  
  // 获取对应设备的URL，如果找不到则使用默认URL
  const url = deviceUrls[deviceId] || deviceUrls['68ee03050a9ab42ae5847f9e_text2'];
  
  wx.request({
    url: url,
    data: '',
    method: 'GET', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
    header: {'content-type': 'application/json', 'X-Auth-Token': token }, //请求的header  
    success: function(res) {// success
      // console.log("设备" + deviceId + "影子获取成功:");
      // console.log(res.data);//打印完整消息
      
      // 过滤掉text服务，只保留DataGet服务
      if (res.data && res.data.shadow && Array.isArray(res.data.shadow)) {
        res.data.shadow = res.data.shadow.filter(item => item.service_id !== 'text');
      }
      
      // 调用外部传入的success回调，并传递过滤后的数据
      if (callbacks.success && typeof callbacks.success === 'function') {
        callbacks.success(res.data);
      }
    },
    fail: function(error) {
      // fail
      console.log("设备" + deviceId + "影子获取失败");//打印完整消息
      console.error("失败详情:", error);
      // 调用外部传入的fail回调，并传递错误信息
      if (callbacks.fail && typeof callbacks.fail === 'function') {
        callbacks.fail(error);
      }
    },
    complete: function() {
      // complete
      // console.log("设备" + deviceId + "影子获取完成");//打印完整消息
      // 调用外部传入的complete回调
      if (callbacks.complete && typeof callbacks.complete === 'function') {
        callbacks.complete();
      }
    }
  });
}

/**
 * 解析设备影子数据为前端可用格式
 * @param {Object} deviceShadow - 设备影子原始数据
 * @returns {Object} 解析后的数据
 */
const parseDeviceData = (deviceShadow) => {
  if (!deviceShadow || !deviceShadow.shadow) {
    return getDefaultDeviceData();
  }

  // 过滤掉text服务，只保留DataGet服务的数据
  // 查找DataGet服务的数据
  const dataGetService = deviceShadow.shadow.find(item => item.service_id === 'DataGet');
  const properties = dataGetService?.reported?.properties || {};

  const pickProperty = (obj, keys) => {
    for (const key of keys) {
      if (Object.prototype.hasOwnProperty.call(obj, key)) {
        return obj[key];
      }
    }
    return undefined;
  };

  const toNumberOrNull = (value) => {
    if (value === null || value === undefined || value === '') return null;
    const num = Number(value);
    return Number.isNaN(num) ? null : num;
  };

  const toBooleanOrNull = (value) => {
    if (value === null || value === undefined) return null;
    return !!value;
  };

  return {
    temperature: toNumberOrNull(pickProperty(properties, ['Temp', 'temp', 'Temperature', 'temperature'])),
    humidity: toNumberOrNull(pickProperty(properties, ['Hum', 'hum', 'Humidity', 'humidity'])),
    light: toNumberOrNull(pickProperty(properties, ['Light', 'light', 'Illuminance', 'illuminance'])),
    soilHumidity: toNumberOrNull(pickProperty(properties, ['Soil_hum', 'SoilHum', 'Soil_Hum', 'SoilHumidity', 'soilHumidity', 'soil_hum', 'soil_humidity'])),
    waterPump: toBooleanOrNull(pickProperty(properties, ['Pump_Status', 'pump_status', 'PumpStatus', 'pumpStatus', 'waterPump'])),
    fan: toBooleanOrNull(pickProperty(properties, ['Fan_Status', 'fan_status', 'FanStatus', 'fanStatus', 'fan'])),
    autoMode: toBooleanOrNull(pickProperty(properties, ['Auto_Mode', 'auto_mode', 'AutoMode', 'autoMode']))
  };
};

/**
 * 获取默认设备数据
 * @returns {Object} 默认设备数据
 */
const getDefaultDeviceData = () => {
  return {
    temperature: null,
    humidity: null,
    light: null,
    soilHumidity: null,
    waterPump: null,
    fan: null,
    autoMode: null
  };
};

/**
 * 获取所有设备数据
 * 为每个设备ID调用getDeviceShadow函数获取影子数据
 * @param {Array<string>} deviceIds - 设备ID数组，可选，默认为系统中的两个设备
 * @returns {Promise<Array<Object>>} 所有设备的数据数组
 */
const getAllDevicesData = async function(deviceIds = ['68ee03050a9ab42ae5847f9e_text2', '68ee03050a9ab42ae5847f9e_test']) {
  // 检查登录状态，如果未登录则不获取数据
  try {
    const app = getApp();
    // 如果app存在且提供了isLoggedIn方法，并且返回false，则表示未登录
    if (app && typeof app.isLoggedIn === 'function' && !app.isLoggedIn()) {
      console.log('用户未登录，不获取设备数据');
      // 返回空数组或默认数据，但不进行数据解析
      return [];
    }
  } catch (error) {
    // 如果获取app实例失败，继续执行（可能是在非页面环境中调用）
  }
  
  // console.log('开始获取所有设备数据...');
  
  // 创建一个函数来获取单个设备数据并添加延迟
  const fetchDeviceWithDelay = async (deviceId, index) => {
    // 再次检查登录状态，确保在异步过程中状态未改变
    try {
      const app = getApp();
      if (app && typeof app.isLoggedIn === 'function' && !app.isLoggedIn()) {
        console.log('用户未登录，停止获取设备数据');
        return {
          id: index + 1,
          deviceId: deviceId,
          error: true,
          errorMessage: '用户未登录',
          temperature: null,
          humidity: null,
          light: null,
          soilHumidity: null,
          waterPump: null,
          fan: null,
          autoMode: null
        };
      }
    } catch (error) {
      // 忽略错误，继续执行
    }
    
    // console.log(`正在获取设备${index + 1}的数据，设备ID: ${deviceId}`);
    
    // 如果不是第一个设备，添加延迟避免请求过于密集
    if (index > 0) {
      await new Promise(resolve => setTimeout(resolve, 300));
    }
    
    // 调用getDeviceShadow获取设备数据，使用Promise包装回调
    return new Promise((resolve, reject) => {
      getDeviceShadow(deviceId, {
        success: (data) => {
          // 再次检查登录状态，避免在异步回调中解析数据
          try {
            const app = getApp();
            if (app && typeof app.isLoggedIn === 'function' && !app.isLoggedIn()) {
              console.log('用户未登录，不解析设备数据');
              resolve({
                id: index + 1,
                deviceId: deviceId,
                error: true,
                errorMessage: '用户未登录',
                temperature: null,
                humidity: null,
                light: null,
                soilHumidity: null,
                waterPump: null,
                fan: null,
                autoMode: null
              });
              return;
            }
          } catch (error) {
            // 忽略错误，继续解析
          }
          
          // console.log(`设备${index + 1}数据获取成功`, data);
          // 解析设备数据
          const parsedData = parseDeviceData(data);
          console.log(`设备${index + 1}解析后数据:`, parsedData);
          resolve({
            id: index + 1,
            deviceId: deviceId,
            ...parsedData
          });
        },
        fail: (error) => {
          console.error(`设备${index + 1}数据获取失败`, error);
          // 失败时不返回默认数据，返回包含设备ID和错误状态的空数据对象
          resolve({
            id: index + 1,
            deviceId: deviceId,
            error: true,
            errorMessage: error.message || '获取数据失败',
            // 只保留必要的空值字段
            temperature: null,
            humidity: null,
            light: null,
            soilHumidity: null,
            waterPump: null,
            fan: null,
            autoMode: null
          });
        }
      });
    });
  };
  
  // 按顺序获取所有设备数据
  const devicesData = [];
  for (let i = 0; i < deviceIds.length; i++) {
    try {
      const deviceData = await fetchDeviceWithDelay(deviceIds[i], i);
      devicesData.push(deviceData);
    } catch (error) {
      console.error(`设备${i + 1}处理过程中出错:`, error);
      // 单个设备出错时，不返回默认数据，而是记录错误
      devicesData.push({
        id: i + 1,
        deviceId: deviceIds[i],
        error: true,
        errorMessage: error.message || '处理数据失败',
        temperature: null,
        humidity: null,
        light: null,
        soilHumidity: null,
        waterPump: null,
        fan: null,
        autoMode: null
      });
    }
  }
  
  // console.log('所有设备数据获取完成:', devicesData);
  return devicesData;
}

/**
 * 发送设备控制命令
 * @param {string} deviceId - 设备ID
 * @param {string} serviceId - 服务ID
 * @param {Object} command - 命令内容
 * @returns {Promise<Object>} 控制结果
 */
const sendDeviceCommand = async (deviceId, serviceId, command) => {
  try {
    // 优先从缓存中获取token
    let token = wx.getStorageSync('token');
    if (!token) {
    
      token = await getToken();
    } else {
    
    }
    // 设备URL映射表
  const deviceUrls = {
    '68ee03050a9ab42ae5847f9e_text2': 'https://6d5aef22d8.st1.iotda-app.cn-east-3.myhuaweicloud.com:443/v5/iot/e703e7c456ff41379cb07e5416c765f4/devices/68ee03050a9ab42ae5847f9e_text2/commands',
    '68ee03050a9ab42ae5847f9e_test': 'https://6d5aef22d8.st1.iotda-app.cn-east-3.myhuaweicloud.com:443/v5/iot/e703e7c456ff41379cb07e5416c765f4/devices/68ee03050a9ab42ae5847f9e_test/commands' // 假设第二个设备的URL
  };
  
  // 获取对应设备的URL，如果找不到则使用默认URL
  const url = deviceUrls[deviceId] || deviceUrls['68ee03050a9ab42ae5847f9e_text2'];
    
    console.log(`连接的url:${url}`);
    
    // 根据命令内容确定command_name
    let commandName = 'Control';
    if (command.hasOwnProperty('fan_status')) {
      commandName = 'fan_control';
    } else if (command.hasOwnProperty('pump_status')) {
      commandName = 'pump_control';
    } else if (command.hasOwnProperty('auto_mode')) {
      commandName = 'auto_mode';
    } else if (command.hasOwnProperty('setting_threshold')) {
      commandName = 'setting_threshold';
    }
    
    // 构建发送的数据，使用正确的service_id为'DataGet'
    const requestData = {
      service_id: 'DataGet',
      command_name: commandName,
      paras: command
    };
    
    return new Promise((resolve, reject) => {
      wx.request({
        url: url,
        method: 'POST',
        header: {
          'Content-Type': 'application/json',
          'X-Auth-Token': token
        },
        data: requestData,
        success: (res) => {
          console.log('发送的数据:', JSON.stringify(requestData));
          
          resolve(res);
        },
        fail: (err) => {
          console.log('发送的数据:', JSON.stringify(requestData));
          console.error(`向设备${deviceId}发送命令失败:`, err);
          // 如果token过期，清除缓存并重新获取
          if (err.errMsg && err.errMsg.includes('401')) {
          
            wx.removeStorageSync('token');
            // 重新获取token并发送命令
            getToken().then(newToken => {
              wx.request({
                url: url,
                method: 'POST',
                header: {
                  'Content-Type': 'application/json',
                  'X-Auth-Token': newToken
                },
                data: requestData,
                success: (res) => {
                  resolve(res);
                },
                fail: (retryErr) => {
                  console.error(`重新向设备${deviceId}发送命令失败:`, retryErr);
                  reject(retryErr);
                }
              });
            }).catch(tokenErr => {
              reject(tokenErr);
            });
          } else {
            reject(err);
          }
        }
      });
    });
  } catch (error) {
    console.error(`发送设备${deviceId}命令异常:`, error);
    throw error;
  }
};
const sendthreshold = async (deviceId, serviceId, command,temp_threshold,humidity_threshold) => {
  try {
    // 优先从缓存中获取token
    let token = wx.getStorageSync('token');
    if (!token) {
      token = await getToken();
    } else {
      // 检查token是否过期
      const tokenExpiration = wx.getStorageSync('tokenExpiration');
      if (tokenExpiration && Date.now() >= tokenExpiration) {
        token = await getToken();
      }
    }
    
    // 根据设备ID字符串提取设备号，转换为整数类型
    // 假设deviceId格式为"68ee03050a9ab42ae5847f9e_text2"或"68ee03050a9ab42ae5847f9e_test"
    let deviceNum = 1; // 默认设备1
    if (deviceId.includes('_test')) {
      deviceNum = 2; // 设备2
    } else if (deviceId.includes('_text2')) {
      deviceNum = 1; // 设备1
    }
    
    // 构建包含温度和湿度阈值的命令对象，使用与服务器要求完全匹配的参数名称
    const thresholdCommand = {
      temp_threshold: Number(temp_threshold), // 确保是数字类型
      hum_threshold: Number(humidity_threshold), // 使用hum_threshold而不是humidity_threshold
      device_id: deviceNum // 设备号，整数类型
    };
    
    // 设置命令名称为阈值设置
    const thresholdCommandWithName = {
      ...thresholdCommand,
      setting_threshold: true // 用于识别这是阈值设置命令
    };
    
    console.log('发送阈值设置命令:', thresholdCommandWithName);
    return await sendDeviceCommand(deviceId, serviceId, thresholdCommandWithName);
  } catch (error) {
    console.error(`发送设备${deviceId}阈值设置命令异常:`, error);
    throw error;
  }
};
// 导出API
module.exports = {
  getToken: getToken,
  getDeviceShadow: getDeviceShadow,
  getAllDevicesData: getAllDevicesData,
  sendDeviceCommand: sendDeviceCommand,
  parseDeviceData: parseDeviceData,
  sendthreshold: sendthreshold,
  // 导出连接信息常量
  BASE_URL: BASE_URL,
  API_VERSION: API_VERSION,
  IAM_TOKEN_URL: IAM_TOKEN_URL
};
