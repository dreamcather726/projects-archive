import router from '../../router/index.js'
import { useAuth } from '../../auth.js'
import { showToast } from '../../ui/toast.js'

const DEBUG_HTTP = import.meta.env.DEV && !['0', 'false'].includes(String(import.meta.env.VITE_HTTP_DEBUG || '').toLowerCase())

function joinUrl(base, path) {
  const b = String(base || '').replace(/\/+$/, '')
  const p = String(path || '').replace(/^\/+/, '')
  if (!b) return '/' + p
  return b + '/' + p
}

function toBodyInfo(body) {
  if (body === undefined || body === null) return undefined
  if (typeof FormData !== 'undefined' && body instanceof FormData) return { type: 'FormData' }
  if (typeof Blob !== 'undefined' && body instanceof Blob) return { type: 'Blob', size: body.size, mime: body.type }
  if (typeof body === 'string') return { type: 'string', length: body.length }
  if (typeof body === 'object') return { type: 'object', keys: Object.keys(body) }
  return { type: typeof body }
}

function toQuery(params) {
  if (!params || typeof params !== 'object') return ''
  const sp = new URLSearchParams()
  for (const [k, v] of Object.entries(params)) {
    if (v === undefined || v === null || v === '') continue
    sp.append(k, String(v))
  }
  const s = sp.toString()
  return s ? `?${s}` : ''
}

export class ApiError extends Error {
  constructor(message, { code, requestId, status } = {}) {
    super(message || 'Request Error')
    this.name = 'ApiError'
    this.code = code
    this.requestId = requestId
    this.status = status
  }
}

function formatErrorMessage(message, requestId) {
  const m = String(message || '请求失败')
  const rid = requestId ? `（requestId: ${requestId}）` : ''
  return m + rid
}

function handleAuthFailure() {
  const { clearAuth } = useAuth()
  clearAuth()
  const path = router.currentRoute?.value?.path || router.currentRoute?.path || ''
  if (DEBUG_HTTP) {
    console.warn('[http] auth failure -> redirect to login', { from: path || '/' })
  }
  if (path !== '/user/login') {
    router.replace({ path: '/user/login', query: { redirect: path || '/' } })
  }
}

async function parseJsonSafe(res) {
  try {
    return await res.json()
  } catch {
    return null
  }
}

export async function request(method, url, { params, body, headers, timeoutMs = 12000, silent = false } = {}) {
  const baseURL = import.meta.env.VITE_API_BASE_URL || ''
  const fullUrl = joinUrl(baseURL, url) + toQuery(params)
  const startedAt = Date.now()

  const ctrl = new AbortController()
  const timer = window.setTimeout(() => ctrl.abort(), timeoutMs)

  try {
    const { accessToken } = useAuth()

    const h = new Headers(headers || {})
    if (!h.has('Accept')) h.set('Accept', 'application/json')
    if (accessToken.value) h.set('Authorization', `Bearer ${accessToken.value}`)

    let payload = body
    const isForm = typeof FormData !== 'undefined' && body instanceof FormData
    const isBlob = typeof Blob !== 'undefined' && body instanceof Blob
    if (body !== undefined && body !== null && !isForm && !isBlob && typeof body === 'object') {
      if (!h.has('Content-Type')) h.set('Content-Type', 'application/json')
      payload = JSON.stringify(body)
    }

    if (DEBUG_HTTP) {
      console.debug('[http] request', { method, url, params, body: toBodyInfo(body), timeoutMs })
    }

    const res = await fetch(fullUrl, { method, headers: h, body: payload, signal: ctrl.signal, credentials: 'include' })
    const status = res.status
    const json = await parseJsonSafe(res)
    const durationMs = Date.now() - startedAt

    if (!res.ok) {
      const requestId = json?.requestId
      const message = json?.message || `HTTP ${status}`
      if (status === 401) handleAuthFailure()
      const err = new ApiError(message, { status, requestId, code: json?.code })
      if (DEBUG_HTTP) {
        console.warn('[http] http error', { method, url, status, code: json?.code, requestId, durationMs, message: err.message })
      }
      if (!silent) showToast(formatErrorMessage(err.message, err.requestId))
      throw err
    }

    if (!json || typeof json !== 'object') {
      const err = new ApiError('响应格式错误', { status })
      if (DEBUG_HTTP) {
        console.warn('[http] invalid json', { method, url, status, durationMs })
      }
      if (!silent) showToast(err.message)
      throw err
    }

    const code = json.code
    const message = json.message
    const requestId = json.requestId

    if (code === 0) {
      if (DEBUG_HTTP) {
        console.debug('[http] ok', { method, url, status, code, requestId, durationMs })
      }
      return json.data
    }

    if (code === 40001 || code === 401) handleAuthFailure()
    const err = new ApiError(message || '请求失败', { code, requestId, status })
    if (DEBUG_HTTP) {
      console.warn('[http] api error', { method, url, status, code, requestId, durationMs, message: err.message })
    }
    if (!silent) showToast(formatErrorMessage(err.message, err.requestId))
    throw err
  } catch (e) {
    if (e?.name === 'AbortError') {
      const err = new ApiError('请求超时', {})
      if (DEBUG_HTTP) {
        console.warn('[http] timeout', { method, url, timeoutMs })
      }
      if (!silent) showToast(err.message)
      throw err
    }
    if (!silent && !(e instanceof ApiError)) {
      showToast('网络异常，请稍后重试')
    }
    throw e
  } finally {
    window.clearTimeout(timer)
  }
}

export function get(url, options) {
  return request('GET', url, options)
}

export function post(url, body, options) {
  return request('POST', url, { ...(options || {}), body })
}
