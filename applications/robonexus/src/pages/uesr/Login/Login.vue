<template>
  <div class="auth-page">
    <div class="bg" aria-hidden="true">
      <div class="bubble bubble-a"></div>
      <div class="bubble bubble-b"></div>
    </div>

    <div class="card">
      <router-link class="back" to="/">←</router-link>

      <div class="title">欢迎回来</div>
      <div class="sub">登录以访问你的 RoboNexus 账户</div>

      <form class="form" @submit.prevent="onSubmit">
        <label class="field">
          <div class="label">手机号 / 邮箱</div>
          <input v-model="account" class="input" placeholder="请输入手机号或邮箱" autocomplete="username" />
        </label>

        <label class="field">
          <div class="row">
            <div class="label">密码</div>
          </div>
          <input v-model="password" class="input" type="password" placeholder="请输入密码" autocomplete="current-password" />
        </label>

        <div class="agreement-row">
          <div class="agreement">
            登录即表示你同意
            <a class="link" href="javascript:void(0)">用户协议</a>
            与
            <a class="link" href="javascript:void(0)">隐私政策</a>
          </div>
          <router-link class="link forgot" to="/user/forgot-password">忘记密码？</router-link>
        </div>

        <button class="primary" type="submit">立即登录</button>
      </form>

      <div class="auth-footer">
        没有账号？
        <router-link class="link" to="/user/register">立即注册</router-link>
      </div>
    </div>
  </div>
</template>

<script setup>
import { onMounted, ref } from 'vue'
import { useAuth } from '../../../auth.js'
import { useRouter } from 'vue-router'
import { useRoute } from 'vue-router'
import { showToast } from '../../../ui/toast.js'
import { login } from '../../../services/api/auth.js'

const account = ref('')
const password = ref('')
const router = useRouter()
const route = useRoute()
const { isLoggedIn, setUser, setTokens } = useAuth()
const isSubmitting = ref(false)

onMounted(() => {
  if (isLoggedIn.value) {
    const redirect = route.query?.redirect
    router.replace(typeof redirect === 'string' && redirect ? redirect : '/user/profile')
  }
})

async function onSubmit() {
  if (isLoggedIn.value) {
    const redirect = route.query?.redirect
    router.replace(typeof redirect === 'string' && redirect ? redirect : '/user/profile')
    return
  }
  const a = account.value.trim()
  const p = password.value
  if (!a) {
    showToast('请输入手机号/邮箱')
    return
  }
  if (!p) {
    showToast('请输入密码')
    return
  }

  if (import.meta.env.DEV && a === 'admin' && p === '123') {
    setTokens({ accessToken: 'REDACTED_CONFIGURE_LOCALLY', refreshToken: 'dev-admin-refresh', expiresIn: 24 * 3600 })
    setUser({ username: 'admin', displayName: '管理员', role: 'admin', loginAt: new Date().toISOString() })
    showToast('已使用测试账号登录')
    const redirect = route.query?.redirect
    router.replace(typeof redirect === 'string' && redirect ? redirect : '/user/profile')
    return
  }

  if (isSubmitting.value) return
  isSubmitting.value = true
  try {
    const data = await login({ account: a, password: p })
    setTokens({
      accessToken: data?.accessToken || '',
      refreshToken: data?.refreshToken || '',
      expiresIn: Number(data?.expiresIn) || 0
    })
    setUser({ username: a, displayName: a, role: 'user', loginAt: new Date().toISOString() })
    const redirect = route.query?.redirect
    router.replace(typeof redirect === 'string' && redirect ? redirect : '/user/profile')
  } finally {
    isSubmitting.value = false
  }
}
</script>

<style scoped src="./Login.css"></style>
