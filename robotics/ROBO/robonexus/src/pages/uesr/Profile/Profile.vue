<template>
  <div class="profile-page">
    <div class="container">
      <header class="profile-banner">
        <div class="profile-banner-bg" aria-hidden="true"></div>
        <div class="profile-banner-card">
          <div class="profile-head">
            <div class="avatar" aria-hidden="true">{{ initials }}</div>
            <div class="profile-head-meta">
              <div class="name-row">
                <div class="name">{{ displayName }}</div>
                <span class="badge">{{ roleLabel }}</span>
              </div>
              <div class="sub">
                <span class="muted">账号：{{ user?.username }}</span>
                <span class="dot">·</span>
                <span class="muted">最近登录：{{ lastLoginText }}</span>
              </div>
            </div>

            <div class="profile-head-actions">
              <button class="btn ghost" type="button" @click="onEdit">编辑资料</button>
              <button class="btn" type="button" @click="onCreate">发布内容</button>
              <button class="btn danger" type="button" @click="onLogout">退出登录</button>
            </div>
          </div>

          <div class="profile-stats">
            <button class="stat" type="button" @click="activeTab = 'courses'">
              <div class="stat-value">{{ counts.learningCourses }}</div>
              <div class="stat-label">在学课程</div>
            </button>
            <div class="stat">
              <div class="stat-value">{{ counts.doneLessons }}</div>
              <div class="stat-label">已完成课时</div>
            </div>
            <div class="stat">
              <div class="stat-value">{{ counts.learningHours }}</div>
              <div class="stat-label">学习时长</div>
            </div>
            <div class="stat">
              <div class="stat-value">{{ counts.streakDays }}</div>
              <div class="stat-label">连续学习</div>
            </div>
          </div>

          <div class="profile-actions-row">
            <router-link class="link-btn" to="/courses">去课程中心</router-link>
            <button class="link-btn" type="button" @click="onComingSoon">学习计划</button>
            <button class="link-btn" type="button" @click="onComingSoon">我的证书</button>
          </div>
        </div>
      </header>

      <div class="layout">
        <div class="main">
          <div class="tabs">
            <button class="tab" :class="{ active: activeTab === 'courses' }" type="button" @click="activeTab = 'courses'">课程学习</button>
            <button class="tab" :class="{ active: activeTab === 'posts' }" type="button" @click="activeTab = 'posts'">帖子</button>
            <button class="tab" :class="{ active: activeTab === 'favorites' }" type="button" @click="activeTab = 'favorites'">收藏</button>
            <button class="tab" :class="{ active: activeTab === 'resources' }" type="button" @click="activeTab = 'resources'">资源</button>
            <button class="tab" :class="{ active: activeTab === 'settings' }" type="button" @click="activeTab = 'settings'">设置</button>
          </div>

          <div v-if="activeTab === 'courses'" class="panel">
            <div class="panel-head">
              <div class="panel-title">我的课程</div>
              <label class="search">
                <span class="sr-only">搜索课程</span>
                <input v-model="courseKeyword" type="search" placeholder="搜索课程标题/讲师…" />
              </label>
            </div>

            <div v-if="isLoadingMyCourses" class="empty">课程加载中…</div>

            <div v-else-if="filteredMyCourses.length" class="course-grid">
              <div v-for="c in filteredMyCourses" :key="c.id" class="course-card">
                <div class="course-thumb">
                  <img v-if="c.coverUrl" class="course-thumb-img" :src="c.coverUrl" :alt="c.title" loading="lazy" />
                </div>
                <div class="course-body">
                  <div class="course-title">{{ c.title }}</div>
                  <div class="course-sub muted">{{ c.subtitle }}</div>
                  <div class="course-meta">
                    <span class="tag">{{ c.level }}</span>
                    <span class="dot">·</span>
                    <span class="muted">{{ c.teacher }}</span>
                  </div>

                  <div class="course-progress">
                    <div class="progress-top">
                      <span class="muted">进度</span>
                      <span class="progress-pct">{{ c.progress.percent }}%</span>
                    </div>
                    <div class="progress-bar" role="progressbar" :aria-valuenow="c.progress.percent" aria-valuemin="0" aria-valuemax="100">
                      <div class="progress-fill" :style="{ width: c.progress.percent + '%' }"></div>
                    </div>
                    <div class="progress-sub muted">
                      已完成 {{ c.progress.doneLessons }}/{{ c.progress.totalLessons }} 课时
                      <span v-if="c.progress.lastLesson"> · 上次：{{ c.progress.lastLesson }}</span>
                      <span> · </span>
                      <span class="homework-badge" :class="{ ok: isHomeworkSubmitted(c.id) }">作业{{ isHomeworkSubmitted(c.id) ? '已提交' : '未提交' }}</span>
                    </div>
                  </div>

                  <div class="course-actions">
                    <button class="btn small" type="button" @click="onContinue(c)">继续学习</button>
                    <button class="btn ghost small" type="button" @click="openHomework(c)">提交作业</button>
                  </div>
                </div>
              </div>
            </div>

            <div v-else class="empty">你还没有加入课程，先去课程中心选课吧。</div>
          </div>

          <div v-else-if="activeTab === 'posts'" class="panel">
            <div class="panel-head">
              <div class="panel-title">我的帖子</div>
              <label class="search">
                <span class="sr-only">搜索帖子</span>
                <input v-model="keyword" type="search" placeholder="搜索标题/标签…" />
              </label>
            </div>
            <div v-if="filteredPosts.length" class="list">
              <button v-for="p in filteredPosts" :key="p.id" class="item" type="button" @click="onOpen(p)">
                <div class="item-main">
                  <div class="item-title">{{ p.title }}</div>
                  <div class="item-desc">{{ p.desc }}</div>
                  <div class="item-tags">
                    <span v-for="t in p.tags" :key="t" class="tag">{{ t }}</span>
                  </div>
                </div>
                <div class="item-meta">
                  <div class="meta-line">{{ p.time }}</div>
                  <div class="meta-line muted">{{ p.views }} 阅读 · {{ p.likes }} 赞 · {{ p.comments }} 评论</div>
                </div>
              </button>
            </div>
            <div v-else class="empty">暂无内容</div>
          </div>

          <div v-else-if="activeTab === 'favorites'" class="panel">
            <div class="panel-head">
              <div class="panel-title">我的收藏</div>
              <button class="link-btn" type="button" @click="onComingSoon">管理</button>
            </div>
            <div class="tiles">
              <button v-for="f in favorites" :key="f.id" class="tile" type="button" @click="onOpen(f)">
                <div class="tile-title">{{ f.title }}</div>
                <div class="tile-sub muted">{{ f.desc }}</div>
              </button>
            </div>
          </div>

          <div v-else-if="activeTab === 'resources'" class="panel">
            <div class="panel-head">
              <div class="panel-title">资源与下载</div>
              <button class="link-btn" type="button" @click="onComingSoon">上传资源</button>
            </div>
            <div class="resources">
              <div v-for="r in resources" :key="r.name" class="resource">
                <div class="res-left">
                  <div class="res-name">{{ r.name }}</div>
                  <div class="res-sub muted">{{ r.size }}</div>
                </div>
                <button class="btn ghost small" type="button" @click="onDownload(r)">下载</button>
              </div>
            </div>
          </div>

          <div v-else class="panel">
            <div class="panel-head">
              <div class="panel-title">账户设置</div>
              <button class="link-btn" type="button" @click="onComingSoon">更多设置</button>
            </div>

            <div class="settings">
              <div class="setting">
                <div class="setting-label">昵称</div>
                <div class="setting-value">{{ user?.displayName || '-' }}</div>
              </div>
              <div class="setting">
                <div class="setting-label">登录方式</div>
                <div class="setting-value">账号密码</div>
              </div>
              <div class="setting">
                <div class="setting-label">二次验证</div>
                <button class="link-btn" type="button" @click="onComingSoon">未开启</button>
              </div>
              <div class="setting">
                <div class="setting-label">隐私</div>
                <button class="link-btn" type="button" @click="onComingSoon">管理</button>
              </div>
            </div>
          </div>
        </div>

        <aside class="side">
          <div class="card side-card">
            <div class="side-title">个人数据</div>
            <div class="side-grid">
              <div class="side-kpi">
                <div class="kpi-value">{{ counts.points }}</div>
                <div class="kpi-label">积分</div>
              </div>
              <div class="side-kpi">
                <div class="kpi-value">{{ counts.rank }}</div>
                <div class="kpi-label">排名</div>
              </div>
              <div class="side-kpi">
                <div class="kpi-value">{{ counts.followers }}</div>
                <div class="kpi-label">粉丝</div>
              </div>
              <div class="side-kpi">
                <div class="kpi-value">{{ counts.following }}</div>
                <div class="kpi-label">关注</div>
              </div>
            </div>
          </div>

          <div class="card side-card">
            <div class="side-title">快捷入口</div>
            <div class="quick">
              <button class="quick-item" type="button" @click="onComingSoon">我的帖子</button>
              <button class="quick-item" type="button" @click="onComingSoon">我的评论</button>
              <button class="quick-item" type="button" @click="onComingSoon">消息通知</button>
              <button class="quick-item" type="button" @click="onComingSoon">账号安全</button>
            </div>
          </div>

          <div class="card side-card">
            <div class="side-title">说明</div>
            <div class="hint muted">当前为前端本地登录示例，后续可替换为后端鉴权与用户中心接口。</div>
            <div class="hint muted">测试账号：admin / 123123</div>
          </div>
        </aside>
      </div>

      <div v-if="homeworkOpen" class="upload-mask" @click.self="closeHomework">
        <div class="upload-dialog" role="dialog" aria-modal="true" aria-label="提交作业">
          <div class="upload-head">
            <div class="upload-title">提交作业</div>
            <button class="upload-close" type="button" @click="closeHomework">×</button>
          </div>

          <div class="upload-sub muted">{{ homeworkCourseTitle }}</div>

          <div class="upload-body">
            <div class="upload-section">
              <div class="upload-section-title">辅助文档</div>
              <div class="upload-list">
                <div v-for="d in homeworkDocs" :key="d.id" class="upload-item">
                  <div class="upload-item-main">
                    <div class="upload-item-name">{{ d.name }}</div>
                    <div class="upload-item-sub muted">{{ d.desc }}</div>
                  </div>
                  <button class="link-btn" type="button" @click="markDocRead(d)">{{ isDocRead(d.id) ? '已读' : '阅读' }}</button>
                </div>
              </div>
            </div>

            <div class="upload-section">
              <div class="upload-section-title">作业要求</div>
              <div class="upload-assignment-title">{{ homeworkAssignment.title }}</div>
              <div class="upload-assignment-desc muted">{{ homeworkAssignment.desc }}</div>
              <label class="upload-check">
                <input v-model="homeworkAssignmentRead" type="checkbox" @change="persistHomeworkDraft" />
                <span>我已阅读作业要求</span>
              </label>
            </div>

            <label class="upload-drop" :class="{ disabled: !homeworkCanUpload }">
              <input class="upload-input" type="file" :disabled="!homeworkCanUpload" @change="onHomeworkFileChange" />
              <div class="upload-drop-title">{{ homeworkCanUpload ? '点击选择文件' : '阅读完成后解锁上传' }}</div>
              <div class="upload-drop-sub muted">
                {{ homeworkCanUpload ? '支持 zip/pdf/doc/docx/ppt/pptx/png/jpg（示例）' : '请先完成辅助文档与作业要求阅读' }}
              </div>
            </label>

            <div class="upload-file">
              <div class="upload-file-label muted">已选择：</div>
              <div class="upload-file-name">{{ homeworkFileName }}</div>
            </div>
          </div>

          <div class="upload-actions">
            <button class="btn ghost" type="button" @click="closeHomework">取消</button>
            <button class="btn" type="button" :disabled="!homeworkCanSubmit" @click="submitHomework">提交</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { useAuth } from '../../../auth.js'
