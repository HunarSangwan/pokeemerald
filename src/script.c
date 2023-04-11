#include "global.h"
#include "script.h"
#include "event_data.h"
#include "mystery_gift.h"
#include "util.h"
#include "constants/event_objects.h"
#include "constants/map_scripts.h"

#define RAM_SCRIPT_MAGIC 51

enum {
    SCRIPT_MODE_STOPPED,
    SCRIPT_MODE_BYTECODE,
    SCRIPT_MODE_NATIVE,
};

enum {
    CONTEXT_RUNNING,
    CONTEXT_WAITING,
    CONTEXT_SHUTDOWN,
};

extern const u8 *gRamScriptRetAddr;

static u8 sGlobalScriptContextStatus;
static struct ScriptContext sGlobalScriptContext;
static struct ScriptContext sImmediateScriptContext;
static bool8 sLockFieldControls;

extern ScrCmdFunc gScriptCmdTable[];
extern ScrCmdFunc gScriptCmdTableEnd[];
extern void *gNullScriptPtr;

static void LoadCustomStarterIcon(u8 taskId);
static void Task_HandleCustomStarterInputNature(u8 taskId);
static void Task_HandleCustomStarterMenuRefresh(u8 taskId);
static void Task_HandleCustomStarterChoiceMenuInput(u8 taskId);
static void Task_CreateCustomStarterChoiceMenu(u8 taskId);
static void Task_GiveCustomStarterToPlayer(u8 taskId);

void InitScriptContext(struct ScriptContext *ctx, void *cmdTable, void *cmdTableEnd)
{
    s32 i;

    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
    ctx->stackDepth = 0;
    ctx->nativePtr = NULL;
    ctx->cmdTable = cmdTable;
    ctx->cmdTableEnd = cmdTableEnd;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->data); i++)
        ctx->data[i] = 0;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->stack); i++)
        ctx->stack[i] = NULL;
}

u8 SetupBytecodeScript(struct ScriptContext *ctx, const u8 *ptr)
{
    ctx->scriptPtr = ptr;
    ctx->mode = SCRIPT_MODE_BYTECODE;
    return 1;
}

void SetupNativeScript(struct ScriptContext *ctx, bool8 (*ptr)(void))
{
    ctx->mode = SCRIPT_MODE_NATIVE;
    ctx->nativePtr = ptr;
}

void StopScript(struct ScriptContext *ctx)
{
    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
}

bool8 RunScriptCommand(struct ScriptContext *ctx)
{
    if (ctx->mode == SCRIPT_MODE_STOPPED)
        return FALSE;

    switch (ctx->mode)
    {
    case SCRIPT_MODE_STOPPED:
        return FALSE;
    case SCRIPT_MODE_NATIVE:
        // Try to call a function in C
        // Continue to bytecode if no function or it returns TRUE
        if (ctx->nativePtr)
        {
            if (ctx->nativePtr() == TRUE)
                ctx->mode = SCRIPT_MODE_BYTECODE;
            return TRUE;
        }
        ctx->mode = SCRIPT_MODE_BYTECODE;
        // fallthrough
    case SCRIPT_MODE_BYTECODE:
        while (1)
        {
            u8 cmdCode;
            ScrCmdFunc *func;

            if (!ctx->scriptPtr)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            if (ctx->scriptPtr == gNullScriptPtr)
            {
                while (1)
                    asm("svc 2"); // HALT
            }

            cmdCode = *(ctx->scriptPtr);
            ctx->scriptPtr++;
            func = &ctx->cmdTable[cmdCode];

            if (func >= ctx->cmdTableEnd)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            if ((*func)(ctx) == TRUE)
                return TRUE;
        }
    }

    return TRUE;
}

static bool8 ScriptPush(struct ScriptContext *ctx, const u8 *ptr)
{
    if (ctx->stackDepth + 1 >= (int)ARRAY_COUNT(ctx->stack))
    {
        return TRUE;
    }
    else
    {
        ctx->stack[ctx->stackDepth] = ptr;
        ctx->stackDepth++;
        return FALSE;
    }
}

static const u8 *ScriptPop(struct ScriptContext *ctx)
{
    if (ctx->stackDepth == 0)
        return NULL;

    ctx->stackDepth--;
    return ctx->stack[ctx->stackDepth];
}

void ScriptJump(struct ScriptContext *ctx, const u8 *ptr)
{
    ctx->scriptPtr = ptr;
}

