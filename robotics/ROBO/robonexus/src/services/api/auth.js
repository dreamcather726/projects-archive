import { post } from '../http/client.js'

export function login({ account, password }) {
  return post('/api/v1/auth/login', { account, password })
}

export function sendSms({ phone, scene }) {
  return post('/api/v1/auth/sms/send', { phone, scene })
}

export function register({ phone, code, password }) {
  return post('/api/v1/auth/register', { phone, code, password })
}

export function resetPassword({ phone, code, newPassword }) {
  return post('/api/v1/auth/password/reset', { phone, code, newPassword })
}