import { useRouter } from 'vue-router'
import { showToast } from '../../../ui/toast.js'
import { useCourses } from '../../Courses/coursesStore.js'

const router = useRouter()
const { user, isLoggedIn, clearAuth, setUser } = useAuth()
const { courses, isLoadingList: isLoadingCoursesList, loadCourses } = useCourses()

const activeTab = ref('courses')
const keyword = ref('')
const courseKeyword = ref('')
const homeworkOpen = ref(false)
const homeworkCourseId = ref('')
const homeworkFile = ref(null)

const homeworkCourseTitle = computed(() => {
  const id = homeworkCourseId.value
  if (!id) return ''
  const found = (courses.value || []).find(c => c.id === id)
  return found?.title || ''
})

const homeworkFileName = computed(() => homeworkFile.value?.name || '未选择文件')

const homeworkDocsRead = ref({})
const homeworkAssignmentRead = ref(false)

const homeworkDocs = computed(() => {
  const id = homeworkCourseId.value
  if (!id) return []
  if (id === 'ros2-embodied') {
    return [
      { id: 'ros2-doc-1', name: 'ROS2 工程结构与依赖管理（PDF）', desc: '包组织、colcon、workspace 规范（示例）' },
      { id: 'ros2-doc-2', name: 'QoS 与实时性调参清单（PDF）', desc: '可靠性/延迟/带宽权衡（示例）' }
    ]
  }
  if (id === 'harmonyos-robot') {
    return [
      { id: 'hmos-doc-1', name: 'HarmonyOS 分布式基础（PDF）', desc: '设备发现、协同与状态同步（示例）' },
      { id: 'hmos-doc-2', name: '任务编排案例（PDF）', desc: '任务拆分、容错与回滚（示例）' }
    ]
  }
  if (id === 'vision-perception') {
    return [
      { id: 'vision-doc-1', name: '相机标定与数据链路（PDF）', desc: '标定、同步、预处理（示例）' },
      { id: 'vision-doc-2', name: '部署与性能优化（PDF）', desc: '推理耗时与吞吐优化（示例）' }
    ]
  }
  return [
    { id: 'slam-doc-1', name: 'SLAM 联调排查清单（PDF）', desc: '漂移、重定位与日志分析（示例）' },
    { id: 'slam-doc-2', name: '控制闭环调参指南（PDF）', desc: '稳定性与延迟优化（示例）' }
  ]
})

