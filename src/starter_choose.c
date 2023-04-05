#include "global.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "starter_choose.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
#include "trig.h"
#include "window.h"
#include "constants/songs.h"
#include "constants/rgb.h"

#define STARTER_MON_COUNT   3

// Position of the sprite of the selected starter Pokemon
#define STARTER_PKMN_POS_X (DISPLAY_WIDTH / 2)
#define STARTER_PKMN_POS_Y 64

#define TAG_POKEBALL_SELECT 0x1000
#define TAG_STARTER_CIRCLE  0x1001

static void CB2_StarterChoose(void);
static void ClearStarterLabel(void);
static void Task_StarterChoose(u8 taskId);
static void Task_HandleStarterChooseInput(u8 taskId);
static void Task_WaitForStarterSprite(u8 taskId);
static void Task_AskConfirmStarter(u8 taskId);
static void Task_HandleConfirmStarterInput(u8 taskId);
static void Task_DeclineStarter(u8 taskId);
static void Task_MoveStarterChooseCursor(u8 taskId);
static void Task_CreateStarterLabel(u8 taskId);
static void CreateStarterPokemonLabel(u8 selection);
static u8 CreatePokemonFrontSprite(u16 species, u8 x, u8 y);
static void SpriteCB_SelectionHand(struct Sprite *sprite);
static void SpriteCB_Pokeball(struct Sprite *sprite);
static void SpriteCB_StarterPokemon(struct Sprite *sprite);

static u16 sStarterLabelWindowId;

const u16 gBirchBagGrass_Pal[] = INCBIN_U16("graphics/starter_choose/tiles.gbapal");
static const u16 sPokeballSelection_Pal[] = INCBIN_U16("graphics/starter_choose/pokeball_selection.gbapal");
static const u16 sStarterCircle_Pal[] = INCBIN_U16("graphics/starter_choose/starter_circle.gbapal");
const u32 gBirchBagTilemap[] = INCBIN_U32("graphics/starter_choose/birch_bag.bin.lz");
const u32 gBirchGrassTilemap[] = INCBIN_U32("graphics/starter_choose/birch_grass.bin.lz");
const u32 gBirchBagGrass_Gfx[] = INCBIN_U32("graphics/starter_choose/tiles.4bpp.lz");
const u32 gPokeballSelection_Gfx[] = INCBIN_U32("graphics/starter_choose/pokeball_selection.4bpp.lz");
static const u32 sStarterCircle_Gfx[] = INCBIN_U32("graphics/starter_choose/starter_circle.4bpp.lz");

static const struct WindowTemplate sWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 15,
        .width = 24,
        .height = 4,
        .paletteNum = 14,
        .baseBlock = 0x0200
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sWindowTemplate_ConfirmStarter =
{
    .bg = 0,
    .tilemapLeft = 24,
    .tilemapTop = 9,
    .width = 5,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0x0260
};

static const struct WindowTemplate sWindowTemplate_StarterLabel =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 13,
    .height = 4,
    .paletteNum = 14,
    .baseBlock = 0x0274
};

static const u8 sPokeballCoords[STARTER_MON_COUNT][2] =
{
    {60, 64},
    {120, 88},
    {180, 64},
};

static const u8 sStarterLabelCoords[STARTER_MON_COUNT][2] =
{
    {0, 9},
    {16, 10},
    {8, 4},
};

static const u16 sStarterMon[STARTER_MON_COUNT] =
{
    SPECIES_TREECKO,//
    SPECIES_TORCHIC,
    SPECIES_MUDKIP,
};

static const u16 sStarterMonBulbasaur[STARTER_MON_COUNT] =
{
    SPECIES_BULBASAUR,
    SPECIES_BULBASAUR,
    SPECIES_BULBASAUR,
};

static const u16 sStarterMonIvysaur[STARTER_MON_COUNT] =
{
    SPECIES_IVYSAUR,
    SPECIES_IVYSAUR,
    SPECIES_IVYSAUR,
};

static const u16 sStarterMonVenusaur[STARTER_MON_COUNT] =
{
    SPECIES_VENUSAUR,
    SPECIES_VENUSAUR,
    SPECIES_VENUSAUR,
};

static const u16 sStarterMonCharmander[STARTER_MON_COUNT] =
{
    SPECIES_CHARMANDER,
    SPECIES_CHARMANDER,
    SPECIES_CHARMANDER,
};

static const u16 sStarterMonCharmeleon[STARTER_MON_COUNT] =
{
    SPECIES_CHARMELEON,
    SPECIES_CHARMELEON,
    SPECIES_CHARMELEON,
};

static const u16 sStarterMonCharizard[STARTER_MON_COUNT] =
{
    SPECIES_CHARIZARD,
    SPECIES_CHARIZARD,
    SPECIES_CHARIZARD,
};

static const u16 sStarterMonSquirtle[STARTER_MON_COUNT] =
{
    SPECIES_SQUIRTLE,
    SPECIES_SQUIRTLE,
    SPECIES_SQUIRTLE,
};

static const u16 sStarterMonWartortle[STARTER_MON_COUNT] =
{
    SPECIES_WARTORTLE,
    SPECIES_WARTORTLE,
    SPECIES_WARTORTLE,
};

static const u16 sStarterMonBlastoise[STARTER_MON_COUNT] =
{
    SPECIES_BLASTOISE,
    SPECIES_BLASTOISE,
    SPECIES_BLASTOISE,
};

static const u16 sStarterMonCaterpie[STARTER_MON_COUNT] =
{
    SPECIES_CATERPIE,
    SPECIES_CATERPIE,
    SPECIES_CATERPIE,
};


static const u16 sStarterMonMetapod[STARTER_MON_COUNT] =
{
    SPECIES_METAPOD,
    SPECIES_METAPOD,
    SPECIES_METAPOD,
};


static const u16 sStarterMonButterfree[STARTER_MON_COUNT] =
{
    SPECIES_BUTTERFREE,
    SPECIES_BUTTERFREE,
    SPECIES_BUTTERFREE,
};


static const u16 sStarterMonWeedle[STARTER_MON_COUNT] =
{
    SPECIES_WEEDLE,
    SPECIES_WEEDLE,
    SPECIES_WEEDLE,
};


static const u16 sStarterMonKakuna[STARTER_MON_COUNT] =
{
    SPECIES_KAKUNA,
    SPECIES_KAKUNA,
    SPECIES_KAKUNA,
};


static const u16 sStarterMonBeedrill[STARTER_MON_COUNT] =
{
    SPECIES_BEEDRILL,
    SPECIES_BEEDRILL,
    SPECIES_BEEDRILL,
};

static const u16 sStarterMonPidgey[STARTER_MON_COUNT] =
{
    SPECIES_PIDGEY,
    SPECIES_PIDGEY,
    SPECIES_PIDGEY,
};

static const u16 sStarterMonPidgeotto[STARTER_MON_COUNT] =
{
    SPECIES_PIDGEOTTO,
    SPECIES_PIDGEOTTO,
    SPECIES_PIDGEOTTO,
};

static const u16 sStarterMonPidgeot[STARTER_MON_COUNT] =
{
    SPECIES_PIDGEOT,
    SPECIES_PIDGEOT,
    SPECIES_PIDGEOT,
};

static const u16 sStarterMonRattata[STARTER_MON_COUNT] =
{
    SPECIES_RATTATA,
    SPECIES_RATTATA,
    SPECIES_RATTATA,
};

static const u16 sStarterMonRaticate[STARTER_MON_COUNT] =
{
    SPECIES_RATICATE,
    SPECIES_RATICATE,
    SPECIES_RATICATE,
};

static const u16 sStarterMonSpearow[STARTER_MON_COUNT] =
{
    SPECIES_SPEAROW,
    SPECIES_SPEAROW,
    SPECIES_SPEAROW,
};

static const u16 sStarterMonFearow[STARTER_MON_COUNT] =
{
    SPECIES_FEAROW,
    SPECIES_FEAROW,
    SPECIES_FEAROW,
};

static const u16 sStarterMonEkans[STARTER_MON_COUNT] =
{
    SPECIES_EKANS,
    SPECIES_EKANS,
    SPECIES_EKANS,
};

static const u16 sStarterMonArbok[STARTER_MON_COUNT] =
{
    SPECIES_ARBOK,
    SPECIES_ARBOK,
    SPECIES_ARBOK,
};

static const u16 sStarterMonPikachu[STARTER_MON_COUNT] =
{
    SPECIES_PIKACHU,
    SPECIES_PIKACHU,
    SPECIES_PIKACHU,
};

static const u16 sStarterMonRaichu[STARTER_MON_COUNT] =
{
    SPECIES_RAICHU,
    SPECIES_RAICHU,
    SPECIES_RAICHU,
};

static const u16 sStarterMonSandshrew[STARTER_MON_COUNT] =
{
    SPECIES_SANDSHREW,
    SPECIES_SANDSHREW,
    SPECIES_SANDSHREW,
};

static const u16 sStarterMonSandslash[STARTER_MON_COUNT] =
{
    SPECIES_SANDSLASH,
    SPECIES_SANDSLASH,
    SPECIES_SANDSLASH,
};

static const u16 sStarterMonNidoranF[STARTER_MON_COUNT] =
{
    SPECIES_NIDORAN_F,
    SPECIES_NIDORAN_F,
    SPECIES_NIDORAN_F,
};

static const u16 sStarterMonNidorina[STARTER_MON_COUNT] =
{
    SPECIES_NIDORINA,
    SPECIES_NIDORINA,
    SPECIES_NIDORINA,
};

static const u16 sStarterMonNidoqueen[STARTER_MON_COUNT] =
{
    SPECIES_NIDOQUEEN,
    SPECIES_NIDOQUEEN,
    SPECIES_NIDOQUEEN,
};

static const u16 sStarterMonNidoranM[STARTER_MON_COUNT] =
{
    SPECIES_NIDORAN_M,
    SPECIES_NIDORAN_M,
    SPECIES_NIDORAN_M,
};

static const u16 sStarterMonNidorino[STARTER_MON_COUNT] =
{
    SPECIES_NIDORINO,
    SPECIES_NIDORINO,
    SPECIES_NIDORINO,
};

static const u16 sStarterMonNidoking[STARTER_MON_COUNT] =
{
    SPECIES_NIDOKING,
    SPECIES_NIDOKING,
    SPECIES_NIDOKING,
};

static const u16 sStarterMonClefairy[STARTER_MON_COUNT] =
{
    SPECIES_CLEFAIRY,
    SPECIES_CLEFAIRY,
    SPECIES_CLEFAIRY,
};

static const u16 sStarterMonClefable[STARTER_MON_COUNT] =
{
    SPECIES_CLEFABLE,
    SPECIES_CLEFABLE,
    SPECIES_CLEFABLE,
};

static const u16 sStarterMonVulpix[STARTER_MON_COUNT] =
{
    SPECIES_VULPIX,
    SPECIES_VULPIX,
    SPECIES_VULPIX,
};

static const u16 sStarterMonNinetales[STARTER_MON_COUNT] =
{
    SPECIES_NINETALES,
    SPECIES_NINETALES,
    SPECIES_NINETALES,
};

static const u16 sStarterMonJigglypuff[STARTER_MON_COUNT] =
{
    SPECIES_JIGGLYPUFF,
    SPECIES_JIGGLYPUFF,
    SPECIES_JIGGLYPUFF,
};

static const u16 sStarterMonWigglytuff[STARTER_MON_COUNT] =
{
    SPECIES_WIGGLYTUFF,
    SPECIES_WIGGLYTUFF,
    SPECIES_WIGGLYTUFF,
};

static const u16 sStarterMonZubat[STARTER_MON_COUNT] =
{
    SPECIES_ZUBAT,
    SPECIES_ZUBAT,
    SPECIES_ZUBAT,
};

static const u16 sStarterMonGolbat[STARTER_MON_COUNT] =
{
    SPECIES_GOLBAT,
    SPECIES_GOLBAT,
    SPECIES_GOLBAT,
};

static const u16 sStarterMonOddish[STARTER_MON_COUNT] =
{
    SPECIES_ODDISH,
    SPECIES_ODDISH,
    SPECIES_ODDISH,
};

static const u16 sStarterMonGloom[STARTER_MON_COUNT] =
{
    SPECIES_GLOOM,
    SPECIES_GLOOM,
    SPECIES_GLOOM,
};

static const u16 sStarterMonVileplume[STARTER_MON_COUNT] =
{
    SPECIES_VILEPLUME,
    SPECIES_VILEPLUME,
    SPECIES_VILEPLUME,
};

static const u16 sStarterMonParas[STARTER_MON_COUNT] =
{
    SPECIES_PARAS,
    SPECIES_PARAS,
    SPECIES_PARAS,
};

static const u16 sStarterMonParasect[STARTER_MON_COUNT] =
{
    SPECIES_PARASECT,
    SPECIES_PARASECT,
    SPECIES_PARASECT,
};

static const u16 sStarterMonVenonat[STARTER_MON_COUNT] =
{
    SPECIES_VENONAT,
    SPECIES_VENONAT,
    SPECIES_VENONAT,
};

static const u16 sStarterMonVenomoth[STARTER_MON_COUNT] =
{
    SPECIES_VENOMOTH,
    SPECIES_VENOMOTH,
    SPECIES_VENOMOTH,
};

static const u16 sStarterMonDiglett[STARTER_MON_COUNT] =
{
    SPECIES_DIGLETT,
    SPECIES_DIGLETT,
    SPECIES_DIGLETT,
};

static const u16 sStarterMonDugtrio[STARTER_MON_COUNT] =
{
    SPECIES_DUGTRIO,
    SPECIES_DUGTRIO,
    SPECIES_DUGTRIO,
};

static const u16 sStarterMonMeowth[STARTER_MON_COUNT] =
{
    SPECIES_MEOWTH,
    SPECIES_MEOWTH,
    SPECIES_MEOWTH,
};

static const u16 sStarterMonPersian[STARTER_MON_COUNT] =
{
    SPECIES_PERSIAN,
    SPECIES_PERSIAN,
    SPECIES_PERSIAN,
};

static const u16 sStarterMonPsyduck[STARTER_MON_COUNT] =
{
    SPECIES_PSYDUCK,
    SPECIES_PSYDUCK,
    SPECIES_PSYDUCK,
};

static const u16 sStarterMonGolduck[STARTER_MON_COUNT] =
{
    SPECIES_GOLDUCK,
    SPECIES_GOLDUCK,
    SPECIES_GOLDUCK,
};

static const u16 sStarterMonMankey[STARTER_MON_COUNT] =
{
    SPECIES_MANKEY,
    SPECIES_MANKEY,
    SPECIES_MANKEY,
};

static const u16 sStarterMonPrimeape[STARTER_MON_COUNT] =
{
    SPECIES_PRIMEAPE,
    SPECIES_PRIMEAPE,
    SPECIES_PRIMEAPE,
};

static const u16 sStarterMonGrowlithe[STARTER_MON_COUNT] =
{
    SPECIES_GROWLITHE,
    SPECIES_GROWLITHE,
    SPECIES_GROWLITHE,
};

