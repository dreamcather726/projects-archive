<script setup>
import { computed } from 'vue'
import Navbar from './components/Navbar.vue'
import Footer from './components/Footer.vue'
import Toast from './components/Toast.vue'
import { useRoute } from 'vue-router'

const route = useRoute()
const showLayout = computed(() => {
  const path = route.path || '/'
  const hiddenPaths = ['/user/login', '/user/register', '/user/forgot-password']
  return !hiddenPaths.includes(path)
})
</script>

<template>
  <div id="app">
    <Navbar v-if="showLayout" />
    <main class="app-main" :class="{ 'with-navbar': showLayout }">
      <router-view />
    </main>
    <Footer v-if="showLayout" />
    <Toast />
  </div>
</template>
