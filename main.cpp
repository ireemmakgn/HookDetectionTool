// =============================================================================
//  Hook Tespit Araci (Hook Detection Tool) - OOP ile C++ Ornegi
//  Ders: Nesneye Yonelik Programlama
//
//  Aciklama:
//    Bu proje; MemoryManager, HookSimulator ve HookDetector siniflarini
//    kullanarak bir fonksiyonun bellekteki ilk baytini degistirip (0xE9 - JMP)
//    "inline hook" senaryosunu tamamen kendi icinde simule eder ve tespit eder.
//    Hicbir harici araç gerekmez; VS Code'da derleyip calistirabilirsiniz.
//
//  Derleme (g++ / MinGW - Windows):
//    g++ -o hook_detector main.cpp -static -std=c++17
//
//  Derleme (MSVC - Developer Command Prompt):
//    cl /EHsc /std:c++17 main.cpp
// =============================================================================

#include <iostream>
#include <iomanip>
#include <vector>
#include <windows.h>

// =============================================================================
//  MemoryManager Sinifi
//  Sorumluluk: Surec bellegi uzerinde okuma ve yazma islemlerini yonetir.
//  OOP Prensibi: Single Responsibility (Tek Sorumluluk)
// =============================================================================
class MemoryManager {
public:
    // Belirtilen adresten `size` kadar bayt okur.
    // Basarisiz olursa bos vektor doner.
    std::vector<unsigned char> readBytes(void* address, size_t size) const {
        std::vector<unsigned char> buffer(size);
        SIZE_T bytesRead = 0;

        BOOL success = ReadProcessMemory(
            GetCurrentProcess(),
            address,
            buffer.data(),
            size,
            &bytesRead
        );

        if (!success || bytesRead != size) {
            std::cerr << "[-] HATA: Bellek okuma basarisiz! (Adres: "
                      << address << ")" << std::endl;
            return {};
        }
        return buffer;
    }

    // Belirtilen adrese `bytes` vektorundeki verileri yazar.
    // Basarisiz olursa false doner.
    bool writeBytes(void* address, const std::vector<unsigned char>& bytes) const {
        SIZE_T bytesWritten = 0;

        BOOL success = WriteProcessMemory(
            GetCurrentProcess(),
            address,
            bytes.data(),
            bytes.size(),
            &bytesWritten
        );

        if (!success || bytesWritten != bytes.size()) {
            std::cerr << "[-] HATA: Bellek yazma basarisiz! (Adres: "
                      << address << ")" << std::endl;
            return false;
        }
        return true;
    }
};


// =============================================================================
//  MemoryProtectionGuard Sinifi (RAII)
//  Sorumluluk: VirtualProtect ile bellek izin degisikliklerini guvenli
//              sekilde yonetir. Kapsam (scope) sonunda eski izinleri otomatik
//              olarak geri yukler.
//  OOP Prensibi: RAII (Resource Acquisition Is Initialization),
//               Encapsulation (Kapsulleme)
// =============================================================================
class MemoryProtectionGuard {
private:
    void*  m_address;       // Korunan bellek adresi
    size_t m_size;          // Korunan alan buyuklugu
    DWORD  m_oldProtect;    // Orijinal koruma bayraklari
    bool   m_isActive;      // Guard aktif mi?

public:
    // Constructor: VirtualProtect ile yeni izni uygular.
    MemoryProtectionGuard(void* address, size_t size, DWORD newProtect)
        : m_address(address), m_size(size), m_oldProtect(0), m_isActive(false)
    {
        BOOL success = VirtualProtect(m_address, m_size, newProtect, &m_oldProtect);
        if (success) {
            m_isActive = true;
        } else {
            std::cerr << "[-] HATA: VirtualProtect (uygulama) basarisiz!" << std::endl;
        }
    }

    // Destructor: Kapsam bitince orijinal izni otomatik geri yukler (RAII).
    ~MemoryProtectionGuard() {
        if (m_isActive) {
            DWORD temp;
            VirtualProtect(m_address, m_size, m_oldProtect, &temp);
        }
    }

    bool isActive() const { return m_isActive; }

    // Kopyalama ve atama operatorlerini devre disi birak (tekil sahiplik).
    MemoryProtectionGuard(const MemoryProtectionGuard&)            = delete;
    MemoryProtectionGuard& operator=(const MemoryProtectionGuard&) = delete;
};


