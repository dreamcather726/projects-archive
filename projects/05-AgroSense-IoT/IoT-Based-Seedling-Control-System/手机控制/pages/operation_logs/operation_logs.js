// operation_logs.js - 操作日志页面
// 导入API配置
const { API } = require('../../utils/apiConfig');
Page({
  data: {
    logs: [],
    isAdmin: false,
    refreshing: false,
    loadingMore: false,
    currentPage: 1,
    pageSize: 20,
    hasMoreData: true,
    totalLogs: 0
  },

  onLoad() {
    // 检查用户权限
    this.checkAdminPermission();
    
    // 只有管理员才能加载日志
    if (this.data.isAdmin) {
      this.loadOperationLogs();
    }
  },

  // 检查管理员权限
  checkAdminPermission() {
    const userInfo = wx.getStorageSync('userInfo');
    const isAdmin = userInfo && (userInfo.userType === 'admin' || userInfo.isAdmin);
    
    this.setData({
      isAdmin: isAdmin
    });

    if (!isAdmin) {
      wx.showToast({
        title: '只有管理员才能查看日志',
        icon: 'none'
      });
      // 延迟返回，让用户看到提示
      setTimeout(() => {
        wx.navigateBack();
      }, 1500);
    }
  },

  // 加载操作日志
  loadOperationLogs() {
    // 重置分页参数
    this.setData({
      currentPage: 1,
      logs: [],
      hasMoreData: true
    });
    // 从服务器获取操作日志
    this.fetchLogsFromServer();
  },

  // 返回上一页
  goBack() {
    wx.navigateBack();
  },

  // 刷新日志
  refreshLogs() {
    this.setData({ 
      refreshing: true,
      currentPage: 1,
      logs: [],
      hasMoreData: true
    });
    this.fetchLogsFromServer().then(() => {
      this.setData({ refreshing: false });
    }).catch(() => {
      this.setData({ refreshing: false });
    });
  },
  
  // 加载更多日志（下拉加载）
  loadMoreLogs() {
    // 检查是否已有正在加载的状态或没有更多数据
    if (this.data.loadingMore || !this.data.hasMoreData) return;
    
    this.setData({ loadingMore: true });
    
    // 增加页码
    const nextPage = this.data.currentPage + 1;
    
    // 调用获取日志方法，传入新页码
    this.fetchLogsFromServer(nextPage).then(() => {
      this.setData({ 
        loadingMore: false,
        currentPage: nextPage 
      });
    }).catch(() => {
      this.setData({ loadingMore: false });
    });
  },

  // 从服务器获取操作日志
  fetchLogsFromServer(page = 1) {
    return new Promise((resolve, reject) => {
      if (page === 1) {
        wx.showLoading({
          title: '加载日志中...',
        });
      }
      
      wx.request({
      url: API.OPERATION_LOGS, // 使用配置的API地址
      method: 'GET',
        data: {
          page: page,
          pageSize: this.data.pageSize
        },
        success: (res) => {
          if (res.data.success && res.data.logs) {
              // 优化日志格式化，添加类型分类
              
              const formattedLogs = res.data.logs.map(log => {
                // 使用created_at而不是timestamp
                const date = new Date(log.created_at);
                const formattedDate = date.toLocaleString('zh-CN', {
                  year: 'numeric',
                  month: '2-digit',
                  day: '2-digit',
                  hour: '2-digit',
                  minute: '2-digit',
                  second: '2-digit'
                });
                
                // 确定日志类型
                let logType = 'info';
                if (log.operation_result === 'failure' || log.error_message) {
                  logType = 'error';
                } else if (log.operation_type && log.operation_type.includes('warning')) {
                  logType = 'warning';
                } else if (log.operation_result === 'success') {
                  logType = 'success';
                }
                
                // 构建结构化日志对象
                return {
                  id: log.id || Date.now() + Math.random(),
                  time: `${formattedDate}`,
                  type: logType,
                  deviceId: log.device_id || 'N/A',
                  userName: log.name || '未知用户',
                  userId: log.user_id,
                  operationType: this.getOperationTypeName(log.operation_type),
                  operationResult: log.operation_result,
                  errorMessage: log.error_message
                 
                };
              });
            
            // 合并或替换日志数据
            const newLogs = page === 1 ? formattedLogs : [...this.data.logs, ...formattedLogs];
            
            // 更新状态
            this.setData({
              logs: newLogs,
              totalLogs: res.data.pagination.total,
              hasMoreData: newLogs.length < res.data.pagination.total
            });
            
            resolve(formattedLogs);
          } else {
            reject(new Error('无有效日志数据'));
          }
        },
        fail: (err) => {
          console.error('获取服务器日志失败:', err);
          wx.showToast({
            title: '获取日志失败',
            icon: 'none'
          });
          reject(err);
        },
        complete: () => {
          if (page === 1) {
            wx.hideLoading();
          }
        }
      });
    });
  },
  
  // 获取操作类型的中文名称
  getOperationTypeName(operationType) {
    const typeMap = {
      'send_command': '发送命令',
      'send_command_failed': '命令发送失败',
      'toggle_auto_mode': '切换自动模式',
      'setting_threshold': '设置阈值',
      'device_connection': '设备连接',
      'login': '用户登录',
      'logout': '用户登出',
      'user_register': '用户注册',
      'update_threshold': '更新阈值',
      'set_threshold': '阈值设置',
      'check_threshold': '阈值检查',
      'toggle_manual_mode': '切换模式',
      'toggle_pump': '切换水泵',
      'toggle_fan': '切换风扇',
      
    };
    return typeMap[operationType] || operationType;
  },
  
});