const homeworkAssignment = computed(() => {
  const id = homeworkCourseId.value
  if (id === 'ros2-embodied') {
    return {
      title: '作业 1：实现一个最小 ROS2 数据链路',
      desc: '完成一个包含 publisher/subscriber 的小工程，并提供运行截图与简短说明（示例）。'
    }
  }
  if (id === 'harmonyos-robot') {
    return {
      title: '作业 1：分布式任务编排 Demo',
      desc: '实现设备发现 + 任务触发流程，提交代码包与说明文档（示例）。'
    }
  }
  if (id === 'vision-perception') {
    return {
      title: '作业 1：视觉模块数据预处理',
      desc: '给出标定参数说明 + 数据预处理流程图，并提供示例输入输出（示例）。'
    }
  }
  return {
    title: '作业 1：SLAM + 控制一体化联调记录',
    desc: '提交一份联调记录：问题 -> 现象 -> 排查 -> 结论（示例）。'
  }
})

const homeworkAllDocsRead = computed(() => {
  const docs = homeworkDocs.value
  if (!docs.length) return true
  return docs.every(d => Boolean(homeworkDocsRead.value?.[d.id]))
})

const homeworkCanUpload = computed(() => homeworkAllDocsRead.value && homeworkAssignmentRead.value)
const homeworkCanSubmit = computed(() => homeworkCanUpload.value && Boolean(homeworkFile.value))

