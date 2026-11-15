#include "game/completemanager.h"
#include <filesystem>
#include <stdio.h>
#include <ctype.h>

namespace wi {

std::string CompleteManager::Key(const char *pszLvlFilename) {
    std::string key(pszLvlFilename);
    for (size_t i = 0; i < key.size(); i++) {
        key[i] = (char)tolower((unsigned char)key[i]);
    }
    return key;
}

void CompleteManager::Init() {
    complete_.clear();
    std::error_code ec;
    std::filesystem::directory_iterator it(
            std::filesystem::u8path(completedir_), ec);
    for (; !ec && it != std::filesystem::directory_iterator();
            it.increment(ec)) {
        if (it->is_regular_file(ec)) {
            complete_.insert(Key(it->path().filename().u8string().c_str()));
        }
    }
}

bool CompleteManager::IsComplete(const MissionIdentifier *pmiid) {
    return complete_.count(Key(pmiid->szLvlFilename)) != 0;
}

void CompleteManager::MarkComplete(const MissionIdentifier *pmiid) {
    std::string key = Key(pmiid->szLvlFilename);
    std::string path = completedir_ + key;
    FILE *pf = fopen(path.c_str(), "w");
    if (pf != NULL) {
        fclose(pf);
        complete_.insert(key);
    }
}

} // namespace wi
