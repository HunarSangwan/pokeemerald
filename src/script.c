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



static const u8 sCustomMonID[]         =   _("Species:  {STR_VAR_3}\nName: {STR_VAR_1}    \n\n{STR_VAR_2}");

static const u8 digitInidicator_1[]    =   _("{LEFT_ARROW}+1{RIGHT_ARROW}        ");
static const u8 digitInidicator_5[]    =   _("{LEFT_ARROW}+5{RIGHT_ARROW}        ");
static const u8 digitInidicator_10[]   =   _("{LEFT_ARROW}+10{RIGHT_ARROW}       ");
static const u8 digitInidicator_25[]   =   _("{LEFT_ARROW}+25{RIGHT_ARROW}       ");
static const u8 digitInidicator_50[]   =   _("{LEFT_ARROW}+50{RIGHT_ARROW}       ");
static const u8 digitInidicator_100[]  =   _("{LEFT_ARROW}+100{RIGHT_ARROW}      ");

static const u8 * const sText_DigitIndicator[] =
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

// This runs every frame, recieves input and updates the information shown
void Task_HandleCustomStarterChoiceMenuInput(u8 taskId)
{
    if (JOY_NEW(DPAD_ANY))
    {
        PlaySE(SE_SELECT);

        // Deals with selecting a mon
        if (JOY_NEW(DPAD_UP))
        {
            gTasks[taskId].tskSpeciesId += sPowersofDigitIndicator[gTasks[taskId].tskDigitSelectedIndex];
            if (gTasks[taskId].tskSpeciesId >= NUM_SPECIES)
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
        StringExpandPlaceholders(gStringVar4, sCustomMonID);
        AddTextPrinterParameterized(gTasks[taskId].tskWindowId, 1, gStringVar4, 1, 1, 0, NULL);

        // Destroy the old sprite and reload the new one
        FreeAndDestroyMonIconSprite(&gSprites[gTasks[taskId].tskMonSpriteIconId]);          // Destroy previous icon
        FreeMonIconPalettes();                                                              // Free space for new pallete
        LoadMonIconPalette(gTasks[taskId].tskSpeciesId);                                    // Loads pallete for current mon
        gTasks[taskId].tskMonSpriteIconId = CreateMonIcon(gTasks[taskId].tskSpeciesId, SpriteCB_MonIcon, 210, 50, 4, 0, 0); //Create pokemon sprite
        gSprites[gTasks[taskId].tskMonSpriteIconId].oam.priority = 0;
    }

    // Handle the A_BUTTON/B_BUTTON input
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        // Destroy the Icon Sprite
        FreeMonIconPalettes();
        FreeAndDestroyMonIconSprite(&gSprites[gTasks[taskId].tskMonSpriteIconId]);

        // Set VAR_CUSTOM_STARTER 
        PlaySE(MUS_LEVEL_UP);
        if (JOY_NEW(A_BUTTON))
            VarSet(VAR_CUSTOM_STARTER, gTasks[taskId].tskSpeciesId);
        else
            VarSet(VAR_CUSTOM_STARTER, 0);  // Default to the regular Hoenn starters

        // Destroy window and restore the player
        ClearStdWindowAndFrame(gTasks[taskId].tskWindowId, TRUE);
        RemoveWindow(gTasks[taskId].tskWindowId);
        ScriptContext_Enable();
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        DestroyTask(taskId);

        return;
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
    StringExpandPlaceholders(gStringVar4, sCustomMonID);
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