function isDocRead(docId) {
  return Boolean(homeworkDocsRead.value?.[docId])
}

function safeParse(raw) {
  try {
    return JSON.parse(raw)
  } catch {
    return null
  }
}

const progressMap = ref({})

function progressStorageKey() {
  const u = user.value?.username
  if (!u) return null
  return `robonexus_course_progress_${u}`
}

function seedInt(text) {
  const s = String(text || '')
  let x = 0
  for (let i = 0; i < s.length; i += 1) {
    x = (x * 131 + s.charCodeAt(i)) % 100000
  }
  return x
}

function defaultProgressFor(courseId) {
  const base = seedInt(courseId)
  const totalLessons = 18 + (base % 8)
  const doneLessons = Math.min(totalLessons, Math.max(1, Math.floor((base % 60) / 6)))
  return {
    doneLessons,
    totalLessons,
    lastLesson: `第 ${String(Math.max(1, doneLessons)).padStart(2, '0')} 讲`,
    updatedAt: Date.now() - (base % 7) * 86400000
  }
}

function loadProgress() {
  const key = progressStorageKey()
  if (!key) return
  const parsed = safeParse(localStorage.getItem(key) || '')
  if (parsed && typeof parsed === 'object') {
    progressMap.value = parsed
  }
}

function saveProgress() {
  const key = progressStorageKey()
  if (!key) return
  localStorage.setItem(key, JSON.stringify(progressMap.value || {}))
}

