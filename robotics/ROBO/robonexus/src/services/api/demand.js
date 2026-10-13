import { get, post } from '../http/client.js'

export function listDemands({ page = 1, size = 20, sort = 'latest', status, keyword } = {}) {
  return get('/api/v1/demands', { params: { page, size, sort, status, keyword } })
}

export function createDemand(payload) {
  return post('/api/v1/demands', payload)
}

