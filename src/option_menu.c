#include "global.h"
#include "option_menu.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "gba/m4a_internal.h"
#include "constants/rgb.h"
#include "event_data.h"

#define tMenuSelection data[0]
#define tTextSpeed data[1]
#define tBattleSceneOff data[2]
#define tBattleStyle data[3]
#define tSound data[4]
#define tButtonMode data[5]
#define tStarterChoice data[6]

enum
{
    MENUITEM_TEXTSPEED,
    MENUITEM_BATTLESCENE,
    MENUITEM_BATTLESTYLE,
    MENUITEM_SOUND,
    MENUITEM_BUTTONMODE,
    MENUITEM_STARTERCHOICE,
    MENUITEM_CANCEL,
    MENUITEM_COUNT,
};

enum
{
    WIN_HEADER,
    WIN_OPTIONS
};

#define YPOS_TEXTSPEED        (MENUITEM_TEXTSPEED * 16)
#define YPOS_BATTLESCENE      (MENUITEM_BATTLESCENE * 16)
#define YPOS_BATTLESTYLE      (MENUITEM_BATTLESTYLE * 16)
#define YPOS_SOUND            (MENUITEM_SOUND * 16)
#define YPOS_BUTTONMODE       (MENUITEM_BUTTONMODE * 16)
#define YPOS_STARTERCHOICE    (MENUITEM_STARTERCHOICE * 16)

static void Task_OptionMenuFadeIn(u8 taskId);
static void Task_OptionMenuProcessInput(u8 taskId);
static void Task_OptionMenuSave(u8 taskId);
static void Task_OptionMenuFadeOut(u8 taskId);
static void HighlightOptionMenuItem(u8 selection);
static u8 TextSpeed_ProcessInput(u8 selection);
static void TextSpeed_DrawChoices(u8 selection);
static u8 BattleScene_ProcessInput(u8 selection);
static void BattleScene_DrawChoices(u8 selection);
static u8 BattleStyle_ProcessInput(u8 selection);
static void BattleStyle_DrawChoices(u8 selection);
static u8 Sound_ProcessInput(u8 selection);
static void Sound_DrawChoices(u8 selection);
static u8 StarterChoice_ProcessInput(u8 selection);
static void StarterChoice_DrawChoices(u8 selection);
static u8 ButtonMode_ProcessInput(u8 selection);
static void ButtonMode_DrawChoices(u8 selection);
static void DrawHeaderText(void);
static void DrawOptionMenuTexts(void);
static void DrawBgWindowFrames(void);

EWRAM_DATA static bool8 sArrowPressed = FALSE;

static const u16 sOptionMenuText_Pal[] = INCBIN_U16("graphics/interface/option_menu_text.gbapal");
// note: this is only used in the Japanese release
static const u8 sEqualSignGfx[] = INCBIN_U8("graphics/interface/option_menu_equals_sign.4bpp");

static const u8 *const sOptionMenuItemsNames[MENUITEM_COUNT] =
{
    [MENUITEM_TEXTSPEED]       = gText_TextSpeed,
    [MENUITEM_BATTLESCENE]     = gText_BattleScene,
    [MENUITEM_BATTLESTYLE]     = gText_BattleStyle,
    [MENUITEM_SOUND]           = gText_Sound,
    [MENUITEM_BUTTONMODE]      = gText_ButtonMode,
    [MENUITEM_STARTERCHOICE]   = gText_StarterChoice,
    [MENUITEM_CANCEL]          = gText_OptionMenuCancel,
};

