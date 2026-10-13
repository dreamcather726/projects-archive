<template>
  <div class="page-container">
    <div class="container">
      <div class="layout">
        <div class="main">
          <section v-if="isDetail" class="detail">
            <div class="detail-header">
              <router-link class="detail-back" to="/forum">返回论坛列表</router-link>
              <div class="detail-id">帖子 ID：{{ postId }}</div>
            </div>

            <h1 class="detail-title">{{ currentPost?.title || '帖子不存在或已被删除' }}</h1>

            <div v-if="currentPost" class="detail-meta">
              <span class="detail-author">作者：{{ currentPost.author }}</span>
              <span class="detail-dot">·</span>
              <span>发布：{{ currentPost.date }}</span>
              <span class="detail-dot">·</span>
              <span>浏览 {{ currentPost.views }}</span>
              <span class="detail-dot">·</span>
              <span>回复 {{ currentPost.replies }}</span>
              <span class="detail-dot">·</span>
              <span>赞 {{ currentPost.likes }}</span>
            </div>

            <template v-if="currentPost && currentDoc">
              <div class="article-head">
                <div class="detail-tags">
                  <span v-for="tag in currentPost.tags || []" :key="tag" class="badge">{{ tag }}</span>
                </div>
                <div class="article-actions">
                  <button class="mini" type="button" @click="onNeedLogin('关注')">关注</button>
                  <button class="mini ghost" type="button" @click="onShare">分享</button>
                </div>
              </div>

              <div class="author-row">
                <div class="author-left">
                  <span class="author-avatar" aria-hidden="true">{{ currentPost.author.slice(0, 1) }}</span>
                  <div class="author-info">
                    <div class="author-name-strong">{{ currentPost.author }}</div>
                    <div class="author-sub">发布于 {{ currentPost.date }} · 阅读 {{ currentPost.views }}</div>
                  </div>
                </div>
                <div class="author-right">
                  <span v-for="b in currentDoc.badges" :key="b" class="author-badge">{{ b }}</span>
                </div>
              </div>

              <div class="article-body">
                <p class="article-p">{{ currentDoc.intro }}</p>
                <div class="callout">{{ currentDoc.callout }}</div>

                <section v-for="s in currentDoc.sections" :key="s.anchor" class="article-section" :id="s.anchor">
                  <h2 class="article-h2">{{ s.title }}</h2>
                  <template v-for="(b, idx) in s.blocks" :key="`${s.anchor}-${idx}`">
                    <p v-if="b.type === 'p'" class="article-p">{{ b.text }}</p>
                    <ul v-else-if="b.type === 'ul'" class="article-ul">
                      <li v-for="(item, i) in b.items" :key="i">{{ item }}</li>
                    </ul>
                    <div v-else-if="b.type === 'code'" class="code-wrap">
                      <button class="code-copy" type="button" @click="copyToClipboard(b.code, `${s.anchor}-${idx}`)">
                        {{ copiedKey === `${s.anchor}-${idx}` ? '已复制' : '复制' }}
                      </button>
                      <pre class="code"><code>{{ b.code }}</code></pre>
                    </div>
                    <div v-else-if="b.type === 'image'" class="img">
                      <div class="img-ph" aria-hidden="true"></div>
                      <div class="img-cap">{{ b.caption }}</div>
                    </div>
                  </template>
                </section>

                <div class="reactions">
                  <button class="reaction" type="button" @click="onNeedLogin('点赞')">赞 {{ currentPost.likes }}</button>
                  <button class="reaction" type="button" @click="onNeedLogin('收藏')">收藏</button>
                  <button class="reaction" type="button" @click="onScrollToComments">评论 {{ currentPost.replies }}</button>
                </div>
              </div>

              <div class="comments">
                <div class="comments-title">社区讨论（{{ currentComments.length }}）</div>
                <div class="comment-box">
                  <textarea v-model="commentDraft" class="comment-input" rows="3" placeholder="写下你的评论（支持 Markdown）…" :disabled="isPostingComment"></textarea>
                  <div class="comment-actions">
                    <button class="comment-btn" type="button" :disabled="isPostingComment" @click="onPublishComment">发布评论</button>
                  </div>
                </div>

                <div class="comment-list">
                  <div v-for="c in currentComments" :key="c.id" class="comment-item">
                    <div class="comment-avatar" aria-hidden="true">{{ c.author.slice(0, 1) }}</div>
                    <div class="comment-main">
                      <div class="comment-head">
                        <div class="comment-author">{{ c.author }}</div>
                        <div class="comment-date">{{ c.date }}</div>
                      </div>
                      <div class="comment-text">{{ c.text }}</div>
                      <div class="comment-meta">
                        <span>赞 {{ c.likes }}</span>
                        <span class="dot">·</span>
                        <span>回复</span>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </template>

            <div v-else class="detail-card">
              <div class="detail-tags">
                <span v-for="tag in currentPost?.tags || []" :key="tag" class="badge">{{ tag }}</span>
              </div>
              <p class="detail-content">{{ currentPost?.excerpt || '你访问的帖子不存在。' }}</p>
            </div>
          </section>

          <template v-else>
            <section class="stats">
              <div class="stat-card">
                <div class="stat-label">今日新帖</div>
                <div class="stat-value">1,284</div>
                <div class="stat-sub">+12.5% 较昨日</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">热门帖子</div>
                <div class="stat-value">452</div>
                <div class="stat-sub">实时更新中</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">活跃用户</div>
                <div class="stat-value">15,402</div>
                <div class="stat-sub">持续增长</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">累计讨论量</div>
                <div class="stat-value">2.4M</div>
                <div class="stat-sub">+5.1% 本周</div>
              </div>
            </section>

            <section class="feed">
              <div class="feed-toolbar">
                <div class="tabs" role="tablist" aria-label="论坛筛选">
                  <button class="tab" :class="{ active: activeTab === 'latest' }" type="button" @click="activeTab = 'latest'">
                    最新发布
                  </button>
                  <button class="tab" :class="{ active: activeTab === 'hot' }" type="button" @click="activeTab = 'hot'">
                    热门回复
                  </button>
                  <button class="tab" :class="{ active: activeTab === 'expert' }" type="button" @click="activeTab = 'expert'">
                    专家建议
                  </button>
                </div>

                <div class="toolbar-right">
                  <label class="select">
                    <span class="select-label">分类</span>
                    <select v-model="activeCategory">
                      <option value="all">全部</option>
                      <option value="robot">机器人</option>
                      <option value="ai">AI</option>
                      <option value="hardware">硬件</option>
                      <option value="career">职业发展</option>
                    </select>
                  </label>
                  <label class="search">
                    <span class="sr-only">搜索</span>
                    <input v-model="keyword" type="search" placeholder="搜索帖子标题/作者…" />
                  </label>
                  <button class="mini" type="button" @click="openCreatePost">发布帖子</button>
                </div>
              </div>

              <div class="post-list">
                <div v-if="isLoadingPosts" class="detail-card">
                  <p class="detail-content">加载中…</p>
                </div>
                <div v-else-if="!visiblePosts.length" class="detail-card">
                  <p class="detail-content">暂无</p>
                </div>
                <router-link v-for="post in visiblePosts" :key="post.id" class="post-card" :to="`/forum/${post.id}`">
                  <div class="post-top">
                    <div class="badges">
                      <span v-for="tag in post.tags" :key="tag" class="badge">{{ tag }}</span>
                    </div>
                    <span class="post-date">{{ post.date }}</span>
                  </div>

                  <div class="post-title">{{ post.title }}</div>
                  <div class="post-excerpt">{{ post.excerpt }}</div>

                  <div class="post-meta">
                    <div class="author">
                      <span class="avatar" aria-hidden="true">{{ post.author.slice(0, 1) }}</span>
                      <span class="author-name">{{ post.author }}</span>
                    </div>
                    <div class="metrics">
                      <span class="metric">浏览 {{ post.views }}</span>
                      <span class="metric">回复 {{ post.replies }}</span>
                      <span class="metric">赞 {{ post.likes }}</span>
                    </div>
                  </div>
                </router-link>
              </div>
            </section>
          </template>
        </div>

        <aside class="sidebar">
          <template v-if="isDetail">
            <section class="panel">
              <div class="panel-title">作者信息</div>
              <div v-if="currentPost" class="author-card">
                <div class="author-card-top">
                  <span class="author-card-avatar" aria-hidden="true">{{ currentPost.author.slice(0, 1) }}</span>
                  <div class="author-card-main">
                    <div class="author-card-name">{{ currentPost.author }}</div>
                    <div class="author-card-sub">帖子：{{ currentPost.replies }} · 赞：{{ currentPost.likes }}</div>
                  </div>
                </div>
                <button class="author-card-btn" type="button">关注作者</button>
              </div>
            </section>

            <section v-if="currentDoc" class="panel">
              <div class="panel-title">文章目录</div>
              <div class="toc">
                <button v-for="s in currentDoc.sections" :key="s.anchor" class="toc-item" type="button" @click="scrollToAnchor(s.anchor)">
                  {{ s.title }}
                </button>
              </div>
            </section>

            <section class="panel">
              <div class="panel-title">相关话题</div>
              <div class="tags">
                <button v-for="t in hotTags.slice(0, 6)" :key="t" class="tag" type="button" @click="keyword = t">
                  {{ t }}
                </button>
              </div>
            </section>
          </template>

          <template v-else>
            <section class="panel connected">
              <div class="panel-title">Connected</div>
              <div class="panel-sub">今日签到（10 KTX）</div>
            </section>

            <section class="panel">
              <div class="panel-header">
                <div class="panel-title">活跃开发者</div>
                <div class="panel-action">今日</div>
              </div>
              <div class="people">
                <div v-for="p in activePeople" :key="p.name" class="person">
                  <span class="person-avatar" aria-hidden="true">{{ p.name.slice(0, 1) }}</span>
                  <div class="person-main">
                    <div class="person-name">{{ p.name }}</div>
                    <div class="person-sub">{{ p.role }}</div>
                  </div>
                  <div class="person-score">{{ p.delta }}</div>
                </div>
              </div>
            </section>

            <section class="panel">
              <div class="panel-title">热门话题</div>
              <div class="tags">
                <button v-for="t in hotTags" :key="t" class="tag" type="button" @click="keyword = t">
                  {{ t }}
                </button>
              </div>
            </section>

            <section class="panel">
              <div class="panel-title">推荐学习</div>
              <div class="recommend">
                <div class="recommend-cover" aria-hidden="true"></div>
                <div class="recommend-main">
                  <div class="recommend-title">从零到一搭建机器人项目</div>
                  <div class="recommend-sub">视频课程 · 12 讲</div>
                </div>
              </div>
            </section>
          </template>
        </aside>
      </div>
    </div>

    <div v-if="createPostOpen" class="compose-mask" @click.self="closeCreatePost">
      <div class="compose-dialog" role="dialog" aria-modal="true" aria-label="发布帖子">
        <div class="compose-head">
          <div class="compose-title">发布帖子</div>
          <button class="compose-close" type="button" @click="closeCreatePost">×</button>
        </div>

        <div class="compose-body">
          <label class="compose-field">
            <div class="compose-label">标题</div>
            <input v-model="createPostForm.title" class="compose-input" placeholder="请输入标题" />
          </label>

          <label class="compose-field">
            <div class="compose-label">分类</div>
            <select v-model="createPostForm.category" class="compose-input">
              <option value="robot">机器人</option>
              <option value="ai">AI</option>
              <option value="hardware">硬件</option>
              <option value="career">职业发展</option>
            </select>
          </label>

          <label class="compose-field">
            <div class="compose-label">标签</div>
            <input v-model="createPostForm.tags" class="compose-input" placeholder="用英文逗号分隔，如 ROS2,SLAM" />
          </label>

          <label class="compose-field">
            <div class="compose-label">内容</div>
            <textarea v-model="createPostForm.content" class="compose-textarea" rows="7" placeholder="支持 Markdown / 富文本（以服务端为准）"></textarea>
          </label>
        </div>

        <div class="compose-actions">
          <button class="mini ghost" type="button" @click="closeCreatePost">取消</button>
          <button class="mini" type="button" :disabled="isCreatingPost" @click="submitCreatePost">发布</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { showToast } from '../../ui/toast.js'