static const u16 sStarterMonArcanine[STARTER_MON_COUNT] =
{
    SPECIES_ARCANINE,
    SPECIES_ARCANINE,
    SPECIES_ARCANINE,
};

static const u16 sStarterMonPoliwag[STARTER_MON_COUNT] =
{
    SPECIES_POLIWAG,
    SPECIES_POLIWAG,
    SPECIES_POLIWAG,
};

static const u16 sStarterMonPoliwhirl[STARTER_MON_COUNT] =
{
    SPECIES_POLIWHIRL,
    SPECIES_POLIWHIRL,
    SPECIES_POLIWHIRL,
};

static const u16 sStarterMonPoliwrath[STARTER_MON_COUNT] =
{
    SPECIES_POLIWRATH,
    SPECIES_POLIWRATH,
    SPECIES_POLIWRATH,
};

static const u16 sStarterMonAbra[STARTER_MON_COUNT] =
{
    SPECIES_ABRA,
    SPECIES_ABRA,
    SPECIES_ABRA,
};

static const u16 sStarterMonKadabra[STARTER_MON_COUNT] =
{
    SPECIES_KADABRA,
    SPECIES_KADABRA,
    SPECIES_KADABRA,
};

static const u16 sStarterMonAlakazam[STARTER_MON_COUNT] =
{
    SPECIES_ALAKAZAM,
    SPECIES_ALAKAZAM,
    SPECIES_ALAKAZAM,
};

static const u16 sStarterMonMachop[STARTER_MON_COUNT] =
{
    SPECIES_MACHOP,
    SPECIES_MACHOP,
    SPECIES_MACHOP,
};

static const u16 sStarterMonMachoke[STARTER_MON_COUNT] =
{
    SPECIES_MACHOKE,
    SPECIES_MACHOKE,
    SPECIES_MACHOKE,
};

static const u16 sStarterMonMachamp[STARTER_MON_COUNT] =
{
    SPECIES_MACHAMP,
    SPECIES_MACHAMP,
    SPECIES_MACHAMP,
};

static const u16 sStarterMonBellsprout[STARTER_MON_COUNT] =
{
    SPECIES_BELLSPROUT,
    SPECIES_BELLSPROUT,
    SPECIES_BELLSPROUT,
};

static const u16 sStarterMonWeepinbell[STARTER_MON_COUNT] =
{
    SPECIES_WEEPINBELL,
    SPECIES_WEEPINBELL,
    SPECIES_WEEPINBELL,
};

static const u16 sStarterMonVictreebel[STARTER_MON_COUNT] =
{
    SPECIES_VICTREEBEL,
    SPECIES_VICTREEBEL,
    SPECIES_VICTREEBEL,
};

static const u16 sStarterMonTentacool[STARTER_MON_COUNT] =
{
    SPECIES_TENTACOOL,
    SPECIES_TENTACOOL,
    SPECIES_TENTACOOL,
};

static const u16 sStarterMonTentacruel[STARTER_MON_COUNT] =
{
    SPECIES_TENTACRUEL,
    SPECIES_TENTACRUEL,
    SPECIES_TENTACRUEL,
};

static const u16 sStarterMonGeodude[STARTER_MON_COUNT] =
{
    SPECIES_GEODUDE,
    SPECIES_GEODUDE,
    SPECIES_GEODUDE,
};

static const u16 sStarterMonGraveler[STARTER_MON_COUNT] =
{
    SPECIES_GRAVELER,
    SPECIES_GRAVELER,
    SPECIES_GRAVELER,
};

static const u16 sStarterMonGolem[STARTER_MON_COUNT] =
{
    SPECIES_GOLEM,
    SPECIES_GOLEM,
    SPECIES_GOLEM,
};

static const u16 sStarterMonPonyta[STARTER_MON_COUNT] =
{
    SPECIES_PONYTA,
    SPECIES_PONYTA,
    SPECIES_PONYTA,
};

static const u16 sStarterMonRapidash[STARTER_MON_COUNT] =
{
    SPECIES_RAPIDASH,
    SPECIES_RAPIDASH,
    SPECIES_RAPIDASH,
};

static const u16 sStarterMonSlowpoke[STARTER_MON_COUNT] =
{
    SPECIES_SLOWPOKE,
    SPECIES_SLOWPOKE,
    SPECIES_SLOWPOKE,
};

static const u16 sStarterMonSlowbro[STARTER_MON_COUNT] =
{
    SPECIES_SLOWBRO,
    SPECIES_SLOWBRO,
    SPECIES_SLOWBRO,
};

static const u16 sStarterMonMagnemite[STARTER_MON_COUNT] =
{
    SPECIES_MAGNEMITE,
    SPECIES_MAGNEMITE,
    SPECIES_MAGNEMITE,
};

static const u16 sStarterMonMagneton[STARTER_MON_COUNT] =
{
    SPECIES_MAGNETON,
    SPECIES_MAGNETON,
    SPECIES_MAGNETON,
};

static const u16 sStarterMonFarfetchd[STARTER_MON_COUNT] =
{
    SPECIES_FARFETCHD,
    SPECIES_FARFETCHD,
    SPECIES_FARFETCHD,
};

static const u16 sStarterMonDoduo[STARTER_MON_COUNT] =
{
    SPECIES_DODUO,
    SPECIES_DODUO,
    SPECIES_DODUO,
};

static const u16 sStarterMonDodrio[STARTER_MON_COUNT] =
{
    SPECIES_DODRIO,
    SPECIES_DODRIO,
    SPECIES_DODRIO,
};

static const u16 sStarterMonSeel[STARTER_MON_COUNT] =
{
    SPECIES_SEEL,
    SPECIES_SEEL,
    SPECIES_SEEL,
};

static const u16 sStarterMonDewgong[STARTER_MON_COUNT] =
{
    SPECIES_DEWGONG,
    SPECIES_DEWGONG,
    SPECIES_DEWGONG,
};

static const u16 sStarterMonGrimer[STARTER_MON_COUNT] =
{
    SPECIES_GRIMER,
    SPECIES_GRIMER,
    SPECIES_GRIMER,
};

static const u16 sStarterMonMuk[STARTER_MON_COUNT] =
{
    SPECIES_MUK,
    SPECIES_MUK,
    SPECIES_MUK,
};

static const u16 sStarterMonShellder[STARTER_MON_COUNT] =
{
    SPECIES_SHELLDER,
    SPECIES_SHELLDER,
    SPECIES_SHELLDER,
};

static const u16 sStarterMonCloyster[STARTER_MON_COUNT] =
{
    SPECIES_CLOYSTER,
    SPECIES_CLOYSTER,
    SPECIES_CLOYSTER,
};

static const u16 sStarterMonGastly[STARTER_MON_COUNT] =
{
    SPECIES_GASTLY,
    SPECIES_GASTLY,
    SPECIES_GASTLY,
};

static const u16 sStarterMonHaunter[STARTER_MON_COUNT] =
{
    SPECIES_HAUNTER,
    SPECIES_HAUNTER,
    SPECIES_HAUNTER,
};

static const u16 sStarterMonGengar[STARTER_MON_COUNT] =
{
    SPECIES_GENGAR,
    SPECIES_GENGAR,
    SPECIES_GENGAR,
};

static const u16 sStarterMonOnix[STARTER_MON_COUNT] =
{
    SPECIES_ONIX,
    SPECIES_ONIX,
    SPECIES_ONIX,
};

static const u16 sStarterMonDrowzee[STARTER_MON_COUNT] =
{
    SPECIES_DROWZEE,
    SPECIES_DROWZEE,
    SPECIES_DROWZEE,
};

static const u16 sStarterMonHypno[STARTER_MON_COUNT] =
{
    SPECIES_HYPNO,
    SPECIES_HYPNO,
    SPECIES_HYPNO,
};

static const u16 sStarterMonKrabby[STARTER_MON_COUNT] =
{
    SPECIES_KRABBY,
    SPECIES_KRABBY,
    SPECIES_KRABBY,
};

static const u16 sStarterMonKingler[STARTER_MON_COUNT] =
{
    SPECIES_KINGLER,
    SPECIES_KINGLER,
    SPECIES_KINGLER,
};

static const u16 sStarterMonVoltorb[STARTER_MON_COUNT] =
{
    SPECIES_VOLTORB,
    SPECIES_VOLTORB,
    SPECIES_VOLTORB,
};

static const u16 sStarterMonElectrode[STARTER_MON_COUNT] =
{
    SPECIES_ELECTRODE,
    SPECIES_ELECTRODE,
    SPECIES_ELECTRODE,
};

static const u16 sStarterMonExeggcute[STARTER_MON_COUNT] =
{
    SPECIES_EXEGGCUTE,
    SPECIES_EXEGGCUTE,
    SPECIES_EXEGGCUTE,
};

static const u16 sStarterMonExeggutor[STARTER_MON_COUNT] =
{
    SPECIES_EXEGGUTOR,
    SPECIES_EXEGGUTOR,
    SPECIES_EXEGGUTOR,
};

static const u16 sStarterMonCubone[STARTER_MON_COUNT] =
{
    SPECIES_CUBONE,
    SPECIES_CUBONE,
    SPECIES_CUBONE,
};

static const u16 sStarterMonMarowak[STARTER_MON_COUNT] =
{
    SPECIES_MAROWAK,
    SPECIES_MAROWAK,
    SPECIES_MAROWAK,
};

static const u16 sStarterMonHitmonlee[STARTER_MON_COUNT] =
{
    SPECIES_HITMONLEE,
    SPECIES_HITMONLEE,
    SPECIES_HITMONLEE,
};

static const u16 sStarterMonHitmonchan[STARTER_MON_COUNT] =
{
    SPECIES_HITMONCHAN,
    SPECIES_HITMONCHAN,
    SPECIES_HITMONCHAN,
};

static const u16 sStarterMonLickitung[STARTER_MON_COUNT] =
{
    SPECIES_LICKITUNG,
    SPECIES_LICKITUNG,
    SPECIES_LICKITUNG,
};

static const u16 sStarterMonKoffing[STARTER_MON_COUNT] =
{
    SPECIES_KOFFING,
    SPECIES_KOFFING,
    SPECIES_KOFFING,
};

static const u16 sStarterMonWeezing[STARTER_MON_COUNT] =
{
    SPECIES_WEEZING,
    SPECIES_WEEZING,
    SPECIES_WEEZING,
};

static const u16 sStarterMonRhyhorn[STARTER_MON_COUNT] =
{
    SPECIES_RHYHORN,
    SPECIES_RHYHORN,
    SPECIES_RHYHORN,
};

static const u16 sStarterMonRhydon[STARTER_MON_COUNT] =
{
    SPECIES_RHYDON,
    SPECIES_RHYDON,
    SPECIES_RHYDON,
};

static const u16 sStarterMonChansey[STARTER_MON_COUNT] =
{
    SPECIES_CHANSEY,
    SPECIES_CHANSEY,
    SPECIES_CHANSEY,
};

static const u16 sStarterMonTangela[STARTER_MON_COUNT] =
{
    SPECIES_TANGELA,
    SPECIES_TANGELA,
    SPECIES_TANGELA,
};

static const u16 sStarterMonKangaskhan[STARTER_MON_COUNT] =
{
    SPECIES_KANGASKHAN,
    SPECIES_KANGASKHAN,
    SPECIES_KANGASKHAN,
};

static const u16 sStarterMonHorsea[STARTER_MON_COUNT] =
{
    SPECIES_HORSEA,
    SPECIES_HORSEA,
    SPECIES_HORSEA,
};

static const u16 sStarterMonSeadra[STARTER_MON_COUNT] =
{
    SPECIES_SEADRA,
    SPECIES_SEADRA,
    SPECIES_SEADRA,
};

static const u16 sStarterMonGoldeen[STARTER_MON_COUNT] =
{
    SPECIES_GOLDEEN,
    SPECIES_GOLDEEN,
    SPECIES_GOLDEEN,
};

static const u16 sStarterMonSeaking[STARTER_MON_COUNT] =
{
    SPECIES_SEAKING,
    SPECIES_SEAKING,
    SPECIES_SEAKING,
};

static const u16 sStarterMonStaryu[STARTER_MON_COUNT] =
{
    SPECIES_STARYU,
    SPECIES_STARYU,
    SPECIES_STARYU,
};

static const u16 sStarterMonStarmie[STARTER_MON_COUNT] =
{
    SPECIES_STARMIE,
    SPECIES_STARMIE,
    SPECIES_STARMIE,
};

static const u16 sStarterMonMrMime[STARTER_MON_COUNT] =
{
    SPECIES_MR_MIME,
    SPECIES_MR_MIME,
    SPECIES_MR_MIME,
};

static const u16 sStarterMonScyther[STARTER_MON_COUNT] =
{
    SPECIES_SCYTHER,
    SPECIES_SCYTHER,
    SPECIES_SCYTHER,
};

static const u16 sStarterMonJynx[STARTER_MON_COUNT] =
{
    SPECIES_JYNX,
    SPECIES_JYNX,
    SPECIES_JYNX,
};

static const u16 sStarterMonElectabuzz[STARTER_MON_COUNT] =
{
    SPECIES_ELECTABUZZ,
    SPECIES_ELECTABUZZ,
    SPECIES_ELECTABUZZ,
};

static const u16 sStarterMonMagmar[STARTER_MON_COUNT] =
{
    SPECIES_MAGMAR,
    SPECIES_MAGMAR,
    SPECIES_MAGMAR,
};

static const u16 sStarterMonPinsir[STARTER_MON_COUNT] =
{
    SPECIES_PINSIR,
    SPECIES_PINSIR,
    SPECIES_PINSIR,
};

static const u16 sStarterMonTauros[STARTER_MON_COUNT] =
{
    SPECIES_TAUROS,
    SPECIES_TAUROS,
    SPECIES_TAUROS,
};

static const u16 sStarterMonMagikarp[STARTER_MON_COUNT] =
{
    SPECIES_MAGIKARP,
    SPECIES_MAGIKARP,
    SPECIES_MAGIKARP,
};

static const u16 sStarterMonGyarados[STARTER_MON_COUNT] =
{
    SPECIES_GYARADOS,
    SPECIES_GYARADOS,
    SPECIES_GYARADOS,
};

static const u16 sStarterMonLapras[STARTER_MON_COUNT] =
{
    SPECIES_LAPRAS,
    SPECIES_LAPRAS,
    SPECIES_LAPRAS,
};

static const u16 sStarterMonDitto[STARTER_MON_COUNT] =
{
    SPECIES_DITTO,
    SPECIES_DITTO,
    SPECIES_DITTO,
};

static const u16 sStarterMonEevee[STARTER_MON_COUNT] =
{
    SPECIES_EEVEE,
    SPECIES_EEVEE,
    SPECIES_EEVEE,
};

static const u16 sStarterMonVaporeon[STARTER_MON_COUNT] =
{
    SPECIES_VAPOREON,
    SPECIES_VAPOREON,
    SPECIES_VAPOREON,
};

static const u16 sStarterMonJolteon[STARTER_MON_COUNT] =
{
    SPECIES_JOLTEON,
    SPECIES_JOLTEON,
    SPECIES_JOLTEON,
};

