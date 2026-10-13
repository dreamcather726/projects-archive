const express=require('express')
const bodyParser =require('body-parser')
const app=express()
const mysql = require('mysql')

// 配置中间件
app.use(bodyParser.json())
// 添加CORS支持，允许微信小程序访问
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*')
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
  res.header('Access-Control-Allow-Headers', 'Content-Type, Authorization')
  if (req.method === 'OPTIONS') {
    return res.sendStatus(200)
  }
  next()
})

// 创建数据库连接池
const pool = mysql.createPool({
  host: process.env.MYSQL_HOST || 'localhost',
  user: process.env.MYSQL_USER || 'root',
  password: process.env.MYSQL_PASSWORD || '18257752165',
  database: process.env.MYSQL_DATABASE || 'iot-data',
  connectionLimit: Number(process.env.MYSQL_CONNECTION_LIMIT || 10),
  multipleStatements: false
})

pool.query(
  `
  CREATE TABLE IF NOT EXISTS sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    device_id VARCHAR(128) NOT NULL,
    device_no INT NULL,
    temperature DECIMAL(10,2) NULL,
    humidity DECIMAL(10,2) NULL,
    light DECIMAL(10,2) NULL,
    soil_humidity DECIMAL(10,2) NULL,
    water_pump TINYINT(1) NULL,
    fan TINYINT(1) NULL,
    auto_mode TINYINT(1) NULL,
    user_id INT NULL,
    collected_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_device_time (device_id, collected_at)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
  `,
  (err) => {
    if (err) {
      console.error('初始化sensor_data表失败:', err)
    }
  }
)

// 登录API
app.post('/login', (req, res) => {
  const { username, password, loginMode } = req.body
  // console.log('登录请求:', { username, loginMode })
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err)
      return res.status(500).json({ success: false, message: '数据库连接失败' })
    }
    
    // 查询用户信息
    connection.query('SELECT * FROM users WHERE username = ?', [username], (error, results) => {
      // 释放连接回池中
      connection.release()
      // console.log(results)
      if (error) {
        console.error('查询用户失败:', error)
        return res.status(500).json({ success: false, message: '查询用户失败' })
      }
      
      // 检查用户是否存在
      if (results.length === 0) {
        return res.json({ success: false, message: '用户不存在' })
      }
      
      const user = results[0]
      
      // 检查密码（实际项目中应该使用密码哈希验证）
      if (user.password !== password) {
        return res.json({ success: false, message: '密码错误' })
      }
   
      // 检查用户类型和登录模式
      const isAdmin = user.user_type === 'admin'
      
      // console.log('用户类型验证:', { username, actualType: user.user_type, isAdmin: isAdmin, requestedMode: loginMode })
      
      if (loginMode === 'admin') {
        if (!isAdmin) {
          console.log('阻止非管理员使用管理员登录模式:', username)
          return res.json({ success: false, message: '只有管理员才能使用管理员登录模式' })
        }
        console.log('管理员登录成功:', username)
      } else {
        console.log('普通登录成功:', username)
      }
      
      // 登录成功 - 添加userid和name字段
      res.json({
        success: true,
        username: user.username,
        name: user.name || user.username, // 返回用户名字，如果没有则使用用户名
        userid: user.id, // 返回用户ID
        userType: user.user_type,
        isAdmin: isAdmin,
        loginMode: loginMode
      })
    })
  })
})

app.post('/login_record',(req,res)=>{
  console.log(req.body)
  const { userid, operation_type, device_id, operation_result, error_message } = req.body
 
  
  // 验证必要参数，放宽对userid的验证要求
  if (!operation_type || !device_id) {
    console.error('参数验证失败：缺少必要参数');
    return res.status(400).json({ error: '必要参数缺失：operation_type和device_id不能为空' });
  }
  
  // 准备插入数据
  const logData = {
    user_id: userid,
    device_id: device_id,
    operation_type: operation_type,
    operation_result: operation_result || 'success',
    error_message: error_message || null
  };
   
  const query = "INSERT INTO operation_logs SET ?";
   
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ error: '数据库连接失败' });
    }
    
    // 确保错误信息正确记录到数据库中
    // 如果请求中包含错误信息，自动设置操作结果为failure（数据库enum类型要求）
    const finalLogData = {
      ...logData,
      operation_result: error_message ? 'failure' : (operation_result === 'failed' ? 'failure' : (operation_result || 'success')),
      error_message: error_message || null
    };

    
    connection.query(query, finalLogData, (error, results) => {
      connection.release(); // 释放连接回池中
      
      if (error) {
        console.error('添加操作日志失败:', error);
        // 即使记录日志失败，也返回原始操作的错误信息
        return res.status(500).json({ error: '数据库操作失败', details: error.message });
      }
      
      console.log('操作日志添加成功，记录ID:', results.insertId);
      res.json({
        success: true,
        log_id: results.insertId,
        message: '操作日志记录成功'
      });
    })
  })
})