void ScriptCall(struct ScriptContext *ctx, const u8 *ptr)
{
    ScriptPush(ctx, ctx->scriptPtr);
    ctx->scriptPtr = ptr;
}

void ScriptReturn(struct ScriptContext *ctx)
{
    ctx->scriptPtr = ScriptPop(ctx);
}

u16 ScriptReadHalfword(struct ScriptContext *ctx)
{
    u16 value = *(ctx->scriptPtr++);
    value |= *(ctx->scriptPtr++) << 8;
    return value;
}

u32 ScriptReadWord(struct ScriptContext *ctx)
{
    u32 value0 = *(ctx->scriptPtr++);
    u32 value1 = *(ctx->scriptPtr++);
    u32 value2 = *(ctx->scriptPtr++);
    u32 value3 = *(ctx->scriptPtr++);
    return (((((value3 << 8) + value2) << 8) + value1) << 8) + value0;
}

void LockPlayerFieldControls(void)
{
    sLockFieldControls = TRUE;
}

void UnlockPlayerFieldControls(void)
{
    sLockFieldControls = FALSE;
}

bool8 ArePlayerFieldControlsLocked(void)
{
    return sLockFieldControls;
}

// The ScriptContext_* functions work with the primary script context,
// which yields control back to native code should the script make a wait call.

// Checks if the global script context is able to be run right now.
bool8 ScriptContext_IsEnabled(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_RUNNING)
        return TRUE;
    else
        return FALSE;
}

// Re-initializes the global script context to zero.
void ScriptContext_Init(void)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
}

// Runs the script until the script makes a wait* call, then returns true if
// there's more script to run, or false if the script has hit the end.
// This function also returns false if the context is finished
// or waiting (after a call to _Stop)
bool8 ScriptContext_RunScript(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_SHUTDOWN)
        return FALSE;

    if (sGlobalScriptContextStatus == CONTEXT_WAITING)
        return FALSE;

    LockPlayerFieldControls();

    if (!RunScriptCommand(&sGlobalScriptContext))
    {
        sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
        UnlockPlayerFieldControls();
        return FALSE;
    }

    return TRUE;
}

// Sets up a new script in the global context and enables the context
void ScriptContext_SetupScript(const u8 *ptr)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sGlobalScriptContext, ptr);
    LockPlayerFieldControls();
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
}

// Puts the script into waiting mode; usually called from a wait* script command.
void ScriptContext_Stop(void)
{
    sGlobalScriptContextStatus = CONTEXT_WAITING;
}

// Puts the script into running mode.
void ScriptContext_Enable(void)
{
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
    LockPlayerFieldControls();
}

// Sets up and runs a script in its own context immediately. The script will be
// finished when this function returns. Used mainly by all of the map header
// scripts (except the frame table scripts).
void RunScriptImmediately(const u8 *ptr)
{
    InitScriptContext(&sImmediateScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sImmediateScriptContext, ptr);
    while (RunScriptCommand(&sImmediateScriptContext) == TRUE);
}

u8 *MapHeaderGetScriptTable(u8 tag)
{
    const u8 *mapScripts = gMapHeader.mapScripts;

    if (!mapScripts)
        return NULL;

    while (1)
    {
        if (!*mapScripts)
            return NULL;
        if (*mapScripts == tag)
        {
            mapScripts++;
            return T2_READ_PTR(mapScripts);
        }
        mapScripts += 5;
    }
}

void MapHeaderRunScriptType(u8 tag)
{
    u8 *ptr = MapHeaderGetScriptTable(tag);
    if (ptr)
        RunScriptImmediately(ptr);
}

u8 *MapHeaderCheckScriptTable(u8 tag)
{
    u8 *ptr = MapHeaderGetScriptTable(tag);

    if (!ptr)
        return NULL;

    while (1)
    {
        u16 varIndex1;
        u16 varIndex2;

        // Read first var (or .2byte terminal value)
        varIndex1 = T1_READ_16(ptr);
        if (!varIndex1)
            return NULL; // Reached end of table
        ptr += 2;

        // Read second var
        varIndex2 = T1_READ_16(ptr);
        ptr += 2;

        // Run map script if vars are equal
        if (VarGet(varIndex1) == VarGet(varIndex2))
            return T2_READ_PTR(ptr);
        ptr += 4;
    }
}

void RunOnLoadMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_LOAD);
}

void RunOnTransitionMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_TRANSITION);
}

void RunOnResumeMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RESUME);
}

void RunOnReturnToFieldMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RETURN_TO_FIELD);
}

void RunOnDiveWarpMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_DIVE_WARP);
}

bool8 TryRunOnFrameMapScript(void)
{
    u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_FRAME_TABLE);

    if (!ptr)
        return FALSE;

    ScriptContext_SetupScript(ptr);
    return TRUE;
}

void TryRunOnWarpIntoMapScript(void)
{
    u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE);
    if (ptr)
        RunScriptImmediately(ptr);
}

u32 CalculateRamScriptChecksum(void)
{
    return CalcCRC16WithTable((u8 *)(&gSaveBlock1Ptr->ramScript.data), sizeof(gSaveBlock1Ptr->ramScript.data));
}

void ClearRamScript(void)
{
    CpuFill32(0, &gSaveBlock1Ptr->ramScript, sizeof(struct RamScript));
}

bool8 InitRamScript(const u8 *script, u16 scriptSize, u8 mapGroup, u8 mapNum, u8 objectId)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;

    ClearRamScript();

    if (scriptSize > sizeof(scriptData->script))
        return FALSE;

    scriptData->magic = RAM_SCRIPT_MAGIC;
    scriptData->mapGroup = mapGroup;
    scriptData->mapNum = mapNum;
    scriptData->objectId = objectId;
    memcpy(scriptData->script, script, scriptSize);
    gSaveBlock1Ptr->ramScript.checksum = CalculateRamScriptChecksum();
    return TRUE;
}

const u8 *GetRamScript(u8 objectId, const u8 *script)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    gRamScriptRetAddr = NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return script;
    if (scriptData->mapGroup != gSaveBlock1Ptr->location.mapGroup)
        return script;
    if (scriptData->mapNum != gSaveBlock1Ptr->location.mapNum)
        return script;
    if (scriptData->objectId != objectId)
        return script;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return script;
    }
    else
    {
        gRamScriptRetAddr = script;
        return scriptData->script;
    }
}

#define NO_OBJECT OBJ_EVENT_ID_PLAYER

bool32 ValidateSavedRamScript(void)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return FALSE;
    if (scriptData->mapGroup != MAP_GROUP(UNDEFINED))
        return FALSE;
    if (scriptData->mapNum != MAP_NUM(UNDEFINED))
        return FALSE;
    if (scriptData->objectId != NO_OBJECT)
        return FALSE;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
        return FALSE;
    return TRUE;
}

u8 *GetSavedRamScriptIfValid(void)
{
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (!ValidateSavedWonderCard())
        return NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return NULL;
    if (scriptData->mapGroup != MAP_GROUP(UNDEFINED))
        return NULL;
    if (scriptData->mapNum != MAP_NUM(UNDEFINED))
        return NULL;
    if (scriptData->objectId != NO_OBJECT)
        return NULL;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return NULL;
    }
    else
    {
        return scriptData->script;
    }
}

void InitRamScript_NoObjectEvent(u8 *script, u16 scriptSize)
{
    if (scriptSize > sizeof(gSaveBlock1Ptr->ramScript.data.script))
        scriptSize = sizeof(gSaveBlock1Ptr->ramScript.data.script);
    InitRamScript(script, scriptSize, MAP_GROUP(UNDEFINED), MAP_NUM(UNDEFINED), NO_OBJECT);
}

#include "task.h"
#include "window.h"
#include "menu.h"
#include "string_util.h"
#include "event_object_lock.h"
#include "field_player_avatar.h"
#include "data.h"
#include "random.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "pokemon_icon.h"
#include "script_pokemon_util.h"
#include "map_name_popup.h"
#include "sound.h"
#include "constants/songs.h"
#include "constants/items.h"

#define tskSpeciesId             data[0]
#define tskDigitSelectedIndex    data[1]  // The digitInidicator_* stuff
#define tskMonSpriteIconId       data[2]
#define tskWindowId              data[3]
#define tskNatureId              data[4]
#define tskFuncId                data[5]  // Keeps track of what function/task is currently running
#define tskIVsCounter            data[6]  // Keeps count on the current IV to set
#define tskHpIVs                 data[7]
#define tskAtkIVs                data[8]
#define tskDefIVs                data[9]
#define tskSpatkIVs              data[10]
#define tskSpdefIVs              data[11]
#define tskSpdIVs                data[12]


