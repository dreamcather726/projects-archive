import { useAuthStore } from './stores/authStore.js'

export function useAuth() {
  return useAuthStore()
}
