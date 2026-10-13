<template>
  <div class="auth-page">
    <div class="bg" aria-hidden="true">
      <div class="bubble bubble-a"></div>
      <div class="bubble bubble-b"></div>
    </div>

    <div class="card">
      <router-link class="back" to="/user/login">←</router-link>

      <div class="title">找回密码</div>
      <div class="sub">通过绑定的手机号重置你的账户密码</div>

      <div class="steps" aria-label="找回密码步骤">
        <div class="step" :class="{ active: step === 1 }">1</div>
        <div class="line"></div>
        <div class="step" :class="{ active: step === 2 }">2</div>
        <div class="line"></div>
        <div class="step" :class="{ active: step === 3 }">3</div>
      </div>

      <form class="form" @submit.prevent="onNext">
        <label class="field">
          <div class="label">注册手机号</div>
          <div class="phone-row">
            <select v-model="countryCode" class="select">
              <option value="+86">+86</option>
              <option value="+852">+852</option>
              <option value="+853">+853</option>
              <option value="+886">+886</option>
            </select>
            <input v-model="phone" class="input" placeholder="请输入手机号" inputmode="tel" autocomplete="tel" :disabled="step !== 1" />
          </div>
        </label>

        <label class="field">
          <div class="label">短信验证码</div>
          <div class="code-row">
            <input v-model="code" class="input" placeholder="6位验证码" inputmode="numeric" :disabled="step !== 1" />
            <button class="ghost" type="button" :disabled="step !== 1 || smsLeft > 0 || isSending" @click="onSendCode">
              {{ smsLeft > 0 ? `${smsLeft}s` : '获取验证码' }}
            </button>
          </div>
        </label>

        <template v-if="step === 2">
          <label class="field">
            <div class="label">新密码</div>
            <input v-model="newPassword" class="input" type="password" placeholder="请输入新密码" autocomplete="new-password" />
          </label>

          <label class="field">
            <div class="label">确认新密码</div>
            <input v-model="confirmPassword" class="input" type="password" placeholder="请再次输入新密码" autocomplete="new-password" />
          </label>
        </template>

        <button class="primary" type="submit" :disabled="isSubmitting">
          {{ step === 1 ? '下一步' : step === 2 ? '确认重置' : '已完成' }}
        </button>
      </form>

      <div class="auth-footer">
        遇到问题？
        <a class="link" href="javascript:void(0)">联系客服</a>
      </div>
    </div>
  </div>
</template>

<script setup>
import { onBeforeUnmount, ref } from 'vue'
import { useRouter } from 'vue-router'
import { showToast } from '../../../ui/toast.js'
import { resetPassword, sendSms } from '../../../services/api/auth.js'
import { isPasswordStrongEnough, isPhoneLike } from '../../../utils/validators.js'

const countryCode = ref('+86')
const phone = ref('')
const code = ref('')
const newPassword = ref('')
const confirmPassword = ref('')
const step = ref(1)

const smsLeft = ref(0)
const isSending = ref(false)
const isSubmitting = ref(false)
const router = useRouter()
let smsTimer = null

function normalizePhone() {
  return (phone.value || '').trim()
}

function startCountdown() {
  smsLeft.value = 60
  if (smsTimer) window.clearInterval(smsTimer)
  smsTimer = window.setInterval(() => {
    smsLeft.value = Math.max(0, smsLeft.value - 1)
    if (smsLeft.value === 0 && smsTimer) {
      window.clearInterval(smsTimer)
      smsTimer = null
    }
  }, 1000)
}

onBeforeUnmount(() => {
  if (smsTimer) window.clearInterval(smsTimer)
})

async function onSendCode() {
  const p = normalizePhone()
  if (!p) {
    showToast('请输入手机号')
    return
  }
  if (!isPhoneLike(p)) {
    showToast('手机号格式不正确')
    return
  }
  if (smsLeft.value > 0 || isSending.value) return
  isSending.value = true
  try {
    await sendSms({ phone: p, scene: 'reset_password' })
    showToast('验证码已发送')
    startCountdown()
  } finally {
    isSending.value = false
  }
}

async function onNext() {
  if (step.value === 1) {
    const p = normalizePhone()
    if (!p) {
      showToast('请输入手机号')
      return
    }
    if (!isPhoneLike(p)) {
      showToast('手机号格式不正确')
      return
    }
    if (!code.value.trim() || code.value.trim().length !== 6) {
      showToast('请输入6位验证码')
      return
    }
    step.value = 2
    return
  }

  if (step.value !== 2) return
  if (!isPasswordStrongEnough(newPassword.value)) {
    showToast('密码至少6位')
    return
  }
  if (newPassword.value !== confirmPassword.value) {
    showToast('两次密码输入不一致')
    return
  }

  if (isSubmitting.value) return
  isSubmitting.value = true
  try {
    await resetPassword({ phone: normalizePhone(), code: code.value.trim(), newPassword: newPassword.value })
    step.value = 3
    showToast('密码已重置，请登录')
    router.replace('/user/login')
  } finally {
    isSubmitting.value = false
  }
}
</script>

<style scoped src="./ForgotPassword.css"></style>
