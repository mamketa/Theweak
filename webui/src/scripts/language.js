// Import core translations statically
import enTranslations from '../locales/strings/en.json'
import languages from '../locales/languages.json'

// Cache for translations
const cachedEnglishTranslations = enTranslations
let currentTranslations = null
let currentLang = 'en'

// Dynamic imports for non-English translations
const translationModules = import.meta.glob(
  '../locales/strings/!(en).json',
  { eager: false }
)

// Navbar / static UI translations
const navbarTranslations = {
  en: {
    nav: {
      home: 'Home',
      settings: 'Settings',
      about: 'About'
    },
    about: {
      title: 'About'
    }
  }
  // tambahin language lain kalau perlu
}

// ===============================
// SINGLE SOURCE OF TRUTH
// ===============================
function getTranslationSync(key, ...args) {
  const keys = key.split('.')

  // 1️⃣ Navbar / static translations (priority)
  let value = navbarTranslations[currentLang] || navbarTranslations.en
  for (const k of keys) {
    value = value?.[k]
    if (value === undefined) break
  }

  if (typeof value === 'string') {
    return formatArgs(value, args)
  }

  // 2️⃣ Current language
  if (currentTranslations) {
    value = currentTranslations
    for (const k of keys) {
      value = value?.[k]
      if (value === undefined) break
    }

    if (typeof value === 'string') {
      return formatArgs(value, args)
    }
  }

  // 3️⃣ Fallback to English
  value = cachedEnglishTranslations
  for (const k of keys) {
    value = value?.[k]
    if (value === undefined) break
  }

  if (typeof value === 'string') {
    return formatArgs(value, args)
  }

  // 4️⃣ Ultimate fallback
  return key
}

// Helper for {0}, {1} formatting
function formatArgs(text, args) {
  if (!args.length) return text
  return text.replace(/\{(\d+)\}/g, (m, i) =>
    args[i] !== undefined ? args[i] : m
  )
}

// Expose globally
window.getTranslation = getTranslationSync

// ===============================
// LOADING
// ===============================
async function loadTranslations(lang) {
  if (lang === 'en') return cachedEnglishTranslations

  const filePath = `../locales/strings/${lang}.json`
  if (!translationModules[filePath]) {
    console.warn(`No translation file for ${lang}, fallback to English`)
    return cachedEnglishTranslations
  }

  try {
    const module = await translationModules[filePath]()
    return module.default
  } catch (e) {
    console.error(`Failed to load ${lang} translations`, e)
    return cachedEnglishTranslations
  }
}

// ===============================
// APPLY
// ===============================
function applyTranslations(translations) {
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const key = el.getAttribute('data-i18n')
    el.textContent = getTranslationSync(key)
  })
}

// ===============================
// INIT
// ===============================
async function initI18n() {
  const selector = document.getElementById('languageSelector')
  if (!selector) return

  const allLanguages = { en: 'English', ...languages }

  selector.innerHTML = ''
  for (const [code, name] of Object.entries(allLanguages)) {
    const opt = document.createElement('option')
    opt.value = code
    opt.textContent = name
    selector.appendChild(opt)
  }

  const savedLang = localStorage.getItem('selectedLanguage')
  currentLang = savedLang || 'en'
  selector.value = currentLang

  currentTranslations = await loadTranslations(currentLang)
  applyTranslations(currentTranslations)

  selector.addEventListener('change', async e => {
    const newLang = e.target.value
    const oldLang = currentLang
    const oldTranslations = currentTranslations

    try {
      currentLang = newLang
      currentTranslations = await loadTranslations(newLang)
      applyTranslations(currentTranslations)
      localStorage.setItem('selectedLanguage', newLang)
    } catch (err) {
      console.error('Language switch failed', err)
      currentLang = oldLang
      currentTranslations = oldTranslations
      selector.value = oldLang
    }
  })
}

// ===============================
// BOOT
// ===============================
if (document.readyState !== 'loading') {
  initI18n()
} else {
  document.addEventListener('DOMContentLoaded', initI18n)
}