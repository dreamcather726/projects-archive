import { computed, ref } from 'vue'

const STORAGE_KEY = 'robonexus_auth'
const LEGACY_USER_KEY = 'robonexus_user'
const DEBUG_AUTH = import.meta.env.DEV && !['0', 'false'].includes(String(import.meta.env.VITE_AUTH_DEBUG || '').toLowerCase())

function safeParse(raw) {
  try {
    return JSON.parse(raw)
  } catch {
    return null
  }
}

function readStored() {
  const parsed = safeParse(localStorage.getItem(STORAGE_KEY) || '')
  if (parsed && typeof parsed === 'object') return parsed
  const legacy = safeParse(localStorage.getItem(LEGACY_USER_KEY) || '')
  if (legacy && typeof legacy === 'object') {
    return { user: legacy }
  }
  return {}
}

const state = ref(readStored())

const user = computed(() => state.value?.user || null)
const accessToken = computed(() => state.value?.accessToken || '')
const refreshToken = computed(() => state.value?.refreshToken || '')
const expiresAt = computed(() => state.value?.expiresAt || 0)

const isLoggedIn = computed(() => Boolean(accessToken.value))

function persist(next) {
  state.value = next
  localStorage.setItem(STORAGE_KEY, JSON.stringify(next))
  if (next?.user) {
    localStorage.setItem(LEGACY_USER_KEY, JSON.stringify(next.user))
  } else {
    localStorage.removeItem(LEGACY_USER_KEY)
  }
}

function setUser(nextUser) {
  const next = { ...(state.value || {}) }
  next.user = nextUser
  persist(next)
  if (DEBUG_AUTH) {
    console.debug('[auth] setUser', { username: nextUser?.username || nextUser?.phone || '' })
  }
}

function setTokens({ accessToken, refreshToken, expiresIn } = {}) {
  const next = { ...(state.value || {}) }
  if (typeof accessToken === 'string') next.accessToken = accessToken
  if (typeof refreshToken === 'string') next.refreshToken = refreshToken
  if (typeof expiresIn === 'number' && Number.isFinite(expiresIn)) {
    next.expiresAt = Date.now() + expiresIn * 1000
  }
  persist(next)
  if (DEBUG_AUTH) {
    console.debug('[auth] setTokens', { hasAccessToken: Boolean(accessToken), hasRefreshToken: Boolean(refreshToken), expiresIn })
  }
}

function clearAuth() {
  persist({})
  if (DEBUG_AUTH) {
    console.debug('[auth] clearAuth')
  }
}

export function useAuthStore() {
  return {
    user,
    accessToken,
    refreshToken,
    expiresAt,
    isLoggedIn,
    setUser,
    setTokens,
    clearAuth
  }
}

