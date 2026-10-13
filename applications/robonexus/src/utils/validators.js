export function isPhoneLike(phone) {
  const s = String(phone || '').trim()
  return /^1\d{10}$/.test(s) || /^\d{7,15}$/.test(s)
}

export function isPasswordStrongEnough(pw) {
  const s = String(pw || '')
  return s.length >= 6
}

