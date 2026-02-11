// Navigation System for WebUI

class NavigationManager {
  constructor() {
    this.currentPage = 'home';
    this.pages = ['home', 'settings', 'about'];
    this.init();
  }

  init() {
    // Initialize navbar buttons
    this.pages.forEach(page => {
      const button = document.getElementById(`nav_${page}`);
      if (button) {
        button.addEventListener('click', () => this.switchPage(page));
      }
    });

    // Clone language selector to all pages
    this.cloneLanguageSelectors();
    
    // Add social link handlers
    this.setupSocialLinks();
    
    // Update version in about page
    this.updateAboutPage();
  }

  switchPage(page) {
    // Update active button
    this.pages.forEach(p => {
      const button = document.getElementById(`nav_${p}`);
      const pageElement = document.getElementById(`${p}_page`);
      
      if (button) {
        if (p === page) {
          button.classList.add('active');
        } else {
          button.classList.remove('active');
        }
      }
      
      if (pageElement) {
        if (p === page) {
          pageElement.classList.remove('hidden');
        } else {
          pageElement.classList.add('hidden');
        }
      }
    });
    
    this.currentPage = page;
    
    // Refresh page-specific content
    this.refreshPageContent(page);
  }

  cloneLanguageSelectors() {
    const mainSelector = document.getElementById('languageSelector');
    if (!mainSelector) return;
    
    // Clone to settings page
    const settingsSelector = document.getElementById('languageSelector2');
    const aboutSelector = document.getElementById('languageSelector3');
    
    if (settingsSelector && mainSelector.innerHTML) {
      settingsSelector.innerHTML = mainSelector.innerHTML;
      settingsSelector.value = mainSelector.value;
      settingsSelector.addEventListener('change', (e) => {
        mainSelector.value = e.target.value;
        mainSelector.dispatchEvent(new Event('change'));
      });
      
      // Sync changes from main selector
      mainSelector.addEventListener('change', (e) => {
        settingsSelector.value = e.target.value;
      });
    }
    
    if (aboutSelector && mainSelector.innerHTML) {
      aboutSelector.innerHTML = mainSelector.innerHTML;
      aboutSelector.value = mainSelector.value;
      aboutSelector.addEventListener('change', (e) => {
        mainSelector.value = e.target.value;
        mainSelector.dispatchEvent(new Event('change'));
      });
      
      // Sync changes from main selector
      mainSelector.addEventListener('change', (e) => {
        aboutSelector.value = e.target.value;
      });
    }
  }

  setupSocialLinks() {
    // Telegram Channel
    const telegramBtn = document.getElementById('telegram_btn');
    if (telegramBtn) {
      telegramBtn.addEventListener('click', () => {
        window.open('https://t.me/Neovelocity', '_blank');
      });
    }
    
    // YouTube Channel (update with actual link)
    const youtubeBtn = document.getElementById('youtube_btn');
    if (youtubeBtn) {
      youtubeBtn.addEventListener('click', () => {
        window.open('https://youtube.com/@Neovelocity', '_blank');
      });
    }
    
    // GitHub Repository (update with actual link)
    const githubBtn = document.getElementById('github_btn');
    if (githubBtn) {
      githubBtn.addEventListener('click', () => {
        window.open('https://github.com/neovelocity', '_blank');
      });
    }
    
    // Official Website
    const websiteBtn = document.getElementById('website_btn');
    if (websiteBtn) {
      websiteBtn.addEventListener('click', () => {
        window.open('https://t.me/Neovelocity', '_blank');
      });
    }
    
    // Donate Button in About page
    const donateAboutBtn = document.getElementById('donate_btn');
    if (donateAboutBtn) {
      donateAboutBtn.addEventListener('click', () => {
        window.open('https://t.me/Neovelocity/232', '_blank');
      });
    }
  }

  updateAboutPage() {
    // Copy module version to about page
    const moduleVersion = document.getElementById('module_version');
    const aboutVersion = document.getElementById('module_version_about');
    
    if (moduleVersion && aboutVersion) {
      // Update initially
      aboutVersion.textContent = `Version ${moduleVersion.textContent}`;
      
      // Update if module version changes
      const observer = new MutationObserver(() => {
        aboutVersion.textContent = `Version ${moduleVersion.textContent}`;
      });
      
      observer.observe(moduleVersion, { childList: true, characterData: true });
    }
  }

  refreshPageContent(page) {
    // You can add page-specific refresh logic here
    switch(page) {
      case 'home':
        // Refresh daemon status when switching to home
        if (typeof getServiceState === 'function') {
          getServiceState();
        }
        break;
      case 'settings':
        // Refresh settings if needed
        break;
      case 'about':
        // Refresh about page if needed
        break;
    }
  }
}

// Initialize navigation when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
  window.navigationManager = new NavigationManager();
  
  // Apply translations to navbar
  if (typeof getTranslationSync === 'function') {
    document.querySelectorAll('[data-i18n^="nav."]').forEach(el => {
      const key = el.getAttribute('data-i18n');
      const translation = getTranslationSync(key);
      if (translation && translation !== key) {
        el.textContent = translation;
      }
    });
  }
});