enum
{
    CUSTOM_STARTER_CREATE_MENU,
    CUSTOM_STARTER_HANDLE_SPECIES,
    CUSTOM_STARTER_HANDLE_IVS,
    CUSTOM_STARTER_HANDLE_NATURE,
    CUSTOM_STARTER_GIVE_MON,  // The task is destroyed here
};

static const u8 sCustomStarterText_ID[]         =   _("Species:  {STR_VAR_3}\nName: {STR_VAR_1}    \n\n{STR_VAR_2}");
static const u8 sCustomStarterText_Nature[]     =   _("Name:  {STR_VAR_3}\nNature: {STR_VAR_1}    \n\n{STR_VAR_2}");

static const u8 sCustomStarterText_HpIV[]       =   _("IV Hp:                   \n    {STR_VAR_3}            \n             \n{STR_VAR_2}          ");
static const u8 sCustomStarterText_AtkIV[]      =   _("IV Attack:               \n    {STR_VAR_3}            \n             \n{STR_VAR_2}          ");
static const u8 sCustomStarterText_DefIV[]      =   _("IV Defense:               \n    {STR_VAR_3}            \n             \n{STR_VAR_2}          ");
static const u8 sCustomStarterText_SpatkIV[]    =   _("IV Speed:               \n    {STR_VAR_3}            \n             \n{STR_VAR_2}          ");
static const u8 sCustomStarterText_SpdefIV[]    =   _("IV Sp. Attack:               \n    {STR_VAR_3}            \n             \n{STR_VAR_2}          ");
static const u8 sCustomStarterText_SpdIV[]      =   _("IV Sp. Defense:               \n    {STR_VAR_3}            \n             \n{STR_VAR_2}          ");

static const u8 digitInidicator_1[]    =   _("{LEFT_ARROW}+1{RIGHT_ARROW}        ");
static const u8 digitInidicator_5[]    =   _("{LEFT_ARROW}+5{RIGHT_ARROW}        ");
static const u8 digitInidicator_10[]   =   _("{LEFT_ARROW}+10{RIGHT_ARROW}       ");
static const u8 digitInidicator_25[]   =   _("{LEFT_ARROW}+25{RIGHT_ARROW}       ");
static const u8 digitInidicator_50[]   =   _("{LEFT_ARROW}+50{RIGHT_ARROW}       ");
static const u8 digitInidicator_100[]  =   _("{LEFT_ARROW}+100{RIGHT_ARROW}      ");

static const u8 *const sText_DigitIndicator[] =
{
    digitInidicator_1,
    digitInidicator_5,
    digitInidicator_10,
    digitInidicator_25,
    digitInidicator_50,
    digitInidicator_100
};

static const u8 sPowersofDigitIndicator[] = {1, 5, 10, 25, 50, 100}; 

static const struct WindowTemplate sChooseMonDisplayWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 4 + 13, // 4 + 15
    .tilemapTop = 1,
    .width = 12, // 10
    .height = 2 * 4,
    .paletteNum = 15,
    .baseBlock = 1,
};

