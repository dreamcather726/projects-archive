import { createApp } from 'vue'
import './style.css'
import App from './App.vue'
import router from './router'

// 应用入口：创建 Vue 应用实例，并把路由挂载到全局
const app = createApp(App)
// 让 <router-link>/<router-view> 在全局可用
app.use(router)
// 把整个应用渲染到 index.html 里的 #app 节点
app.mount('#app')
