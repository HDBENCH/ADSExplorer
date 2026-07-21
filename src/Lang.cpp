#include "Lang.h"

bool IsJapaneseUI() {
    static int cached = -1;
    if (cached < 0) {
        LANGID ui = GetUserDefaultUILanguage();
        cached = (PRIMARYLANGID(ui) == LANG_JAPANESE) ? 1 : 0;
    }
    return cached != 0;
}