void Task_GiveCustomStarterToPlayer(u8 taskId)
{
    struct Pokemon customStarter;
    u8 nationalDexNum;

    gTasks[taskId].tskFuncId = CUSTOM_STARTER_GIVE_MON;

    // Create the mon at lvl 5 with the specified nature
    CreateMonWithNature(&customStarter, gTasks[taskId].tskSpeciesId, 5, 32, gTasks[taskId].tskNatureId);

    // Set the IVs
    SetMonData(&customStarter, MON_DATA_HP_IV, &gTasks[taskId].tskHpIVs);
    SetMonData(&customStarter, MON_DATA_ATK_IV, &gTasks[taskId].tskAtkIVs);
    SetMonData(&customStarter, MON_DATA_DEF_IV, &gTasks[taskId].tskDefIVs);
    SetMonData(&customStarter, MON_DATA_SPATK_IV, &gTasks[taskId].tskSpatkIVs);
    SetMonData(&customStarter, MON_DATA_SPDEF_IV, &gTasks[taskId].tskSpdefIVs);
    SetMonData(&customStarter, MON_DATA_SPEED_IV, &gTasks[taskId].tskSpdIVs);

    // Update the stats
    CalculateMonStats(&customStarter);

    // Set Player Data on mon
    SetMonData(&customStarter, MON_DATA_OT_NAME, gSaveBlock2Ptr->playerName);
    SetMonData(&customStarter, MON_DATA_OT_GENDER, &gSaveBlock2Ptr->playerGender);

    // Add mon to first slot in party and update the global var
    CopyMon(&gPlayerParty[0], &customStarter, sizeof(customStarter));
    gPlayerPartyCount = 1;

    // Update Natdex seen & caught
    nationalDexNum = SpeciesToNationalPokedexNum(gTasks[taskId].tskSpeciesId);
    GetSetPokedexFlag(nationalDexNum, FLAG_SET_SEEN);
    GetSetPokedexFlag(nationalDexNum, FLAG_SET_CAUGHT);

    // Destroy the Icon Sprite
    FreeAndDestroyMonIconSprite(&gSprites[gTasks[taskId].tskMonSpriteIconId]);
    FreeMonIconPalettes();
    
    // Clean up window, destroy task and restore control to the player
    ClearStdWindowAndFrame(gTasks[taskId].tskWindowId, TRUE);
    RemoveWindow(gTasks[taskId].tskWindowId);
    ScriptContext_Enable();
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    DestroyTask(taskId);
}

// Self Explanatory
void LoadCustomStarterIcon(u8 taskId)
{
    // Destroy the old sprite and reload the new one
    FreeAndDestroyMonIconSprite(&gSprites[gTasks[taskId].tskMonSpriteIconId]);          // Destroy previous icon
    FreeMonIconPalettes();                                                              // Free space for new pallete
    LoadMonIconPalette(gTasks[taskId].tskSpeciesId);                                    // Loads pallete for current mon
    gTasks[taskId].tskMonSpriteIconId = CreateMonIcon(gTasks[taskId].tskSpeciesId, SpriteCB_MonIcon, 210, 50, 4, 0, 0); //Create pokemon sprite
    gSprites[gTasks[taskId].tskMonSpriteIconId].oam.priority = 0;
}

// Handles setting the IVs
void Task_HandleCustomStarterInputIVs(u8 taskId)
{
    s16 *currentIVtoSetPtr;
    const u8 *currentIVText;

    gTasks[taskId].tskFuncId = CUSTOM_STARTER_HANDLE_IVS;

    switch (gTasks[taskId].tskIVsCounter)  // Decides the right IV/IV-related thing to use
    {
        case 0: // HP IV
            currentIVtoSetPtr = &gTasks[taskId].tskHpIVs;
            currentIVText = sCustomStarterText_HpIV;
            break;
        case 1: // ATK IV
            currentIVtoSetPtr = &gTasks[taskId].tskAtkIVs;
            currentIVText = sCustomStarterText_AtkIV;
            break;
        case 2: // DEF IV
            currentIVtoSetPtr = &gTasks[taskId].tskDefIVs;
            currentIVText = sCustomStarterText_DefIV;
            break;
        case 3: // SPATK IV
            currentIVtoSetPtr = &gTasks[taskId].tskSpatkIVs;
            currentIVText = sCustomStarterText_SpatkIV;
            break;
        case 4: // SPDEF IV
            currentIVtoSetPtr = &gTasks[taskId].tskSpdefIVs;
            currentIVText = sCustomStarterText_SpdefIV;
            break;
        case 5: // SPEED IV
            currentIVtoSetPtr = &gTasks[taskId].tskSpdIVs;
            currentIVText = sCustomStarterText_SpdIV;
            break;
    }

    // Reload the names. There might be a noticable drop in performance because of this code chunk, I'll optimize it later
    StringCopy(gStringVar2, sText_DigitIndicator[gTasks[taskId].tskDigitSelectedIndex]);
    ConvertIntToDecimalStringN(gStringVar3, *currentIVtoSetPtr, STR_CONV_MODE_LEADING_ZEROS, 2);       // Reload IV
    StringCopyPadded(gStringVar3, gStringVar3, CHAR_SPACE, 15);
    StringExpandPlaceholders(gStringVar4, currentIVText);
    AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);

    if (JOY_NEW(DPAD_ANY))
    {
        PlaySE(SE_SELECT);

        // Deals with selecting a nature
        if (JOY_NEW(DPAD_UP))
        {
            *currentIVtoSetPtr += sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (*currentIVtoSetPtr > MAX_PER_STAT_IVS)
                *currentIVtoSetPtr = MAX_PER_STAT_IVS;
        }
        if (JOY_NEW(DPAD_DOWN))
        {
            *currentIVtoSetPtr -= sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (*currentIVtoSetPtr < 0)
                *currentIVtoSetPtr = 0;
        }
        // Deals with changing the digit indicator
        if (JOY_NEW(DPAD_LEFT))
        {
            if (gTasks[taskId].tskDigitSelectedIndex > 0)
                gTasks[taskId].tskDigitSelectedIndex -= 1;
        }
        if (JOY_NEW(DPAD_RIGHT))
        {
            if (gTasks[taskId].tskDigitSelectedIndex < (ARRAY_COUNT(sPowersofDigitIndicator) - 1))
                gTasks[taskId].tskDigitSelectedIndex += 1;
        }

        // Reload names
        /*StringCopy(gStringVar2, sText_DigitIndicator[gTasks[taskId].tskDigitSelectedIndex]);
        ConvertIntToDecimalStringN(gStringVar3, *currentIVtoSetPtr, STR_CONV_MODE_LEADING_ZEROS, 2);       // Reload IV
        StringCopyPadded(gStringVar3, gStringVar3, CHAR_SPACE, 15);
        StringExpandPlaceholders(gStringVar4, currentIVText);
        AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);*/

        //LoadCustomStarterIcon(taskId);
    }

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        if (JOY_NEW(B_BUTTON))
        {
            if (gTasks[taskId].tskIVsCounter > 0)
                gTasks[taskId].tskIVsCounter--;     // Move to previous IV
        }

        if (gTasks[taskId].tskIVsCounter < 5)  // All IVs have not been inputed
        {
            gTasks[taskId].tskIVsCounter++;     // Move on to next IV
        }
        else  // All IVs have been inputted
        {
            // Move on to the next task
            gTasks[taskId].func = Task_HandleCustomStarterMenuRefresh;
        }
    }
}