//处理post请求
app.post('/',(req,res) => {
  console.log(req.body)
  res.json(req.body)
})

app.post('/show',(req,res)=>{
  console.log(req.body.name)
  const a=req.body.name
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err)
      return res.status(500).json({error: '数据库连接失败'})
    }
    
    connection.query("select * from users where username=?", [a], function(error,results,fields){
      connection.release()
      
      if(error) {
        console.error('查询错误:', error);
        return res.status(500).json({error: '数据库查询失败'});
      }
      res.json(results)
      console.log(results)
    })
  })
})

app.get('/',(req,res)=>{
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err)
      return res.status(500).json({error: '数据库连接失败'})
    }
    
    // 查询用户表数据
    connection.query('select * from users',function(error,results,fields){
      connection.release()
      
      if(error) {
        console.error('查询错误:', error);
        return res.status(500).json({error: '数据库查询失败'});
      }
      res.json(results)
      console.log('成功查询到用户数据:', results.length, '条记录')
    })
  })
})

// 更新阈值设置接口
app.post('/update_threshold', (req, res) => {
  const { device_id, temp_threshold, soil_humidity_threshold, updated_by } = req.body;
  
  // 验证必要参数
  if (!device_id || temp_threshold === undefined || soil_humidity_threshold === undefined || !updated_by) {
    console.error('参数验证失败：缺少必要参数');
    return res.status(400).json({ error: '必要参数缺失：device_id、temp_threshold、soil_humidity_threshold和updated_by不能为空' });
  }
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ error: '数据库连接失败' });
    }
    
    // 使用单纯的UPDATE语句，不使用INSERT
    const query = `
      UPDATE threshold_settings 
      SET 
        temp_threshold = ?, 
        soil_humidity_threshold = ?, 
        updated_by = ?, 
        updated_at = NOW() 
      WHERE 
        device_id = ?
    `;
    
    // 修改参数顺序，将device_id移到最后作为WHERE条件
    connection.query(query, [temp_threshold, soil_humidity_threshold, updated_by, device_id], (error, results) => {
      console.log(query, [temp_threshold, soil_humidity_threshold, updated_by, device_id])
      connection.release(); // 释放连接回池中
      
      if (error) {
        console.error('更新阈值设置失败:', error);
        return res.status(500).json({ error: '数据库操作失败', details: error.message });
      }
      
      console.log(`设备${device_id}阈值设置更新成功`);
      res.json({
        success: true,
        message: '阈值设置更新成功',
        device_id: device_id
      });
    });
  });
});

app.post('/sensor_data', (req, res) => {
  const {
    device_id,
    device_no,
    temperature,
    humidity,
    light,
    soil_humidity,
    water_pump,
    fan,
    auto_mode,
    user_id,
    collected_at
  } = req.body || {}

  const normalizeDeviceId = (rawDeviceId, rawDeviceNo) => {
    if (rawDeviceId !== null && rawDeviceId !== undefined && rawDeviceId !== '') {
      const asNumber = Number(rawDeviceId)
      if (!Number.isNaN(asNumber) && (asNumber === 1 || asNumber === 2)) {
        return String(asNumber)
      }
      const asString = String(rawDeviceId)
      if (asString.includes('_text2')) return '1'
      if (asString.includes('_test')) return '2'
    }

    if (rawDeviceNo !== null && rawDeviceNo !== undefined && rawDeviceNo !== '') {
      const n = Number(rawDeviceNo)
      if (!Number.isNaN(n) && (n === 1 || n === 2)) return String(n)
    }

    return null
  }

  const normalizedDeviceId = normalizeDeviceId(device_id, device_no)
  if (!normalizedDeviceId) {
    return res.status(400).json({ success: false, message: 'device_id必须为1或2' })
  }

  const toNullableNumber = (v) => {
    if (v === null || v === undefined || v === '') return null
    const n = Number(v)
    return Number.isNaN(n) ? null : n
  }

  const toNullableBoolInt = (v) => {
    if (v === null || v === undefined) return null
    return v ? 1 : 0
  }

  const toDateOrNow = (v) => {
    if (v === null || v === undefined || v === '') return new Date()
    const d = new Date(v)
    if (Number.isNaN(d.getTime())) return new Date()
    return d
  }

  const sql = `
    INSERT INTO soil_data (
      device_id,
      temperature,
      humidity,
      light,
      soil_humidity,
      water_pump,
      fan,
      auto_mode,
      collected_at
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
  `

  const params = [
    normalizedDeviceId,
    toNullableNumber(temperature),
    toNullableNumber(humidity),
    toNullableNumber(light),
    toNullableNumber(soil_humidity),
    toNullableBoolInt(water_pump),
    toNullableBoolInt(fan),
    toNullableBoolInt(auto_mode),
    toDateOrNow(collected_at)
  ]

  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err)
      return res.status(500).json({ success: false, message: '数据库连接失败' })
    }

    connection.query(sql, params, (error, results) => {
      connection.release()

      if (error) {
        console.error('保存传感器数据失败:', error)
        return res.status(500).json({ success: false, message: '数据库操作失败', details: error.message })
      }

      res.json({
        success: true,
        message: 'soil_data插入成功',
        id: results.insertId,
        affectedRows: results.affectedRows
      })
    })
  })
})