static const struct WindowTemplate sOptionMenuWinTemplates[] =
{
    [WIN_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 14,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sOptionMenuBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const u16 sOptionMenuBg_Pal[] = {RGB(17, 18, 31)};

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitOptionMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sOptionMenuBgTemplates, ARRAY_COUNT(sOptionMenuBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        InitWindows(sOptionMenuWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_DARKEN);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sOptionMenuBg_Pal, BG_PLTT_ID(0), sizeof(sOptionMenuBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sOptionMenuText_Pal, BG_PLTT_ID(1), sizeof(sOptionMenuText_Pal));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        DrawHeaderText();
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_OPTIONS);
        DrawOptionMenuTexts();
        gMain.state++;
    case 9:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 10:
    {
        u8 taskId = CreateTask(Task_OptionMenuFadeIn, 0);

        gTasks[taskId].tMenuSelection = 0;
        gTasks[taskId].tTextSpeed = gSaveBlock2Ptr->optionsTextSpeed;
        gTasks[taskId].tBattleSceneOff = gSaveBlock2Ptr->optionsBattleSceneOff;
        gTasks[taskId].tBattleStyle = gSaveBlock2Ptr->optionsBattleStyle;
        gTasks[taskId].tSound = gSaveBlock2Ptr->optionsSound;
        gTasks[taskId].tButtonMode = gSaveBlock2Ptr->optionsButtonMode;
        gTasks[taskId].tStarterChoice = gSaveBlock2Ptr->optionsStarterChoice;

        TextSpeed_DrawChoices(gTasks[taskId].tTextSpeed);
        BattleScene_DrawChoices(gTasks[taskId].tBattleSceneOff);
        BattleStyle_DrawChoices(gTasks[taskId].tBattleStyle);
        Sound_DrawChoices(gTasks[taskId].tSound);
        ButtonMode_DrawChoices(gTasks[taskId].tButtonMode);
        StarterChoice_DrawChoices(gTasks[taskId].tStarterChoice);
        HighlightOptionMenuItem(gTasks[taskId].tMenuSelection);

        CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
        gMain.state++;
        break;
    }
    case 11:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_OptionMenuFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_OptionMenuProcessInput;
}

static void Task_OptionMenuProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tMenuSelection == MENUITEM_CANCEL)
            gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_OptionMenuSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gTasks[taskId].tMenuSelection > 0)
            gTasks[taskId].tMenuSelection--;
        else
            gTasks[taskId].tMenuSelection = MENUITEM_CANCEL;
        HighlightOptionMenuItem(gTasks[taskId].tMenuSelection);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (gTasks[taskId].tMenuSelection < MENUITEM_CANCEL)
            gTasks[taskId].tMenuSelection++;
        else
            gTasks[taskId].tMenuSelection = 0;
        HighlightOptionMenuItem(gTasks[taskId].tMenuSelection);
    }
    else
    {
        u8 previousOption;

        switch (gTasks[taskId].tMenuSelection)
        {
        case MENUITEM_TEXTSPEED:
            previousOption = gTasks[taskId].tTextSpeed;
            gTasks[taskId].tTextSpeed = TextSpeed_ProcessInput(gTasks[taskId].tTextSpeed);

            if (previousOption != gTasks[taskId].tTextSpeed)
                TextSpeed_DrawChoices(gTasks[taskId].tTextSpeed);
            break;
        case MENUITEM_BATTLESCENE:
            previousOption = gTasks[taskId].tBattleSceneOff;
            gTasks[taskId].tBattleSceneOff = BattleScene_ProcessInput(gTasks[taskId].tBattleSceneOff);

            if (previousOption != gTasks[taskId].tBattleSceneOff)
                BattleScene_DrawChoices(gTasks[taskId].tBattleSceneOff);
            break;
        case MENUITEM_BATTLESTYLE:
            previousOption = gTasks[taskId].tBattleStyle;
            gTasks[taskId].tBattleStyle = BattleStyle_ProcessInput(gTasks[taskId].tBattleStyle);

            if (previousOption != gTasks[taskId].tBattleStyle)
                BattleStyle_DrawChoices(gTasks[taskId].tBattleStyle);
            break;
        case MENUITEM_SOUND:
            previousOption = gTasks[taskId].tSound;
            gTasks[taskId].tSound = Sound_ProcessInput(gTasks[taskId].tSound);

            if (previousOption != gTasks[taskId].tSound)
                Sound_DrawChoices(gTasks[taskId].tSound);
            break;
        case MENUITEM_BUTTONMODE:
            previousOption = gTasks[taskId].tButtonMode;
            gTasks[taskId].tButtonMode = ButtonMode_ProcessInput(gTasks[taskId].tButtonMode);

            if (previousOption != gTasks[taskId].tButtonMode)
                ButtonMode_DrawChoices(gTasks[taskId].tButtonMode);
            break;
        case MENUITEM_STARTERCHOICE:
            previousOption = gTasks[taskId].tStarterChoice;
            gTasks[taskId].tStarterChoice = StarterChoice_ProcessInput(gTasks[taskId].tStarterChoice);

            if (previousOption != gTasks[taskId].tStarterChoice)
                StarterChoice_DrawChoices(gTasks[taskId].tStarterChoice);
            break;
        default:
            return;
        }

        if (sArrowPressed)
        {
            sArrowPressed = FALSE;
            CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
        }
    }
}

static void Task_OptionMenuSave(u8 taskId)
{
    gSaveBlock2Ptr->optionsTextSpeed = gTasks[taskId].tTextSpeed;
    gSaveBlock2Ptr->optionsBattleSceneOff = gTasks[taskId].tBattleSceneOff;
    gSaveBlock2Ptr->optionsBattleStyle = gTasks[taskId].tBattleStyle;
    gSaveBlock2Ptr->optionsSound = gTasks[taskId].tSound;
    gSaveBlock2Ptr->optionsButtonMode = gTasks[taskId].tButtonMode;
    gSaveBlock2Ptr->optionsStarterChoice = gTasks[taskId].tStarterChoice;

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_OptionMenuFadeOut;
}

static void Task_OptionMenuFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

static void HighlightOptionMenuItem(u8 index)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(index * 16 + 40, index * 16 + 56));
}

static void DrawOptionMenuChoice(const u8 *text, u8 x, u8 y, u8 style)
{
    u8 dst[16];
    u16 i;

    for (i = 0; *text != EOS && i < ARRAY_COUNT(dst) - 1; i++)
        dst[i] = *(text++);

    if (style != 0)
    {
        dst[2] = TEXT_COLOR_RED;
        dst[5] = TEXT_COLOR_LIGHT_RED;
    }

    dst[i] = EOS;
    AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, dst, x, y + 1, TEXT_SKIP_DRAW, NULL);
}

static u8 TextSpeed_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection <= 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
            selection--;
        else
            selection = 2;

        sArrowPressed = TRUE;
    }
    return selection;
}