static const u16 sStarterMonFlareon[STARTER_MON_COUNT] =
{
    SPECIES_FLAREON,
    SPECIES_FLAREON,
    SPECIES_FLAREON,
};

static const u16 sStarterMonPorygon[STARTER_MON_COUNT] =
{
    SPECIES_PORYGON,
    SPECIES_PORYGON,
    SPECIES_PORYGON,
};

static const u16 sStarterMonOmanyte[STARTER_MON_COUNT] =
{
    SPECIES_OMANYTE,
    SPECIES_OMANYTE,
    SPECIES_OMANYTE,
};

static const u16 sStarterMonOmastar[STARTER_MON_COUNT] =
{
    SPECIES_OMASTAR,
    SPECIES_OMASTAR,
    SPECIES_OMASTAR,
};

static const u16 sStarterMonKabuto[STARTER_MON_COUNT] =
{
    SPECIES_KABUTO,
    SPECIES_KABUTO,
    SPECIES_KABUTO,
};

static const u16 sStarterMonKabutops[STARTER_MON_COUNT] =
{
    SPECIES_KABUTOPS,
    SPECIES_KABUTOPS,
    SPECIES_KABUTOPS,
};

static const u16 sStarterMonAerodactyl[STARTER_MON_COUNT] =
{
    SPECIES_AERODACTYL,
    SPECIES_AERODACTYL,
    SPECIES_AERODACTYL,
};

static const u16 sStarterMonSnorlax[STARTER_MON_COUNT] =
{
    SPECIES_SNORLAX,
    SPECIES_SNORLAX,
    SPECIES_SNORLAX,
};

static const u16 sStarterMonArticuno[STARTER_MON_COUNT] =
{
    SPECIES_ARTICUNO,
    SPECIES_ARTICUNO,
    SPECIES_ARTICUNO,
};

static const u16 sStarterMonZapdos[STARTER_MON_COUNT] =
{
    SPECIES_ZAPDOS,
    SPECIES_ZAPDOS,
    SPECIES_ZAPDOS,
};

static const u16 sStarterMonMoltres[STARTER_MON_COUNT] =
{
    SPECIES_MOLTRES,
    SPECIES_MOLTRES,
    SPECIES_MOLTRES,
};

static const u16 sStarterMonDratini[STARTER_MON_COUNT] =
{
    SPECIES_DRATINI,
    SPECIES_DRATINI,
    SPECIES_DRATINI,
};

static const u16 sStarterMonDragonair[STARTER_MON_COUNT] =
{
    SPECIES_DRAGONAIR,
    SPECIES_DRAGONAIR,
    SPECIES_DRAGONAIR,
};

static const u16 sStarterMonDragonite[STARTER_MON_COUNT] =
{
    SPECIES_DRAGONITE,
    SPECIES_DRAGONITE,
    SPECIES_DRAGONITE,
};

static const u16 sStarterMonMewtwo[STARTER_MON_COUNT] =
{
    SPECIES_MEWTWO,
    SPECIES_MEWTWO,
    SPECIES_MEWTWO,
};

static const u16 sStarterMonMew[STARTER_MON_COUNT] =
{
    SPECIES_MEWTWO,
    SPECIES_MEWTWO,
    SPECIES_MEWTWO,
};

static const u16 sStarterMonChikorita[STARTER_MON_COUNT] =
{
    SPECIES_CHIKORITA,
    SPECIES_CHIKORITA,
    SPECIES_CHIKORITA,
};

static const u16 sStarterMonBayleef[STARTER_MON_COUNT] =
{
    SPECIES_BAYLEEF,
    SPECIES_BAYLEEF,
    SPECIES_BAYLEEF,
};

static const u16 sStarterMonMeganium[STARTER_MON_COUNT] =
{
    SPECIES_MEGANIUM,
    SPECIES_MEGANIUM,
    SPECIES_MEGANIUM,
};

static const u16 sStarterMonCyndaquil[STARTER_MON_COUNT] =
{
    SPECIES_CYNDAQUIL,
    SPECIES_CYNDAQUIL,
    SPECIES_CYNDAQUIL,
};

static const u16 sStarterMonQuilava[STARTER_MON_COUNT] =
{
    SPECIES_QUILAVA,
    SPECIES_QUILAVA,
    SPECIES_QUILAVA,
};

static const u16 sStarterMonTyphlosion[STARTER_MON_COUNT] =
{
    SPECIES_TYPHLOSION,
    SPECIES_TYPHLOSION,
    SPECIES_TYPHLOSION,
};

static const u16 sStarterMonTotodile[STARTER_MON_COUNT] =
{
    SPECIES_TOTODILE,
    SPECIES_TOTODILE,
    SPECIES_TOTODILE,
};

static const u16 sStarterMonCroconaw[STARTER_MON_COUNT] =
{
    SPECIES_CROCONAW,
    SPECIES_CROCONAW,
    SPECIES_CROCONAW,
};

static const u16 sStarterMonFeraligatr[STARTER_MON_COUNT] =
{
    SPECIES_FERALIGATR,
    SPECIES_FERALIGATR,
    SPECIES_FERALIGATR,
};

static const u16 sStarterMonSentret[STARTER_MON_COUNT] =
{
    SPECIES_SENTRET,
    SPECIES_SENTRET,
    SPECIES_SENTRET,
};

static const u16 sStarterMonFurret[STARTER_MON_COUNT] =
{
    SPECIES_FURRET,
    SPECIES_FURRET,
    SPECIES_FURRET,
};

static const u16 sStarterMonHoothoot[STARTER_MON_COUNT] =
{
    SPECIES_HOOTHOOT,
    SPECIES_HOOTHOOT,
    SPECIES_HOOTHOOT,
};

static const u16 sStarterMonNoctowl[STARTER_MON_COUNT] =
{
    SPECIES_NOCTOWL,
    SPECIES_NOCTOWL,
    SPECIES_NOCTOWL,
};

static const u16 sStarterMonLedyba[STARTER_MON_COUNT] =
{
    SPECIES_LEDYBA,
    SPECIES_LEDYBA,
    SPECIES_LEDYBA,
};

static const u16 sStarterMonLedian[STARTER_MON_COUNT] =
{
    SPECIES_LEDIAN,
    SPECIES_LEDIAN,
    SPECIES_LEDIAN,
};

static const u16 sStarterMonSpinarak[STARTER_MON_COUNT] =
{
    SPECIES_SPINARAK,
    SPECIES_SPINARAK,
    SPECIES_SPINARAK,
};

static const u16 sStarterMonAriados[STARTER_MON_COUNT] =
{
    SPECIES_ARIADOS,
    SPECIES_ARIADOS,
    SPECIES_ARIADOS,
};

static const u16 sStarterMonCrobat[STARTER_MON_COUNT] =
{
    SPECIES_CROBAT,
    SPECIES_CROBAT,
    SPECIES_CROBAT,
};

static const u16 sStarterMonChinchou[STARTER_MON_COUNT] =
{
    SPECIES_CHINCHOU,
    SPECIES_CHINCHOU,
    SPECIES_CHINCHOU,
};

static const u16 sStarterMonLanturn[STARTER_MON_COUNT] =
{
    SPECIES_LANTURN,
    SPECIES_LANTURN,
    SPECIES_LANTURN,
};

static const u16 sStarterMonPichu[STARTER_MON_COUNT] =
{
    SPECIES_PICHU,
    SPECIES_PICHU,
    SPECIES_PICHU,
};

static const u16 sStarterMonCleffa[STARTER_MON_COUNT] =
{
    SPECIES_CLEFFA,
    SPECIES_CLEFFA,
    SPECIES_CLEFFA,
};

static const u16 sStarterMonIgglybuff[STARTER_MON_COUNT] =
{
    SPECIES_IGGLYBUFF,
    SPECIES_IGGLYBUFF,
    SPECIES_IGGLYBUFF,
};

static const u16 sStarterMonTogepi[STARTER_MON_COUNT] =
{
    SPECIES_TOGEPI,
    SPECIES_TOGEPI,
    SPECIES_TOGEPI,
};

static const u16 sStarterMonTogetic[STARTER_MON_COUNT] =
{
    SPECIES_TOGETIC,
    SPECIES_TOGETIC,
    SPECIES_TOGETIC,
};

static const u16 sStarterMonNatu[STARTER_MON_COUNT] =
{
    SPECIES_NATU,
    SPECIES_NATU,
    SPECIES_NATU,
};

static const u16 sStarterMonXatu[STARTER_MON_COUNT] =
{
    SPECIES_XATU,
    SPECIES_XATU,
    SPECIES_XATU,
};

static const u16 sStarterMonMareep[STARTER_MON_COUNT] =
{
    SPECIES_MAREEP,
    SPECIES_MAREEP,
    SPECIES_MAREEP,
};

static const u16 sStarterMonFlaaffy[STARTER_MON_COUNT] =
{
    SPECIES_FLAAFFY,
    SPECIES_FLAAFFY,
    SPECIES_FLAAFFY,
};

static const u16 sStarterMonAmpharos[STARTER_MON_COUNT] =
{
    SPECIES_AMPHAROS,
    SPECIES_AMPHAROS,
    SPECIES_AMPHAROS,
};

static const u16 sStarterMonBellossom[STARTER_MON_COUNT] =
{
    SPECIES_BELLOSSOM,
    SPECIES_BELLOSSOM,
    SPECIES_BELLOSSOM,
};

static const u16 sStarterMonMarill[STARTER_MON_COUNT] =
{
    SPECIES_MARILL,
    SPECIES_MARILL,
    SPECIES_MARILL,
};

static const u16 sStarterMonAzumarill[STARTER_MON_COUNT] =
{
    SPECIES_AZUMARILL,
    SPECIES_AZUMARILL,
    SPECIES_AZUMARILL,
};

static const u16 sStarterMonSudowoodo[STARTER_MON_COUNT] =
{
    SPECIES_SUDOWOODO,
    SPECIES_SUDOWOODO,
    SPECIES_SUDOWOODO,
};

static const u16 sStarterMonPolitoed[STARTER_MON_COUNT] =
{
    SPECIES_POLITOED,
    SPECIES_POLITOED,
    SPECIES_POLITOED,
};

static const u16 sStarterMonHoppip[STARTER_MON_COUNT] =
{
    SPECIES_HOPPIP,
    SPECIES_HOPPIP,
    SPECIES_HOPPIP,
};

static const u16 sStarterMonSkiploom[STARTER_MON_COUNT] =
{
    SPECIES_SKIPLOOM,
    SPECIES_SKIPLOOM,
    SPECIES_SKIPLOOM,
};

static const u16 sStarterMonJumpluff[STARTER_MON_COUNT] =
{
    SPECIES_JUMPLUFF,
    SPECIES_JUMPLUFF,
    SPECIES_JUMPLUFF,
};

static const u16 sStarterMonAipom[STARTER_MON_COUNT] =
{
    SPECIES_AIPOM,
    SPECIES_AIPOM,
    SPECIES_AIPOM,
};

static const u16 sStarterMonSunkern[STARTER_MON_COUNT] =
{
    SPECIES_SUNKERN,
    SPECIES_SUNKERN,
    SPECIES_SUNKERN,
};

static const u16 sStarterMonSunflora[STARTER_MON_COUNT] =
{
    SPECIES_SUNFLORA,
    SPECIES_SUNFLORA,
    SPECIES_SUNFLORA,
};

static const u16 sStarterMonYanma[STARTER_MON_COUNT] =
{
    SPECIES_YANMA,
    SPECIES_YANMA,
    SPECIES_YANMA,
};

static const u16 sStarterMonWooper[STARTER_MON_COUNT] =
{
    SPECIES_WOOPER,
    SPECIES_WOOPER,
    SPECIES_WOOPER,
};

static const u16 sStarterMonQuagsire[STARTER_MON_COUNT] =
{
    SPECIES_QUAGSIRE,
    SPECIES_QUAGSIRE,
    SPECIES_QUAGSIRE,
};

static const u16 sStarterMonEspeon[STARTER_MON_COUNT] =
{
    SPECIES_ESPEON,
    SPECIES_ESPEON,
    SPECIES_ESPEON,
};

static const u16 sStarterMonUmbreon[STARTER_MON_COUNT] =
{
    SPECIES_UMBREON,
    SPECIES_UMBREON,
    SPECIES_UMBREON,
};

static const u16 sStarterMonMurkrow[STARTER_MON_COUNT] =
{
    SPECIES_MURKROW,
    SPECIES_MURKROW,
    SPECIES_MURKROW,
};

static const u16 sStarterMonSlowking[STARTER_MON_COUNT] =
{
    SPECIES_SLOWKING,
    SPECIES_SLOWKING,
    SPECIES_SLOWKING,
};

static const u16 sStarterMonMisdreavus[STARTER_MON_COUNT] =
{
    SPECIES_MISDREAVUS,
    SPECIES_MISDREAVUS,
    SPECIES_MISDREAVUS,
};

static const u16 sStarterMonUnown[STARTER_MON_COUNT] =
{
    SPECIES_UNOWN,
    SPECIES_UNOWN,
    SPECIES_UNOWN,
};

static const u16 sStarterMonWobbuffet[STARTER_MON_COUNT] =
{
    SPECIES_WOBBUFFET,
    SPECIES_WOBBUFFET,
    SPECIES_WOBBUFFET,
};

static const u16 sStarterMonGirafarig[STARTER_MON_COUNT] =
{
    SPECIES_GIRAFARIG,
    SPECIES_GIRAFARIG,
    SPECIES_GIRAFARIG,
};

static const u16 sStarterMonPineco[STARTER_MON_COUNT] =
{
    SPECIES_PINECO,
    SPECIES_PINECO,
    SPECIES_PINECO,
};

static const u16 sStarterMonForretress[STARTER_MON_COUNT] =
{
    SPECIES_FORRETRESS,
    SPECIES_FORRETRESS,
    SPECIES_FORRETRESS,
};

static const u16 sStarterMonDunsparce[STARTER_MON_COUNT] =
{
    SPECIES_DUNSPARCE,
    SPECIES_DUNSPARCE,
    SPECIES_DUNSPARCE,
};


static const u16 sStarterMonGligar[STARTER_MON_COUNT] =
{
    SPECIES_GLIGAR,
    SPECIES_GLIGAR,
    SPECIES_GLIGAR,
};
static const u16 sStarterMonSteelix[STARTER_MON_COUNT] =
{
    SPECIES_STEELIX,
    SPECIES_STEELIX,
    SPECIES_STEELIX,
};

static const u16 sStarterMonSnubbull[STARTER_MON_COUNT] =
{
    SPECIES_SNUBBULL,
    SPECIES_SNUBBULL,
    SPECIES_SNUBBULL,
};

static const u16 sStarterMonGranbull[STARTER_MON_COUNT] =
{
    SPECIES_GRANBULL,
    SPECIES_GRANBULL,
    SPECIES_GRANBULL,
};

static const u16 sStarterMonQwilfish[STARTER_MON_COUNT] =
{
    SPECIES_QWILFISH,
    SPECIES_QWILFISH,
    SPECIES_QWILFISH,
};

