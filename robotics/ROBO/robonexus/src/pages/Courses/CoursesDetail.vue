<template>
  <div class="courses-page">
    <div class="container">
      <div v-if="isLoadingDetail" class="loading">
        <div class="loading-title"></div>
        <div class="loading-sub"></div>
        <div class="loading-video"></div>
      </div>

      <div v-else-if="!course" class="empty">
        <div class="empty-title">课程不存在或已下架</div>
        <router-link class="empty-link" to="/courses">返回课程列表</router-link>
      </div>

      <div v-else class="layout">
        <main class="main">
          <header class="header">
            <div class="meta-row">
              <div class="breadcrumb">{{ course.breadcrumb }}</div>
              <div class="rating">
                <span class="stars" aria-label="评分">★★★★★</span>
                <span class="score">{{ course.rating }}</span>
                <span class="count">（{{ course.ratingCount }} 评价）</span>
              </div>
            </div>
            <h1 class="title">{{ course.title }}</h1>
            <p class="desc">{{ course.subtitle }}</p>
          </header>

          <section class="video-card">
            <div class="video">
              <button class="play" type="button" @click="onPlay">▶</button>
              <div class="video-info">
                <div class="video-line">{{ course.videoHint }}</div>
                <div class="video-time">{{ course.videoTime }}</div>
              </div>
            </div>
          </section>

          <nav class="tabs" aria-label="课程导航">
            <button class="tab" :class="{ active: activeTab === 'detail' }" type="button" @click="activeTab = 'detail'">课程详情</button>
            <button class="tab" :class="{ active: activeTab === 'reviews' }" type="button" @click="activeTab = 'reviews'">课程评价（{{ reviews.length }}）</button>
            <button class="tab" :class="{ active: activeTab === 'discuss' }" type="button" @click="activeTab = 'discuss'">讨论区（{{ comments.length }}）</button>
          </nav>

          <section v-if="activeTab === 'detail'" class="content">
            <div class="card">
              <div class="card-title">课程介绍</div>
              <div class="card-body">
                <p class="p">{{ course.intro }}</p>
              </div>
            </div>

            <div class="card">
              <div class="card-title">学习目标</div>
              <div class="card-body">
                <div class="goals">
                  <div v-for="(g, i) in course.goals" :key="i" class="goal">
                    <span class="dot" aria-hidden="true"></span>
                    <span class="goal-text">{{ g }}</span>
                  </div>
                </div>
              </div>
            </div>

            <div class="card">
              <div class="card-title">资料下载</div>
              <div class="card-body">
                <div class="downloads">
                  <div v-for="f in course.downloads" :key="f.name" class="download">
                    <div class="file">
                      <div class="file-icon" aria-hidden="true">{{ f.ext.toUpperCase() }}</div>
                      <div class="file-main">
                        <div class="file-name">{{ f.name }}</div>
                        <div class="file-sub">{{ f.size }}</div>
                      </div>
                    </div>
                    <button class="dl-btn" type="button" @click="onDownload(f)">下载</button>
                  </div>
                </div>
              </div>
            </div>
          </section>

          <section v-else-if="activeTab === 'reviews'" class="content">
            <div class="card">
              <div class="card-title">课程评价</div>
              <div class="card-body">
                <div class="review-list">
                  <div v-for="r in reviews" :key="r.id" class="review">
                    <div class="avatar" aria-hidden="true">{{ r.author.slice(0, 1) }}</div>
                    <div class="review-main">
                      <div class="review-head">
                        <div class="review-author">{{ r.author }}</div>
                        <div class="review-date">{{ r.date }}</div>
                      </div>
                      <div class="review-stars" aria-label="评分">{{ '★★★★★'.slice(0, r.stars) }}{{ '★★★★★'.slice(r.stars).replace(/./g, '☆') }}</div>
                      <div class="review-text">{{ r.text }}</div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </section>

          <section v-else class="content">
            <div class="card">
              <div class="card-title">讨论区</div>
              <div class="card-body">
                <div class="comment-box">
                  <textarea v-model="draft" class="comment-input" rows="3" placeholder="写下你的讨论内容（支持 Markdown）…"></textarea>
                  <div class="comment-actions">
                    <button class="comment-btn" type="button" @click="onPost">发布讨论</button>
                  </div>
                </div>

                <div class="comment-list">
                  <div v-for="c in comments" :key="c.id" class="comment">
                    <div class="avatar" aria-hidden="true">{{ c.author.slice(0, 1) }}</div>
                    <div class="comment-main">
                      <div class="comment-head">
                        <div class="comment-author">{{ c.author }}</div>
                        <div class="comment-date">{{ c.date }}</div>
                      </div>
                      <div class="comment-text">{{ c.text }}</div>
                      <div class="comment-meta">
                        <span>赞 {{ c.likes }}</span>
                        <span class="sep">·</span>
                        <button class="link-btn" type="button">回复</button>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </section>
        </main>

        <aside class="side">
          <section class="card buy">
            <div class="buy-top">
              <div class="price">{{ course.currency === '免费' ? '免费' : course.price }}</div>
              <div class="price-sub">{{ course.priceSub }}</div>
            </div>
            <button class="buy-btn" type="button" @click="onBuy">立即购买并开始学习</button>
            <div class="buy-hint">{{ course.buyHint }}</div>
          </section>

          <section class="card outline">
            <div class="outline-head">
              <div class="outline-title">课程大纲</div>
              <div class="outline-progress">
                已完成 {{ outlineProgress }}
                <span class="sep">·</span>
                <span class="outline-lock" :class="{ ok: homeworkSubmitted }">{{ homeworkSubmitted ? '作业已提交' : '提交作业解锁后续课时' }}</span>
              </div>
            </div>
            <div class="outline-body">
              <div v-for="(g, gi) in course.outline" :key="gi" class="outline-group">
                <div class="outline-group-title">{{ g.title }}</div>
                <button
                  v-for="(l, li) in g.lessons"
                  :key="l.no"
                  class="lesson"
                  :class="{ locked: isLessonLocked(gi, li) }"
                  type="button"
                  :disabled="isLessonLocked(gi, li)"
                  @click="onSelectLesson(l, gi, li)"
                >
                  <span class="lesson-left">
                    <span class="lesson-dot" :class="{ done: l.done }" aria-hidden="true"></span>
                    <span class="lesson-no">{{ l.no }}</span>
                    <span class="lesson-title">{{ l.title }}</span>
                    <span v-if="isLessonLocked(gi, li)" class="lesson-lock">作业解锁</span>
                  </span>
                  <span class="lesson-time">{{ l.time }}</span>
                </button>
              </div>
            </div>
          </section>

          <section class="card teacher">
            <div class="teacher-title">授课讲师</div>
            <div class="teacher-card">
              <div class="teacher-top">
                <div class="teacher-avatar" aria-hidden="true">{{ course.teacherDetail.name.slice(0, 1) }}</div>
                <div class="teacher-main">
                  <div class="teacher-name">{{ course.teacherDetail.name }}</div>
                  <div class="teacher-sub">{{ course.teacherDetail.role }}</div>
                </div>
              </div>
              <div class="teacher-desc">{{ course.teacherDetail.desc }}</div>
              <button class="teacher-btn" type="button" @click="onFollow">关注讲师</button>
            </div>
          </section>
        </aside>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { useRoute } from 'vue-router'
