#include "embedded_fonts.h"

namespace EmbeddedFonts {
    // For now, we don't have embedded font data, so these return nullptr/0
    // In a production build, you would generate this using binary_to_compressed_c.cpp
    const unsigned char* GetJetBrainsMonoData() {
        return nullptr;
    }
    
    int GetJetBrainsMonoDataSize() {
        return 0;
    }
    
    bool IsEmbeddedFontAvailable() {
        return false;
    }
}