// =============================================================================
//  HookSimulator Sinifi
//  Sorumluluk: Hedef fonksiyonun ilk baytini 0xE9 (JMP) ile degistirerek
//              yapay bir "inline hook" olusturur; geri almak icin restore()
//              metodunu saglar.
//  OOP Prensibi: Encapsulation, RAII (MemoryProtectionGuard uzerinden)
// =============================================================================
class HookSimulator {
private:
    void*                     m_targetAddress;   // Hook'lanacak fonksiyon adresi
    std::vector<unsigned char> m_originalBytes;   // Orijinal baytlar (geri alma icin)
    MemoryManager             m_memMgr;
    bool                      m_isHooked;

    static constexpr size_t HOOK_SIZE = 5; // JMP komutu 5 bayt (E9 + 4 bayt offset)

public:
    explicit HookSimulator(void* targetAddress)
        : m_targetAddress(targetAddress), m_isHooked(false)
    {}

    // Destructor: Nesne yok edilmeden once hook'u otomatik kaldir.
    ~HookSimulator() {
        if (m_isHooked) {
            restore();
        }
    }

    // Hook'u uygular: ilk 5 bayta JMP (0xE9) + sahte offset yazar.
    bool apply() {
        if (m_isHooked) {
            std::cout << "[!] Fonksiyon zaten hook'lu." << std::endl;
            return false;
        }

        // 1. Adim: Orijinal baytlari yedekle.
        m_originalBytes = m_memMgr.readBytes(m_targetAddress, HOOK_SIZE);
        if (m_originalBytes.empty()) return false;

        // 2. Adim: Yazma iznini ac (RAII Guard ile).
        MemoryProtectionGuard guard(m_targetAddress, HOOK_SIZE, PAGE_EXECUTE_READWRITE);
        if (!guard.isActive()) return false;

        // 3. Adim: Sahte JMP baytlarini olustur (0xE9 + 4 bayt sahte offset).
        //    Gercek bir hook'ta offset hesaplanir; biz sadece simule ediyoruz.
        std::vector<unsigned char> hookBytes = {0xE9, 0xDE, 0xAD, 0xBE, 0xEF};

        // 4. Adim: Baytlari yaz.
        if (!m_memMgr.writeBytes(m_targetAddress, hookBytes)) return false;

        m_isHooked = true;
        std::cout << "[+] Hook basariyla uygulandi (0xE9 yazildi)." << std::endl;
        return true;
        // Guard kapsam bitti -> destructor orijinal izni otomatik geri yukler.
    }

    // Hook'u kaldirir: orijinal baytlari geri yazar.
    bool restore() {
        if (!m_isHooked) {
            std::cout << "[!] Kaldirilacak aktif hook bulunamadi." << std::endl;
            return false;
        }

        MemoryProtectionGuard guard(m_targetAddress, HOOK_SIZE, PAGE_EXECUTE_READWRITE);
        if (!guard.isActive()) return false;

        if (!m_memMgr.writeBytes(m_targetAddress, m_originalBytes)) return false;

        m_isHooked = false;
        std::cout << "[+] Hook kaldirildi, orijinal baytlar geri yuklendi." << std::endl;
        return true;
    }

    bool isHooked() const { return m_isHooked; }

    // Kopyalama/atama yasak (tekil kaynak sahipligi).
    HookSimulator(const HookSimulator&)            = delete;
    HookSimulator& operator=(const HookSimulator&) = delete;
};


// =============================================================================
//  HookDetector Sinifi
//  Sorumluluk: Verilen fonksiyon adresindeki baytlari okuyarak bilinen
//              hook imzalarini (0xE9 JMP vb.) tespit eder.
//  OOP Prensibi: Single Responsibility, Encapsulation
// =============================================================================
class HookDetector {
private:
    MemoryManager m_memMgr;

    static constexpr size_t SCAN_SIZE = 5;