import { createComment, createPost, getPost, listComments, listPosts } from '../../services/api/forum.js'
import { useAuth } from '../../auth.js'

const activeTab = ref('latest')
const activeCategory = ref('all')
const keyword = ref('')
const isLoadingPosts = ref(false)
const isLoadingDetail = ref(false)
const posts = ref([])
const postDetail = ref(null)
const postDoc = ref(null)
const comments = ref([])
const commentDraft = ref('')
const isPostingComment = ref(false)
const createPostOpen = ref(false)
const isCreatingPost = ref(false)
const createPostForm = ref({ title: '', category: 'robot', tags: '', content: '' })

const route = useRoute()
const router = useRouter()
const postId = computed(() => route.params?.id || '')
const isDetail = computed(() => Boolean(postId.value))

const { user, isLoggedIn } = useAuth()

const activePeople = ref([
  { name: 'NeuralLink_Fan', role: '算法工程师', delta: '+36' },
  { name: 'TylerPanda', role: '全栈开发', delta: '+12' },
  { name: 'Matrix_Dev', role: '嵌入式', delta: '+9' }
])

const hotTags = ref(['#SLAM', '#机器人硬件', '#HarmonyOS_NEXT', '#具身智能', '#ROS2', '#仿真', '#控制', '#MLM_Robotics'])

