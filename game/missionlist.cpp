#include "game/ht.h"
#include <algorithm>
#include <set>

namespace wi {

MissionList *CreateMissionList() {
    MissionList *pml = new MissionList;
    if (!pml->Init()) {
        delete pml;
        return NULL;
    }
    return pml;
}

bool MissionList::Init() {
    m_vecLvl.clear();

    // Levels from the game data and every mounted mission pack. A pack can
    // replace a level with the same name; list it once.

    std::set<std::string> setSeen;
    char szFn[kcbFilename];
    Enum enm;
    while (gpakr.EnumFiles(&enm, PACKENUM_ALL, szFn, sizeof(szFn))) {
        int cch = (int)strlen(szFn);
        if (cch < 4 || strcmp(&szFn[cch - 4], ".lvl") != 0) {
            continue;
        }
        if (GetMissionType(szFn) == kmtUnknown) {
            continue;
        }
        std::string strLower = szFn;
        std::transform(strLower.begin(), strLower.end(), strLower.begin(),
                [](unsigned char ch) { return (char)tolower(ch); });
        if (!setSeen.insert(strLower).second) {
            continue;
        }
        m_vecLvl.push_back(szFn);
    }

    // Story, then challenge, then custom missions, each sorted by filename

    std::sort(m_vecLvl.begin(), m_vecLvl.end(),
            [](const std::string& a, const std::string& b) {
        MissionType mtA = GetMissionType(a.c_str());
        MissionType mtB = GetMissionType(b.c_str());
        if (mtA != mtB) {
            return mtA < mtB;
        }
        return a < b;
    });
    return true;
}

int MissionList::GetCount() {
    return (int)m_vecLvl.size();
}

bool MissionList::GetMissionDescription(int i, MissionDescription *pmd) {
    if (i < 0 || i >= GetCount()) {
        return false;
    }
    const char *pszLvl = m_vecLvl[i].c_str();
    pmd->mt = GetMissionType(pszLvl);

    IniReader *pini = LoadIniFile(gpakr, pszLvl);
    if (pini == NULL) {
        return false;
    }
    strncpyz(pmd->szLvlTitle, "<untitled>", sizeof(pmd->szLvlTitle));
    pini->GetPropertyValue("General", "Title", pmd->szLvlTitle,
            sizeof(pmd->szLvlTitle));
    delete pini;
    return true;
}

bool MissionList::GetMissionIdentifier(int i, MissionIdentifier *pmiid) {
    if (i < 0 || i >= GetCount()) {
        return false;
    }
    strncpyz(pmiid->szLvlFilename, m_vecLvl[i].c_str(),
            sizeof(pmiid->szLvlFilename));
    return true;
}

MissionType MissionList::GetMissionType(const char *pszLvl) {
    if (pszLvl[0] != 0 && pszLvl[1] == '_') {
        switch (pszLvl[0]) {
        case 's':
        case 'S':
            return kmtStory;

        case 'c':
        case 'C':
            return kmtChallenge;

        case 'm':   // the original multiplayer maps (not offered in single player)
        case 'M':
        case 'd':   // demo
        case 'D':
            return kmtUnknown;
        }
    }
    return kmtCustom;
}

} // namespace wi
