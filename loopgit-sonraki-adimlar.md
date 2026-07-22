# LoopGit — Sonraki Adım Önerileri

Mevcut durum: UI/UX, Local Files ve History perspektifleri, GitFlow otomasyonu, custom commit graph tamamlanmış. Aşağıdaki öneriler öncelik sırasına göre gruplanmıştır.

## Yüksek Öncelik (SmartGit'in yerini gerçekten alması için kritik)

### 1. Conflict Resolution UI
Merge/pull sırasında conflict çıktığında GUI'den yönetim yok. En azından:
- Conflict'li dosyaları ayrı bir listede göster
- `<<<<<<<`, `=======`, `>>>>>>>` marker'larını dosya içinde highlight'la
- Tam 3-way merge editörü şart değil, ama "conflict farkındalığı" katmanı olmalı

### 2. Stash Desteği
- `git_stash_save` / `git_stash_apply` / `git_stash_pop` libgit2'de doğrudan mevcut
- Hızlı context switch gerektiren günlerde (iş ↔ Loopoid geçişi gibi) sık kullanılır

### 3. Credential/Auth Yönetimi
- SSH key mi, HTTPS + token mı kullanılıyor netleştirilmeli
- `git_credential_callback` mekanizması düzgün handle edilmeli, aksi halde farklı makine/repo'da sessizce patlayabilir

## Orta Öncelik (günlük kullanımda kısa sürede ihtiyaç duyulacaklar)

### 4. Interactive Rebase
Commit squash/reorder/reword. Sık commit atma alışkanlığı (özellikle LLM-ajan tabanlı geliştirmede) düşünülürse push öncesi history temizleme ihtiyacı doğar.

### 5. Cherry-pick
Bir branch'teki tek commit'i başka branch'e taşıma. Ortak kod parçaları olan repolar arası (örn. Link/Edge/Apex ürün ailesi) bug fix taşımada işe yarar.

### 6. Blame View
Satır bazında "kim, ne zaman, hangi commit'te" görünümü.

### 7. Tag Yönetimi
Release tag'leri oluşturma/listeleme — firmware/PCB revizyon takibi için.

### 8. .gitignore Editörü
Sağ tık → "ignore this file/extension" gibi hızlı ekleme.

## Düşük Öncelik (olsa güzel olur)

### 9. Multi-repo Dashboard
Sekmeler arası hangi repoda commit edilmemiş değişiklik var, tek ekrandan görme. Paralel yürüyen çok sayıda proje (Loopoid Link 1100, BLE freelance, CNC sender) için değerli olabilir.

### 10. Submodule Desteği
libgit2 veya başka bir C kütüphanesi submodule olarak kullanılıyorsa GUI'den yönetim.

### 11. Klavye Kısayolları
Stage (S), commit (Ctrl+Enter), fetch (F5) gibi — SmartGit'ten geçişte "muscle memory" kaybını azaltır.

### 12. Commit Mesajı Crash-Safety
Uygulama çökerse/kapatılırsa yazılmakta olan commit mesajının otomatik taslak olarak kaydedilmesi.

## Muhtemelen Gerek Yok (over-engineering riski, tek kullanıcı için düşük ROI)
- GPG commit signing
- Git worktrees yönetimi
- Git hooks yönetimi
- Git LFS desteği

## Genel Değerlendirme
En yüksek getiri/efor oranı: **conflict resolution + stash + interactive rebase** üçlüsü. Bunlar tamamlandığında araç günlük kullanımda gerçek anlamda SmartGit'in yerini alabilir; geri kalanı ihtiyaç çıktıkça eklenebilir.
 