const currentPost = computed(() => postDetail.value)
const currentDoc = computed(() => postDoc.value)
const currentComments = computed(() => comments.value || [])

const copiedKey = ref('')

async function copyToClipboard(text, key) {
  try {
    if (navigator?.clipboard?.writeText) {
      await navigator.clipboard.writeText(text)
    } else {
      const textarea = document.createElement('textarea')
      textarea.value = text
      textarea.setAttribute('readonly', 'true')
      textarea.style.position = 'fixed'
      textarea.style.opacity = '0'
      textarea.style.pointerEvents = 'none'
      document.body.appendChild(textarea)
      textarea.select()
      document.execCommand('copy')
      document.body.removeChild(textarea)
    }
    copiedKey.value = key
    window.setTimeout(() => {
      if (copiedKey.value === key) copiedKey.value = ''
    }, 1200)
  } catch {
    showToast('复制失败，请手动选择文本复制')
  }
}

function scrollToAnchor(anchor) {
  const el = document.getElementById(anchor)
  if (!el) return
  el.scrollIntoView({ behavior: 'smooth', block: 'start' })
}

const visiblePosts = computed(() => {
  const normalizedKeyword = keyword.value.trim().toLowerCase()

  const filtered = posts.value
    .filter(p => (activeCategory.value === 'all' ? true : p.category === activeCategory.value))
    .filter(p => {
      if (!normalizedKeyword) return true
      return (p.title + ' ' + p.author).toLowerCase().includes(normalizedKeyword)
    })

  if (activeTab.value === 'latest') {
    return [...filtered].sort((a, b) => (a.date < b.date ? 1 : -1))
  }
  if (activeTab.value === 'hot') {
    return [...filtered].sort((a, b) => b.replies - a.replies)
  }
  return [...filtered].sort((a, b) => b.likes - a.likes)
})

