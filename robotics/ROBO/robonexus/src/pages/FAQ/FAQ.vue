<template>
  <div class="bounty-page">
    <div class="bounty-top">
      <div class="container">
        <div class="top-row">
          <div class="brand">
            <span class="brand-mark" aria-hidden="true">k</span>
            <span class="brand-text">RoboNexus 悬赏令</span>
          </div>
          <div class="top-actions">
            <button class="ghost" type="button" @click="onComingSoon">当前悬赏</button>
            <button class="ghost" type="button" @click="onComingSoon">英雄榜单</button>
            <button class="ghost" type="button" @click="onComingSoon">完成记录</button>
            <button class="primary" type="button" @click="openCreate">发布需求令</button>
          </div>
        </div>
        <div class="ticker">
          <span class="dot" aria-hidden="true"></span>
          <span class="ticker-text">紧急悬赏：XX机器人公司算法未达成指标需下周 SLAM 稳定闭环调参，当前赏金已累计至 ¥150,000！</span>
        </div>
      </div>
    </div>

    <div class="container">
      <header class="bounty-hero">
        <div class="bounty-hero-title">RoboNexus 悬赏令</div>
        <div class="bounty-hero-sub">连接全球顶尖开发者与高难度技术需求，通过区块链技术保障赏金结算，助力具身智能、AI 算法与机器人硬件的协作优化。</div>
      </header>

      <div class="toolbar">
        <div class="tabs" role="tablist" aria-label="需求分类">
          <button v-for="t in tabs" :key="t.key" class="tab" :class="{ active: activeTab === t.key }" type="button" @click="activeTab = t.key">
            {{ t.label }}
          </button>
        </div>
        <div class="sort">
          <span class="sort-label">排序：</span>
          <select v-model="sortBy" class="sort-select">
            <option value="latest">latest</option>
            <option value="reward">reward</option>
          </select>
        </div>
      </div>

      <section class="grid">
        <div v-if="isLoading" class="card" style="grid-column: 1 / -1;">
          <div class="title">加载中…</div>
        </div>
        <div v-else-if="!visibleBounties.length" class="card" style="grid-column: 1 / -1;">
          <div class="title">暂无</div>
        </div>
        <template v-else>
          <article v-for="b in visibleBounties" :key="b.id" class="card">
            <div class="card-top">
              <span class="status">{{ b.status }}</span>
              <div class="reward">{{ b.reward }}</div>
            </div>

            <div class="title">{{ b.title }}</div>
            <div class="org">
              <span class="org-dot" aria-hidden="true"></span>
              <span class="org-text">{{ b.org }}</span>
            </div>

            <div class="tags">
              <span v-for="tag in b.tags" :key="tag" class="tag">{{ tag }}</span>
            </div>

            <div class="meta">
              <div class="meta-row">
                <span class="meta-k">截止日期：</span>
                <span class="meta-v">{{ b.deadline }}</span>
              </div>
              <div class="meta-row">
                <span class="meta-k">已交付任务：</span>
                <span class="meta-v">{{ b.delivered }}</span>
              </div>
            </div>

            <button class="btn" type="button" @click="onComingSoon">查看详情</button>
          </article>
        </template>
      </section>
    </div>

    <div v-if="createOpen" class="create-mask" @click.self="closeCreate">
      <div class="create-dialog" role="dialog" aria-modal="true" aria-label="发布需求令">
        <div class="create-head">
          <div class="create-title">发布需求令</div>
          <button class="create-close" type="button" @click="closeCreate">×</button>
        </div>
        <div class="create-body">
          <label class="create-field">
            <div class="create-label">标题</div>
            <input v-model="form.title" class="create-input" placeholder="请输入需求标题" />
          </label>
          <label class="create-field">
            <div class="create-label">机构/团队</div>
            <input v-model="form.org" class="create-input" placeholder="请输入发布方名称" />
          </label>
          <div class="create-row">
            <label class="create-field">
              <div class="create-label">赏金</div>
              <input v-model="form.reward" class="create-input" placeholder="例如 15000 RXT" />
            </label>
            <label class="create-field">
              <div class="create-label">截止日期</div>
              <input v-model="form.deadline" class="create-input" placeholder="YYYY-MM-DD" />
            </label>
          </div>
          <label class="create-field">
            <div class="create-label">标签</div>
            <input v-model="form.tags" class="create-input" placeholder="用英文逗号分隔，如 LLM,Vision,ROS2" />
          </label>
          <label class="create-field">
            <div class="create-label">说明</div>
            <textarea v-model="form.desc" class="create-textarea" rows="4" placeholder="描述需求背景、交付物与验收标准"></textarea>
          </label>
        </div>
        <div class="create-actions">
          <button class="ghost" type="button" @click="closeCreate">取消</button>
          <button class="primary" type="button" :disabled="isSubmitting" @click="submitCreate">提交</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { showToast } from '../../ui/toast.js'
