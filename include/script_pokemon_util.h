#ifndef GUARD_SCRIPT_POKEMON_UTIL_H
#define GUARD_SCRIPT_POKEMON_UTIL_H

u8 ScriptGiveMon(u16 species, u8 level, u16 item, u32 unused1, u32 unused2, u8 unused3);
u8 ScriptGiveEgg(u16 species);
void CreateScriptedWildMon(u16, u8, u16);
void ScriptSetMonMoveSlot(u8, u16, u8);
void ReducePlayerPartyToSelectedMons(void);
void HealPlayerParty(void);

#endif // GUARD_SCRIPT_POKEMON_UTIL_H
