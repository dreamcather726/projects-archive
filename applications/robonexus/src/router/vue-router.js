// 简化的 Vue Router 4 实现（只覆盖本项目用到的最小能力）
// 目标：
// 1) 支持 <router-link> / <router-view>
// 2) 支持 history 模式地址切换（push/replace）
// 3) 支持浏览器前进/后退（popstate）
// 4) 支持最基础的动态路由参数：/forum/:id
import { ref, inject, provide, computed, h } from 'vue'

// 用 Symbol 做依赖注入的 key，避免字符串冲突
const RouterSymbol = Symbol()

function escapeRegex(text) {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
}

function compilePath(path) {
  if (path === '/') {
    return { regex: /^\/$/, paramNames: [] }
  }

  const paramNames = []
  const segments = path.split('/').filter(Boolean)
  const patternSegments = segments.map(seg => {
    if (seg.startsWith(':')) {
      const name = seg.slice(1)
      paramNames.push(name)
      return '([^/]+)'
    }
    return escapeRegex(seg)
  })

  const pattern = '^/' + patternSegments.join('/') + '$'
  return { regex: new RegExp(pattern), paramNames }
}

function matchPath(compiled, targetPath) {
  const match = compiled.regex.exec(targetPath)
  if (!match) return null

  const params = {}
  for (let i = 0; i < compiled.paramNames.length; i += 1) {
    params[compiled.paramNames[i]] = decodeURIComponent(match[i + 1] || '')
  }
  return params
}

export function createRouter(options) {
  const rawRoutes = options.routes || []
  const compiledRoutes = rawRoutes.map(route => ({
    ...route,
    _compiled: compilePath(route.path)
  }))

  const router = {
    // 当前路由信息（响应式），变化后会触发 router-view 重新渲染
    currentRoute: ref({ path: '/', name: '', params: {}, query: {} }),
    // 路由表：[{ path, name, component }]
    routes: rawRoutes,
    _compiledRoutes: compiledRoutes,
    // history 实现：createWebHistory/createWebHashHistory
    history: options.history,
    
    // 统一处理 push/replace 传参：既支持 '/path'，也支持 { path: '/path' }
    resolve(to) {
      if (typeof to === 'string') {
        return { path: to }
      }
      return to
    },
    
    // 导航：新增一条历史记录
    push(to) {
      const location = this.resolve(to)
      this.history.push(location.path)
      this.updateRoute(location)
    },
    
    // 导航：替换当前历史记录
    replace(to) {
      const location = this.resolve(to)
      this.history.replace(location.path)
      this.updateRoute(location)
    },
    
    // 根据 location.path 在 routes 里做最简单的“静态路径精确匹配”
    updateRoute(location) {
      const matched = this._compiledRoutes
        .map(r => {
          const params = matchPath(r._compiled, location.path)
          return params ? { route: r, params } : null
        })
        .find(Boolean)

      if (!matched) {
        this.currentRoute.value = { path: location.path, params: {}, query: {} }
        return
      }

      const { route, params } = matched
      this.currentRoute.value = { ...route, path: location.path, params, query: {} }
    },
    
    install(app) {
      // 兼容 Options API：this.$router / this.$route
      app.config.globalProperties.$router = this
      app.config.globalProperties.$route = computed(() => this.currentRoute.value)
      
      // <router-link>：渲染成 <a>，点击后阻止默认跳转，改为 SPA 内部导航
      app.component('router-link', {
        props: ['to'],
        setup(props, { slots, attrs }) {
          const router = inject(RouterSymbol)
          const href = computed(() => {
            if (typeof props.to === 'string') return props.to
            return props.to.path || '/'
          })
          
          const isActive = computed(() => {
            return router.currentRoute.value.path === href.value
          })
          
          const onClick = (e) => {
            // 保留浏览器默认行为：新窗口打开、组合键、非左键点击等
            if (e.defaultPrevented) return
            if (attrs?.target === '_blank') return
            if (e.metaKey || e.altKey || e.ctrlKey || e.shiftKey) return
            if (typeof e.button === 'number' && e.button !== 0) return
            e.preventDefault()
            router.push(props.to)
          }
          
          return () => {
            const classList = []
            if (attrs?.class) classList.push(attrs.class)
            // 仅提供最基础的激活态 class：router-link-active
            if (isActive.value) classList.push('router-link-active')

            return h(
              'a',
              {
                ...attrs,
                href: href.value,
                onClick,
                class: classList.length ? classList : undefined
              },
              slots.default ? slots.default() : href.value
            )
          }
        }
      })
      
      // <router-view>：根据 currentRoute 渲染对应页面组件
      app.component('router-view', {
        setup() {
          const router = inject(RouterSymbol)
          const currentRoute = router.currentRoute
          
          return () => {
            const route = currentRoute.value
            if (!route || !route.component) return null
            
            const Component = route.component
            if (typeof Component === 'function') {
              // 这里暂不实现异步组件（本项目路由目前使用静态 import）
              return null
            }
            return h(Component)
          }
        }
      })
      
      // Provide：让 useRouter/useRoute 以及 router-link/router-view 能拿到 router 实例
      provide(RouterSymbol, this)
      app.provide(RouterSymbol, this)
      
      // 初始化路由：首次进入时根据地址栏 path 渲染页面
      const initialPath = window.location.pathname
      this.updateRoute({ path: initialPath })
      
      // 监听浏览器前进/后退
      window.addEventListener('popstate', () => {
        this.updateRoute({ path: window.location.pathname })
      })
    }
  }
  
  return router
}

export function createWebHistory() {
  return {
    push(path) {
      window.history.pushState({}, '', path)
    },
    replace(path) {
      window.history.replaceState({}, '', path)
    }
  }
}

export function createWebHashHistory() {
  return {
    push(path) {
      window.location.hash = path
    },
    replace(path) {
      window.location.replace(window.location.pathname + '#' + path)
    }
  }
}

export function useRouter() {
  return inject(RouterSymbol)
}

export function useRoute() {
  const router = inject(RouterSymbol)
  return router?.currentRoute || ref({})
}
