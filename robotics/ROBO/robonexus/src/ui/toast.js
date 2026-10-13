import { reactive } from 'vue'

export const toast = reactive({
  open: false,
  message: '',
  accent: '#234d8f',
  radius: 14,
  shadow: '0 12px 30px rgba(0, 0, 0, 0.18)',
  duration: 180,
  timeout: 1600
})

let timerId = null

export function showToast(message, options = {}) {
  if (timerId) {
    window.clearTimeout(timerId)
    timerId = null
  }

  const styles = getComputedStyle(document.documentElement)
  const tokenAccent = styles.getPropertyValue('--brand-primary')?.trim() || undefined
  const tokenRadius = styles.getPropertyValue('--toast-radius')?.trim() || undefined
  const tokenShadow = styles.getPropertyValue('--toast-shadow')?.trim() || undefined
  const tokenDuration = styles.getPropertyValue('--toast-duration')?.trim() || undefined

  const parsedRadius = tokenRadius ? parseInt(tokenRadius, 10) : NaN
  const parsedDuration = tokenDuration ? parseInt(tokenDuration, 10) : NaN

  toast.message = String(message || '')
  toast.accent = options.accent ?? tokenAccent ?? toast.accent
  toast.radius = options.radius ?? (Number.isFinite(parsedRadius) ? parsedRadius : toast.radius)
  toast.shadow = options.shadow ?? tokenShadow ?? toast.shadow
  toast.duration = options.duration ?? (Number.isFinite(parsedDuration) ? parsedDuration : toast.duration)
  toast.timeout = options.timeout ?? toast.timeout
  toast.open = true

  timerId = window.setTimeout(() => {
    toast.open = false
  }, toast.timeout)
}

export function hideToast() {
  if (timerId) {
    window.clearTimeout(timerId)
    timerId = null
  }
  toast.open = false
}