    // Okunan baytlari okunabilir hex formatta yazdirir.
    void printBytes(const std::vector<unsigned char>& bytes) const {
        std::cout << "    [Baytlar] ";
        for (unsigned char b : bytes) {
            std::cout << "0x" << std::hex << std::uppercase
                      << std::setw(2) << std::setfill('0')
                      << static_cast<int>(b) << " ";
        }
        std::cout << std::dec << std::endl;
    }

public:
    // Fonksiyon adresini tarar; hook tespit edilirse true doner.
    bool scan(void* functionAddress) {
        std::cout << "\n[*] Taranıyor: 0x" << std::hex << std::uppercase
                  << reinterpret_cast<uintptr_t>(functionAddress)
                  << std::dec << std::endl;

        std::vector<unsigned char> bytes = m_memMgr.readBytes(functionAddress, SCAN_SIZE);
        if (bytes.empty()) return false;

        printBytes(bytes);

        // --- Imza Kontrolu: 0xE9 = Relative JMP (en yaygin inline hook) ---
        if (bytes[0] == 0xE9) {
            std::cout << "    [Imza] 0xE9 - Relative JMP tespit edildi!" << std::endl;
            return true;
        }

        // --- Imza Kontrolu: 0xFF 0x25 = Absolute JMP (64-bit hook yontemi) ---
        if (bytes[0] == 0xFF && bytes[1] == 0x25) {
            std::cout << "    [Imza] 0xFF 0x25 - Absolute JMP tespit edildi!" << std::endl;
            return true;
        }

        std::cout << "    [Imza] Bilinen hook imzasi bulunamadi." << std::endl;
        return false;
    }
};


// =============================================================================
//  Hedef Fonksiyon
//  Projede "masum" bir fonksiyon olarak konumlanir.
//  HookSimulator bu fonksiyonun ilk baytlarini degistirecektir.
// =============================================================================
void hedefFonksiyon() {
    std::cout << "  >> Ben masum bir fonksiyonum, normal calisiyorum..." << std::endl;
}


// =============================================================================
//  main() - Senaryo Akisi
// =============================================================================
int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   Hook Tespit Araci (Hook Detection Tool)  " << std::endl;
    std::cout << "   OOP ile C++ Demonstrasyonu               " << std::endl;
    std::cout << "============================================" << std::endl;

    void* adres = reinterpret_cast<void*>(&hedefFonksiyon);
    HookDetector  detector;
    HookSimulator simulator(adres);

    // ------------------------------------------------------------------
    // ADIM 1: Temiz tarama (hook yok)
    // ------------------------------------------------------------------
    std::cout << "\n>>> ADIM 1: Fonksiyon hook'lanmadan once taranıyor..." << std::endl;
    hedefFonksiyon(); // Fonksiyon normal calisir.

    bool hookTespit = detector.scan(adres);
    if (!hookTespit) {
        std::cout << "\n[SONUC] Hook Yok - Fonksiyon temiz gorunuyor." << std::endl;
    } else {
        std::cout << "\n[SONUC] UYARI: Hook Tespit Edildi!" << std::endl;
    }

    // ------------------------------------------------------------------
    // ADIM 2: Hook uygulama (simulasyon)
    // ------------------------------------------------------------------
    std::cout << "\n>>> ADIM 2: Yapay hook uygulanıyor (0xE9 yaziliyor)..." << std::endl;
    simulator.apply();

    // ------------------------------------------------------------------
    // ADIM 3: Hook sonrasi tarama
    // ------------------------------------------------------------------
    std::cout << "\n>>> ADIM 3: Fonksiyon hook'landiktan sonra taranıyor..." << std::endl;

    hookTespit = detector.scan(adres);
    if (hookTespit) {
        std::cout << "\n[SONUC] !!! UYARI: Hook Tespit Edildi (0xE9 JMP) !!!" << std::endl;
    } else {
        std::cout << "\n[SONUC] Hook Yok." << std::endl;
    }

    // ------------------------------------------------------------------
    // ADIM 4: Hook kaldirma ve dogrulama
    // ------------------------------------------------------------------
    std::cout << "\n>>> ADIM 4: Hook kaldırılıyor (orijinal baytlar geri yukleniyor)..." << std::endl;
    simulator.restore();

    std::cout << "\n>>> ADIM 5: Temizlik dogrulamasi - fonksiyon tekrar taranıyor..." << std::endl;
    hookTespit = detector.scan(adres);
    if (!hookTespit) {
        std::cout << "\n[SONUC] Hook Yok - Fonksiyon temizlendi, normal calisıyor." << std::endl;
    }

    // Temizlendikten sonra fonksiyonu cagir (segfault olmadan calismali).
    hedefFonksiyon();

    std::cout << "\n============================================" << std::endl;
    std::cout << "   Demo tamamlandi. Cikmak icin Enter'a basin." << std::endl;
    std::cout << "============================================" << std::endl;
    std::cin.get();
    return 0;
}