import { useCourses } from './coursesStore.js'
import { useAuth } from '../../auth.js'
import { showToast } from '../../ui/toast.js'

const route = useRoute()
const { isLoadingDetail, loadCourseDetail } = useCourses()
const { user, isLoggedIn } = useAuth()

const activeTab = ref('detail')
const draft = ref('')
const course = ref(null)
const homeworkSubmitted = ref(false)

const reviews = ref([
  { id: 'r1', author: 'HarmonyCoder', date: '2天前', stars: 5, text: '结构清晰，工程实践部分很实用，尤其是容器化部署与日志分析。' },
  { id: 'r2', author: 'RoboEngineer_01', date: '5天前', stars: 4, text: '讲解细致，配套资料完善，希望后续能增加更多真实项目案例。' }
])

const comments = ref([
  { id: 'c1', author: 'Matrix_Master', date: '7小时前', text: '请问 Humble 与 Foxy 的差异在工程上主要体现在哪些点？', likes: 12 },
  { id: 'c2', author: 'Curiosity_User', date: '3小时前', text: 'SLAM 部分如果用 LIO-SAM，课程里的接口如何替换？', likes: 4 }
])

const outlineProgress = computed(() => {
  const outline = course.value?.outline || []
  const total = outline.reduce((acc, g) => acc + g.lessons.length, 0)
  const done = outline.reduce((acc, g) => acc + g.lessons.filter(l => l.done).length, 0)
  return `${done}/${total}`
})

function safeParse(raw) {
  try {
    return JSON.parse(raw)
  } catch {
    return null
  }
}

function homeworkStorageKey() {
  const u = user.value?.username
  if (!u) return null
  return `robonexus_course_homework_${u}`
}

function refreshHomeworkState(courseId) {
  if (!courseId) return
  if (!isLoggedIn.value) {
    homeworkSubmitted.value = false
    return
  }
  const key = homeworkStorageKey()
  if (!key) {
    homeworkSubmitted.value = false
    return
  }
  const parsed = safeParse(localStorage.getItem(key) || '')
  const record = parsed && typeof parsed === 'object' ? parsed?.[courseId] : null
  homeworkSubmitted.value = Boolean(record?.submittedAt)
}

function lessonGlobalIndex(gi, li) {
  const outline = course.value?.outline || []
  let idx = 0
  for (let i = 0; i < gi; i += 1) {
    idx += outline[i]?.lessons?.length || 0
  }
  return idx + li
}

function isLessonLocked(gi, li) {
  if (homeworkSubmitted.value) return false
  return lessonGlobalIndex(gi, li) >= 2
}

async function refresh() {
  const id = route.params?.id
  if (!id) return
  activeTab.value = 'detail'
  course.value = await loadCourseDetail(id)
  refreshHomeworkState(id)
  window.scrollTo({ top: 0, behavior: 'smooth' })
}

watch(
  () => route.params?.id,
  () => {
    refresh()
  },
  { immediate: true }
)

function onPlay() {
  alert('视频播放功能待接入后端')
}

function onBuy() {
  alert('购买功能待接入后端')
}

function onDownload(file) {
  alert(`下载：${file.name}（待接入后端）`)
}

function onSelectLesson(lesson, gi, li) {
  if (isLessonLocked(gi, li)) {
    showToast(isLoggedIn.value ? '请先提交作业解锁后续课时' : '请先登录并提交作业解锁后续课时')
    return
  }
  alert(`播放课时：${lesson.no}. ${lesson.title}（待接入后端）`)
}

function onFollow() {
  alert('关注讲师功能待接入后端')
}

function onPost() {
  const text = draft.value.trim()
  if (!text) return
  comments.value.unshift({ id: String(Date.now()), author: 'admin', date: '刚刚', text, likes: 0 })
  draft.value = ''
}
</script>

<style scoped src="./Courses.css"></style>
