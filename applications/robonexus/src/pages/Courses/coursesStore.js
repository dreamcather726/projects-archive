import { computed, ref } from 'vue'

const courses = ref([])
const isLoadingList = ref(false)
const isLoadingDetail = ref(false)

const coursesById = computed(() => {
  const map = new Map()
  for (const c of courses.value) {
    if (c?.id) map.set(c.id, c)
  }
  return map
})

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms))
}

async function loadCourses() {
  if (courses.value.length) return courses.value
  if (isLoadingList.value) return courses.value

  isLoadingList.value = true
  try {
    await sleep(280)

    courses.value = [
      {
        id: 'ros2-embodied',
        title: 'ROS2 具身智能系统架构与实战',
        subtitle: '从系统架构到传感器融合与实时控制，完成具身智能的工程化落地。',
        coverUrl: '/covers/ros2.svg',
        category: 'ros2',
        level: '进阶',
        teacher: 'Dr. Zhang',
        learners: 1240,
        rating: 4.8,
        price: 2980,
        currency: 'RXT'
      },
      {
        id: 'harmonyos-robot',
        title: '基于 HarmonyOS 的具身智能应用开发',
        subtitle: '面向分布式协同场景，构建设备互联与任务编排的机器人应用。',
        coverUrl: '/covers/ros2.svg',
        category: 'harmonyos',
        level: '入门',
        teacher: 'Liu He',
        learners: 3520,
        rating: 4.6,
        price: 0,
        currency: '免费'
      },
      {
        id: 'vision-perception',
        title: '具身智能中的视觉感知：从数据到现实',
        subtitle: '感知数据链路、标定与部署，快速搭建可复用的视觉模块。',
        coverUrl: '/covers/ros2.svg',
        category: 'ai',
        level: '专题',
        teacher: 'Dr. Wang',
        learners: 880,
        rating: 4.7,
        price: 1200,
        currency: 'RXT'
      },
      {
        id: 'control-slam',
        title: '仿真到真实：控制与 SLAM 一体化工程实践',
        subtitle: '完成从 Gazebo 到真机的闭环调试与性能优化。',
        coverUrl: '/covers/ros2.svg',
        category: 'robot',
        level: '进阶',
        teacher: 'Sarah Chen',
        learners: 2300,
        rating: 4.5,
        price: 800,
        currency: 'RXT'
      }
    ]

    return courses.value
  } finally {
    isLoadingList.value = false
  }
}

async function loadCourseDetail(courseId) {
  await loadCourses()
  if (!courseId) return null

  isLoadingDetail.value = true
  try {
    await sleep(220)

    const base = coursesById.value.get(courseId) || null
    if (!base) return null

    if (courseId === 'ros2-embodied') {
      return {
        ...base,
        breadcrumb: '课程中心 · 机器人课程 · ROS2 · 具身智能系统架构',
        ratingCount: 1240,
        priceSub: 'RXT/套（含证书）',
        buyHint: '本课程支持试看部分章节；购买后解锁全部内容与资料下载。',
        videoHint: '课程导学与案例预览（示例）',
        videoTime: '12:45 / 38:20',
        intro:
          '本课程面向机器人/具身智能方向的工程师与学习者，重点解决“从算法到系统”的落地问题。你将掌握 ROS2 的节点通信、参数与生命周期管理、数据流与多传感器融合的工程实践，并学会构建可维护、可扩展的机器人软件架构。',
        goals: [
          '理解 ROS2 通信模型与节点架构，能够独立搭建工程骨架',
          '掌握常见传感器（LiDAR、相机、IMU）数据接入与同步',
          '完成 SLAM + 路径规划 + 控制链路的端到端联调',
          '具备工程化调参、日志分析与性能优化能力'
        ],
        downloads: [
          { name: 'ROS2 架构设计指南.pdf', ext: 'pdf', size: '12.8 MB' },
          { name: '课程配套源码仓库', ext: 'git', size: 'GitHub Repository' }
        ],
        outline: [
          {
            title: '第一部分：系统架构基础',
            lessons: [
              { no: '01', title: '具身智能软件架构与 ROS2 基础', time: '15:02', done: true },
              { no: '02', title: 'Docker 与 ROS2 Humble 环境配置', time: '28:45', done: true },
              { no: '03', title: '核心通信机制：Pub/Sub 深度解析', time: '38:20', done: false }
            ]
          },
          {
            title: '第二部分：感知与 SLAM',
            lessons: [
              { no: '04', title: '多源传感器同步与数据预处理', time: '19:08', done: false },
              { no: '05', title: '基于 Cartographer 的 2D 建图实战', time: '32:14', done: false },
              { no: '06', title: '定位与重定位策略', time: '21:36', done: false }
            ]
          }
        ],
        teacherDetail: {
          name: 'Dr. Zhang',
          role: '具身智能系统架构师 · 机器人研发负责人',
          desc: '专注机器人系统工程化落地与性能优化，主导多个具身智能项目的架构设计与上线交付。'
        }
      }
    }

    return {
      ...base,
      breadcrumb: '课程中心 · RoboNexus 学院',
      ratingCount: Math.round((base.learners || 0) / 2),
      priceSub: base.currency === '免费' ? '免费课程' : `${base.currency}/套`,
      buyHint: '当前为示例课程详情，后续可接入后端加载完整信息。',
      videoHint: '课程预览（示例）',
      videoTime: '03:12 / 24:00',
      intro: '这是一个示例课程详情页，用于验证课程列表到详情页的跳转、参数传递与数据加载流程。',
      goals: ['完成课程列表到详情页的导航', '实现基础的状态管理与数据缓存', '保证响应式显示一致'],
      downloads: [{ name: '课程资料包.zip', ext: 'zip', size: '3.2 MB' }],
      outline: [
        {
          title: '第一部分：概览',
          lessons: [
            { no: '01', title: '课程介绍与学习路线', time: '08:12', done: false },
            { no: '02', title: '关键概念与准备工作', time: '15:48', done: false }
          ]
        }
      ],
      teacherDetail: { name: base.teacher, role: '讲师', desc: '讲师信息示例，后续由后端返回。' }
    }
  } finally {
    isLoadingDetail.value = false
  }
}

export function useCourses() {
  return {
    courses,
    coursesById,
    isLoadingList,
    isLoadingDetail,
    loadCourses,
    loadCourseDetail
  }
}