import { createDemand, listDemands } from '../../services/api/demand.js'

const tabs = [
  { key: 'all', label: '全部需求' },
  { key: 'control', label: '机器人控制' },
  { key: 'planning', label: '计算机视觉' },
  { key: 'llm', label: 'AI 大模型' },
  { key: 'hardware', label: '硬件设计' },
  { key: 'data', label: '数据采集' }
]

const activeTab = ref('all')
const sortBy = ref('latest')

const bounties = ref([])
const isLoading = ref(false)

function normalizeDemand(item) {
  const tags = Array.isArray(item?.tags) ? item.tags : typeof item?.tags === 'string' ? item.tags.split(',').map(s => s.trim()).filter(Boolean) : []
  const reward = item?.reward || item?.bounty || item?.budget || ''
  const delivered = item?.delivered ?? item?.deliveredCount ?? item?.submissions ?? 0
  return {
    id: item?.id,
    status: item?.status || '招募中',
    reward: reward ? String(reward) : '-',
    title: item?.title || '',
    org: item?.org || item?.organization || item?.company || '',
    tags,
    deadline: (item?.deadline || item?.dueAt || item?.dueDate || '').slice(0, 10),
    delivered: String(delivered)
  }
}

async function fetchDemands() {
  isLoading.value = true
  try {
    const data = await listDemands({ page: 1, size: 20, sort: sortBy.value })
    const list = Array.isArray(data?.list) ? data.list : []
    bounties.value = list.map(normalizeDemand)
  } finally {
    isLoading.value = false
  }
}

onMounted(() => {
  fetchDemands()
})

watch(sortBy, () => {
  fetchDemands()
})

const visibleBounties = computed(() => {
  let list = bounties.value

  if (activeTab.value === 'llm') {
    list = list.filter(b => b.tags.some(t => t.toLowerCase().includes('llm')))
  } else if (activeTab.value === 'planning') {
    list = list.filter(b => b.tags.some(t => t.toLowerCase().includes('vision')))
  } else if (activeTab.value === 'hardware') {
    list = list.filter(b => b.tags.some(t => t.includes('硬件')))
  } else if (activeTab.value === 'control') {
    list = list.filter(b => b.title.includes('机械臂') || b.title.includes('控制'))
  } else if (activeTab.value === 'data') {
    list = list.filter(b => b.tags.some(t => t.includes('数据')))
  }

  if (sortBy.value === 'reward') {
    return [...list].sort((a, b) => (a.reward < b.reward ? 1 : -1))
  }
  return [...list].reverse()
})

const createOpen = ref(false)
const isSubmitting = ref(false)
const form = ref({ title: '', org: '', reward: '', deadline: '', tags: '', desc: '' })

function openCreate() {
  createOpen.value = true
}

function closeCreate() {
  createOpen.value = false
  form.value = { title: '', org: '', reward: '', deadline: '', tags: '', desc: '' }
}

async function submitCreate() {
  const payload = {
    title: form.value.title.trim(),
    org: form.value.org.trim(),
    reward: form.value.reward.trim(),
    deadline: form.value.deadline.trim(),
    tags: form.value.tags.split(',').map(s => s.trim()).filter(Boolean),
    desc: form.value.desc.trim()
  }
  if (!payload.title) {
    showToast('请输入标题')
    return
  }
  if (!payload.org) {
    showToast('请输入机构/团队')
    return
  }
  if (!payload.deadline) {
    showToast('请输入截止日期')
    return
  }
  if (isSubmitting.value) return
  isSubmitting.value = true
  try {
    await createDemand(payload)
    showToast('发布成功')
    closeCreate()
    fetchDemands()
  } finally {
    isSubmitting.value = false
  }
}

function onComingSoon() {
  showToast('功能正在开发，敬请期待')
}
</script>

<style scoped src="./FAQ.css"></style>