function homeworkStorageKey() {
  const u = user.value?.username
  if (!u) return null
  return `robonexus_course_homework_${u}`
}

const homeworkMap = ref({})

function loadHomework() {
  const key = homeworkStorageKey()
  if (!key) return
  const parsed = safeParse(localStorage.getItem(key) || '')
  if (parsed && typeof parsed === 'object') {
    homeworkMap.value = parsed
  }
}

function saveHomework() {
  const key = homeworkStorageKey()
  if (!key) return
  localStorage.setItem(key, JSON.stringify(homeworkMap.value || {}))
}

function persistHomeworkDraft() {
  const courseId = homeworkCourseId.value
  if (!courseId) return
  const existing = homeworkMap.value?.[courseId]
  const nextEntry = {
    ...(existing && typeof existing === 'object' ? existing : {}),
    docsRead: homeworkDocsRead.value || {},
    assignmentRead: homeworkAssignmentRead.value,
    updatedAt: Date.now()
  }
  homeworkMap.value = { ...(homeworkMap.value || {}), [courseId]: nextEntry }
  saveHomework()
}

function markDocRead(doc) {
  if (!doc?.id) return
  homeworkDocsRead.value = { ...(homeworkDocsRead.value || {}), [doc.id]: true }
  persistHomeworkDraft()
  showToast(`已完成阅读：${doc.name}`)
}

function isHomeworkSubmitted(courseId) {
  return Boolean(homeworkMap.value?.[courseId]?.submittedAt)
}

function ensureEnrollment() {
  const u = user.value
  if (!u) return
  if (Array.isArray(u.enrolledCourseIds) && u.enrolledCourseIds.length) return
  const defaults = ['ros2-embodied', 'harmonyos-robot', 'control-slam']
  setUser({ ...u, enrolledCourseIds: defaults })
}

onMounted(async () => {
  if (!isLoggedIn.value) {
    router.replace('/user/login')
    return
  }
  ensureEnrollment()
  loadProgress()
  loadHomework()
  await loadCourses()
})

