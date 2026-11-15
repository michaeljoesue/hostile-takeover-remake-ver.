#ifndef __MISSIONLIST_H__
#define __MISSIONLIST_H__

#include "game/ht.h"
#include <string>
#include <vector>

namespace wi {

// Single player missions. Story missions are s_*.lvl, challenge missions are
// c_*.lvl. Any other level, except multiplayer maps (m_*.lvl), comes from a
// custom mission pack.

enum MissionType { kmtStory, kmtChallenge, kmtCustom, kmtUnknown };

struct MissionDescription { // md
    char szLvlTitle[kcbLevelTitle];
    MissionType mt;
};

struct MissionIdentifier { // miid
    char szLvlFilename[kcbFilename];
};

class MissionList
{
public:
    bool Init();
    int GetCount();
    bool GetMissionDescription(int i, MissionDescription *pmd);
    bool GetMissionIdentifier(int i, MissionIdentifier *pmiid);

    static MissionType GetMissionType(const char *pszLvl);

private:
    std::vector<std::string> m_vecLvl;
};

extern MissionList *CreateMissionList();

} // namespace wi

#endif // __MISSIONLIST_H__
