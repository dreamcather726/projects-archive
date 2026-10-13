<template>
  <div class="courses-list-page">
    <div class="container">
      <header class="courses-hero">
        <div class="hero-block hero-main">
          <div class="courses-hero-title">
            <div class="brand">RoboNexus 学院：</div>
            <div class="slogan">重塑你的具身智能技能栈</div>
          </div>
          <div class="courses-hero-sub">从基础工程化到前沿 HarmonyOS/具身智能系统架构，与你一起 580K+ 开发者共同进阶。</div>
          <div class="courses-hero-actions">
            <label class="search">
              <span class="sr-only">搜索课程</span>
              <input v-model="keyword" type="search" placeholder="搜索课程：具身智能、ROS2、HarmonyOS…" />
            </label>
            <button class="create" type="button" @click="onCreate">立即开课</button>
          </div>
        </div>

        <div class="hero-block hero-stats">
          <div class="stats">
            <div class="stat">
              <div class="stat-value">1,240+</div>
              <div class="stat-label">课程评价</div>
            </div>
            <div class="stat">
              <div class="stat-value">85,600+</div>
              <div class="stat-label">学习者</div>
            </div>
            <div class="stat">
              <div class="stat-value">42,000+</div>
              <div class="stat-label">学习笔记</div>
            </div>
            <div class="stat">
              <div class="stat-value">350+</div>
              <div class="stat-label">讲师与作者</div>
            </div>
          </div>
        </div>
      </header>

      <div class="filters">
        <div class="filter-block">
          <div class="filter-label">课程分类：</div>
          <div class="filter-row">
            <button class="pill" :class="{ active: activeCategory === 'all' }" type="button" @click="activeCategory = 'all'">全部</button>
            <button class="pill" :class="{ active: activeCategory === 'ros2' }" type="button" @click="activeCategory = 'ros2'">ROS2</button>
            <button class="pill" :class="{ active: activeCategory === 'harmonyos' }" type="button" @click="activeCategory = 'harmonyos'">HarmonyOS</button>
            <button class="pill" :class="{ active: activeCategory === 'ai' }" type="button" @click="activeCategory = 'ai'">AI 大模型</button>
            <button class="pill" :class="{ active: activeCategory === 'robot' }" type="button" @click="activeCategory = 'robot'">机器人</button>
          </div>
          <div class="filter-break" aria-hidden="true"></div>
          <div class="filter-label">难度等级：</div>
          <div class="filter-row">
            <button class="pill" :class="{ active: activeLevel === 'all' }" type="button" @click="activeLevel = 'all'">全部</button>
            <button class="pill" :class="{ active: activeLevel === '入门' }" type="button" @click="activeLevel = '入门'">入门</button>
            <button class="pill" :class="{ active: activeLevel === '进阶' }" type="button" @click="activeLevel = '进阶'">进阶</button>
            <button class="pill" :class="{ active: activeLevel === '专题' }" type="button" @click="activeLevel = '专题'">专题</button>
          </div>
        </div>
      </div>

      <section class="section">
        <div class="section-head">
          <div class="section-title">为你推荐的课程</div>
          <div class="section-right">
            <div class="section-sub">根据你的浏览与学习偏好智能推荐（示例）</div>
            <label class="select">
              <span class="select-label">排序方式</span>
              <select v-model="sortBy">
                <option value="hot">热度优先</option>
                <option value="new">最新更新</option>
                <option value="rating">评分优先</option>
              </select>
            </label>
          </div>
        </div>

        <div v-if="isLoadingList" class="grid">
          <div v-for="i in 8" :key="i" class="card skeleton">
            <div class="thumb"></div>
            <div class="line"></div>
            <div class="line short"></div>
            <div class="meta"></div>
          </div>
        </div>

        <div v-else-if="!visibleCourses.length" class="empty-state">暂无</div>

        <div v-else class="grid">
          <router-link v-for="c in pageCourses" :key="c.id" class="card" :to="`/courses/${c.id}`">
            <div class="thumb">
              <img v-if="c.coverUrl" class="thumb-img" :src="c.coverUrl" :alt="c.title" loading="lazy" />
            </div>
            <div class="card-body">
              <div class="card-title">{{ c.title }}</div>
              <div class="card-sub">{{ c.subtitle }}</div>
              <div class="card-meta">
                <span class="tag">{{ c.level }}</span>
                <span class="dot">·</span>
                <span class="teacher">{{ c.teacher }}</span>
                <span class="dot">·</span>
                <span class="learners">{{ c.learners }} 学习</span>
              </div>
              <div class="card-bottom">
                <div class="price">{{ c.currency === '免费' ? '免费' : `${c.price} ${c.currency}` }}</div>
                <div class="rating">{{ c.rating }} ★</div>
              </div>
            </div>
          </router-link>
        </div>

        <div v-if="!isLoadingList && pageCount > 1" class="pager">
          <button class="pager-btn" type="button" :disabled="page === 1" @click="page = page - 1">上一页</button>
          <button v-for="p in pageCount" :key="p" class="pager-num" :class="{ active: p === page }" type="button" @click="page = p">
            {{ p }}
          </button>
          <button class="pager-btn" type="button" :disabled="page === pageCount" @click="page = page + 1">下一页</button>
        </div>
      </section>

      <section class="mentors">
        <div class="mentors-title">跟随行业大咖学习</div>
        <div class="mentors-sub">来自 RoboNexus 学院的明星讲师与作者（示例）</div>
        <div class="mentors-grid">
          <div v-for="m in mentors" :key="m.name" class="mentor">
            <div class="mentor-avatar" aria-hidden="true"></div>
            <div class="mentor-name">{{ m.name }}</div>
            <div class="mentor-role">{{ m.role }}</div>
          </div>
        </div>
      </section>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { useCourses } from './coursesStore.js'

