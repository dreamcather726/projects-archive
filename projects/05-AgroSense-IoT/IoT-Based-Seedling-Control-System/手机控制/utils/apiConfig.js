// API配置文件 - 集中管理所有后端接口地址
// 这样修改API地址时只需要修改一处，方便维护

// 后端服务基础URL
const BASE_URL = 'http://10.103.195.37:3000';

// 导出所有API接口地址
export const API = {
  // 基础接口
  BASE: BASE_URL,
  
  // 登录注册相关
  LOGIN: `${BASE_URL}/login`,
  REGISTER: `${BASE_URL}/register`,
  
  // 日志记录相关
  LOGIN_RECORD: `${BASE_URL}/login_record`,
  REGISTER_RECORD: `${BASE_URL}/register_record`,
  OPERATION_LOGS: `${BASE_URL}/get_operation_logs`,
  
  // 阈值相关
  THRESHOLDS: `${BASE_URL}/thresholds`,
  GET_THRESHOLDS: `${BASE_URL}/get_thresholds`,
  UPDATE_THRESHOLD: `${BASE_URL}/update_threshold`,

  SENSOR_DATA: `${BASE_URL}/sensor_data`,
};

// 导出基础URL，方便某些需要拼接URL的场景
export { BASE_URL };