function stripHtml(input) {
  const s = String(input || '')
  if (!s) return ''
  return s.replace(/<[^>]+>/g, ' ').replace(/\s+/g, ' ').trim()
}

function normalizePostItem(item) {
  return {
    id: item?.id,
    title: item?.title || '',
    excerpt: item?.excerpt || stripHtml(item?.content).slice(0, 120),
    author: item?.author?.name || item?.author?.username || item?.author?.displayName || '匿名',
    date: (item?.createdAt || '').slice(0, 10) || '',
    tags: Array.isArray(item?.tags) ? item.tags : [],
    category: item?.category || 'all',
    views: Number(item?.views) || 0,
    replies: Number(item?.replies) || 0,
    likes: Number(item?.likes) || 0
  }
}

function buildDocFromDetail(detail) {
  const contentText = stripHtml(detail?.content || '')
  return {
    intro: detail?.excerpt || '',
    callout: '',
    badges: [],
    sections: [
      {
        anchor: 'content',
        title: '正文',
        blocks: [{ type: 'p', text: contentText || detail?.excerpt || '' }]
      }
    ]
  }
}

async function fetchPosts() {
  isLoadingPosts.value = true
  try {
    const data = await listPosts({
      page: 1,
      size: 20,
      sort: activeTab.value,
      category: activeCategory.value === 'all' ? undefined : activeCategory.value,
      keyword: keyword.value.trim() || undefined
    })
    const list = Array.isArray(data?.list) ? data.list : []
    posts.value = list.map(normalizePostItem)
  } finally {
    isLoadingPosts.value = false
  }
}