// Handles setting the nature
void Task_HandleCustomStarterInputNature(u8 taskId)
{
    gTasks[taskId].tskFuncId = CUSTOM_STARTER_HANDLE_NATURE;

    if (JOY_NEW(DPAD_ANY))
    {
        PlaySE(SE_SELECT);

        // Deals with selecting a nature
        if (JOY_NEW(DPAD_UP))
        {
            gTasks[taskId].tskNatureId += sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (gTasks[taskId].tskNatureId > (NUM_NATURES - 1))
                gTasks[taskId].tskNatureId = (NUM_NATURES - 1);
        }
        if (JOY_NEW(DPAD_DOWN))
        {
            gTasks[taskId].tskNatureId -= sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (gTasks[taskId].tskNatureId < 0)
                gTasks[taskId].tskNatureId = 0;
        }
        // Deals with changing the digit indicator
        if (JOY_NEW(DPAD_LEFT))
        {
            if (gTasks[taskId].tskDigitSelectedIndex > 0)
                gTasks[taskId].tskDigitSelectedIndex -= 1;
        }
        if (JOY_NEW(DPAD_RIGHT))
        {
            if (gTasks[taskId].tskDigitSelectedIndex < (ARRAY_COUNT(sPowersofDigitIndicator) - 1))
                gTasks[taskId].tskDigitSelectedIndex += 1;
        }

        // Update the text and then re-print it
        StringCopy(gStringVar2, sText_DigitIndicator[gTasks[taskId].tskDigitSelectedIndex]);              // Reloads the Digit indicator
        StringCopy(gStringVar1, gNatureNamePointers[gTasks[taskId].tskNatureId]);                         // Reloads the Nature Name
        StringCopyPadded(gStringVar1, gStringVar1, CHAR_SPACE, 15);
        StringCopy(gStringVar3, gSpeciesNames[gTasks[taskId].tskSpeciesId]);                              // Reloads the Species Name
        StringExpandPlaceholders(gStringVar4, sCustomStarterText_Nature);
        AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);

        //LoadCustomStarterIcon(taskId);
    }

    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        PlaySE(MUS_LEVEL_UP);

        // Randomize nature
        if (JOY_NEW(B_BUTTON))
            gTasks[taskId].tskNatureId = Random() % (NUM_NATURES - 1);

        // Move on to the next task
        gTasks[taskId].func = Task_HandleCustomStarterMenuRefresh;
    }
}

