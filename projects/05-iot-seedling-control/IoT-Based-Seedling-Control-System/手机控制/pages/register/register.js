// register.js - 用户注册页面
// 导入API配置
const { API } = require('../../utils/apiConfig');
Page({
  data: {
    username: '',
    password: '',
    confirmPassword: '',
    registerButtonText: '注册',
    userType: 'user', // 默认普通用户
    authCode: '',
    name: ''
  },
  
  // 账号输入处理
  onUsernameInput(e) {
    this.setData({
      username: e.detail.value
    });
  },
  
  // 名字输入处理
  onNameInput(e) {
    this.setData({
      name: e.detail.value
    });
  },
  
  // 密码输入处理
  onPasswordInput(e) {
    this.setData({
      password: e.detail.value
    });
  },
  
  // 确认密码输入处理
  onConfirmPasswordInput(e) {
    this.setData({
      confirmPassword: e.detail.value
    });
  },
  
  // 用户类型选择处理
  onUserTypeChange(e) {
    this.setData({
      userType: e.detail.value
    });
  },
  
  // 授权码输入处理
  onAuthCodeInput(e) {
    this.setData({
      authCode: e.detail.value
    });
  },
  
  // 注册处理
  register() {
    const { username, password, confirmPassword, userType, authCode, name } = this.data;
    
    // 表单验证
    if (!username) {
      wx.showToast({
        title: '请输入账号',
        icon: 'none'
      });
      return;
    }
    
    if (!name) {
      wx.showToast({
        title: '请输入名字',
        icon: 'none'
      });
      return;
    }
    
    if (!password) {
      wx.showToast({
        title: '请输入密码',
        icon: 'none'
      });
      return;
    }
    
    if (password !== confirmPassword) {
      wx.showToast({
        title: '两次输入的密码不一致',
        icon: 'none'
      });
      return;
    }
    
    if (password.length < 6) {
      wx.showToast({
        title: '密码长度至少6位',
        icon: 'none'
      });
      return;
    }
    
    // 管理员授权码验证
    if (userType === 'admin') {
      if (!authCode) {
        wx.showToast({
          title: '请输入管理员授权码',
          icon: 'none'
        });
        return;
      }
      
      if (authCode !== 'hyt') {
        wx.showToast({
          title: '管理员授权码错误',
          icon: 'none'
        });
        return;
      }
    }
    
    // 设置注册按钮为加载状态
    this.setData({ registerButtonText: '注册中...' });
    
    // 调用后端注册API
    wx.request({
      url: API.REGISTER, // 使用配置的API地址
      method: 'POST',
      data: {
          username: username,
          password: password,
          name: name,
          userType: userType || 'user',
          authCode: userType === 'admin' ? authCode : null
        },
      success: (res) => {
        // console.log('注册请求成功:', res.data);
        
        if (res.data.success) {
          // 记录注册成功日志
          this.register_record('success', null);
          
          wx.showToast({
            title: '注册成功',
            icon: 'success'
          });
          
          // 延迟跳转到登录页面
          setTimeout(() => {
            this.switchToLogin();
          }, 1500);
        } else {
          // 记录注册失败日志
          this.register_record('failure', res.data.message || '注册失败');
          
          // 保持后端返回的错误信息，例如'账号已存在'
          wx.showToast({
            title: res.data.message || '注册失败',
            icon: 'error'
          });
        }
      },
      fail: (err) => {
        console.error('注册请求失败:', err);
        
        // 记录网络错误日志
        this.register_record('failure', '网络连接失败: ' + JSON.stringify(err));
        
        wx.showToast({
          title: '注册失败，请检查网络连接',
          icon: 'error'
        });
      },
      complete: () => {
        // 恢复按钮状态
        this.setData({ registerButtonText: '注册' });
      }
    });
  },
  
  // 记录注册操作日志
  register_record(operation_result, error_message) {
    // console.log('=== 开始记录注册操作日志 ===');
    // console.log('操作结果:', operation_result);
    // console.log('错误信息:', error_message);
    
    const requestData = {
      operation_type: 'register',
      operation_result: operation_result,
      username: this.data.username,
      user_type: this.data.userType,
      error_message: error_message
    };
    
    // console.log('准备发送到后端的日志数据:', requestData);
    
    wx.request({
      url: API.REGISTER_RECORD, // 使用配置的API地址
      method: 'POST',
      data: requestData,
      success: (res) => {
        console.log('注册日志记录成功');
      },
      fail: (err) => {
        console.error('记录注册日志失败:', err);
      },
      complete: () => {
        // console.log('=== 注册日志记录完成 ===');
      }
    });
  },
  
  // 切换到登录页面
  switchToLogin() {
    wx.navigateTo({ url: '/pages/logs/logs' });
  }
})