static const u16 sStarterMonScizor[STARTER_MON_COUNT] =
{
    SPECIES_SCIZOR,
    SPECIES_SCIZOR,
    SPECIES_SCIZOR,
};

static const u16 sStarterMonShuckle[STARTER_MON_COUNT] =
{
    SPECIES_SHUCKLE,
    SPECIES_SHUCKLE,
    SPECIES_SHUCKLE,
};

static const u16 sStarterMonHeracross[STARTER_MON_COUNT] =
{
    SPECIES_HERACROSS,
    SPECIES_HERACROSS,
    SPECIES_HERACROSS,
};

static const u16 sStarterMonSneasel[STARTER_MON_COUNT] =
{
    SPECIES_SNEASEL,
    SPECIES_SNEASEL,
    SPECIES_SNEASEL,
};

static const u16 sStarterMonTeddiursa[STARTER_MON_COUNT] =
{
    SPECIES_TEDDIURSA,
    SPECIES_TEDDIURSA,
    SPECIES_TEDDIURSA,
};

static const u16 sStarterMonUrsaring[STARTER_MON_COUNT] =
{
    SPECIES_URSARING,
    SPECIES_URSARING,
    SPECIES_URSARING,
};

static const u16 sStarterMonSlugma[STARTER_MON_COUNT] =
{
    SPECIES_SLUGMA,
    SPECIES_SLUGMA,
    SPECIES_SLUGMA,
};

static const u16 sStarterMonMagcargo[STARTER_MON_COUNT] =
{
    SPECIES_MAGCARGO,
    SPECIES_MAGCARGO,
    SPECIES_MAGCARGO,
};

static const u16 sStarterMonSwinub[STARTER_MON_COUNT] =
{
    SPECIES_SWINUB,
    SPECIES_SWINUB,
    SPECIES_SWINUB,
};

static const u16 sStarterMonPiloswine[STARTER_MON_COUNT] =
{
    SPECIES_PILOSWINE,
    SPECIES_PILOSWINE,
    SPECIES_PILOSWINE,
};

static const u16 sStarterMonCorsola[STARTER_MON_COUNT] =
{
    SPECIES_CORSOLA,
    SPECIES_CORSOLA,
    SPECIES_CORSOLA,
};

static const u16 sStarterMonRemoraid[STARTER_MON_COUNT] =
{
    SPECIES_REMORAID,
    SPECIES_REMORAID,
    SPECIES_REMORAID,
};

static const u16 sStarterMonOctillery[STARTER_MON_COUNT] =
{
    SPECIES_OCTILLERY,
    SPECIES_OCTILLERY,
    SPECIES_OCTILLERY,
};

static const u16 sStarterMonDelibird[STARTER_MON_COUNT] =
{
    SPECIES_DELIBIRD,
    SPECIES_DELIBIRD,
    SPECIES_DELIBIRD,
};

static const u16 sStarterMonMantine[STARTER_MON_COUNT] =
{
    SPECIES_MANTINE,
    SPECIES_MANTINE,
    SPECIES_MANTINE,
};

static const u16 sStarterMonSkarmory[STARTER_MON_COUNT] =
{
    SPECIES_SKARMORY,
    SPECIES_SKARMORY,
    SPECIES_SKARMORY,
};

static const u16 sStarterMonHoundour[STARTER_MON_COUNT] =
{
    SPECIES_HOUNDOUR,
    SPECIES_HOUNDOUR,
    SPECIES_HOUNDOUR,
};

static const u16 sStarterMonHoundoom[STARTER_MON_COUNT] =
{
    SPECIES_HOUNDOOM,
    SPECIES_HOUNDOOM,
    SPECIES_HOUNDOOM,
};

static const u16 sStarterMonKingdra[STARTER_MON_COUNT] =
{
    SPECIES_KINGDRA,
    SPECIES_KINGDRA,
    SPECIES_KINGDRA,
};

static const u16 sStarterMonPhanpy[STARTER_MON_COUNT] =
{
    SPECIES_PHANPY,
    SPECIES_PHANPY,
    SPECIES_PHANPY,
};

static const u16 sStarterMonDonphan[STARTER_MON_COUNT] =
{
    SPECIES_DONPHAN,
    SPECIES_DONPHAN,
    SPECIES_DONPHAN,
};

static const u16 sStarterMonPorygon2[STARTER_MON_COUNT] =
{
    SPECIES_PORYGON2,
    SPECIES_PORYGON2,
    SPECIES_PORYGON2,
};

static const u16 sStarterMonStantler[STARTER_MON_COUNT] =
{
    SPECIES_STANTLER,
    SPECIES_STANTLER,
    SPECIES_STANTLER,
};

static const u16 sStarterMonSmeargle[STARTER_MON_COUNT] =
{
    SPECIES_SMEARGLE,
    SPECIES_SMEARGLE,
    SPECIES_SMEARGLE,
};

static const u16 sStarterMonTyrogue[STARTER_MON_COUNT] =
{
    SPECIES_TYROGUE,
    SPECIES_TYROGUE,
    SPECIES_TYROGUE,
};

static const u16 sStarterMonHitmontop[STARTER_MON_COUNT] =
{
    SPECIES_HITMONTOP,
    SPECIES_HITMONTOP,
    SPECIES_HITMONTOP,
};

static const u16 sStarterMonSmoochum[STARTER_MON_COUNT] =
{
    SPECIES_SMOOCHUM,
    SPECIES_SMOOCHUM,
    SPECIES_SMOOCHUM,
};

static const u16 sStarterMonElekid[STARTER_MON_COUNT] =
{
    SPECIES_ELEKID,
    SPECIES_ELEKID,
    SPECIES_ELEKID,
};

static const u16 sStarterMonMagby[STARTER_MON_COUNT] =
{
    SPECIES_MAGBY,
    SPECIES_MAGBY,
    SPECIES_MAGBY,
};

static const u16 sStarterMonMiltank[STARTER_MON_COUNT] =
{
    SPECIES_MILTANK,
    SPECIES_MILTANK,
    SPECIES_MILTANK,
};

static const u16 sStarterMonBlissey[STARTER_MON_COUNT] =
{
    SPECIES_BLISSEY,
    SPECIES_BLISSEY,
    SPECIES_BLISSEY,
};

static const u16 sStarterMonRaikou[STARTER_MON_COUNT] =
{
    SPECIES_RAIKOU,
    SPECIES_RAIKOU,
    SPECIES_RAIKOU,
};

static const u16 sStarterMonEntei[STARTER_MON_COUNT] =
{
    SPECIES_ENTEI,
    SPECIES_ENTEI,
    SPECIES_ENTEI,
};

static const u16 sStarterMonSuicune[STARTER_MON_COUNT] =
{
    SPECIES_SUICUNE,
    SPECIES_SUICUNE,
    SPECIES_SUICUNE,
};

static const u16 sStarterMonLarvitar[STARTER_MON_COUNT] =
{
    SPECIES_LARVITAR,
    SPECIES_LARVITAR,
    SPECIES_LARVITAR,
};

static const u16 sStarterMonPupitar[STARTER_MON_COUNT] =
{
    SPECIES_PUPITAR,
    SPECIES_PUPITAR,
    SPECIES_PUPITAR,
};

static const u16 sStarterMonTyranitar[STARTER_MON_COUNT] =
{
    SPECIES_TYRANITAR,
    SPECIES_TYRANITAR,
    SPECIES_TYRANITAR,
};

static const u16 sStarterMonLugia[STARTER_MON_COUNT] =
{
    SPECIES_LUGIA,
    SPECIES_LUGIA,
    SPECIES_LUGIA,
};

static const u16 sStarterMonHooh[STARTER_MON_COUNT] =
{
    SPECIES_HO_OH,
    SPECIES_HO_OH,
    SPECIES_HO_OH,
};

static const u16 sStarterMonCelebi[STARTER_MON_COUNT] =
{
    SPECIES_CELEBI,
    SPECIES_CELEBI,
    SPECIES_CELEBI,
};

static const u16 sStarterMonTreecko[STARTER_MON_COUNT] =
{
    SPECIES_TREECKO,
    SPECIES_TREECKO,
    SPECIES_TREECKO,
};

static const u16 sStarterMonGrovyle[STARTER_MON_COUNT] =
{
    SPECIES_GROVYLE,
    SPECIES_GROVYLE,
    SPECIES_GROVYLE,
};

static const u16 sStarterMonSceptile[STARTER_MON_COUNT] =
{
    SPECIES_SCEPTILE,
    SPECIES_SCEPTILE,
    SPECIES_SCEPTILE,
};

static const u16 sStarterMonTorchic[STARTER_MON_COUNT] =
{
    SPECIES_TORCHIC,
    SPECIES_TORCHIC,
    SPECIES_TORCHIC,
};

static const u16 sStarterMonCombusken[STARTER_MON_COUNT] =
{
    SPECIES_COMBUSKEN,
    SPECIES_COMBUSKEN,
    SPECIES_COMBUSKEN,
};

static const u16 sStarterMonBlaziken[STARTER_MON_COUNT] =
{
    SPECIES_BLAZIKEN,
    SPECIES_BLAZIKEN,
    SPECIES_BLAZIKEN,
};

static const u16 sStarterMonMudkip[STARTER_MON_COUNT] =
{
    SPECIES_MUDKIP,
    SPECIES_MUDKIP,
    SPECIES_MUDKIP,
};

static const u16 sStarterMonMarshtomp[STARTER_MON_COUNT] =
{
    SPECIES_MARSHTOMP,
    SPECIES_MARSHTOMP,
    SPECIES_MARSHTOMP,
};

static const u16 sStarterMonSwampert[STARTER_MON_COUNT] =
{
    SPECIES_SWAMPERT,
    SPECIES_SWAMPERT,
    SPECIES_SWAMPERT,
};

static const u16 sStarterMonPoochyena[STARTER_MON_COUNT] =
{
    SPECIES_POOCHYENA,
    SPECIES_POOCHYENA,
    SPECIES_POOCHYENA,
};

static const u16 sStarterMonMightyena[STARTER_MON_COUNT] =
{
    SPECIES_MIGHTYENA,
    SPECIES_MIGHTYENA,
    SPECIES_MIGHTYENA,
};

static const u16 sStarterMonZigzagoon[STARTER_MON_COUNT] =
{
    SPECIES_ZIGZAGOON,
    SPECIES_ZIGZAGOON,
    SPECIES_ZIGZAGOON,
};

static const u16 sStarterMonLinoone[STARTER_MON_COUNT] =
{
    SPECIES_LINOONE,
    SPECIES_LINOONE,
    SPECIES_LINOONE,
};

static const u16 sStarterMonWurmple[STARTER_MON_COUNT] =
{
    SPECIES_WURMPLE,
    SPECIES_WURMPLE,
    SPECIES_WURMPLE,
};

static const u16 sStarterMonSilcoon[STARTER_MON_COUNT] =
{
    SPECIES_SILCOON,
    SPECIES_SILCOON,
    SPECIES_SILCOON,
};

static const u16 sStarterMonBeautifly[STARTER_MON_COUNT] =
{
    SPECIES_BEAUTIFLY,
    SPECIES_BEAUTIFLY,
    SPECIES_BEAUTIFLY,
};

static const u16 sStarterMonCascoon[STARTER_MON_COUNT] =
{
    SPECIES_CASCOON,
    SPECIES_CASCOON,
    SPECIES_CASCOON,
};

static const u16 sStarterMonDustox[STARTER_MON_COUNT] =
{
    SPECIES_DUSTOX,
    SPECIES_DUSTOX,
    SPECIES_DUSTOX,
};

static const u16 sStarterMonLotad[STARTER_MON_COUNT] =
{
    SPECIES_LOTAD,
    SPECIES_LOTAD,
    SPECIES_LOTAD,
};

static const u16 sStarterMonLombre[STARTER_MON_COUNT] =
{
    SPECIES_LOMBRE,
    SPECIES_LOMBRE,
    SPECIES_LOMBRE,
};

static const u16 sStarterMonLudicolo[STARTER_MON_COUNT] =
{
    SPECIES_LUDICOLO,
    SPECIES_LUDICOLO,
    SPECIES_LUDICOLO,
};

static const u16 sStarterMonSeedot[STARTER_MON_COUNT] =
{
    SPECIES_SEEDOT,
    SPECIES_SEEDOT,
    SPECIES_SEEDOT,
};

static const u16 sStarterMonNuzleaf[STARTER_MON_COUNT] =
{
    SPECIES_NUZLEAF,
    SPECIES_NUZLEAF,
    SPECIES_NUZLEAF,
};

static const u16 sStarterMonShiftry[STARTER_MON_COUNT] =
{
    SPECIES_SHIFTRY,
    SPECIES_SHIFTRY,
    SPECIES_SHIFTRY,
};

static const u16 sStarterMonTaillow[STARTER_MON_COUNT] =
{
    SPECIES_TAILLOW,
    SPECIES_TAILLOW,
    SPECIES_TAILLOW,
};

static const u16 sStarterMonSwellow[STARTER_MON_COUNT] =
{
    SPECIES_SWELLOW,
    SPECIES_SWELLOW,
    SPECIES_SWELLOW,
};

static const u16 sStarterMonWingull[STARTER_MON_COUNT] =
{
    SPECIES_WINGULL,
    SPECIES_WINGULL,
    SPECIES_WINGULL,
};

static const u16 sStarterMonPelipper[STARTER_MON_COUNT] =
{
    SPECIES_PELIPPER,
    SPECIES_PELIPPER,
    SPECIES_PELIPPER,
};

static const u16 sStarterMonRalts[STARTER_MON_COUNT] =
{
    SPECIES_RALTS,
    SPECIES_RALTS,
    SPECIES_RALTS,
};

static const u16 sStarterMonKirlia[STARTER_MON_COUNT] =
{
    SPECIES_KIRLIA,
    SPECIES_KIRLIA,
    SPECIES_KIRLIA,
};

static const u16 sStarterMonGardevoir[STARTER_MON_COUNT] =
{
    SPECIES_GARDEVOIR,
    SPECIES_GARDEVOIR,
    SPECIES_GARDEVOIR,
};

static const u16 sStarterMonSurskit[STARTER_MON_COUNT] =
{
    SPECIES_SURSKIT,
};

static const u16 sStarterMonMasquerain[STARTER_MON_COUNT] =
{
    SPECIES_MASQUERAIN,
    SPECIES_MASQUERAIN,
    SPECIES_MASQUERAIN,
};

static const u16 sStarterMonShroomish[STARTER_MON_COUNT] =
{
    SPECIES_SHROOMISH,
    SPECIES_SHROOMISH,
    SPECIES_SHROOMISH,
};

static const u16 sStarterMonBreloom[STARTER_MON_COUNT] =
{
    SPECIES_BRELOOM,
    SPECIES_BRELOOM,
    SPECIES_BRELOOM,
};

static const u16 sStarterMonSlakoth[STARTER_MON_COUNT] =
{
    SPECIES_SLAKOTH,
    SPECIES_SLAKOTH,
    SPECIES_SLAKOTH,
};

