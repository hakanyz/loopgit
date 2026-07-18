# Qt Git GUI İstemcisi — Proje Planı

## Amaç
SmartGit yerine geçebilecek, kişisel ihtiyaca yönelik minimal ama kullanılabilir bir Git GUI istemcisi. Windows + Linux desteği, libgit2 tabanlı, Qt Widgets ile yazılacak.

## Teknoloji Seçimi
- **Dil:** C++17
- **UI Framework:** Qt Widgets (QML değil — daha az soyutlama, native performans, senin Qt/TouchGFX deneyiminle uyumlu)
- **Git Motoru:** libgit2 (C kütüphanesi, shell'e çıkmadan native git işlemleri)
- **Build:** CMake
- **Versiyon Kontrolü:** Proje kendisi de git ile takip edilecek (her fazda commit)

## Faz Planı (her faz bağımsız test edilebilir olmalı)

### Faz 0 — İskelet
- Boş Qt Widgets projesi, CMake yapılandırması
- libgit2 kütüphanesinin projeye entegrasyonu (find_package veya submodule)
- "Klasör seç" dialog'u + seçilen klasörün geçerli bir git repo olup olmadığını kontrol etme
- **Çıktı:** Derlenen, klasör seçtirebilen boş pencere

### Faz 1 — Durum Görüntüleme (Read-Only)
- `git status` karşılığı: değişen/yeni/silinen dosyaların listesi (QListWidget veya QTreeView)
- Mevcut branch adını gösterme
- Son N commit'i log olarak listeleme (mesaj, yazar, tarih)
- **Çıktı:** Repo durumunu görsel olarak izleyebildiğin salt-okunur bir araç

### Faz 2 — Temel Yazma İşlemleri
- Dosya stage/unstage etme (checkbox veya sürükle-bırak)
- Commit mesajı yazma + commit atma
- Push / Pull (basit, conflict yoksa)
- **Çıktı:** Günlük commit/push/pull işlerini GUI'den yapabiliyorsun

### Faz 3 — Diff Görüntüleme
- Seçilen dosyanın diff'ini QPlainTextEdit içinde satır satır +/- renklendirmesiyle gösterme
- Syntax highlight opsiyonel, önce sade unified diff yeterli
- **Çıktı:** Değişiklikleri commit öncesi gözden geçirebiliyorsun

### Faz 4 — Branch Yönetimi
- Branch listesi, yeni branch oluşturma, checkout
- Basit merge (fast-forward veya conflict'siz durumlar)
- **Çıktı:** Branch işlemleri GUI'den yapılabiliyor

### Faz 5 (opsiyonel, zaman kalırsa) — Commit Graph
- QPainter ile basit ağaç görselleştirmesi
- Bu en zahmetli kısım, MVP için şart değil

## LLM ile Çalışma Prensipleri (Antigravity için)
1. Her fazı ayrı ayrı iste — "hepsini yap" deme
2. Her fazın sonunda derle, çalıştır, git commit at — ajan bir sonraki fazda bozarsa geri dönebil
3. libgit2 fonksiyon imzalarını LLM yanlış hatırlayabilir (hallucination riski) — derleyici hata verirse resmi libgit2 dokümantasyonuyla karşılaştır
4. CMake/build hatalarını ajana geri ver, kendin patchlemeye çalışmadan önce ajana bir şans tanı
5. Ajana net, tek sorumluluklu görevler ver: "X fonksiyonunu şu davranışla yaz" > "git client yap"

## Başarı Kriteri (MVP)
Faz 0-3 çalışıyorsa, günlük kullanım için SmartGit'e ihtiyaç duymadan:
- Repo durumunu görebiliyorsun
- Stage/commit/push/pull yapabiliyorsun
- Diff'i gözden geçirebiliyorsun

Bu noktadan sonrası (branch yönetimi, commit graph) "nice to have."
