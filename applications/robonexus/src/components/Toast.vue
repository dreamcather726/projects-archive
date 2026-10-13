<template>
  <transition name="toast">
    <div v-if="toast.open" class="toast" :style="styleVars" role="status" aria-live="polite">
      <div class="toast-body">{{ toast.message }}</div>
    </div>
  </transition>
</template>

<script setup>
import { computed } from 'vue'
import { toast } from '../ui/toast.js'

const styleVars = computed(() => ({
  '--toast-accent': toast.accent,
  '--toast-radius': `${toast.radius}px`,
  '--toast-shadow': toast.shadow,
  '--toast-duration': `${toast.duration}ms`
}))
</script>

<style scoped>
.toast {
  position: fixed;
  top: calc(var(--navbar-height, 70px) + 14px);
  left: 50%;
  transform: translateX(-50%);
  z-index: 2000;
  max-width: calc(100vw - 24px);
  background: var(--toast-accent);
  border-radius: var(--toast-radius);
  box-shadow: var(--toast-shadow);
  padding: 10px 14px;
}

.toast-body {
  color: #ffffff;
  font-size: 14px;
  font-weight: 700;
  line-height: 1.4;
  letter-spacing: 0.2px;
  text-align: center;
  white-space: nowrap;
}

.toast-enter-active,
.toast-leave-active {
  transition: opacity var(--toast-duration), transform var(--toast-duration);
}

.toast-enter-from,
.toast-leave-to {
  opacity: 0;
  transform: translateX(-50%) translateY(-6px);
}

.toast-enter-to,
.toast-leave-from {
  opacity: 1;
  transform: translateX(-50%) translateY(0);
}
</style>