static const u16 sStarterMonVigoroth[STARTER_MON_COUNT] =
{
    SPECIES_VIGOROTH,
    SPECIES_VIGOROTH,
    SPECIES_VIGOROTH,
};

static const u16 sStarterMonSlaking[STARTER_MON_COUNT] =
{
    SPECIES_SLAKING,
    SPECIES_SLAKING,
    SPECIES_SLAKING,
};

static const u16 sStarterMonNincada[STARTER_MON_COUNT] =
{
    SPECIES_NINCADA,
    SPECIES_NINCADA,
    SPECIES_NINCADA,
};

static const u16 sStarterMonNinjask[STARTER_MON_COUNT] =
{
    SPECIES_NINJASK,
    SPECIES_NINJASK,
    SPECIES_NINJASK,
};

static const u16 sStarterMonShedinja[STARTER_MON_COUNT] =
{
    SPECIES_SHEDINJA,
    SPECIES_SHEDINJA,
    SPECIES_SHEDINJA,
};

static const u16 sStarterMonWhismur[STARTER_MON_COUNT] =
{
    SPECIES_WHISMUR,
    SPECIES_WHISMUR,
    SPECIES_WHISMUR,
};

static const u16 sStarterMonLoudred[STARTER_MON_COUNT] =
{
    SPECIES_LOUDRED,
    SPECIES_LOUDRED,
    SPECIES_LOUDRED,
};

static const u16 sStarterMonExploud[STARTER_MON_COUNT] =
{
    SPECIES_EXPLOUD,
    SPECIES_EXPLOUD,
    SPECIES_EXPLOUD,
};

static const u16 sStarterMonMakuhita[STARTER_MON_COUNT] =
{
    SPECIES_MAKUHITA,
    SPECIES_MAKUHITA,
    SPECIES_MAKUHITA,
};

static const u16 sStarterMonHariyama[STARTER_MON_COUNT] =
{
    SPECIES_HARIYAMA,
    SPECIES_HARIYAMA,
    SPECIES_HARIYAMA,
};

static const u16 sStarterMonAzurill[STARTER_MON_COUNT] =
{
    SPECIES_AZURILL,
    SPECIES_AZURILL,
    SPECIES_AZURILL,
};

static const u16 sStarterMonNosepass[STARTER_MON_COUNT] =
{
    SPECIES_NOSEPASS,
    SPECIES_NOSEPASS,
    SPECIES_NOSEPASS,
};

static const u16 sStarterMonSkitty[STARTER_MON_COUNT] =
{
    SPECIES_SKITTY,
    SPECIES_SKITTY,
    SPECIES_SKITTY,
};

static const u16 sStarterMonDelcatty[STARTER_MON_COUNT] =
{
    SPECIES_DELCATTY,
    SPECIES_DELCATTY,
    SPECIES_DELCATTY,
};

static const u16 sStarterMonSableye[STARTER_MON_COUNT] =
{
    SPECIES_SABLEYE,
    SPECIES_SABLEYE,
    SPECIES_SABLEYE,
};

static const u16 sStarterMonMawile[STARTER_MON_COUNT] =
{
    SPECIES_MAWILE,
    SPECIES_MAWILE,
    SPECIES_MAWILE,
};

static const u16 sStarterMonAron[STARTER_MON_COUNT] =
{
    SPECIES_ARON,
    SPECIES_ARON,
    SPECIES_ARON,
};

static const u16 sStarterMonLairon[STARTER_MON_COUNT] =
{
    SPECIES_LAIRON,
    SPECIES_LAIRON,
    SPECIES_LAIRON,
};

static const u16 sStarterMonAggron[STARTER_MON_COUNT] =
{
    SPECIES_AGGRON,
    SPECIES_AGGRON,
    SPECIES_AGGRON,
};

static const u16 sStarterMonMeditite[STARTER_MON_COUNT] =
{
    SPECIES_MEDITITE,
    SPECIES_MEDITITE,
    SPECIES_MEDITITE,
};

static const u16 sStarterMonMedicham[STARTER_MON_COUNT] =
{
    SPECIES_MEDICHAM,
    SPECIES_MEDICHAM,
    SPECIES_MEDICHAM,
};

static const u16 sStarterMonElectrike[STARTER_MON_COUNT] =
{
    SPECIES_ELECTRIKE,
    SPECIES_ELECTRIKE,
    SPECIES_ELECTRIKE,
};

static const u16 sStarterMonManectric[STARTER_MON_COUNT] =
{
    SPECIES_MANECTRIC,
    SPECIES_MANECTRIC,
    SPECIES_MANECTRIC,
};

static const u16 sStarterMonPlusle[STARTER_MON_COUNT] =
{
    SPECIES_PLUSLE,
    SPECIES_PLUSLE,
    SPECIES_PLUSLE,
};

static const u16 sStarterMonMinun[STARTER_MON_COUNT] =
{
    SPECIES_MINUN,
    SPECIES_MINUN,
    SPECIES_MINUN,
};

static const u16 sStarterMonVolbeat[STARTER_MON_COUNT] =
{
    SPECIES_VOLBEAT,
    SPECIES_VOLBEAT,
    SPECIES_VOLBEAT,
};

static const u16 sStarterMonIllumise[STARTER_MON_COUNT] =
{
    SPECIES_ILLUMISE,
    SPECIES_ILLUMISE,
    SPECIES_ILLUMISE,
};

static const u16 sStarterMonRoselia[STARTER_MON_COUNT] =
{
    SPECIES_ROSELIA,
    SPECIES_ROSELIA,
    SPECIES_ROSELIA,
};

static const u16 sStarterMonGulpin[STARTER_MON_COUNT] =
{
    SPECIES_GULPIN,
    SPECIES_GULPIN,
    SPECIES_GULPIN,
};

static const u16 sStarterMonSwalot[STARTER_MON_COUNT] =
{
    SPECIES_SWALOT,
    SPECIES_SWALOT,
    SPECIES_SWALOT,
};

static const u16 sStarterMonCarvanha[STARTER_MON_COUNT] =
{
    SPECIES_CARVANHA,
    SPECIES_CARVANHA,
    SPECIES_CARVANHA,
};

static const u16 sStarterMonSharpedo[STARTER_MON_COUNT] =
{
    SPECIES_SHARPEDO,
    SPECIES_SHARPEDO,
    SPECIES_SHARPEDO,
};

static const u16 sStarterMonWailmer[STARTER_MON_COUNT] =
{
    SPECIES_WAILMER,
    SPECIES_WAILMER,
    SPECIES_WAILMER,
};

static const u16 sStarterMonWailord[STARTER_MON_COUNT] =
{
    SPECIES_WAILORD,
    SPECIES_WAILORD,
    SPECIES_WAILORD,
};

static const u16 sStarterMonNumel[STARTER_MON_COUNT] =
{
    SPECIES_NUMEL,
    SPECIES_NUMEL,
    SPECIES_NUMEL,
};

static const u16 sStarterMonCamerupt[STARTER_MON_COUNT] =
{
    SPECIES_CAMERUPT,
    SPECIES_CAMERUPT,
    SPECIES_CAMERUPT,
};

static const u16 sStarterMonTorkoal[STARTER_MON_COUNT] =
{
    SPECIES_TORKOAL,
    SPECIES_TORKOAL,
    SPECIES_TORKOAL,
};

static const u16 sStarterMonSpoink[STARTER_MON_COUNT] =
{
    SPECIES_SPOINK,
    SPECIES_SPOINK,
    SPECIES_SPOINK,
};

static const u16 sStarterMonGrumpig[STARTER_MON_COUNT] =
{
    SPECIES_GRUMPIG,
    SPECIES_GRUMPIG,
    SPECIES_GRUMPIG,
};

static const u16 sStarterMonSpinda[STARTER_MON_COUNT] =
{
    SPECIES_SPINDA,
    SPECIES_SPINDA,
    SPECIES_SPINDA,
};

static const u16 sStarterMonTrapinch[STARTER_MON_COUNT] =
{
    SPECIES_TRAPINCH,
    SPECIES_TRAPINCH,
    SPECIES_TRAPINCH,
};

static const u16 sStarterMonVibrava[STARTER_MON_COUNT] =
{
    SPECIES_VIBRAVA,
    SPECIES_VIBRAVA,
    SPECIES_VIBRAVA,
};

static const u16 sStarterMonFlygon[STARTER_MON_COUNT] =
{
    SPECIES_FLYGON,
    SPECIES_FLYGON,
    SPECIES_FLYGON,
};

static const u16 sStarterMonCacnea[STARTER_MON_COUNT] =
{
    SPECIES_CACNEA,
    SPECIES_CACNEA,
    SPECIES_CACNEA,
};

static const u16 sStarterMonCacturne[STARTER_MON_COUNT] =
{
    SPECIES_CACTURNE,
    SPECIES_CACTURNE,
    SPECIES_CACTURNE,
};

static const u16 sStarterMonSwablu[STARTER_MON_COUNT] =
{
    SPECIES_SWABLU,
    SPECIES_SWABLU,
    SPECIES_SWABLU,
};

static const u16 sStarterMonAltaria[STARTER_MON_COUNT] =
{
    SPECIES_ALTARIA,
    SPECIES_ALTARIA,
    SPECIES_ALTARIA,
};

static const u16 sStarterMonZangoose[STARTER_MON_COUNT] =
{
    SPECIES_ZANGOOSE,
    SPECIES_ZANGOOSE,
    SPECIES_ZANGOOSE,
};

static const u16 sStarterMonSeviper[STARTER_MON_COUNT] =
{
    SPECIES_SEVIPER,
    SPECIES_SEVIPER,
    SPECIES_SEVIPER,
};

static const u16 sStarterMonLunatone[STARTER_MON_COUNT] =
{
    SPECIES_LUNATONE,
    SPECIES_LUNATONE,
    SPECIES_LUNATONE,
};

static const u16 sStarterMonSolrock[STARTER_MON_COUNT] =
{
    SPECIES_SOLROCK,
    SPECIES_SOLROCK,
    SPECIES_SOLROCK,
};

static const u16 sStarterMonBarboach[STARTER_MON_COUNT] =
{
    SPECIES_BARBOACH,
    SPECIES_BARBOACH,
    SPECIES_BARBOACH,
};

static const u16 sStarterMonWhiscash[STARTER_MON_COUNT] =
{
    SPECIES_WHISCASH,
    SPECIES_WHISCASH,
    SPECIES_WHISCASH,
};

static const u16 sStarterMonCorphish[STARTER_MON_COUNT] =
{
    SPECIES_CORPHISH,
    SPECIES_CORPHISH,
    SPECIES_CORPHISH,
};

static const u16 sStarterMonCrawdaunt[STARTER_MON_COUNT] =
{
    SPECIES_CRAWDAUNT,
    SPECIES_CRAWDAUNT,
    SPECIES_CRAWDAUNT,
};

static const u16 sStarterMonBaltoy[STARTER_MON_COUNT] =
{
    SPECIES_BALTOY,
    SPECIES_BALTOY,
    SPECIES_BALTOY,
};

static const u16 sStarterMonClaydol[STARTER_MON_COUNT] =
{
    SPECIES_CLAYDOL,
    SPECIES_CLAYDOL,
    SPECIES_CLAYDOL,
};

static const u16 sStarterMonLileep[STARTER_MON_COUNT] =
{
    SPECIES_LILEEP,
    SPECIES_LILEEP,
    SPECIES_LILEEP,
};

static const u16 sStarterMonCradily[STARTER_MON_COUNT] =
{
    SPECIES_CRADILY,
    SPECIES_CRADILY,
    SPECIES_CRADILY,
};

static const u16 sStarterMonAnortih[STARTER_MON_COUNT] =
{
    SPECIES_ANORITH,
    SPECIES_ANORITH,
    SPECIES_ANORITH,
};

static const u16 sStarterMonArmaldo[STARTER_MON_COUNT] =
{
    SPECIES_ARMALDO,
    SPECIES_ARMALDO,
    SPECIES_ARMALDO,
};

static const u16 sStarterMonFeebas[STARTER_MON_COUNT] =
{
    SPECIES_FEEBAS,
    SPECIES_FEEBAS,
    SPECIES_FEEBAS,
};

static const u16 sStarterMonMilotic[STARTER_MON_COUNT] =
{
    SPECIES_MILOTIC,
    SPECIES_MILOTIC,
    SPECIES_MILOTIC,
};

static const u16 sStarterMonCastform[STARTER_MON_COUNT] =
{
    SPECIES_CASTFORM,
    SPECIES_CASTFORM,
    SPECIES_CASTFORM,
};

static const u16 sStarterMonKecleon[STARTER_MON_COUNT] =
{
    SPECIES_KECLEON,
    SPECIES_KECLEON,
    SPECIES_KECLEON,
};

static const u16 sStarterMonShuppet[STARTER_MON_COUNT] =
{
    SPECIES_SHUPPET,
    SPECIES_SHUPPET,
    SPECIES_SHUPPET,
};

static const u16 sStarterMonBanette[STARTER_MON_COUNT] =
{
    SPECIES_BANETTE,
    SPECIES_BANETTE,
    SPECIES_BANETTE,
};

static const u16 sStarterMonDuskull[STARTER_MON_COUNT] =
{
    SPECIES_DUSKULL,
    SPECIES_DUSKULL,
    SPECIES_DUSKULL,
};

static const u16 sStarterMonDusclops[STARTER_MON_COUNT] =
{
    SPECIES_DUSCLOPS,
    SPECIES_DUSCLOPS,
    SPECIES_DUSCLOPS,
};

static const u16 sStarterMonTropius[STARTER_MON_COUNT] =
{
    SPECIES_TROPIUS,
    SPECIES_TROPIUS,
    SPECIES_TROPIUS,
};

static const u16 sStarterMonChimecho[STARTER_MON_COUNT] =
{
    SPECIES_CHIMECHO,
    SPECIES_CHIMECHO,
    SPECIES_CHIMECHO,
};

static const u16 sStarterMonAbsol[STARTER_MON_COUNT] =
{
    SPECIES_ABSOL,
    SPECIES_ABSOL,
    SPECIES_ABSOL,
};

static const u16 sStarterMonWynaut[STARTER_MON_COUNT] =
{
    SPECIES_WYNAUT,
    SPECIES_WYNAUT,
    SPECIES_WYNAUT,
};

static const u16 sStarterMonSnorunt[STARTER_MON_COUNT] =
{
    SPECIES_SNORUNT,
    SPECIES_SNORUNT,
    SPECIES_SNORUNT,
};

static const u16 sStarterMonGlalie[STARTER_MON_COUNT] =
{
    SPECIES_GLALIE,
    SPECIES_GLALIE,
    SPECIES_GLALIE,
};

static const u16 sStarterMonSpheal[STARTER_MON_COUNT] =
{
    SPECIES_SPHEAL,
    SPECIES_SPHEAL,
    SPECIES_SPHEAL,
};

static const u16 sStarterMonSealeo[STARTER_MON_COUNT] =
{
    SPECIES_SEALEO,
    SPECIES_SEALEO,
    SPECIES_SEALEO,
};

static const u16 sStarterMonWalrein[STARTER_MON_COUNT] =
{
    SPECIES_WALREIN,
    SPECIES_WALREIN,
    SPECIES_WALREIN,
};

