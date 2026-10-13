<template>
  <div class="home-page">
    <section class="hero">
      <div class="container">
        <div class="hero-content">
          <div class="hero-text">
            <h2>连接具身智能的无限可能</h2>
            <p>全球领先的机器人及人工智能协作平台，不论是算法工程师、创业团队还是爱好者，在这里都能找到你的连接点。</p>
            <div class="hero-buttons">
              <router-link class="btn-primary" to="/faq">发布需求</router-link>
              <router-link class="btn-secondary" to="/courses">课程中心</router-link>
              
            </div>
          </div>
          <div class="hero-image">
            <div class="image-placeholder">
              <div class="robot-image"></div>
              <div class="stat-badge">
                <span>+124%</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section class="stats">
      <div class="container">
        <div class="stats-grid">
          <div class="stat-item">
            <div class="stat-value">580k</div>
            <div class="stat-label">注册开发者</div>
          </div>
          <div class="stat-item">
            <div class="stat-value">12,400</div>
            <div class="stat-label">兼具智能企业</div>
          </div>
          <div class="stat-item">
            <div class="stat-value">¥2.4B</div>
            <div class="stat-label">总融资规模</div>
          </div>
          <div class="stat-item">
            <div class="stat-value">45,000</div>
            <div class="stat-label">万道技术模型</div>
          </div>
        </div>
      </div>
    </section>

    <section class="core-business">
      <div class="container">
        <h3 class="section-title">五大核心业务板块</h3>
        <div class="business-grid">
          <div class="business-item">
            <div class="business-icon">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none">
                <path
                  d="M16 11C17.6569 11 19 9.65685 19 8C19 6.34315 17.6569 5 16 5C14.3431 5 13 6.34315 13 8C13 9.65685 14.3431 11 16 11Z"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
                <path
                  d="M6 17C7.65685 17 9 15.6569 9 14C9 12.3431 7.65685 11 6 11C4.34315 11 3 12.3431 3 14C3 15.6569 4.34315 17 6 17Z"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
                <path
                  d="M16 19C17.6569 19 19 17.6569 19 16C19 14.3431 17.6569 13 16 13C14.3431 13 13 14.3431 13 16C13 17.6569 14.3431 19 16 19Z"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
                <path
                  d="M8.5 11C8.5 11 10.5 12 12 14C13.5 12 15.5 11 15.5 11"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
                <path d="M12 14V17" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
              </svg>
            </div>
            <h4>社区论坛</h4>
            <p>基于LlamaIndex，融入多种技术模型，打造最活跃的机器人开发者社区。</p>
            <a href="#" class="business-link">相关论坛 →</a>
          </div>
          <div class="business-item">
            <div class="business-icon">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none">
                <path d="M20 7L9 18L4 13" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
              </svg>
            </div>
            <h4>安全稳定</h4>
            <p>分布式架构，多节点部署，保障平台稳定运行，确保数据安全。</p>
            <a href="#" class="business-link">详细保障 →</a>
          </div>
          <div class="business-item active">
            <div class="business-icon">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none">
                <path d="M22 12h-4l-3 9L9 3l-3 9H2" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
              </svg>
            </div>
            <h4>课程学习</h4>
            <p>从入门到精通，涵盖机器人、AI、编程等多个领域的专业课程。</p>
            <a href="#" class="business-link">立即学习 →</a>
          </div>
          <div class="business-item">
            <div class="business-icon">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none">
                <path
                  d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
                <circle cx="9" cy="7" r="4" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
                <path
                  d="M23 21v-2a4 4 0 0 0-3-3.87"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
                <path
                  d="M16 3.13a4 4 0 0 1 0 7.75"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                />
              </svg>
            </div>
            <h4>人才招聘</h4>
            <p>专业的机器人人才招聘平台，连接企业与人才，实现精准匹配。</p>
            <a href="#" class="business-link">全球招聘 →</a>
          </div>
          <div class="business-item">
            <div class="business-icon">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none">
                <rect x="2" y="5" width="20" height="14" rx="2" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
                <path d="M16 10a4 4 0 0 1-8 0" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" />
              </svg>
            </div>
            <h4>金融服务</h4>
            <p>为机器人创业项目提供融资、投资、孵化等全方位金融服务。</p>
            <a href="#" class="business-link">融资详情 →</a>
          </div>
        </div>
      </div>
    </section>

    <section class="courses">
      <div class="container">
        <div class="section-header">
          <h3 class="section-title">热门报名课程</h3>
          <a href="#" class="section-link">查看全部课程 →</a>
        </div>
        <div class="courses-grid">
          <div class="course-card">
            <div class="course-image"></div>
            <div class="course-tag">入门</div>
            <h4 class="course-title">AIoT全程课程</h4>
            <div class="course-info">
              <div class="course-teacher">
                <span class="teacher-avatar"></span>
                <span class="teacher-name">张三</span>
              </div>
              <div class="course-price">¥9980</div>
            </div>
            <router-link class="course-btn" to="/courses/ros2-embodied">报名</router-link>
          </div>
          <div class="course-card">
            <div class="course-image"></div>
            <div class="course-tag">进阶</div>
            <h4 class="course-title">火种计划：小模型开发实战营</h4>
            <div class="course-info">
              <div class="course-teacher">
                <span class="teacher-avatar"></span>
                <span class="teacher-name">李四</span>
              </div>
              <div class="course-price">¥9280</div>
            </div>
            <router-link class="course-btn" to="/courses/vision-perception">报名</router-link>
          </div>
          <div class="course-card">
            <div class="course-image"></div>
            <div class="course-tag">入门</div>
            <h4 class="course-title">少年机甲：灵巧手的奥秘</h4>
            <div class="course-info">
              <div class="course-teacher">
                <span class="teacher-avatar"></span>
                <span class="teacher-name">王五</span>
              </div>
              <div class="course-price">¥6980</div>
            </div>
            <router-link class="course-btn" to="/courses/harmonyos-robot">报名</router-link>
          </div>
          <div class="course-card">
            <div class="course-image"></div>
            <div class="course-tag">进阶</div>
            <h4 class="course-title">ROS2小车+SLAM机械臂智能应用项目</h4>
            <div class="course-info">
              <div class="course-teacher">
                <span class="teacher-avatar"></span>
                <span class="teacher-name">赵六</span>
              </div>
              <div class="course-price">¥9800</div>
            </div>
            <router-link class="course-btn" to="/courses/control-slam">报名</router-link>
          </div>
        </div>
      </div>
    </section>
  </div>
</template>

<style scoped src="./Home.css"></style>
