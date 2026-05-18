# Hook Detection Tool (Hook Tespit Aracı) - C++ & OOP Demonstration

Bu proje, Nesneye Yönelik Programlama (OOP) dersi kapsamında geliştirilmiş, bellek manipülasyonu ve siber güvenlik odaklı bir **Inline Hook Tespit ve Simülasyon** aracıdır. Proje, harici bir araca veya sürücüye (driver) ihtiyaç duymadan, tamamen süreç içi (in-process) bellek yönetimiyle çalışır.

## 🚀 Projenin Amacı ve Çalışma Mantığı
Sistem, belirlenen hedef bir fonksiyonun bellek alanındaki ilk baytlarını tarar. Siber saldırganların veya analiz araçlarının sıkça kullandığı `0xE9` (Relative JMP) imzalarını tespit eder. Proje kendi içinde sahte bir hook uygulayarak, tespit motorunun başarıyla çalıştığını simüle eder ve ardından **RAII** prensipleriyle belleği orijinal haline geri döndürür.

## 🛠️ Kullanılan OOP Prensipleri ve Yapılar
* **Single Responsibility Principle (Tek Sorumluluk):** `MemoryManager` sadece bellek okuma/yazma işini yapar, `HookDetector` sadece imza taraması gerçekleştirir.
* **Encapsulation (Kapsülleme):** Kritik bellek adresleri ve orijinal bayt verileri sınıflar içinde `private` üyeler olarak saklanarak dış müdahalelerden korunur.
* **RAII (Resource Acquisition Is Initialization):** Bellek yazma izinlerini (`VirtualProtect`) yönetirken C++'ın modern kaynak yönetimi prensiplerinden faydalanılmıştır.

## 💻 Derleme ve Çalıştırma

### MinGW / GCC (Windows)
```bash
g++ -o hook_detector main.cpp -static -std=c++17
./hook_detector