async function fetchDetail(id) {
  if (!id) return
  isLoadingDetail.value = true
  try {
    const data = await getPost(id)
    postDetail.value = normalizePostItem(data)
    postDoc.value = buildDocFromDetail(data)
  } finally {
    isLoadingDetail.value = false
  }
}

async function fetchComments(id) {
  if (!id) return
  try {
    const data = await listComments(id, { page: 1, size: 50 })
    const list = Array.isArray(data?.list) ? data.list : Array.isArray(data) ? data : []
    comments.value = list.map(c => ({
      id: c?.id,
      author: c?.author?.name || c?.author?.username || c?.author || '匿名',
      date: (c?.createdAt || '').slice(0, 19).replace('T', ' ') || '',
      text: c?.text || c?.content || '',
      likes: Number(c?.likes) || 0
    }))
  } catch {
    comments.value = []
  }
}

let listTimer = null
watch([activeTab, activeCategory, keyword], () => {
  if (isDetail.value) return
  if (listTimer) window.clearTimeout(listTimer)
  listTimer = window.setTimeout(() => {
    fetchPosts()
  }, 250)
})

watch(
  () => postId.value,
  (id) => {
    if (!id) {
      postDetail.value = null
      postDoc.value = null
      comments.value = []
      fetchPosts()
      return
    }
    fetchDetail(id)
    fetchComments(id)
  },
  { immediate: true }
)

onMounted(() => {
  if (!isDetail.value) fetchPosts()
})

async function onPublishComment() {
  const id = postId.value
  if (!id) return
  const text = commentDraft.value.trim()
  if (!text) {
    showToast('请输入评论内容')
    return
  }
  if (!ensureLoggedIn('评论')) return
  if (isPostingComment.value) return
  isPostingComment.value = true
  try {
    await createComment(id, { text })
    commentDraft.value = ''
    await fetchComments(id)
  } finally {
    isPostingComment.value = false
  }
}

function openCreatePost() {
  if (!ensureLoggedIn('发帖')) return
  createPostOpen.value = true
}

function closeCreatePost() {
  createPostOpen.value = false
  createPostForm.value = { title: '', category: 'robot', tags: '', content: '' }
}

async function submitCreatePost() {
  if (!ensureLoggedIn('发帖')) return
  const payload = {
    title: createPostForm.value.title.trim(),
    category: createPostForm.value.category,
    tags: createPostForm.value.tags.split(',').map(s => s.trim()).filter(Boolean),
    content: createPostForm.value.content.trim()
  }
  if (!payload.title) {
    showToast('请输入标题')
    return
  }
  if (!payload.content) {
    showToast('请输入内容')
    return
  }
  if (isCreatingPost.value) return
  isCreatingPost.value = true
  try {
    await createPost(payload)
    showToast('发布成功')
    closeCreatePost()
    fetchPosts()
  } finally {
    isCreatingPost.value = false
  }
}

function ensureLoggedIn(actionName) {
  if (isLoggedIn.value) return true
  showToast(`请先登录后再${actionName}`)
  router.push({ path: '/user/login', query: { redirect: route.fullPath || '/forum' } })
  return false
}

function onNeedLogin(actionName) {
  if (!ensureLoggedIn(actionName)) return
  showToast('功能正在开发，敬请期待')
}

function onShare() {
  showToast('功能正在开发，敬请期待')
}

function onScrollToComments() {
  const el = document.querySelector('.comments')
  if (!el) return
  el.scrollIntoView({ behavior: 'smooth', block: 'start' })
}
</script>

<style scoped src="./Forum.css"></style>
