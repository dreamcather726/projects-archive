<template>
  <header class="navbar">
    <div class="container">
      <div class="navbar-content">
        <div class="logo">
          <router-link class="brand-link" to="/">
            <img class="brand-icon" src="/favicon.svg" alt="RoboNexus" />
            <h1>RoboNexus</h1>
          </router-link>
        </div>
        <nav class="nav-menu">
          <ul>
            <li><router-link to="/">首页</router-link></li>
            <li><router-link to="/forum">社区论坛</router-link></li>
            <li><router-link to="/courses">课程学习</router-link></li>
            <li><router-link to="/faq">需求悬赏</router-link></li>
            <li><button class="nav-link" type="button" @click="onComingSoon">人才招聘</button></li>
            <li><button class="nav-link" type="button" @click="onComingSoon">金融投资</button></li>
          </ul>
        </nav>
        <div class="navbar-actions">
          <template v-if="isLoggedIn">
            <router-link class="user" to="/user/profile">
              <span class="user-avatar" aria-hidden="true">{{ initials }}</span>
              <span class="user-name">{{ displayName }}</span>
            </router-link>
            <button class="btn-logout" type="button" @click="onLogout">退出</button>
          </template>
          <router-link v-else class="btn-login" to="/user/login">登录/注册</router-link>
          <button class="btn-search">
            <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
              <path d="M7.33333 12.6667C10.2789 12.6667 12.6667 10.2789 12.6667 7.33333C12.6667 4.38778 10.2789 2 7.33333 2C4.38778 2 2 4.38778 2 7.33333C2 10.2789 4.38778 12.6667 7.33333 12.6667Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
              <path d="M14 14L10.6667 10.6667" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>
          </button>
        </div>
      </div>
    </div>
  </header>
</template>

<script setup>
import { computed } from 'vue'
import { useAuth } from '../auth.js'
import { useRouter } from 'vue-router'
import { showToast } from '../ui/toast.js'

const router = useRouter()
const { user, isLoggedIn, clearAuth } = useAuth()

const displayName = computed(() => user.value?.displayName || user.value?.username || '用户')
const initials = computed(() => (displayName.value || 'U').slice(0, 1).toUpperCase())

function onLogout() {
  clearAuth()
  router.push('/user/login')
}

function onComingSoon() {
  showToast('功能正在开发，敬请期待')
}
</script>

<style scoped>
.brand-link {
  display: flex;
  align-items: center;
  gap: 10px;
  text-decoration: none;
  color: inherit;
}

.brand-icon {
  width: 28px;
  height: 28px;
  display: block;
}

.logo a {
  text-decoration: none;
  color: inherit;
}

.nav-menu a {
  display: inline-block;
  text-decoration: none;
  color: #333;
  font-size: 14px;
  font-weight: 500;
  padding: 8px 0;
  transition: color 0.3s;
}

.nav-link {
  display: inline-block;
  text-decoration: none;
  color: #333;
  font-size: 14px;
  font-weight: 500;
  padding: 8px 0;
  transition: color 0.3s;
  border: none;
  background: transparent;
  cursor: pointer;
}

.nav-menu a:hover,
.nav-menu a.router-link-active {
  color: #2b5ba9;
}

.nav-link:hover {
  color: #2b5ba9;
}

.btn-login {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 8px 16px;
  border: 1px solid #2b5ba9;
  background-color: #ffffff;
  color: #2b5ba9;
  border-radius: 4px;
  font-size: 14px;
  cursor: pointer;
  transition: all 0.3s;
  text-decoration: none;
}

.btn-login:hover {
  background-color: #2b5ba9;
  color: #ffffff;
}

.user {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  text-decoration: none;
  color: #111827;
  padding: 6px 10px;
  border-radius: 10px;
  border: 1px solid rgba(0, 0, 0, 0.06);
  background: #fff;
}

.user:hover {
  border-color: rgba(43, 91, 169, 0.25);
  background: rgba(43, 91, 169, 0.04);
}

.user-avatar {
  width: 28px;
  height: 28px;
  border-radius: 10px;
  background: rgba(35, 77, 143, 0.12);
  color: #234d8f;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-weight: 900;
  font-size: 12px;
}

.user-name {
  font-size: 13px;
  font-weight: 800;
}

.btn-logout {
  height: 34px;
  padding: 0 12px;
  border-radius: 10px;
  border: 1px solid rgba(0, 0, 0, 0.06);
  background: #fff;
  cursor: pointer;
  font-size: 13px;
  font-weight: 800;
  color: #111827;
}

.btn-logout:hover {
  border-color: rgba(220, 38, 38, 0.25);
  background: rgba(220, 38, 38, 0.06);
  color: #dc2626;
}
</style>