static const u16 sStarterMonClamperl[STARTER_MON_COUNT] =
{
    SPECIES_CLAMPERL,
    SPECIES_CLAMPERL,
    SPECIES_CLAMPERL,
};

static const u16 sStarterMonHuntail[STARTER_MON_COUNT] =
{
    SPECIES_HUNTAIL,
    SPECIES_HUNTAIL,
    SPECIES_HUNTAIL,
};

static const u16 sStarterMonGorebyss[STARTER_MON_COUNT] =
{
    SPECIES_GOREBYSS,
    SPECIES_GOREBYSS,
    SPECIES_GOREBYSS,
};

static const u16 sStarterMonRelicanth[STARTER_MON_COUNT] =
{
    SPECIES_RELICANTH,
    SPECIES_RELICANTH,
    SPECIES_RELICANTH,
};

static const u16 sStarterMonLuvdisc[STARTER_MON_COUNT] =
{
    SPECIES_LUVDISC,
    SPECIES_LUVDISC,
    SPECIES_LUVDISC,
};

static const u16 sStarterMonBagon[STARTER_MON_COUNT] =
{
    SPECIES_BAGON,
    SPECIES_BAGON,
    SPECIES_BAGON,
};

static const u16 sStarterMonShelgon[STARTER_MON_COUNT] =
{
    SPECIES_SHELGON,
    SPECIES_SHELGON,
    SPECIES_SHELGON,
};

static const u16 sStarterMonSalamence[STARTER_MON_COUNT] =
{
    SPECIES_SALAMENCE,
    SPECIES_SALAMENCE,
    SPECIES_SALAMENCE,
};

static const u16 sStarterMonBeldum[STARTER_MON_COUNT] =
{
    SPECIES_BELDUM,
    SPECIES_BELDUM,
    SPECIES_BELDUM,
};

static const u16 sStarterMonMetang[STARTER_MON_COUNT] =
{
    SPECIES_METANG,
    SPECIES_METANG,
    SPECIES_METANG,
};

static const u16 sStarterMonMetagross[STARTER_MON_COUNT] =
{
    SPECIES_METAGROSS,
    SPECIES_METAGROSS,
    SPECIES_METAGROSS,
};

static const u16 sStarterMonRegirock[STARTER_MON_COUNT] =
{
    SPECIES_REGIROCK,
    SPECIES_REGIROCK,
    SPECIES_REGIROCK,
};

static const u16 sStarterMonRegice[STARTER_MON_COUNT] =
{
    SPECIES_REGICE,
    SPECIES_REGICE,
    SPECIES_REGICE,
};

static const u16 sStarterMonRegisteel[STARTER_MON_COUNT] =
{
    SPECIES_REGISTEEL,
    SPECIES_REGISTEEL,
    SPECIES_REGISTEEL,
};

static const u16 sStarterMonLatias[STARTER_MON_COUNT] =
{
    SPECIES_LATIAS,
    SPECIES_LATIAS,
    SPECIES_LATIAS,
};

static const u16 sStarterMonLatios[STARTER_MON_COUNT] =
{
    SPECIES_LATIOS,
    SPECIES_LATIOS,
    SPECIES_LATIOS,
};

static const u16 sStarterMonKyogre[STARTER_MON_COUNT] =
{
    SPECIES_KYOGRE,
    SPECIES_KYOGRE,
    SPECIES_KYOGRE,
};

static const u16 sStarterMonGroudon[STARTER_MON_COUNT] =
{
    SPECIES_GROUDON,
    SPECIES_GROUDON,
    SPECIES_GROUDON,
};

static const u16 sStarterMonRayquaza[STARTER_MON_COUNT] =
{
    SPECIES_RAYQUAZA,
    SPECIES_RAYQUAZA,
    SPECIES_RAYQUAZA,
};

static const u16 sStarterMonJirachi[STARTER_MON_COUNT] =
{
    SPECIES_JIRACHI,
    SPECIES_JIRACHI,
    SPECIES_JIRACHI,
};

static const u16 sStarterMonDeoxys[STARTER_MON_COUNT] =
{
    SPECIES_DEOXYS,
    SPECIES_DEOXYS,
    SPECIES_DEOXYS,
};

static const struct BgTemplate sBgTemplates[3] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 6,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
};

static const u8 sTextColors[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY};

static const struct OamData sOam_Hand =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct OamData sOam_Pokeball =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct OamData sOam_StarterCircle =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0,
};

static const u8 sCursorCoords[][2] =
{
    {60, 32},
    {120, 56},
    {180, 32},
};

