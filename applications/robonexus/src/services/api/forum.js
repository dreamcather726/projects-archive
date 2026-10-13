import { get, post } from '../http/client.js'

export function listPosts({ page = 1, size = 20, sort = 'latest', category, keyword } = {}) {
  return get('/api/v1/forum/posts', { params: { page, size, sort, category, keyword } })
}

export function getPost(id) {
  return get(`/api/v1/forum/posts/${encodeURIComponent(String(id))}`)
}

export function createPost(payload) {
  return post('/api/v1/forum/posts', payload)
}

export function listComments(postId, { page = 1, size = 50 } = {}) {
  return get(`/api/v1/forum/posts/${encodeURIComponent(String(postId))}/comments`, { params: { page, size }, silent: true })
}

export function createComment(postId, payload) {
  return post(`/api/v1/forum/posts/${encodeURIComponent(String(postId))}/comments`, payload)
}