const isLoadingMyCourses = computed(() => isLoadingCoursesList.value)

const displayName = computed(() => user.value?.displayName || user.value?.username || '用户')
const initials = computed(() => (displayName.value || 'U').slice(0, 1).toUpperCase())
const roleLabel = computed(() => {
  const role = user.value?.role
  if (role === 'admin') return '管理员'
  if (role === 'user') return '普通用户'
  return '用户'
})
const lastLoginText = computed(() => {
  const raw = user.value?.loginAt
  if (!raw) return '-'
  const d = new Date(raw)
  if (Number.isNaN(d.getTime())) return raw
  return d.toLocaleString()
})

const enrolledCourseIds = computed(() => {
  const ids = user.value?.enrolledCourseIds
  if (Array.isArray(ids) && ids.length) return ids
  return []
})

const myCourses = computed(() => {
  if (!enrolledCourseIds.value.length) return []
  const map = new Map()
  for (const c of courses.value || []) {
    map.set(c.id, c)
  }
  return enrolledCourseIds.value.map(id => map.get(id)).filter(Boolean)
})

const myCoursesWithProgress = computed(() => {
  return myCourses.value.map(c => {
    const saved = progressMap.value?.[c.id]
    const p = saved && typeof saved === 'object' ? saved : defaultProgressFor(c.id)
    const percent = p.totalLessons ? Math.round((p.doneLessons / p.totalLessons) * 100) : 0
    return { ...c, progress: { ...p, percent } }
  })
})

const filteredMyCourses = computed(() => {
  const k = courseKeyword.value.trim().toLowerCase()
  const list = [...myCoursesWithProgress.value].sort((a, b) => (b.progress.updatedAt || 0) - (a.progress.updatedAt || 0))
  if (!k) return list
  return list.filter(c => `${c.title} ${c.subtitle} ${c.teacher}`.toLowerCase().includes(k))
})

const posts = ref([
  {
    id: 'p1',
    title: 'ROS2 节点通信模型：从概念到工程实践',
    desc: '围绕 Pub/Sub、Service、Action 与 QoS，搭建可维护的数据流与调试方法。',
    tags: ['ROS2', '架构', '工程化'],
    time: '2026-04-05',
    views: 1260,
    likes: 42,
    comments: 18
  },
  {
    id: 'p2',
    title: '具身智能数据链路：相机/IMU/雷达同步与预处理',
    desc: '以真实项目的“数据可用性”为目标，整理同步策略、时间戳与常见坑。',
    tags: ['具身智能', '传感器', '数据'],
    time: '2026-03-29',
    views: 980,
    likes: 36,
    comments: 12
  },
  {
    id: 'p3',
    title: 'HarmonyOS 分布式协同：任务编排的产品化思路',
    desc: '用“业务场景”倒推系统设计：发现、调度、状态同步与失败恢复。',
    tags: ['HarmonyOS', '分布式', '产品'],
    time: '2026-03-12',
    views: 1440,
    likes: 58,
    comments: 9
  },
  {
    id: 'p4',
    title: '从仿真到真实：控制与 SLAM 一体化联调清单',
    desc: '给出一套可复用的排查路径：定位漂移、控制发散、延迟与资源瓶颈。',
    tags: ['SLAM', '控制', '调试'],
    time: '2026-02-18',
    views: 2110,
    likes: 81,
    comments: 27
  }
])

const favorites = ref([
  { id: 'f1', title: '高质量代码块复制：前端最佳实践', desc: '复制体验、fallback 与提示交互整理。' },
  { id: 'f2', title: 'CSS 命名冲突治理：从全局到模块化', desc: '避免 .hero 类污染的治理方案。' },
  { id: 'f3', title: '课程系统前端：列表到详情的缓存策略', desc: 'computed + store 的轻量方案。' }
])