static void TextSpeed_DrawChoices(u8 selection)
{
    u8 styles[3];
    s32 widthSlow, widthMid, widthFast, xMid;

    styles[0] = 0;
    styles[1] = 0;
    styles[2] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_TextSpeedSlow, 104, YPOS_TEXTSPEED, styles[0]);

    widthSlow = GetStringWidth(FONT_NORMAL, gText_TextSpeedSlow, 0);
    widthMid = GetStringWidth(FONT_NORMAL, gText_TextSpeedMid, 0);
    widthFast = GetStringWidth(FONT_NORMAL, gText_TextSpeedFast, 0);

    widthMid -= 94;
    xMid = (widthSlow - widthMid - widthFast) / 2 + 104;
    DrawOptionMenuChoice(gText_TextSpeedMid, xMid, YPOS_TEXTSPEED, styles[1]);

    DrawOptionMenuChoice(gText_TextSpeedFast, GetStringRightAlignXOffset(FONT_NORMAL, gText_TextSpeedFast, 198), YPOS_TEXTSPEED, styles[2]);
}

static u8 BattleScene_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void BattleScene_DrawChoices(u8 selection)
{
    u8 styles[2];

    styles[0] = 0;
    styles[1] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_BattleSceneOn, 104, YPOS_BATTLESCENE, styles[0]);
    DrawOptionMenuChoice(gText_BattleSceneOff, GetStringRightAlignXOffset(FONT_NORMAL, gText_BattleSceneOff, 198), YPOS_BATTLESCENE, styles[1]);
}

static u8 BattleStyle_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        sArrowPressed = TRUE;
    }

    return selection;
}

static void BattleStyle_DrawChoices(u8 selection)
{
    u8 styles[2];

    styles[0] = 0;
    styles[1] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_BattleStyleShift, 104, YPOS_BATTLESTYLE, styles[0]);
    DrawOptionMenuChoice(gText_BattleStyleSet, GetStringRightAlignXOffset(FONT_NORMAL, gText_BattleStyleSet, 198), YPOS_BATTLESTYLE, styles[1]);
}

static u8 Sound_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT))
    {
        selection ^= 1;
        SetPokemonCryStereo(selection);
        sArrowPressed = TRUE;
    }

    return selection;
}

static void Sound_DrawChoices(u8 selection)
{
    u8 styles[2];

    styles[0] = 0;
    styles[1] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_SoundMono, 104, YPOS_SOUND, styles[0]);
    DrawOptionMenuChoice(gText_SoundStereo, GetStringRightAlignXOffset(FONT_NORMAL, gText_SoundStereo, 198), YPOS_SOUND, styles[1]);
}

static u8 StarterChoice_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
			selection--;
        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_RIGHT))
    {
	if (selection < 386)
		selection++;
	sArrowPressed = TRUE;
    }

    return selection;
}