// 获取所有设备阈值设置
app.get('/get_thresholds', (req, res) => {
  const query = 'SELECT device_id, temp_threshold, soil_humidity_threshold FROM threshold_settings';
  
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ success: false, message: '数据库连接失败' });
    }
    
    connection.query(query, (error, results) => {
      // 释放连接回池中
      connection.release();
      
      if (error) {
        console.error('查询设备阈值失败:', error);
        return res.status(500).json({ success: false, message: '查询设备阈值失败' });
      }
      
      res.json({ 
        success: true, 
        thresholds: results 
      });
    });
  });
});

// 用户注册API
app.post('/register', (req, res) => {
  let { username, password, user_type, name } = req.body || {};
  user_type = user_type || 'user';
  // 验证必要参数
  if (!username || !password || !user_type) {
    console.error('注册请求参数缺失:', { username, password, user_type });
    return res.status(400).json({ success: false, message: '账号、密码和用户类型不能为空' });
  }
  
  // 验证用户类型是否有效
  const validUserTypes = ['admin', 'user'];
  if (!validUserTypes.includes(user_type)) {
    console.error('无效的用户类型:', user_type);
    // return res.status(400).json({ success: false, message: '无效的用户类型，必须是admin或user' });
  }
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ success: false, message: '数据库连接失败' });
    }
    
    // 先检查用户名是否已存在
    connection.query('SELECT * FROM users WHERE username = ?', [username], (error, results) => {
      console.log(results)
      if (error) {
        connection.release();
        console.error('查询用户失败:', error);
        return res.status(500).json({ success: false, message: '查询用户失败' });
      }
      
      if (results.length > 0) {
        connection.release();
        console.log('账号已存在:', username);
        return res.json({ success: false, message: '账号已存在' });
      }
      
      // 准备插入数据（与用户表结构匹配）
      const userData = {
        username: username.trim(),
        password,
        user_type,
        name: name ? name.trim() : username.trim(),
        // 时间戳由数据库自动处理，不需要手动设置
      };
      
      // 插入新用户
      connection.query('INSERT INTO users SET ?', userData, (insertError, insertResults) => {
        connection.release();
        
        if (insertError) {
          console.error('注册用户失败:', insertError);
          return res.status(500).json({ success: false, message: '注册失败', details: insertError.message });
        }
        
        console.log('用户注册成功:', { username, user_type, userId: insertResults.insertId });
        
        // 注册成功响应
        res.json({
          success: true,
          message: '注册成功',
          username: username,
          userType: user_type,
          userId: insertResults.insertId
        });
      });
    });
  });
});

// 注册操作日志API
app.post('/register_record', (req, res) => {
  console.log('注册日志请求:', req.body);
  const { username, user_type, operation_result, error_message } = req.body;
  
  // 验证必要参数
  if (!username) {
    console.error('注册日志参数验证失败：缺少必要参数');
    return res.status(400).json({ error: '必要参数缺失：username不能为空' });
  }
  
  // 准备插入数据（符合实际表结构）
  const logData = {
    user_id: null, // 注册时用户ID可能还不存在
    device_id: 'system', // 系统操作
    operation_type: 'user_register',
    operation_result: error_message ? 'failure' : (operation_result || 'success'),
    error_message: error_message || null
    // 移除不存在的字段：username和user_type
  };
  
  const query = "INSERT INTO operation_logs SET ?";
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ error: '数据库连接失败' });
    }
    
    connection.query(query, logData, (error, results) => {
      connection.release(); // 释放连接回池中
      
      if (error) {
        console.error('添加注册日志失败:', error);
        return res.status(500).json({ error: '数据库操作失败', details: error.message });
      }
      
      console.log('注册日志添加成功，记录ID:', results.insertId, '用户名:', username, '用户类型:', user_type);
      res.json({
        success: true,
        log_id: results.insertId,
        message: '注册日志记录成功'
      });
    });
  });
});

