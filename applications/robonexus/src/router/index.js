import { createRouter, createWebHistory } from 'vue-router'
import Home from '../pages/Home/Home.vue'
import Forum from '../pages/Forum/Forum.vue'
import Recruitment from '../pages/Recruitment/Recruitment.vue'
import Courses from '../pages/Courses/Courses.vue'
import CoursesDetail from '../pages/Courses/CoursesDetail.vue'
import FAQ from '../pages/FAQ/FAQ.vue'
import Finance from '../pages/Finance/Finance.vue'
import Login from '../pages/uesr/Login/Login.vue'
import Register from '../pages/uesr/Register/Register.vue'
import ForgotPassword from '../pages/uesr/ForgotPassword/ForgotPassword.vue'
import Profile from '../pages/uesr/Profile/Profile.vue'
import { useAuth } from '../auth.js'

const routes = [
  {
    path: '/',
    name: 'Home',
    component: Home
  },
  {
    path: '/forum',
    name: 'Forum',
    component: Forum
  },
  {
    path: '/forum/:id',
    name: 'ForumDetail',
    component: Forum
  },
  {
    path: '/recruitment',
    name: 'Recruitment',
    component: Recruitment
  },
  {
    path: '/courses',
    name: 'Courses',
    component: Courses
  },
  {
    path: '/courses/:id',
    name: 'CoursesDetail',
    component: CoursesDetail
  },
  {
    path: '/faq',
    name: 'FAQ',
    component: FAQ,
    meta: { requiresAuth: true }
  },
  {
    path: '/finance',
    name: 'Finance',
    component: Finance
  },
  {
    path: '/user/login',
    name: 'Login',
    component: Login
  },
  {
    path: '/user/register',
    name: 'Register',
    component: Register
  },
  {
    path: '/user/forgot-password',
    name: 'ForgotPassword',
    component: ForgotPassword
  },
  {
    path: '/user/profile',
    name: 'UserProfile',
    component: Profile,
    meta: { requiresAuth: true }
  }
]

const router = createRouter({ history: createWebHistory(), routes })

router.beforeEach((to) => {
  if (!to.meta?.requiresAuth) return true
  const { isLoggedIn } = useAuth()
  if (isLoggedIn.value) return true
  return { path: '/user/login', query: { redirect: to.fullPath || to.path } }
})

export default router