static void StarterChoice_DrawChoices(u8 selection)
{
    switch (selection)
	{   
        case 0:
			DrawOptionMenuChoice(gText_Default, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 1:
        DrawOptionMenuChoice(gText_BULBASAUR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 2:
        DrawOptionMenuChoice(gText_IVYSAUR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 3:
        DrawOptionMenuChoice(gText_VENUSAUR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 4:
        DrawOptionMenuChoice(gText_CHARMANDER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 5:
        DrawOptionMenuChoice(gText_CHARMELEON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 6:
        DrawOptionMenuChoice(gText_CHARIZARD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 7:
        DrawOptionMenuChoice(gText_SQUIRTLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 8:
        DrawOptionMenuChoice(gText_WARTORTLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 9:
        DrawOptionMenuChoice(gText_BLASTOISE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 10:
        DrawOptionMenuChoice(gText_CATERPIE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 11:
        DrawOptionMenuChoice(gText_METAPOD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 12:
        DrawOptionMenuChoice(gText_BUTTERFREE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 13:
        DrawOptionMenuChoice(gText_WEEDLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 14:
        DrawOptionMenuChoice(gText_KAKUNA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 15:
        DrawOptionMenuChoice(gText_BEEDRILL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 16:
        DrawOptionMenuChoice(gText_PIDGEY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 17:
        DrawOptionMenuChoice(gText_PIDGEOTTO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 18:
        DrawOptionMenuChoice(gText_PIDGEOT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 19:
        DrawOptionMenuChoice(gText_RATTATA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 20:
        DrawOptionMenuChoice(gText_RATICATE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 21:
        DrawOptionMenuChoice(gText_SPEAROW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 22:
        DrawOptionMenuChoice(gText_FEAROW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 23:
        DrawOptionMenuChoice(gText_EKANS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 24:
        DrawOptionMenuChoice(gText_ARBOK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 25:
        DrawOptionMenuChoice(gText_PIKACHU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 26:
        DrawOptionMenuChoice(gText_RAICHU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 27:
        DrawOptionMenuChoice(gText_SANDSHREW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 28:
        DrawOptionMenuChoice(gText_SANDSLASH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 29:
        DrawOptionMenuChoice(gText_NIDORANF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 30:
        DrawOptionMenuChoice(gText_NIDORINA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 31:
        DrawOptionMenuChoice(gText_NIDOQUEEN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 32:
        DrawOptionMenuChoice(gText_NIDORANM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 33:
        DrawOptionMenuChoice(gText_NIDORINO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 34:
        DrawOptionMenuChoice(gText_NIDOKING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 35:
        DrawOptionMenuChoice(gText_CLEFAIRY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 36:
        DrawOptionMenuChoice(gText_CLEFABLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 37:
        DrawOptionMenuChoice(gText_VULPIX, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 38:
        DrawOptionMenuChoice(gText_NINETALES, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 39:
        DrawOptionMenuChoice(gText_JIGGLYPUFF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 40:
        DrawOptionMenuChoice(gText_WIGGLYTUFF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 41:
        DrawOptionMenuChoice(gText_ZUBAT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 42:
        DrawOptionMenuChoice(gText_GOLBAT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 43:
        DrawOptionMenuChoice(gText_ODDISH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 44:
        DrawOptionMenuChoice(gText_GLOOM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 45:
        DrawOptionMenuChoice(gText_VILEPLUME, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 46:
        DrawOptionMenuChoice(gText_PARAS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 47:
        DrawOptionMenuChoice(gText_PARASECT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 48:
        DrawOptionMenuChoice(gText_VENONAT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 49:
        DrawOptionMenuChoice(gText_VENOMOTH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 50:
        DrawOptionMenuChoice(gText_DIGLETT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 51:
        DrawOptionMenuChoice(gText_DUGTRIO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 52:
        DrawOptionMenuChoice(gText_MEOWTH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 53:
        DrawOptionMenuChoice(gText_PERSIAN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 54:
        DrawOptionMenuChoice(gText_PSYDUCK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 55:
        DrawOptionMenuChoice(gText_GOLDUCK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 56:
        DrawOptionMenuChoice(gText_MANKEY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 57:
        DrawOptionMenuChoice(gText_PRIMEAPE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 58:
        DrawOptionMenuChoice(gText_GROWLITHE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 59:
        DrawOptionMenuChoice(gText_ARCANINE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 60:
        DrawOptionMenuChoice(gText_POLIWAG, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 61:
        DrawOptionMenuChoice(gText_POLIWHIRL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 62:
        DrawOptionMenuChoice(gText_POLIWRATH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 63:
        DrawOptionMenuChoice(gText_ABRA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 64:
        DrawOptionMenuChoice(gText_KADABRA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 65:
        DrawOptionMenuChoice(gText_ALAKAZAM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 66:
        DrawOptionMenuChoice(gText_MACHOP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 67:
        DrawOptionMenuChoice(gText_MACHOKE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 68:
        DrawOptionMenuChoice(gText_MACHAMP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 69:
        DrawOptionMenuChoice(gText_BELLSPROUT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 70:
        DrawOptionMenuChoice(gText_WEEPINBELL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 71:
        DrawOptionMenuChoice(gText_VICTREEBEL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 72:
        DrawOptionMenuChoice(gText_TENTACOOL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 73:
        DrawOptionMenuChoice(gText_TENTACRUEL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 74:
        DrawOptionMenuChoice(gText_GEODUDE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 75:
        DrawOptionMenuChoice(gText_GRAVELER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 76:
        DrawOptionMenuChoice(gText_GOLEM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 77:
        DrawOptionMenuChoice(gText_PONYTA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 78:
        DrawOptionMenuChoice(gText_RAPIDASH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 79:
        DrawOptionMenuChoice(gText_SLOWPOKE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 80:
        DrawOptionMenuChoice(gText_SLOWBRO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 81:
        DrawOptionMenuChoice(gText_MAGNEMITE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 82:
        DrawOptionMenuChoice(gText_MAGNETON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 83:
        DrawOptionMenuChoice(gText_FARFETCHD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 84:
        DrawOptionMenuChoice(gText_DODUO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 85:
        DrawOptionMenuChoice(gText_DODRIO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 86:
        DrawOptionMenuChoice(gText_SEEL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 87:
        DrawOptionMenuChoice(gText_DEWGONG, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 88:
        DrawOptionMenuChoice(gText_GRIMER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 89:
        DrawOptionMenuChoice(gText_MUK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 90:
        DrawOptionMenuChoice(gText_SHELLDER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 91:
        DrawOptionMenuChoice(gText_CLOYSTER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 92:
        DrawOptionMenuChoice(gText_GASTLY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 93:
        DrawOptionMenuChoice(gText_HAUNTER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 94:
        DrawOptionMenuChoice(gText_GENGAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 95:
        DrawOptionMenuChoice(gText_ONIX, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 96:
        DrawOptionMenuChoice(gText_DROWZEE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 97:
        DrawOptionMenuChoice(gText_HYPNO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 98:
        DrawOptionMenuChoice(gText_KRABBY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 99:
        DrawOptionMenuChoice(gText_KINGLER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 100:
        DrawOptionMenuChoice(gText_VOLTORB, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 101:
        DrawOptionMenuChoice(gText_ELECTRODE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 102:
        DrawOptionMenuChoice(gText_EXEGGCUTE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 103:
        DrawOptionMenuChoice(gText_EXEGGUTOR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 104:
        DrawOptionMenuChoice(gText_CUBONE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 105:
        DrawOptionMenuChoice(gText_MAROWAK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 106:
        DrawOptionMenuChoice(gText_HITMONLEE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 107:
        DrawOptionMenuChoice(gText_HITMONCHAN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 108:
        DrawOptionMenuChoice(gText_LICKITUNG, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 109:
        DrawOptionMenuChoice(gText_KOFFING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 110:
        DrawOptionMenuChoice(gText_WEEZING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 111:
        DrawOptionMenuChoice(gText_RHYHORN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 112:
        DrawOptionMenuChoice(gText_RHYDON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 113:
        DrawOptionMenuChoice(gText_CHANSEY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 114:
        DrawOptionMenuChoice(gText_TANGELA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 115:
        DrawOptionMenuChoice(gText_KANGASKHAN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 116:
        DrawOptionMenuChoice(gText_HORSEA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 117:
        DrawOptionMenuChoice(gText_SEADRA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 118:
        DrawOptionMenuChoice(gText_GOLDEEN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 119:
        DrawOptionMenuChoice(gText_SEAKING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 120:
        DrawOptionMenuChoice(gText_STARYU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 121:
        DrawOptionMenuChoice(gText_STARMIE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 122:
        DrawOptionMenuChoice(gText_MR_MIME, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 123:
        DrawOptionMenuChoice(gText_SCYTHER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 124:
        DrawOptionMenuChoice(gText_JYNX, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 125:
        DrawOptionMenuChoice(gText_ELECTABUZZ, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 126:
        DrawOptionMenuChoice(gText_MAGMAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 127:
        DrawOptionMenuChoice(gText_PINSIR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 128:
        DrawOptionMenuChoice(gText_TAUROS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 129:
        DrawOptionMenuChoice(gText_MAGIKARP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 130:
        DrawOptionMenuChoice(gText_GYARADOS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 131:
        DrawOptionMenuChoice(gText_LAPRAS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 132:
        DrawOptionMenuChoice(gText_DITTO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 133:
        DrawOptionMenuChoice(gText_EEVEE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 134:
        DrawOptionMenuChoice(gText_VAPOREON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 135:
        DrawOptionMenuChoice(gText_JOLTEON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 136:
        DrawOptionMenuChoice(gText_FLAREON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 137:
        DrawOptionMenuChoice(gText_PORYGON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 138:
        DrawOptionMenuChoice(gText_OMANYTE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 139:
        DrawOptionMenuChoice(gText_OMASTAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 140:
        DrawOptionMenuChoice(gText_KABUTO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 141:
        DrawOptionMenuChoice(gText_KABUTOPS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 142:
        DrawOptionMenuChoice(gText_AERODACTYL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 143:
        DrawOptionMenuChoice(gText_SNORLAX, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 144:
        DrawOptionMenuChoice(gText_ARTICUNO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 145:
        DrawOptionMenuChoice(gText_ZAPDOS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 146:
        DrawOptionMenuChoice(gText_MOLTRES, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 147:
        DrawOptionMenuChoice(gText_DRATINI, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 148:
        DrawOptionMenuChoice(gText_DRAGONAIR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 149:
        DrawOptionMenuChoice(gText_DRAGONITE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 150:
        DrawOptionMenuChoice(gText_MEWTWO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 151:
        DrawOptionMenuChoice(gText_MEW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 152:
        DrawOptionMenuChoice(gText_CHIKORITA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 153:
        DrawOptionMenuChoice(gText_BAYLEEF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 154:
        DrawOptionMenuChoice(gText_MEGANIUM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 155:
        DrawOptionMenuChoice(gText_CYNDAQUIL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 156:
        DrawOptionMenuChoice(gText_QUILAVA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 157:
        DrawOptionMenuChoice(gText_TYPHLOSION, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 158:
        DrawOptionMenuChoice(gText_TOTODILE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 159:
        DrawOptionMenuChoice(gText_CROCONAW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 160:
        DrawOptionMenuChoice(gText_FERALIGATR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 161:
        DrawOptionMenuChoice(gText_SENTRET, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 162:
        DrawOptionMenuChoice(gText_FURRET, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 163:
        DrawOptionMenuChoice(gText_HOOTHOOT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 164:
        DrawOptionMenuChoice(gText_NOCTOWL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 165:
        DrawOptionMenuChoice(gText_LEDYBA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 166:
        DrawOptionMenuChoice(gText_LEDIAN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 167:
        DrawOptionMenuChoice(gText_SPINARAK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 168:
        DrawOptionMenuChoice(gText_ARIADOS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 169:
        DrawOptionMenuChoice(gText_CROBAT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 170:
        DrawOptionMenuChoice(gText_CHINCHOU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 171:
        DrawOptionMenuChoice(gText_LANTURN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 172:
        DrawOptionMenuChoice(gText_PICHU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 173:
        DrawOptionMenuChoice(gText_CLEFFA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 174:
        DrawOptionMenuChoice(gText_IGGLYBUFF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 175:
        DrawOptionMenuChoice(gText_TOGEPI, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 176:
        DrawOptionMenuChoice(gText_TOGETIC, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 177:
        DrawOptionMenuChoice(gText_NATU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 178:
        DrawOptionMenuChoice(gText_XATU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 179:
        DrawOptionMenuChoice(gText_MAREEP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 180:
        DrawOptionMenuChoice(gText_FLAAFFY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 181:
        DrawOptionMenuChoice(gText_AMPHAROS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 182:
        DrawOptionMenuChoice(gText_BELLOSSOM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 183:
        DrawOptionMenuChoice(gText_MARILL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 184:
        DrawOptionMenuChoice(gText_AZUMARILL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 185:
        DrawOptionMenuChoice(gText_SUDOWOODO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 186:
        DrawOptionMenuChoice(gText_POLITOED, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 187:
        DrawOptionMenuChoice(gText_HOPPIP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 188:
        DrawOptionMenuChoice(gText_SKIPLOOM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 189:
        DrawOptionMenuChoice(gText_JUMPLUFF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 190:
        DrawOptionMenuChoice(gText_AIPOM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 191:
        DrawOptionMenuChoice(gText_SUNKERN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 192:
        DrawOptionMenuChoice(gText_SUNFLORA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 193:
        DrawOptionMenuChoice(gText_YANMA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 194:
        DrawOptionMenuChoice(gText_WOOPER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 195:
        DrawOptionMenuChoice(gText_QUAGSIRE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 196:
        DrawOptionMenuChoice(gText_ESPEON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 197:
        DrawOptionMenuChoice(gText_UMBREON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 198:
        DrawOptionMenuChoice(gText_MURKROW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 199:
        DrawOptionMenuChoice(gText_SLOWKING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 200:
        DrawOptionMenuChoice(gText_MISDREAVUS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 201:
        DrawOptionMenuChoice(gText_UNOWN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 202:
        DrawOptionMenuChoice(gText_WOBBUFFET, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 203:
        DrawOptionMenuChoice(gText_GIRAFARIG, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 204:
        DrawOptionMenuChoice(gText_PINECO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 205:
        DrawOptionMenuChoice(gText_FORRETRESS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 206:
        DrawOptionMenuChoice(gText_DUNSPARCE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 207:
        DrawOptionMenuChoice(gText_GLIGAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 208:
        DrawOptionMenuChoice(gText_STEELIX, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 209:
        DrawOptionMenuChoice(gText_SNUBBULL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 210:
        DrawOptionMenuChoice(gText_GRANBULL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 211:
        DrawOptionMenuChoice(gText_QWILFISH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 212:
        DrawOptionMenuChoice(gText_SCIZOR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 213:
        DrawOptionMenuChoice(gText_SHUCKLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 214:
        DrawOptionMenuChoice(gText_HERACROSS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 215:
        DrawOptionMenuChoice(gText_SNEASEL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 216:
        DrawOptionMenuChoice(gText_TEDDIURSA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 217:
        DrawOptionMenuChoice(gText_URSARING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 218:
        DrawOptionMenuChoice(gText_SLUGMA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 219:
        DrawOptionMenuChoice(gText_MAGCARGO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 220:
        DrawOptionMenuChoice(gText_SWINUB, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 221:
        DrawOptionMenuChoice(gText_PILOSWINE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 222:
        DrawOptionMenuChoice(gText_CORSOLA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 223:
        DrawOptionMenuChoice(gText_REMORAID, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 224:
        DrawOptionMenuChoice(gText_OCTILLERY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 225:
        DrawOptionMenuChoice(gText_DELIBIRD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 226:
        DrawOptionMenuChoice(gText_MANTINE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 227:
        DrawOptionMenuChoice(gText_SKARMORY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 228:
        DrawOptionMenuChoice(gText_HOUNDOUR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 229:
        DrawOptionMenuChoice(gText_HOUNDOOM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 230:
        DrawOptionMenuChoice(gText_KINGDRA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 231:
        DrawOptionMenuChoice(gText_PHANPY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 232:
        DrawOptionMenuChoice(gText_DONPHAN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 233:
        DrawOptionMenuChoice(gText_PORYGON2, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 234:
        DrawOptionMenuChoice(gText_STANTLER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 235:
        DrawOptionMenuChoice(gText_SMEARGLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 236:
        DrawOptionMenuChoice(gText_TYROGUE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 237:
        DrawOptionMenuChoice(gText_HITMONTOP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 238:
        DrawOptionMenuChoice(gText_SMOOCHUM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 239:
        DrawOptionMenuChoice(gText_ELEKID, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 240:
        DrawOptionMenuChoice(gText_MAGBY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 241:
        DrawOptionMenuChoice(gText_MILTANK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 242:
        DrawOptionMenuChoice(gText_BLISSEY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 243:
        DrawOptionMenuChoice(gText_RAIKOU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 244:
        DrawOptionMenuChoice(gText_ENTEI, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 245:
        DrawOptionMenuChoice(gText_SUICUNE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 246:
        DrawOptionMenuChoice(gText_LARVITAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 247:
        DrawOptionMenuChoice(gText_PUPITAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 248:
        DrawOptionMenuChoice(gText_TYRANITAR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 249:
        DrawOptionMenuChoice(gText_LUGIA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 250:
        DrawOptionMenuChoice(gText_HOOH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 251:
        DrawOptionMenuChoice(gText_CELEBI, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 252:
        DrawOptionMenuChoice(gText_TREECKO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 253:
        DrawOptionMenuChoice(gText_GROVYLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 254:
        DrawOptionMenuChoice(gText_SCEPTILE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 255:
        DrawOptionMenuChoice(gText_TORCHIC, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 256:
        DrawOptionMenuChoice(gText_COMBUSKEN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 257:
        DrawOptionMenuChoice(gText_BLAZIKEN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 258:
        DrawOptionMenuChoice(gText_MUDKIP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 259:
        DrawOptionMenuChoice(gText_MARSHTOMP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 260:
        DrawOptionMenuChoice(gText_SWAMPERT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 261:
        DrawOptionMenuChoice(gText_POOCHYENA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 262:
        DrawOptionMenuChoice(gText_MIGHTYENA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 263:
        DrawOptionMenuChoice(gText_ZIGZAGOON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 264:
        DrawOptionMenuChoice(gText_LINOONE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 265:
        DrawOptionMenuChoice(gText_WURMPLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 266:
        DrawOptionMenuChoice(gText_SILCOON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 267:
        DrawOptionMenuChoice(gText_BEAUTIFLY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 268:
        DrawOptionMenuChoice(gText_CASCOON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 269:
        DrawOptionMenuChoice(gText_DUSTOX, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 270:
        DrawOptionMenuChoice(gText_LOTAD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 271:
        DrawOptionMenuChoice(gText_LOMBRE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 272:
        DrawOptionMenuChoice(gText_LUDICOLO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 273:
        DrawOptionMenuChoice(gText_SEEDOT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 274:
        DrawOptionMenuChoice(gText_NUZLEAF, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 275:
        DrawOptionMenuChoice(gText_SHIFTRY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 276:
        DrawOptionMenuChoice(gText_TAILLOW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 277:
        DrawOptionMenuChoice(gText_SWELLOW, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 278:
        DrawOptionMenuChoice(gText_WINGULL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 279:
        DrawOptionMenuChoice(gText_PELIPPER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 280:
        DrawOptionMenuChoice(gText_RALTS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 281:
        DrawOptionMenuChoice(gText_KIRLIA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 282:
        DrawOptionMenuChoice(gText_GARDEVOIR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 283:
        DrawOptionMenuChoice(gText_SURSKIT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 284:
        DrawOptionMenuChoice(gText_MASQUERAIN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 285:
        DrawOptionMenuChoice(gText_SHROOMISH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 286:
        DrawOptionMenuChoice(gText_BRELOOM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 287:
        DrawOptionMenuChoice(gText_SLAKOTH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 288:
        DrawOptionMenuChoice(gText_VIGOROTH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 289:
        DrawOptionMenuChoice(gText_SLAKING, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 290:
        DrawOptionMenuChoice(gText_NINCADA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 291:
        DrawOptionMenuChoice(gText_NINJASK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 292:
        DrawOptionMenuChoice(gText_SHEDINJA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 293:
        DrawOptionMenuChoice(gText_WHISMUR, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 294:
        DrawOptionMenuChoice(gText_LOUDRED, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 295:
        DrawOptionMenuChoice(gText_EXPLOUD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 296:
        DrawOptionMenuChoice(gText_MAKUHITA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 297:
        DrawOptionMenuChoice(gText_HARIYAMA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 298:
        DrawOptionMenuChoice(gText_AZURILL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 299:
        DrawOptionMenuChoice(gText_NOSEPASS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 300:
        DrawOptionMenuChoice(gText_SKITTY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 301:
        DrawOptionMenuChoice(gText_DELCATTY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 302:
        DrawOptionMenuChoice(gText_SABLEYE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 303:
        DrawOptionMenuChoice(gText_MAWILE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 304:
        DrawOptionMenuChoice(gText_ARON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 305:
        DrawOptionMenuChoice(gText_LAIRON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 306:
        DrawOptionMenuChoice(gText_AGGRON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 307:
        DrawOptionMenuChoice(gText_MEDITITE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 308:
        DrawOptionMenuChoice(gText_MEDICHAM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 309:
        DrawOptionMenuChoice(gText_ELECTRIKE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 310:
        DrawOptionMenuChoice(gText_MANECTRIC, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 311:
        DrawOptionMenuChoice(gText_PLUSLE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 312:
        DrawOptionMenuChoice(gText_MINUN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 313:
        DrawOptionMenuChoice(gText_VOLBEAT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 314:
        DrawOptionMenuChoice(gText_ILLUMISE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 315:
        DrawOptionMenuChoice(gText_ROSELIA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 316:
        DrawOptionMenuChoice(gText_GULPIN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 317:
        DrawOptionMenuChoice(gText_SWALOT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 318:
        DrawOptionMenuChoice(gText_CARVANHA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 319:
        DrawOptionMenuChoice(gText_SHARPEDO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 320:
        DrawOptionMenuChoice(gText_WAILMER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 321:
        DrawOptionMenuChoice(gText_WAILORD, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 322:
        DrawOptionMenuChoice(gText_NUMEL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 323:
        DrawOptionMenuChoice(gText_CAMERUPT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 324:
        DrawOptionMenuChoice(gText_TORKOAL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 325:
        DrawOptionMenuChoice(gText_SPOINK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 326:
        DrawOptionMenuChoice(gText_GRUMPIG, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 327:
        DrawOptionMenuChoice(gText_SPINDA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 328:
        DrawOptionMenuChoice(gText_TRAPINCH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 329:
        DrawOptionMenuChoice(gText_VIBRAVA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 330:
        DrawOptionMenuChoice(gText_FLYGON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 331:
        DrawOptionMenuChoice(gText_CACNEA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 332:
        DrawOptionMenuChoice(gText_CACTURNE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 333:
        DrawOptionMenuChoice(gText_SWABLU, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 334:
        DrawOptionMenuChoice(gText_ALTARIA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 335:
        DrawOptionMenuChoice(gText_ZANGOOSE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 336:
        DrawOptionMenuChoice(gText_SEVIPER, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 337:
        DrawOptionMenuChoice(gText_LUNATONE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 338:
        DrawOptionMenuChoice(gText_SOLROCK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 339:
        DrawOptionMenuChoice(gText_BARBOACH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 340:
        DrawOptionMenuChoice(gText_WHISCASH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 341:
        DrawOptionMenuChoice(gText_CORPHISH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 342:
        DrawOptionMenuChoice(gText_CRAWDAUNT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 343:
        DrawOptionMenuChoice(gText_BALTOY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 344:
        DrawOptionMenuChoice(gText_CLAYDOL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 345:
        DrawOptionMenuChoice(gText_LILEEP, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 346:
        DrawOptionMenuChoice(gText_CRADILY, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 347:
        DrawOptionMenuChoice(gText_ANORITH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 348:
        DrawOptionMenuChoice(gText_ARMALDO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 349:
        DrawOptionMenuChoice(gText_FEEBAS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 350:
        DrawOptionMenuChoice(gText_MILOTIC, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 351:
        DrawOptionMenuChoice(gText_CASTFORM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 352:
        DrawOptionMenuChoice(gText_KECLEON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 353:
        DrawOptionMenuChoice(gText_SHUPPET, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 354:
        DrawOptionMenuChoice(gText_BANETTE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 355:
        DrawOptionMenuChoice(gText_DUSKULL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 356:
        DrawOptionMenuChoice(gText_DUSCLOPS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 357:
        DrawOptionMenuChoice(gText_TROPIUS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 358:
        DrawOptionMenuChoice(gText_CHIMECHO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 359:
        DrawOptionMenuChoice(gText_ABSOL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 360:
        DrawOptionMenuChoice(gText_WYNAUT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 361:
        DrawOptionMenuChoice(gText_SNORUNT, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 362:
        DrawOptionMenuChoice(gText_GLALIE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 363:
        DrawOptionMenuChoice(gText_SPHEAL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 364:
        DrawOptionMenuChoice(gText_SEALEO, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 365:
        DrawOptionMenuChoice(gText_WALREIN, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 366:
        DrawOptionMenuChoice(gText_CLAMPERL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 367:
        DrawOptionMenuChoice(gText_HUNTAIL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 368:
        DrawOptionMenuChoice(gText_GOREBYSS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 369:
        DrawOptionMenuChoice(gText_RELICANTH, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 370:
        DrawOptionMenuChoice(gText_LUVDISC, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 371:
        DrawOptionMenuChoice(gText_BAGON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 372:
        DrawOptionMenuChoice(gText_SHELGON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 373:
        DrawOptionMenuChoice(gText_SALAMENCE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 374:
        DrawOptionMenuChoice(gText_BELDUM, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 375:
        DrawOptionMenuChoice(gText_METANG, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 376:
        DrawOptionMenuChoice(gText_METAGROSS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 377:
        DrawOptionMenuChoice(gText_REGIROCK, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 378:
        DrawOptionMenuChoice(gText_REGICE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 379:
        DrawOptionMenuChoice(gText_REGISTEEL, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 380:
        DrawOptionMenuChoice(gText_LATIAS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 381:
        DrawOptionMenuChoice(gText_LATIOS, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 382:
        DrawOptionMenuChoice(gText_KYOGRE, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 383:
        DrawOptionMenuChoice(gText_GROUDON, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 384:
        DrawOptionMenuChoice(gText_RAYQUAZA, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 385:
        DrawOptionMenuChoice(gText_JIRACHI, 104, YPOS_STARTERCHOICE, 0);
            VarSet(VAR_CUSTOM_STARTER, 0);
			break;
		case 386:
        DrawOptionMenuChoice(gText_DEOXYS, 104, YPOS_STARTERCHOICE, 0);;
	}
}

static u8 ButtonMode_ProcessInput(u8 selection)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection <= 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
            selection--;
        else
            selection = 2;

        sArrowPressed = TRUE;
    }
    return selection;
}

static void ButtonMode_DrawChoices(u8 selection)
{
    s32 widthNormal, widthLR, widthLA, xLR;
    u8 styles[3];

    styles[0] = 0;
    styles[1] = 0;
    styles[2] = 0;
    styles[selection] = 1;

    DrawOptionMenuChoice(gText_ButtonTypeNormal, 104, YPOS_BUTTONMODE, styles[0]);

    widthNormal = GetStringWidth(FONT_NORMAL, gText_ButtonTypeNormal, 0);
    widthLR = GetStringWidth(FONT_NORMAL, gText_ButtonTypeLR, 0);
    widthLA = GetStringWidth(FONT_NORMAL, gText_ButtonTypeLEqualsA, 0);

    widthLR -= 94;
    xLR = (widthNormal - widthLR - widthLA) / 2 + 104;
    DrawOptionMenuChoice(gText_ButtonTypeLR, xLR, YPOS_BUTTONMODE, styles[1]);

    DrawOptionMenuChoice(gText_ButtonTypeLEqualsA, GetStringRightAlignXOffset(FONT_NORMAL, gText_ButtonTypeLEqualsA, 198), YPOS_BUTTONMODE, styles[2]);
}

static void DrawHeaderText(void)
{
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, gText_Option, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

static void DrawOptionMenuTexts(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < MENUITEM_COUNT; i++)
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, sOptionMenuItemsNames[i], 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

static void DrawBgWindowFrames(void)
{
    //                     bg, tile,              x, y, width, height, palNum
    // Draw title window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  0, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  3,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  3, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28,  3,  1,  1,  7);

    // Draw options list window frame
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1, 18,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  5,  1, 18,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}