const resources = ref([
  { name: 'RoboNexus 用户中心接口草案.pdf', size: '示例 · 2.4 MB' },
  { name: '个人中心 UI 组件清单.xlsx', size: '示例 · 0.8 MB' }
])

const filteredPosts = computed(() => {
  const k = keyword.value.trim().toLowerCase()
  if (!k) return posts.value
  return posts.value.filter(p => {
    const hay = `${p.title} ${p.desc} ${(p.tags || []).join(' ')}`.toLowerCase()
    return hay.includes(k)
  })
})

const counts = computed(() => {
  const base = user.value || {}
  const doneLessons = myCoursesWithProgress.value.reduce((acc, c) => acc + (c.progress.doneLessons || 0), 0)
  const learningHours = base.learningHours ?? Math.max(0, Math.round((doneLessons * 0.7 + 3) * 10) / 10)
  return {
    posts: posts.value.length,
    favorites: favorites.value.length,
    resources: resources.value.length,
    visits: base.visits ?? 18624,
    learningCourses: myCoursesWithProgress.value.length,
    doneLessons,
    learningHours,
    streakDays: base.streakDays ?? 6,
    points: base.points ?? 2680,
    rank: base.rank ?? 129,
    followers: base.followers ?? 86,
    following: base.following ?? 42
  }
})

function onLogout() {
  clearAuth()
  router.replace('/user/login')
}

function onEdit() {
  const next = { ...(user.value || {}) }
  next.displayName = next.displayName ? next.displayName : '管理员'
  setUser(next)
  showToast('已更新资料（示例）')
}

function onComingSoon() {
  showToast('功能正在开发，敬请期待')
}

function onCreate() {
  showToast('功能正在开发，敬请期待')
}

function onOpen(item) {
  showToast(`打开：${item.title}`)
}

function onDownload(r) {
  showToast(`开始下载：${r.name}`)
}

function onContinue(course) {
  if (!course?.id) return
  const saved = progressMap.value?.[course.id]
  if (!saved || typeof saved !== 'object') {
    progressMap.value = { ...(progressMap.value || {}), [course.id]: defaultProgressFor(course.id) }
    saveProgress()
  }
  router.push(`/courses/${course.id}`)
}

function openHomework(course) {
  if (!course?.id) return
  homeworkCourseId.value = course.id
  const existing = homeworkMap.value?.[course.id]
  const docsRead = existing?.docsRead
  homeworkDocsRead.value = docsRead && typeof docsRead === 'object' ? docsRead : {}
  homeworkAssignmentRead.value = Boolean(existing?.assignmentRead)
  homeworkFile.value = null
  homeworkOpen.value = true
}

function closeHomework() {
  homeworkOpen.value = false
  homeworkCourseId.value = ''
  homeworkFile.value = null
  homeworkDocsRead.value = {}
  homeworkAssignmentRead.value = false
}

function onHomeworkFileChange(e) {
  const file = e?.target?.files?.[0] || null
  homeworkFile.value = file
}

function submitHomework() {
  if (!homeworkCourseId.value) return
  if (!homeworkCanUpload.value) {
    showToast('请先阅读完成辅助文档与作业要求')
    return
  }
  if (!homeworkFile.value) {
    showToast('请先选择要上传的作业文件')
    return
  }
  const existing = homeworkMap.value?.[homeworkCourseId.value]
  const next = {
    ...(homeworkMap.value || {}),
    [homeworkCourseId.value]: {
      ...(existing && typeof existing === 'object' ? existing : {}),
      docsRead: homeworkDocsRead.value || {},
      assignmentRead: homeworkAssignmentRead.value,
      fileName: homeworkFile.value.name,
      size: homeworkFile.value.size,
      type: homeworkFile.value.type,
      submittedAt: Date.now()
    }
  }
  homeworkMap.value = next
  saveHomework()
  showToast('作业已提交（示例）')
  closeHomework()
}
</script>

<style scoped src="./Profile.css"></style>