const { courses, isLoadingList, loadCourses } = useCourses()

const keyword = ref('')
const activeCategory = ref('all')
const activeLevel = ref('all')
const sortBy = ref('hot')
const page = ref(1)
const pageSize = 8

loadCourses()

watch([keyword, activeCategory, activeLevel, sortBy], () => {
  page.value = 1
})

const visibleCourses = computed(() => {
  const k = keyword.value.trim().toLowerCase()
  let list = courses.value

  if (activeCategory.value !== 'all') {
    list = list.filter(c => c.category === activeCategory.value)
  }
  if (activeLevel.value !== 'all') {
    list = list.filter(c => c.level === activeLevel.value)
  }
  if (k) {
    list = list.filter(c => (c.title + ' ' + c.subtitle + ' ' + c.teacher).toLowerCase().includes(k))
  }

  if (sortBy.value === 'rating') {
    list = [...list].sort((a, b) => (b.rating || 0) - (a.rating || 0))
  } else if (sortBy.value === 'new') {
    list = [...list].sort((a, b) => (a.id > b.id ? -1 : 1))
  } else {
    list = [...list].sort((a, b) => (b.learners || 0) - (a.learners || 0))
  }

  return list
})

const pageCount = computed(() => Math.max(1, Math.ceil(visibleCourses.value.length / pageSize)))

const pageCourses = computed(() => {
  const start = (page.value - 1) * pageSize
  const list = visibleCourses.value.slice(start, start + pageSize)
  if (list.length <= 4) return list
  return list.slice(0, 4)
})

const mentors = ref([
  { name: 'Dr. Alex Rivera', role: 'AIoT · 具身智能系统架构' },
  { name: '陈教授', role: '机器人控制 · 强化学习' },
  { name: '山田一郎', role: 'ROS2 工程化 · SLAM' },
  { name: 'Emma Watson', role: 'HarmonyOS · 分布式协同' }
])

function onCreate() {
  alert('开课功能待接入后端')
}
</script>

<style scoped src="./CoursesList.css"></style>
