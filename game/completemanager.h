#ifndef __COMPLETEMANAGER_H__
#define __COMPLETEMANAGER_H__

#include "game/ht.h"
#include <set>
#include <string>

namespace wi {

// Tracks which missions the player has completed. Each completed mission is
// recorded as an empty file, named after the mission, in the completes
// directory.

class CompleteManager {
public:
    CompleteManager(const char *completedir) : completedir_(completedir) {}

    void Init();
    bool IsComplete(const MissionIdentifier *pmiid);
    void MarkComplete(const MissionIdentifier *pmiid);

private:
    static std::string Key(const char *pszLvlFilename);

    std::set<std::string> complete_;
    std::string completedir_;
};
extern CompleteManager *gpcptm;

} // namespace wi

#endif // __COMPLETEMANAGER_H__