// 获取操作日志接口 - 使用视图获取包含用户名称的日志数据
app.get('/get_operation_logs', (req, res) => {
  // 获取查询参数
  const { page = 1, pageSize = 20, device_id, operation_type, operation_result, start_date, end_date } = req.query;
  
  // 计算分页偏移量
  const offset = (page - 1) * pageSize;
  
  // 构建基础查询 - 使用新创建的视图
  let query = 'SELECT * FROM operation_user_info_view WHERE 1=1';
  let params = [];
  
  // 添加过滤条件
  if (device_id) {
    query += ' AND device_id = ?';
    params.push(device_id);
  }
  
  if (operation_type) {
    query += ' AND operation_type = ?';
    params.push(operation_type);
  }
  
  if (operation_result) {
    query += ' AND operation_result = ?';
    params.push(operation_result);
  }
  
  if (start_date) {
    query += ' AND created_at >= ?';
    params.push(start_date);
  }
  
  if (end_date) {
    query += ' AND created_at <= ?';
    params.push(end_date);
  }
  
 
  params.push(parseInt(pageSize), parseInt(offset));
  
  // 获取总记录数的查询
  const countQuery = query.replace('SELECT *', 'SELECT COUNT(*) as total');
  const countParams = [...params].slice(0, -2); // 移除分页参数
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ success: false, message: '数据库连接失败' });
    }
    
    // 先查询总记录数
    connection.query(countQuery, countParams, (countError, countResults) => {
      if (countError) {
        connection.release();
        console.error('查询日志总数失败:', countError);
        return res.status(500).json({ success: false, message: '查询日志总数失败' });
      }
      
      const total = countResults[0].total;
      
      // 再查询日志数据
      connection.query(query, params, (error, results) => {
        connection.release(); // 释放连接回池中
        
        if (error) {
          console.error('查询操作日志失败:', error);
          return res.status(500).json({ success: false, message: '查询操作日志失败' });
        }
        
        // 返回分页数据和总数
        res.json({
          success: true,
          logs: results,
          pagination: {
            total,
            page: parseInt(page),
            pageSize: parseInt(pageSize),
            totalPages: Math.ceil(total / pageSize)
          }
        });
      });
    });
  });
});

// 获取设备列表接口
app.get('/get_devices', (req, res) => {
  const query = 'SELECT DISTINCT device_id FROM operation_logs WHERE device_id != "system" ORDER BY device_id';
  
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ success: false, message: '数据库连接失败' });
    }
    
    connection.query(query, (error, results) => {
      connection.release(); // 释放连接回池中
      
      if (error) {
        console.error('查询设备列表失败:', error);
        return res.status(500).json({ success: false, message: '查询设备列表失败' });
      }
      
      // 提取设备ID数组
      const devices = results.map(item => item.device_id);
      
      res.json({
        success: true,
        devices
      });
    });
  });
});

// 清除旧日志接口（需要管理员权限）
app.delete('/clear_operation_logs', (req, res) => {
  const { days = 30 } = req.query;
  
  // 从数据库连接池获取连接
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ success: false, message: '数据库连接失败' });
    }
    
    // 删除指定天数之前的日志
    const query = 'DELETE FROM operation_logs WHERE created_at < DATE_SUB(NOW(), INTERVAL ? DAY)';
    
    connection.query(query, [parseInt(days)], (error, results) => {
      connection.release(); // 释放连接回池中
      
      if (error) {
        console.error('清除旧日志失败:', error);
        return res.status(500).json({ success: false, message: '清除旧日志失败' });
      }
      
      console.log(`成功清除 ${days} 天前的日志，共 ${results.affectedRows} 条记录`);
      
      res.json({
        success: true,
        message: `成功清除 ${days} 天前的日志`,
        affectedRows: results.affectedRows
      });
    });
  });
});

app.get('/thresholds', (req, res) => {
  const query = 'SELECT * FROM threshold_settings';
  
  pool.getConnection((err, connection) => {
    if (err) {
      console.error('数据库连接失败:', err);
      return res.status(500).json({ success: false, message: '数据库连接失败' });
    }
    
    connection.query(query, (error, results) => {
      connection.release(); // 释放连接回池中
      
      if (error) {
        console.error('查询阈值失败:', error);
        return res.status(500).json({ success: false, message: '查询阈值失败' });
      }
      
      // 构建阈值对象
      const thresholds = {};
      results.forEach(row => {
        thresholds[row.device_id] = {
          temp_threshold: row.temp_threshold,
          soil_humidity_threshold: row.soil_humidity_threshold
        };
      });
      
      res.json({
        success: true,
        thresholds
      });
    });
  });
});

app.listen(3000,()=>{
  console.log('server running at http://127.0.0.1:3000')
})