static const union AnimCmd sAnim_Hand[] =
{
    ANIMCMD_FRAME(48, 30),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_Pokeball_Still[] =
{
    ANIMCMD_FRAME(0, 30),
    ANIMCMD_END,
};

static const union AnimCmd sAnim_Pokeball_Moving[] =
{
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_FRAME(0, 32),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_JUMP(0),
};

static const union AnimCmd sAnim_StarterCircle[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END,
};

static const union AnimCmd * const sAnims_Hand[] =
{
    sAnim_Hand,
};

static const union AnimCmd * const sAnims_Pokeball[] =
{
    sAnim_Pokeball_Still,
    sAnim_Pokeball_Moving,
};

static const union AnimCmd * const sAnims_StarterCircle[] =
{
    sAnim_StarterCircle,
};

static const union AffineAnimCmd sAffineAnim_StarterPokemon[] =
{
    AFFINEANIMCMD_FRAME(16, 16, 0, 0),
    AFFINEANIMCMD_FRAME(16, 16, 0, 15),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_StarterCircle[] =
{
    AFFINEANIMCMD_FRAME(20, 20, 0, 0),
    AFFINEANIMCMD_FRAME(20, 20, 0, 15),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd * const sAffineAnims_StarterPokemon = {sAffineAnim_StarterPokemon};
static const union AffineAnimCmd * const sAffineAnims_StarterCircle[] = {sAffineAnim_StarterCircle};

static const struct CompressedSpriteSheet sSpriteSheet_PokeballSelect[] =
{
    {
        .data = gPokeballSelection_Gfx,
        .size = 0x0800,
        .tag = TAG_POKEBALL_SELECT
    },
    {}
};

static const struct CompressedSpriteSheet sSpriteSheet_StarterCircle[] =
{
    {
        .data = sStarterCircle_Gfx,
        .size = 0x0800,
        .tag = TAG_STARTER_CIRCLE
    },
    {}
};

static const struct SpritePalette sSpritePalettes_StarterChoose[] =
{
    {
        .data = sPokeballSelection_Pal,
        .tag = TAG_POKEBALL_SELECT
    },
    {
        .data = sStarterCircle_Pal,
        .tag = TAG_STARTER_CIRCLE
    },
    {},
};

static const struct SpriteTemplate sSpriteTemplate_Hand =
{
    .tileTag = TAG_POKEBALL_SELECT,
    .paletteTag = TAG_POKEBALL_SELECT,
    .oam = &sOam_Hand,
    .anims = sAnims_Hand,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_SelectionHand
};

static const struct SpriteTemplate sSpriteTemplate_Pokeball =
{
    .tileTag = TAG_POKEBALL_SELECT,
    .paletteTag = TAG_POKEBALL_SELECT,
    .oam = &sOam_Pokeball,
    .anims = sAnims_Pokeball,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Pokeball
};

static const struct SpriteTemplate sSpriteTemplate_StarterCircle =
{
    .tileTag = TAG_STARTER_CIRCLE,
    .paletteTag = TAG_STARTER_CIRCLE,
    .oam = &sOam_StarterCircle,
    .anims = sAnims_StarterCircle,
    .images = NULL,
    .affineAnims = sAffineAnims_StarterCircle,
    .callback = SpriteCB_StarterPokemon
};

// .text
u16 GetStarterPokemon(u16 chosenStarterId)
{
    u16 StarterNatDexNum = VarGet(VAR_CUSTOM_STARTER);

    while (StarterNatDexNum > (NUM_SPECIES - 1)) // Just smh dumb I put to prevent going out of bounds
    {
        StarterNatDexNum = StarterNatDexNum % (NUM_SPECIES - 1);
    }

    if (chosenStarterId > STARTER_MON_COUNT)
        chosenStarterId = 0;
    switch (StarterNatDexNum)
    {
    case 0:
        return sStarterMon[chosenStarterId];
    case 1:
        return sStarterMonBulbasaur[chosenStarterId];
    case 2:
        return sStarterMonIvysaur[chosenStarterId];
    case 3:
        return sStarterMonVenusaur[chosenStarterId];
    case 4:
        return sStarterMonCharmander[chosenStarterId];
    case 5:
        return sStarterMonCharmeleon[chosenStarterId];
    case 6:
        return sStarterMonCharizard[chosenStarterId];
    case 7:
        return sStarterMonSquirtle[chosenStarterId];
    case 8:
        return sStarterMonWartortle[chosenStarterId];
    case 9:
        return sStarterMonBlastoise[chosenStarterId];
    case 10:
        return sStarterMonCaterpie[chosenStarterId];
    case 11:
        return sStarterMonMetapod[chosenStarterId];
    case 12:
        return sStarterMonButterfree[chosenStarterId];
    case 13:
        return sStarterMonWeedle[chosenStarterId];
    case 14:
        return sStarterMonKakuna[chosenStarterId];
    case 15:
        return sStarterMonBeedrill[chosenStarterId];
    case 16:
        return sStarterMonPidgey[chosenStarterId];
    case 17:
        return sStarterMonPidgeotto[chosenStarterId];
    case 18:
        return sStarterMonPidgeot[chosenStarterId];
    case 19:
        return sStarterMonRattata[chosenStarterId];
    case 20:
        return sStarterMonRaticate[chosenStarterId];
    case 21:
        return sStarterMonSpearow[chosenStarterId];
    case 22:
        return sStarterMonFearow[chosenStarterId];
    case 23:
        return sStarterMonEkans[chosenStarterId];
    case 24:
        return sStarterMonArbok[chosenStarterId];
    case 25:
        return sStarterMonPikachu[chosenStarterId];
    case 26:
        return sStarterMonRaichu[chosenStarterId];
    case 27:
        return sStarterMonSandshrew[chosenStarterId];
    case 28:
        return sStarterMonSandslash[chosenStarterId];
    case 29:
        return sStarterMonNidoranF[chosenStarterId];
    case 30:
        return sStarterMonNidorina[chosenStarterId];
    case 31:
        return sStarterMonNidoqueen[chosenStarterId];
    case 32:
        return sStarterMonNidoranM[chosenStarterId];
    case 33:
        return sStarterMonNidorino[chosenStarterId];
    case 34:
        return sStarterMonNidoking[chosenStarterId];
    case 35:
        return sStarterMonClefairy[chosenStarterId];
    case 36:
        return sStarterMonClefable[chosenStarterId];
    case 37:
        return sStarterMonVulpix[chosenStarterId];
    case 38:
        return sStarterMonNinetales[chosenStarterId];
    case 39:
        return sStarterMonJigglypuff[chosenStarterId];
    case 40:
        return sStarterMonWigglytuff[chosenStarterId];
    case 41:
        return sStarterMonZubat[chosenStarterId];
    case 42:
        return sStarterMonGolbat[chosenStarterId];
    case 43:
        return sStarterMonOddish[chosenStarterId];
    case 44:
        return sStarterMonGloom[chosenStarterId];
    case 45:
        return sStarterMonVileplume[chosenStarterId];
    case 46:
        return sStarterMonParas[chosenStarterId];
    case 47:
        return sStarterMonParasect[chosenStarterId];
    case 48:
        return sStarterMonVenonat[chosenStarterId];
    case 49:
        return sStarterMonVenomoth[chosenStarterId];
    case 50:
        return sStarterMonDiglett[chosenStarterId];
    case 51:
        return sStarterMonDugtrio[chosenStarterId];
    case 52:
        return sStarterMonMeowth[chosenStarterId];
    case 53:
        return sStarterMonPersian[chosenStarterId];
    case 54:
        return sStarterMonPsyduck[chosenStarterId];
    case 55:
        return sStarterMonGolduck[chosenStarterId];
    case 56:
        return sStarterMonMankey[chosenStarterId];
    case 57:
        return sStarterMonPrimeape[chosenStarterId];
    case 58:
        return sStarterMonGrowlithe[chosenStarterId];
    case 59:
        return sStarterMonArcanine[chosenStarterId];
    case 60:
        return sStarterMonPoliwag[chosenStarterId];
    case 61:
        return sStarterMonPoliwhirl[chosenStarterId];
    case 62:
        return sStarterMonPoliwrath[chosenStarterId];
    case 63:
        return sStarterMonAbra[chosenStarterId];
    case 64:
        return sStarterMonKadabra[chosenStarterId];
    case 65:
        return sStarterMonAlakazam[chosenStarterId];
    case 66:
        return sStarterMonMachop[chosenStarterId];
    case 67:
        return sStarterMonMachoke[chosenStarterId];
    case 68:
        return sStarterMonMachamp[chosenStarterId];
    case 69:
        return sStarterMonBellsprout[chosenStarterId];
    case 70:
        return sStarterMonWeepinbell[chosenStarterId];
    case 71:
        return sStarterMonVictreebel[chosenStarterId];
    case 72:
        return sStarterMonTentacool[chosenStarterId];
    case 73:
        return sStarterMonTentacruel[chosenStarterId];
    case 74:
        return sStarterMonGeodude[chosenStarterId];
    case 75:
        return sStarterMonGraveler[chosenStarterId];
    case 76:
        return sStarterMonGolem[chosenStarterId];
    case 77:
        return sStarterMonPonyta[chosenStarterId];
    case 78:
        return sStarterMonRapidash[chosenStarterId];
    case 79:
        return sStarterMonSlowpoke[chosenStarterId];
    case 80:
        return sStarterMonSlowbro[chosenStarterId];
    case 81:
        return sStarterMonMagnemite[chosenStarterId];
    case 82:
        return sStarterMonMagneton[chosenStarterId];
    case 83:
        return sStarterMonFarfetchd[chosenStarterId];
    case 84:
        return sStarterMonDoduo[chosenStarterId];
    case 85:
        return sStarterMonDodrio[chosenStarterId];
    case 86:
        return sStarterMonSeel[chosenStarterId];
    case 87:
        return sStarterMonDewgong[chosenStarterId];
    case 88:
        return sStarterMonGrimer[chosenStarterId];
    case 89:
        return sStarterMonMuk[chosenStarterId];
    case 90:
        return sStarterMonShellder[chosenStarterId];
    case 91:
        return sStarterMonCloyster[chosenStarterId];
    case 92:
        return sStarterMonGastly[chosenStarterId];
    case 93:
        return sStarterMonHaunter[chosenStarterId];
    case 94:
        return sStarterMonGengar[chosenStarterId];
    case 95:
        return sStarterMonOnix[chosenStarterId];
    case 96:
        return sStarterMonDrowzee[chosenStarterId];
    case 97:
        return sStarterMonHypno[chosenStarterId];
    case 98:
        return sStarterMonKrabby[chosenStarterId];
    case 99:
        return sStarterMonKingler[chosenStarterId];
    case 100:
        return sStarterMonVoltorb[chosenStarterId];
    case 101:
        return sStarterMonElectrode[chosenStarterId];
    case 102:
        return sStarterMonExeggcute[chosenStarterId];
    case 103:
        return sStarterMonExeggutor[chosenStarterId];
    case 104:
        return sStarterMonCubone[chosenStarterId];
    case 105:
        return sStarterMonMarowak[chosenStarterId];
    case 106:
        return sStarterMonHitmonlee[chosenStarterId];
    case 107:
        return sStarterMonHitmonchan[chosenStarterId];
    case 108:
        return sStarterMonLickitung[chosenStarterId];
    case 109:
        return sStarterMonKoffing[chosenStarterId];
    case 110:
        return sStarterMonWeezing[chosenStarterId];
    case 111:
        return sStarterMonRhyhorn[chosenStarterId];
    case 112:
        return sStarterMonRhydon[chosenStarterId];
    case 113:
        return sStarterMonChansey[chosenStarterId];
    case 114:
        return sStarterMonTangela[chosenStarterId];
    case 115:
        return sStarterMonKangaskhan[chosenStarterId];
    case 116:
        return sStarterMonHorsea[chosenStarterId];
    case 117:
        return sStarterMonSeadra[chosenStarterId];
    case 118:
        return sStarterMonGoldeen[chosenStarterId];
    case 119:
        return sStarterMonSeaking[chosenStarterId];
    case 120:
        return sStarterMonStaryu[chosenStarterId];
    case 121:
        return sStarterMonStarmie[chosenStarterId];
    case 122:
        return sStarterMonMrMime[chosenStarterId];
    case 123:
        return sStarterMonScyther[chosenStarterId];
    case 124:
        return sStarterMonJynx[chosenStarterId];
    case 125:
        return sStarterMonElectabuzz[chosenStarterId];
    case 126:
        return sStarterMonMagmar[chosenStarterId];
    case 127:
        return sStarterMonPinsir[chosenStarterId];
    case 128:
        return sStarterMonTauros[chosenStarterId];
    case 129:
        return sStarterMonMagikarp[chosenStarterId];
    case 130:
        return sStarterMonGyarados[chosenStarterId];
    case 131:
        return sStarterMonLapras[chosenStarterId];
    case 132:
        return sStarterMonDitto[chosenStarterId];
    case 133:
        return sStarterMonEevee[chosenStarterId];
    case 134:
        return sStarterMonVaporeon[chosenStarterId];
    case 135:
        return sStarterMonJolteon[chosenStarterId];
    case 136:
        return sStarterMonFlareon[chosenStarterId];
    case 137:
        return sStarterMonPorygon[chosenStarterId];
    case 138:
        return sStarterMonOmanyte[chosenStarterId];
    case 139:
        return sStarterMonOmastar[chosenStarterId];
    case 140:
        return sStarterMonKabuto[chosenStarterId];
    case 141:
        return sStarterMonKabutops[chosenStarterId];
    case 142:
        return sStarterMonAerodactyl[chosenStarterId];
    case 143:
        return sStarterMonSnorlax[chosenStarterId];
    case 144:
        return sStarterMonArticuno[chosenStarterId];
    case 145:
        return sStarterMonZapdos[chosenStarterId];
    case 146:
        return sStarterMonMoltres[chosenStarterId];
    case 147:
        return sStarterMonDratini[chosenStarterId];
    case 148:
        return sStarterMonDragonair[chosenStarterId];
    case 149:
        return sStarterMonDragonite[chosenStarterId];
    case 150:
        return sStarterMonMewtwo[chosenStarterId];
    case 151:
        return sStarterMonMew[chosenStarterId];
    case 152:
        return sStarterMonChikorita[chosenStarterId];
    case 153:
        return sStarterMonBayleef[chosenStarterId];
    case 154:
        return sStarterMonMeganium[chosenStarterId];
    case 155:
        return sStarterMonCyndaquil[chosenStarterId];
    case 156:
        return sStarterMonQuilava[chosenStarterId];
    case 157:
        return sStarterMonTyphlosion[chosenStarterId];
    case 158:
        return sStarterMonTotodile[chosenStarterId];
    case 159:
        return sStarterMonCroconaw[chosenStarterId];
    case 160:
        return sStarterMonFeraligatr[chosenStarterId];
    case 161:
        return sStarterMonSentret[chosenStarterId];
    case 162:
        return sStarterMonFurret[chosenStarterId];
    case 163:
        return sStarterMonHoothoot[chosenStarterId];
    case 164:
        return sStarterMonNoctowl[chosenStarterId];
    case 165:
        return sStarterMonLedyba[chosenStarterId];
    case 166:
        return sStarterMonLedian[chosenStarterId];
    case 167:
        return sStarterMonSpinarak[chosenStarterId];
    case 168:
        return sStarterMonAriados[chosenStarterId];
    case 169:
        return sStarterMonCrobat[chosenStarterId];
    case 170:
        return sStarterMonChinchou[chosenStarterId];
    case 171:
        return sStarterMonLanturn[chosenStarterId];
    case 172:
        return sStarterMonPichu[chosenStarterId];
    case 173:
        return sStarterMonCleffa[chosenStarterId];
    case 174:
        return sStarterMonIgglybuff[chosenStarterId];
    case 175:
        return sStarterMonTogepi[chosenStarterId];
    case 176:
        return sStarterMonTogetic[chosenStarterId];
    case 177:
        return sStarterMonNatu[chosenStarterId];
    case 178:
        return sStarterMonXatu[chosenStarterId];
    case 179:
        return sStarterMonMareep[chosenStarterId];
    case 180:
        return sStarterMonFlaaffy[chosenStarterId];
    case 181:
        return sStarterMonAmpharos[chosenStarterId];
    case 182:
        return sStarterMonBellossom[chosenStarterId];
    case 183:
        return sStarterMonMarill[chosenStarterId];
    case 184:
        return sStarterMonAzumarill[chosenStarterId];
    case 185:
        return sStarterMonSudowoodo[chosenStarterId];
    case 186:
        return sStarterMonPolitoed[chosenStarterId];
    case 187:
        return sStarterMonHoppip[chosenStarterId];
    case 188:
        return sStarterMonSkiploom[chosenStarterId];
    case 189:
        return sStarterMonJumpluff[chosenStarterId];
    case 190:
        return sStarterMonAipom[chosenStarterId];
    case 191:
        return sStarterMonSunkern[chosenStarterId];
    case 192:
        return sStarterMonSunflora[chosenStarterId];
    case 193:
        return sStarterMonYanma[chosenStarterId];
    case 194:
        return sStarterMonWooper[chosenStarterId];
    case 195:
        return sStarterMonQuagsire[chosenStarterId];
    case 196:
        return sStarterMonEspeon[chosenStarterId];
    case 197:
        return sStarterMonUmbreon[chosenStarterId];
    case 198:
        return sStarterMonMurkrow[chosenStarterId];
    case 199:
        return sStarterMonSlowking[chosenStarterId];
    case 200:
        return sStarterMonMisdreavus[chosenStarterId];
    case 201:
        return sStarterMonUnown[chosenStarterId];
    case 202:
        return sStarterMonWobbuffet[chosenStarterId];
    case 203:
        return sStarterMonGirafarig[chosenStarterId];
    case 204:
        return sStarterMonPineco[chosenStarterId];
    case 205:
        return sStarterMonForretress[chosenStarterId];
    case 206:
        return sStarterMonDunsparce[chosenStarterId];
    case 207:
        return sStarterMonGligar[chosenStarterId];
    case 208:
        return sStarterMonSteelix[chosenStarterId];
    case 209:
        return sStarterMonSnubbull[chosenStarterId];
    case 210:
        return sStarterMonGranbull[chosenStarterId];
    case 211:
        return sStarterMonQwilfish[chosenStarterId];
    case 212:
        return sStarterMonScizor[chosenStarterId];
    case 213:
        return sStarterMonShuckle[chosenStarterId];
    case 214:
        return sStarterMonHeracross[chosenStarterId];
    case 215:
        return sStarterMonSneasel[chosenStarterId];
    case 216:
        return sStarterMonTeddiursa[chosenStarterId];
    case 217:
        return sStarterMonUrsaring[chosenStarterId];
    case 218:
        return sStarterMonSlugma[chosenStarterId];
    case 219:
        return sStarterMonMagcargo[chosenStarterId];
    case 220:
        return sStarterMonSwinub[chosenStarterId];
    case 221:
        return sStarterMonPiloswine[chosenStarterId];
    case 222:
        return sStarterMonCorsola[chosenStarterId];
    case 223:
        return sStarterMonRemoraid[chosenStarterId];
    case 224:
        return sStarterMonOctillery[chosenStarterId];
    case 225:
        return sStarterMonDelibird[chosenStarterId];
    case 226:
        return sStarterMonMantine[chosenStarterId];
    case 227:
        return sStarterMonSkarmory[chosenStarterId];
    case 228:
        return sStarterMonHoundour[chosenStarterId];
    case 229:
        return sStarterMonHoundoom[chosenStarterId];
    case 230:
        return sStarterMonKingdra[chosenStarterId];
    case 231:
        return sStarterMonPhanpy[chosenStarterId];
    case 232:
        return sStarterMonDonphan[chosenStarterId];
    case 233:
        return sStarterMonPorygon2[chosenStarterId];
    case 234:
        return sStarterMonStantler[chosenStarterId];
    case 235:
        return sStarterMonSmeargle[chosenStarterId];
    case 236:
        return sStarterMonTyrogue[chosenStarterId];
    case 237:
        return sStarterMonHitmontop[chosenStarterId];
    case 238:
        return sStarterMonSmoochum[chosenStarterId];
    case 239:
        return sStarterMonElekid[chosenStarterId];
    case 240:
        return sStarterMonMagby[chosenStarterId];
    case 241:
        return sStarterMonMiltank[chosenStarterId];
    case 242:
        return sStarterMonBlissey[chosenStarterId];
    case 243:
        return sStarterMonRaikou[chosenStarterId];
    case 244:
        return sStarterMonEntei[chosenStarterId];
    case 245:
        return sStarterMonSuicune[chosenStarterId];
    case 246:
        return sStarterMonLarvitar[chosenStarterId];
    case 247:
        return sStarterMonPupitar[chosenStarterId];
    case 248:
        return sStarterMonTyranitar[chosenStarterId];
    case 249:
        return sStarterMonLugia[chosenStarterId];
    case 250:
        return sStarterMonHooh[chosenStarterId];
    case 251:
        return sStarterMonCelebi[chosenStarterId];
    case 252:
        return sStarterMonTreecko[chosenStarterId];
    case 253:
        return sStarterMonGrovyle[chosenStarterId];
    case 254:
        return sStarterMonSceptile[chosenStarterId];
    case 255:
        return sStarterMonTorchic[chosenStarterId];
    case 256:
        return sStarterMonCombusken[chosenStarterId];
    case 257:
        return sStarterMonBlaziken[chosenStarterId];
    case 258:
        return sStarterMonMudkip[chosenStarterId];
    case 259:
        return sStarterMonMarshtomp[chosenStarterId];
    case 260:
        return sStarterMonSwampert[chosenStarterId];
    case 261:
        return sStarterMonPoochyena[chosenStarterId];
    case 262:
        return sStarterMonMightyena[chosenStarterId];
    case 263:
        return sStarterMonZigzagoon[chosenStarterId];
    case 264:
        return sStarterMonLinoone[chosenStarterId];
    case 265:
        return sStarterMonWurmple[chosenStarterId];
    case 266:
        return sStarterMonSilcoon[chosenStarterId];
    case 267:
        return sStarterMonBeautifly[chosenStarterId];
    case 268:
        return sStarterMonCascoon[chosenStarterId];
    case 269:
        return sStarterMonDustox[chosenStarterId];
    case 270:
        return sStarterMonLotad[chosenStarterId];
    case 271:
        return sStarterMonLombre[chosenStarterId];
    case 272:
        return sStarterMonLudicolo[chosenStarterId];
    case 273:
        return sStarterMonSeedot[chosenStarterId];
    case 274:
        return sStarterMonNuzleaf[chosenStarterId];
    case 275:
        return sStarterMonShiftry[chosenStarterId];
    case 276:
        return sStarterMonTaillow[chosenStarterId];
    case 277:
        return sStarterMonSwellow[chosenStarterId];
    case 278:
        return sStarterMonWingull[chosenStarterId];
    case 279:
        return sStarterMonPelipper[chosenStarterId];
    case 280:
        return sStarterMonRalts[chosenStarterId];
    case 281:
        return sStarterMonKirlia[chosenStarterId];
    case 282:
        return sStarterMonGardevoir[chosenStarterId];
    case 283:
        return sStarterMonSurskit[chosenStarterId];
    case 284:
        return sStarterMonMasquerain[chosenStarterId];
    case 285:
        return sStarterMonShroomish[chosenStarterId];
    case 286:
        return sStarterMonBreloom[chosenStarterId];
    case 287:
        return sStarterMonSlakoth[chosenStarterId];
    case 288:
        return sStarterMonVigoroth[chosenStarterId];
    case 289:
        return sStarterMonSlaking[chosenStarterId];
    case 290:
        return sStarterMonNincada[chosenStarterId];
    case 291:
        return sStarterMonNinjask[chosenStarterId];
    case 292:
        return sStarterMonShedinja[chosenStarterId];
    case 293:
        return sStarterMonWhismur[chosenStarterId];
    case 294:
        return sStarterMonLoudred[chosenStarterId];
    case 295:
        return sStarterMonExploud[chosenStarterId];
    case 296:
        return sStarterMonMakuhita[chosenStarterId];
    case 297:
        return sStarterMonHariyama[chosenStarterId];
    case 298:
        return sStarterMonAzurill[chosenStarterId];
    case 299:
        return sStarterMonNosepass[chosenStarterId];
    case 300:
        return sStarterMonSkitty[chosenStarterId];
    case 301:
        return sStarterMonDelcatty[chosenStarterId];
    case 302:
        return sStarterMonSableye[chosenStarterId];
    case 303:
        return sStarterMonMawile[chosenStarterId];
    case 304:
        return sStarterMonAron[chosenStarterId];
    case 305:
        return sStarterMonLairon[chosenStarterId];
    case 306:
        return sStarterMonAggron[chosenStarterId];
    case 307:
        return sStarterMonMeditite[chosenStarterId];
    case 308:
        return sStarterMonMedicham[chosenStarterId];
    case 309:
        return sStarterMonElectrike[chosenStarterId];
    case 310:
        return sStarterMonManectric[chosenStarterId];
    case 311:
        return sStarterMonPlusle[chosenStarterId];
    case 312:
        return sStarterMonMinun[chosenStarterId];
    case 313:
        return sStarterMonVolbeat[chosenStarterId];
    case 314:
        return sStarterMonIllumise[chosenStarterId];
    case 315:
        return sStarterMonRoselia[chosenStarterId];
    case 316:
        return sStarterMonGulpin[chosenStarterId];
    case 317:
        return sStarterMonSwalot[chosenStarterId];
    case 318:
        return sStarterMonCarvanha[chosenStarterId];
    case 319:
        return sStarterMonSharpedo[chosenStarterId];
    case 320:
        return sStarterMonWailmer[chosenStarterId];
    case 321:
        return sStarterMonWailord[chosenStarterId];
    case 322:
        return sStarterMonNumel[chosenStarterId];
    case 323:
        return sStarterMonCamerupt[chosenStarterId];
    case 324:
        return sStarterMonTorkoal[chosenStarterId];
    case 325:
        return sStarterMonSpoink[chosenStarterId];
    case 326:
        return sStarterMonGrumpig[chosenStarterId];
    case 327:
        return sStarterMonSpinda[chosenStarterId];
    case 328:
        return sStarterMonTrapinch[chosenStarterId];
    case 329:
        return sStarterMonVibrava[chosenStarterId];
    case 330:
        return sStarterMonFlygon[chosenStarterId];
    case 331:
        return sStarterMonCacnea[chosenStarterId];
    case 332:
        return sStarterMonCacturne[chosenStarterId];
    case 333:
        return sStarterMonSwablu[chosenStarterId];
    case 334:
        return sStarterMonAltaria[chosenStarterId];
    case 335:
        return sStarterMonZangoose[chosenStarterId];
    case 336:
        return sStarterMonSeviper[chosenStarterId];
    case 337:
        return sStarterMonLunatone[chosenStarterId];
    case 338:
        return sStarterMonSolrock[chosenStarterId];
    case 339:
        return sStarterMonBarboach[chosenStarterId];
    case 340:
        return sStarterMonWhiscash[chosenStarterId];
    case 341:
        return sStarterMonCorphish[chosenStarterId];
    case 342:
        return sStarterMonCrawdaunt[chosenStarterId];
    case 343:
        return sStarterMonBaltoy[chosenStarterId];
    case 344:
        return sStarterMonClaydol[chosenStarterId];
    case 345:
        return sStarterMonLileep[chosenStarterId];
    case 346:
        return sStarterMonCradily[chosenStarterId];
    case 347:
        return sStarterMonAnortih[chosenStarterId];
    case 348:
        return sStarterMonArmaldo[chosenStarterId];
    case 349:
        return sStarterMonFeebas[chosenStarterId];
    case 350:
        return sStarterMonMilotic[chosenStarterId];
    case 351:
        return sStarterMonCastform[chosenStarterId];
    case 352:
        return sStarterMonKecleon[chosenStarterId];
    case 353:
        return sStarterMonShuppet[chosenStarterId];
    case 354:
        return sStarterMonBanette[chosenStarterId];
    case 355:
        return sStarterMonDuskull[chosenStarterId];
    case 356:
        return sStarterMonDusclops[chosenStarterId];
    case 357:
        return sStarterMonTropius[chosenStarterId];
    case 358:
        return sStarterMonChimecho[chosenStarterId];
    case 359:
        return sStarterMonAbsol[chosenStarterId];
    case 360:
        return sStarterMonWynaut[chosenStarterId];
    case 361:
        return sStarterMonSnorunt[chosenStarterId];
    case 362:
        return sStarterMonGlalie[chosenStarterId];
    case 363:
        return sStarterMonSpheal[chosenStarterId];
    case 364:
        return sStarterMonSealeo[chosenStarterId];
    case 365:
        return sStarterMonWalrein[chosenStarterId];
    case 366:
        return sStarterMonClamperl[chosenStarterId];
    case 367:
        return sStarterMonHuntail[chosenStarterId];
    case 368:
        return sStarterMonGorebyss[chosenStarterId];
    case 369:
        return sStarterMonRelicanth[chosenStarterId];
    case 370:
        return sStarterMonLuvdisc[chosenStarterId];
    case 371:
        return sStarterMonBagon[chosenStarterId];
    case 372:
        return sStarterMonShelgon[chosenStarterId];
    case 373:
        return sStarterMonSalamence[chosenStarterId];
    case 374:
        return sStarterMonBeldum[chosenStarterId];
    case 375:
        return sStarterMonMetang[chosenStarterId];
    case 376:
        return sStarterMonMetagross[chosenStarterId];
    case 377:
        return sStarterMonRegirock[chosenStarterId];
    case 378:
        return sStarterMonRegice[chosenStarterId];
    case 379:
        return sStarterMonRegisteel[chosenStarterId];
    case 380:
        return sStarterMonLatias[chosenStarterId];
    case 381:
        return sStarterMonLatios[chosenStarterId];
    case 382:
        return sStarterMonKyogre[chosenStarterId];
    case 383:
        return sStarterMonGroudon[chosenStarterId];
    case 384:
        return sStarterMonRayquaza[chosenStarterId];
    case 385:
        return sStarterMonJirachi[chosenStarterId];
    case 386:
        return sStarterMonDeoxys[chosenStarterId];
    default:
        return StarterNatDexNum; //sStarterMonBuffer; // This is really all thats needed
    } 
}

static void VblankCB_StarterChoose(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Data for Task_StarterChoose
#define tStarterSelection   data[0]
#define tPkmnSpriteId       data[1]
#define tCircleSpriteId     data[2]

// Data for sSpriteTemplate_Pokeball
#define sTaskId data[0]
#define sBallId data[1]

void CB2_ChooseStarter(void)
{
    u8 taskId;
    u8 spriteId;

    SetVBlankCallback(NULL);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);

    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);

    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);

    LZ77UnCompVram(gBirchBagGrass_Gfx, (void *)VRAM);
    LZ77UnCompVram(gBirchBagTilemap, (void *)(BG_SCREEN_ADDR(6)));
    LZ77UnCompVram(gBirchGrassTilemap, (void *)(BG_SCREEN_ADDR(7)));

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    InitWindows(sWindowTemplates);

    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 0x2A8, BG_PLTT_ID(13));
    ClearScheduledBgCopiesToVram();
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetAllPicSprites();

    LoadPalette(GetOverworldTextboxPalettePtr(), BG_PLTT_ID(14), PLTT_SIZE_4BPP);
    LoadPalette(gBirchBagGrass_Pal, BG_PLTT_ID(0), sizeof(gBirchBagGrass_Pal));
    LoadCompressedSpriteSheet(&sSpriteSheet_PokeballSelect[0]);
    LoadCompressedSpriteSheet(&sSpriteSheet_StarterCircle[0]);
    LoadSpritePalettes(sSpritePalettes_StarterChoose);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);

    EnableInterrupts(DISPSTAT_VBLANK);
    SetVBlankCallback(VblankCB_StarterChoose);
    SetMainCallback2(CB2_StarterChoose);

    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT1_BG2 | BLDCNT_TGT1_BG3 | BLDCNT_TGT1_OBJ | BLDCNT_TGT1_BD | BLDCNT_EFFECT_DARKEN);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 7);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);

    ShowBg(0);
    ShowBg(2);
    ShowBg(3);

    taskId = CreateTask(Task_StarterChoose, 0);
    gTasks[taskId].tStarterSelection = 1;

    // Create hand sprite
    spriteId = CreateSprite(&sSpriteTemplate_Hand, 120, 56, 2);
    gSprites[spriteId].data[0] = taskId;

    // Create three Pokeball sprites
    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[0][0], sPokeballCoords[0][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 0;

    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[1][0], sPokeballCoords[1][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 1;

    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[2][0], sPokeballCoords[2][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 2;

    sStarterLabelWindowId = WINDOW_NONE;
}

static void CB2_StarterChoose(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void Task_StarterChoose(u8 taskId)
{
    CreateStarterPokemonLabel(gTasks[taskId].tStarterSelection);
    DrawStdFrameWithCustomTileAndPalette(0, FALSE, 0x2A8, 0xD);
    AddTextPrinterParameterized(0, FONT_NORMAL, gText_BirchInTrouble, 0, 1, 0, NULL);
    PutWindowTilemap(0);
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static void Task_HandleStarterChooseInput(u8 taskId)
{
    u8 selection = gTasks[taskId].tStarterSelection;

    if (JOY_NEW(A_BUTTON))
    {
        u8 spriteId;

        ClearStarterLabel();

        // Create white circle background
        spriteId = CreateSprite(&sSpriteTemplate_StarterCircle, sPokeballCoords[selection][0], sPokeballCoords[selection][1], 1);
        gTasks[taskId].tCircleSpriteId = spriteId;

        // Create Pokemon sprite
        spriteId = CreatePokemonFrontSprite(GetStarterPokemon(gTasks[taskId].tStarterSelection), sPokeballCoords[selection][0], sPokeballCoords[selection][1]);
        gSprites[spriteId].affineAnims = &sAffineAnims_StarterPokemon;
        gSprites[spriteId].callback = SpriteCB_StarterPokemon;

        gTasks[taskId].tPkmnSpriteId = spriteId;
        gTasks[taskId].func = Task_WaitForStarterSprite;
    }
    else if (JOY_NEW(DPAD_LEFT) && selection > 0)
    {
        gTasks[taskId].tStarterSelection--;
        gTasks[taskId].func = Task_MoveStarterChooseCursor;
    }
    else if (JOY_NEW(DPAD_RIGHT) && selection < STARTER_MON_COUNT - 1)
    {
        gTasks[taskId].tStarterSelection++;
        gTasks[taskId].func = Task_MoveStarterChooseCursor;
    }
}

static void Task_WaitForStarterSprite(u8 taskId)
{
    if (gSprites[gTasks[taskId].tCircleSpriteId].affineAnimEnded &&
        gSprites[gTasks[taskId].tCircleSpriteId].x == STARTER_PKMN_POS_X &&
        gSprites[gTasks[taskId].tCircleSpriteId].y == STARTER_PKMN_POS_Y)
    {
        gTasks[taskId].func = Task_AskConfirmStarter;
    }
}

static void Task_AskConfirmStarter(u8 taskId)
{
    PlayCry_Normal(GetStarterPokemon(gTasks[taskId].tStarterSelection), 0);
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    AddTextPrinterParameterized(0, FONT_NORMAL, gText_ConfirmStarterChoice, 0, 1, 0, NULL);
    ScheduleBgCopyTilemapToVram(0);
    CreateYesNoMenu(&sWindowTemplate_ConfirmStarter, 0x2A8, 0xD, 0);
    gTasks[taskId].func = Task_HandleConfirmStarterInput;
}

static void Task_HandleConfirmStarterInput(u8 taskId)
{
    u8 spriteId;

    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0:  // YES
        // Return the starter choice and exit.
        gSpecialVar_Result = gTasks[taskId].tStarterSelection;
        ResetAllPicSprites();
        SetMainCallback2(gMain.savedCallback);
        break;
    case 1:  // NO
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        spriteId = gTasks[taskId].tPkmnSpriteId;
        FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
        FreeAndDestroyMonPicSprite(spriteId);

        spriteId = gTasks[taskId].tCircleSpriteId;
        FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
        DestroySprite(&gSprites[spriteId]);
        gTasks[taskId].func = Task_DeclineStarter;
        break;
    }
}

static void Task_DeclineStarter(u8 taskId)
{
    gTasks[taskId].func = Task_StarterChoose;
}

static void CreateStarterPokemonLabel(u8 selection)
{
    u8 categoryText[32];
    struct WindowTemplate winTemplate;
    const u8 *speciesName;
    s32 width;
    u8 labelLeft, labelRight, labelTop, labelBottom;

    u16 species = GetStarterPokemon(selection);
    CopyMonCategoryText(SpeciesToNationalPokedexNum(species), categoryText);
    speciesName = gSpeciesNames[species];

    winTemplate = sWindowTemplate_StarterLabel;
    winTemplate.tilemapLeft = sStarterLabelCoords[selection][0];
    winTemplate.tilemapTop = sStarterLabelCoords[selection][1];

    sStarterLabelWindowId = AddWindow(&winTemplate);
    FillWindowPixelBuffer(sStarterLabelWindowId, PIXEL_FILL(0));

    width = GetStringCenterAlignXOffset(FONT_NARROW, categoryText, 0x68);
    AddTextPrinterParameterized3(sStarterLabelWindowId, FONT_NARROW, width, 1, sTextColors, 0, categoryText);

    width = GetStringCenterAlignXOffset(FONT_NORMAL, speciesName, 0x68);
    AddTextPrinterParameterized3(sStarterLabelWindowId, FONT_NORMAL, width, 17, sTextColors, 0, speciesName);

    PutWindowTilemap(sStarterLabelWindowId);
    ScheduleBgCopyTilemapToVram(0);

    labelLeft = sStarterLabelCoords[selection][0] * 8 - 4;
    labelRight = (sStarterLabelCoords[selection][0] + 13) * 8 + 4;
    labelTop = sStarterLabelCoords[selection][1] * 8;
    labelBottom = (sStarterLabelCoords[selection][1] + 4) * 8;
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(labelLeft, labelRight));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(labelTop, labelBottom));
}

static void ClearStarterLabel(void)
{
    FillWindowPixelBuffer(sStarterLabelWindowId, PIXEL_FILL(0));
    ClearWindowTilemap(sStarterLabelWindowId);
    RemoveWindow(sStarterLabelWindowId);
    sStarterLabelWindowId = WINDOW_NONE;
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    ScheduleBgCopyTilemapToVram(0);
}

static void Task_MoveStarterChooseCursor(u8 taskId)
{
    ClearStarterLabel();
    gTasks[taskId].func = Task_CreateStarterLabel;
}

static void Task_CreateStarterLabel(u8 taskId)
{
    CreateStarterPokemonLabel(gTasks[taskId].tStarterSelection);
    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static u8 CreatePokemonFrontSprite(u16 species, u8 x, u8 y)
{
    u8 spriteId;

    spriteId = CreateMonPicSprite_Affine(species, SHINY_ODDS, 0, MON_PIC_AFFINE_FRONT, x, y, 14, TAG_NONE);
    gSprites[spriteId].oam.priority = 0;
    return spriteId;
}

static void SpriteCB_SelectionHand(struct Sprite *sprite)
{
    // Float up and down above selected pokeball
    sprite->x = sCursorCoords[gTasks[sprite->data[0]].tStarterSelection][0];
    sprite->y = sCursorCoords[gTasks[sprite->data[0]].tStarterSelection][1];
    sprite->y2 = Sin(sprite->data[1], 8);
    sprite->data[1] = (u8)(sprite->data[1]) + 4;
}

static void SpriteCB_Pokeball(struct Sprite *sprite)
{
    // Animate pokeball if currently selected
    if (gTasks[sprite->sTaskId].tStarterSelection == sprite->sBallId)
        StartSpriteAnimIfDifferent(sprite, 1);
    else
        StartSpriteAnimIfDifferent(sprite, 0);
}

static void SpriteCB_StarterPokemon(struct Sprite *sprite)
{
    // Move sprite to upper center of screen
    if (sprite->x > STARTER_PKMN_POS_X)
        sprite->x -= 4;
    if (sprite->x < STARTER_PKMN_POS_X)
        sprite->x += 4;
    if (sprite->y > STARTER_PKMN_POS_Y)
        sprite->y -= 2;
    if (sprite->y < STARTER_PKMN_POS_Y)
        sprite->y += 2;
}
