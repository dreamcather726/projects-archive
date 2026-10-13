<template>
  <div class="auth-page">
    <div class="bg" aria-hidden="true">
      <div class="bubble bubble-a"></div>
      <div class="bubble bubble-b"></div>
    </div>

    <div class="card">
      <router-link class="back" to="/user/login">←</router-link>

      <div class="title">开启探索之旅</div>
      <div class="sub">加入全球 580K+ 开发者，共建更强大的智能未来</div>

      <form class="form" @submit.prevent="onSubmit">
        <label class="field">
          <div class="label">手机号</div>
          <div class="phone-row">
            <select v-model="countryCode" class="select">
              <option value="+86">+86</option>
              <option value="+852">+852</option>
              <option value="+853">+853</option>
              <option value="+886">+886</option>
            </select>
            <input v-model="phone" class="input" placeholder="请输入手机号" inputmode="tel" autocomplete="tel" />
          </div>
        </label>

        <label class="field">
          <div class="label">验证码</div>
          <div class="code-row">
            <input v-model="code" class="input" placeholder="6位数字" inputmode="numeric" />
            <button class="ghost" type="button" :disabled="smsLeft > 0 || isSending" @click="onSendCode">
              {{ smsLeft > 0 ? `${smsLeft}s` : '获取验证码' }}
            </button>
          </div>
        </label>

        <label class="field">
          <div class="label">设置密码</div>
          <input v-model="password" class="input" type="password" placeholder="请输入密码" autocomplete="new-password" />
        </label>

        <label class="field">
          <div class="label">确认密码</div>
          <input v-model="confirmPassword" class="input" type="password" placeholder="请再次输入密码" autocomplete="new-password" />
        </label>

        <div class="agreement">
          注册即表示你同意
          <a class="link" href="javascript:void(0)">用户协议</a>
          与
          <a class="link" href="javascript:void(0)">隐私政策</a>
        </div>

        <button class="primary" type="submit">立即注册</button>
      </form>

      <div class="auth-footer">
        已有账号？
        <router-link class="link" to="/user/login">返回登录</router-link>
      </div>
    </div>
  </div>
</template>

<script setup>
import { onBeforeUnmount, ref } from 'vue'
import { useRouter } from 'vue-router'
import { showToast } from '../../../ui/toast.js'
import { register, sendSms } from '../../../services/api/auth.js'
import { isPasswordStrongEnough, isPhoneLike } from '../../../utils/validators.js'

const countryCode = ref('+86')
const phone = ref('')
const code = ref('')
const password = ref('')
const confirmPassword = ref('')
const router = useRouter()

const smsLeft = ref(0)
const isSending = ref(false)
const isSubmitting = ref(false)
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
    await sendSms({ phone: p, scene: 'register' })
    showToast('验证码已发送')
    startCountdown()
  } finally {
    isSending.value = false
  }
}

async function onSubmit() {
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
  if (!isPasswordStrongEnough(password.value)) {
    showToast('密码至少6位')
    return
  }
  if (password.value !== confirmPassword.value) {
    showToast('两次密码输入不一致')
    return
  }
  if (isSubmitting.value) return
  isSubmitting.value = true
  try {
    await register({ phone: p, code: code.value.trim(), password: password.value })
    showToast('注册成功，请登录')
    router.replace('/user/login')
  } finally {
    isSubmitting.value = false
  }
}
</script>

<style scoped src="./Register.css"></style>
