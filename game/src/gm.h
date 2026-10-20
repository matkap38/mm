#ifndef __GAME_SRC_GM_H__
#define __GAME_SRC_GM_H__
extern void gm_insert(const char * name, BYTE level);
extern BYTE gm_get_level(const char* name, const char* account = NULL);
extern void gm_new_clear();
extern void gm_new_insert( const tAdminInfo & c_rInfo );

#include <set>
#include <string>
typedef struct sGM
{
	tAdminInfo Info;
} tGM;

extern std::map<std::string, tGM> g_map_GM;
#endif