// When switching between the menus for species, ivs, nature, etc This will refrsh the page instea of waiting till a button is pressed
void Task_HandleCustomStarterMenuRefresh(u8 taskId)
{
    // Tbf, this function might be a bit redundant. It would be far more useful if you could go back to previous options
    gTasks[taskId].tskDigitSelectedIndex = 0;           // Reset this

    switch (gTasks[taskId].tskFuncId)
    {
        case CUSTOM_STARTER_CREATE_MENU:
            break;
            
        case CUSTOM_STARTER_HANDLE_SPECIES:
            StringCopy(gStringVar2, sText_DigitIndicator[gTasks[taskId].tskDigitSelectedIndex]);              // Refreshes the Digit indicator
            StringCopy(gStringVar1, gNatureNamePointers[gTasks[taskId].tskNatureId]);                         // Refreshes the Nature Name
            StringCopyPadded(gStringVar1, gStringVar1, CHAR_SPACE, 15);
            StringCopy(gStringVar3, gSpeciesNames[gTasks[taskId].tskSpeciesId]);                              // Refreshes the Species Name
            StringExpandPlaceholders(gStringVar4, sCustomStarterText_Nature);
            AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);

            // Move on to next task
            gTasks[taskId].func = Task_HandleCustomStarterInputNature;
            break;

        case CUSTOM_STARTER_HANDLE_IVS:
            gTasks[taskId].func = Task_GiveCustomStarterToPlayer;
            break;

        case CUSTOM_STARTER_HANDLE_NATURE:
            StringCopy(gStringVar2, sText_DigitIndicator[gTasks[taskId].tskDigitSelectedIndex]);
            ConvertIntToDecimalStringN(gStringVar3, gTasks[taskId].tskHpIVs, STR_CONV_MODE_LEADING_ZEROS, 2);       // Reload IV
            StringCopyPadded(gStringVar3, gStringVar3, CHAR_SPACE, 15);
            StringExpandPlaceholders(gStringVar4, sCustomStarterText_HpIV);
            AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);

            gTasks[taskId].tskIVsCounter = 0; // Reset this
            // Move on to next task
            gTasks[taskId].func = Task_HandleCustomStarterInputIVs;
            break;

        case CUSTOM_STARTER_GIVE_MON:
            break;

    }
}

// This runs every frame, recieves input and updates the information shown for the mon
void Task_HandleCustomStarterChoiceMenuInput(u8 taskId)
{
    gTasks[taskId].tskFuncId = CUSTOM_STARTER_HANDLE_SPECIES;

    if (JOY_NEW(DPAD_ANY))
    {
        PlaySE(SE_SELECT);

        // Deals with selecting a mon
        if (JOY_NEW(DPAD_UP))
        {
            gTasks[taskId].tskSpeciesId += sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (gTasks[taskId].tskSpeciesId >= NUM_SPECIES - 1)
                gTasks[taskId].tskSpeciesId = NUM_SPECIES - 1;
            
            // Unown B till Z have no icons so skip
            if (gTasks[taskId].tskSpeciesId >= SPECIES_OLD_UNOWN_B && gTasks[taskId].tskSpeciesId <= SPECIES_OLD_UNOWN_Z)
                gTasks[taskId].tskSpeciesId = SPECIES_TREECKO;
        }
        if (JOY_NEW(DPAD_DOWN))
        {
            gTasks[taskId].tskSpeciesId -= sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (gTasks[taskId].tskSpeciesId < 1)
                gTasks[taskId].tskSpeciesId = 1;
            
            // Unown B till Z have no icons so skip
            if (gTasks[taskId].tskSpeciesId >= SPECIES_OLD_UNOWN_B && gTasks[taskId].tskSpeciesId <= SPECIES_OLD_UNOWN_Z)
                gTasks[taskId].tskSpeciesId = SPECIES_CELEBI;
        }
        // Deals with changing the digit indicator
        if (JOY_NEW(DPAD_LEFT))
        {
            if (gTasks[taskId].tskDigitSelectedIndex > 0)
                gTasks[taskId].tskDigitSelectedIndex -= 1;
        }
        if (JOY_NEW(DPAD_RIGHT))
        {
            if (gTasks[taskId].tskDigitSelectedIndex < (ARRAY_COUNT(sPowersofDigitIndicator) - 1))
                gTasks[taskId].tskDigitSelectedIndex += 1;
        }

        // Update the text and then re-print it
        StringCopy(gStringVar2, sText_DigitIndicator[gTasks[taskId].tskDigitSelectedIndex]);                   // Reloads the Digit indicator
        StringCopy(gStringVar1, gSpeciesNames[gTasks[taskId].tskSpeciesId]);                                   // Reloads the Species Name
        StringCopyPadded(gStringVar1, gStringVar1, CHAR_SPACE, 15);
        ConvertIntToDecimalStringN(gStringVar3, gTasks[taskId].tskSpeciesId, STR_CONV_MODE_LEADING_ZEROS, 3);  // Natdex Num
        StringExpandPlaceholders(gStringVar4, sCustomStarterText_ID);
        AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);

        LoadCustomStarterIcon(taskId);
    }

    // Handle the A_BUTTON/B_BUTTON input
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        // Clean up and move on
        PlaySE(MUS_LEVEL_UP);

        // Randomize Starter
        if (JOY_NEW(B_BUTTON))
            gTasks[taskId].tskSpeciesId = Random() % (NUM_SPECIES - 1);
            if (gTasks[taskId].tskSpeciesId == SPECIES_NONE)
                gTasks[taskId].tskSpeciesId == SPECIES_BULBASAUR;       // The bulbuous one is a backup
        
        // Move on to next task
        gTasks[taskId].func = Task_HandleCustomStarterMenuRefresh;
    }
}

// Runs once to freeze the player, print the window and sample data of the mon
void Task_CreateCustomStarterChoiceMenu(u8 taskId)
{
    // General Data initialization and others
    gTasks[taskId].tskSpeciesId = SPECIES_BULBASAUR;                                         // Bulbuous Saur
    PlayerFreeze();
    StopPlayerAvatar();
    LockPlayerFieldControls();
    gTasks[taskId].tskFuncId = CUSTOM_STARTER_CREATE_MENU;

    // Create window
    HideMapNamePopUpWindow();
    gTasks[taskId].tskWindowId = AddWindow(&sChooseMonDisplayWindowTemplate);
    DrawStdWindowFrame(gTasks[taskId].tskWindowId, FALSE);
    CopyWindowToVram(gTasks[taskId].tskWindowId, 3);

    // Load in the Text and Number
    StringCopy(gStringVar2, sText_DigitIndicator[0]);
    ConvertIntToDecimalStringN(gStringVar3, 1, STR_CONV_MODE_LEADING_ZEROS, 2);     //This will give 001 (Bulbuous Saur's Natdex num)
    StringCopy(gStringVar1, gSpeciesNames[gTasks[taskId].tskSpeciesId]);            // Loads the first mon being the Bulbuous Saur
    StringCopyPadded(gStringVar1, gStringVar1, CHAR_SPACE, 15);                     // Pads the species name with space
    StringExpandPlaceholders(gStringVar4, sCustomStarterText_ID);
    AddTextPrinterParameterized(gTasks[taskId].tskWindowId, FONT_NORMAL, gStringVar4, 1, 1, 0, NULL);   // Prints all the text we prepared

    // Load Animated Icon Sprite
    FreeMonIconPalettes();                                                //Free space for new pallete
    LoadMonIconPalette(gTasks[taskId].tskSpeciesId);                      //Loads pallete for current mon
    gTasks[taskId].tskMonSpriteIconId = CreateMonIcon(gTasks[taskId].tskSpeciesId, SpriteCB_MonIcon, 210, 50, 4, 0, 0); //Create pokemon sprite
    gSprites[gTasks[taskId].tskMonSpriteIconId].oam.priority = 0;         // Render it on the top layer

    // Move on to next task
    gTasks[taskId].func = Task_HandleCustomStarterChoiceMenuInput;
}

void ChooseCustomStarterFromMenu(void)
{
    CreateTask(Task_CreateCustomStarterChoiceMenu, 0);
}

#undef tskSpeciesId
#undef tskDigitSelectedIndex
#undef tskMonSpriteIconId
#undef tskWindowId
#undef tskNatureId              
#undef tskFuncId                
#undef tskIVsCounter            
#undef tskHpIVs                 
#undef tskAtkIVs                
#undef tskDefIVs                
#undef tskSpatkIVs              
#undef tskSpdefIVs              
#undef tskSpdIVs                