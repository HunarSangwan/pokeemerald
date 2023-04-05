#ifndef GUARD_CONSTANTS_GLOBAL_H
#define GUARD_CONSTANTS_GLOBAL_H
// Invalid Versions show as "----------" in Gen 4 and Gen 5's summary screen.
// In Gens 6 and 7, invalid versions instead show "a distant land" in the summary screen.
// In Gen 4 only, migrated Pokemon with Diamond, Pearl, or Platinum's ID show as "----------".
// Gen 5 and up read Diamond, Pearl, or Platinum's ID as "Sinnoh".
// In Gen 4 and up, migrated Pokemon with HeartGold or SoulSilver's ID show the otherwise unused "Johto" string.
#define VERSION_SAPPHIRE 1
#define VERSION_RUBY 2
#define VERSION_EMERALD 3
#define VERSION_FIRE_RED 4
#define VERSION_LEAF_GREEN 5
#define VERSION_HEART_GOLD 7
#define VERSION_SOUL_SILVER 8
#define VERSION_DIAMOND 10
#define VERSION_PEARL 11
#define VERSION_PLATINUM 12
#define VERSION_GAMECUBE 15

#define LANGUAGE_JAPANESE 1
#define LANGUAGE_ENGLISH  2
#define LANGUAGE_FRENCH   3
#define LANGUAGE_ITALIAN  4
#define LANGUAGE_GERMAN   5
#define LANGUAGE_KOREAN   6 // 6 goes unused but the theory is it was meant to be Korean
#define LANGUAGE_SPANISH  7
#define NUM_LANGUAGES     7

#define GAME_VERSION (VERSION_EMERALD)
#define GAME_LANGUAGE (LANGUAGE_ENGLISH)

// party sizes
#define PARTY_SIZE 6
#define MULTI_PARTY_SIZE (PARTY_SIZE / 2)
#define FRONTIER_PARTY_SIZE         3
#define FRONTIER_DOUBLES_PARTY_SIZE 4
#define FRONTIER_MULTI_PARTY_SIZE   2
#define MAX_FRONTIER_PARTY_SIZE    (max(FRONTIER_PARTY_SIZE,        \
                                    max(FRONTIER_DOUBLES_PARTY_SIZE,\
                                        FRONTIER_MULTI_PARTY_SIZE)))
#define UNION_ROOM_PARTY_SIZE       2

// capacities of various saveblock objects
#define DAYCARE_MON_COUNT 2
#define POKEBLOCKS_COUNT 40
#define OBJECT_EVENTS_COUNT 16
#define MAIL_COUNT (10 + PARTY_SIZE)
#define SECRET_BASES_COUNT 20
#define TV_SHOWS_COUNT 25
#define POKE_NEWS_COUNT 16
#define PC_ITEMS_COUNT 50
#define BAG_ITEMS_COUNT 30
#define BAG_KEYITEMS_COUNT 30
#define BAG_POKEBALLS_COUNT 16
#define BAG_TMHM_COUNT 64
#define BAG_BERRIES_COUNT 46
#define OBJECT_EVENT_TEMPLATES_COUNT 64
#define DECOR_MAX_SECRET_BASE 16
#define DECOR_MAX_PLAYERS_HOUSE 12
#define APPRENTICE_COUNT 4
#define APPRENTICE_MAX_QUESTIONS 9
#define MAX_REMATCH_ENTRIES 100 // only REMATCH_TABLE_ENTRIES (78) are used
#define NUM_CONTEST_WINNERS 13
#define UNION_ROOM_KB_ROW_COUNT 10
#define GIFT_RIBBONS_COUNT 11
#define SAVED_TRENDS_COUNT 5
#define PYRAMID_BAG_ITEMS_COUNT 10

// Number of facilities for Ranking Hall.
// 7 facilities for single mode + tower double mode + tower multi mode.
// Excludes link modes. See RANKING_HALL_* in include/constants/battle_frontier.h
#define HALL_FACILITIES_COUNT 9
// Received via record mixing, 1 for each player other than yourself
#define HALL_RECORDS_COUNT 3

// Battle Frontier level modes.
#define FRONTIER_LVL_50         0
#define FRONTIER_LVL_OPEN       1
#define FRONTIER_LVL_MODE_COUNT 2
#define FRONTIER_LVL_TENT       FRONTIER_LVL_MODE_COUNT // Special usage for indicating Battle Tent

#define TRAINER_ID_LENGTH 4
#define MAX_MON_MOVES 4

#define CONTESTANT_COUNT 4
#define CONTEST_CATEGORY_COOL     0
#define CONTEST_CATEGORY_BEAUTY   1
#define CONTEST_CATEGORY_CUTE     2
#define CONTEST_CATEGORY_SMART    3
#define CONTEST_CATEGORY_TOUGH    4
#define CONTEST_CATEGORIES_COUNT  5

// string lengths
#define ITEM_NAME_LENGTH 14
#define POKEMON_NAME_LENGTH 10
#define PLAYER_NAME_LENGTH 7
#define MAIL_WORDS_COUNT 9
#define EASY_CHAT_BATTLE_WORDS_COUNT 6
#define MOVE_NAME_LENGTH 12
#define NUM_QUESTIONNAIRE_WORDS 4
#define QUIZ_QUESTION_LEN 9
#define WONDER_CARD_TEXT_LENGTH 40
#define WONDER_NEWS_TEXT_LENGTH 40
#define WONDER_CARD_BODY_TEXT_LINES 4
#define WONDER_NEWS_BODY_TEXT_LINES 10
#define TYPE_NAME_LENGTH 6
#define ABILITY_NAME_LENGTH 12
#define TRAINER_NAME_LENGTH 10

#define MAX_STAMP_CARD_STAMPS 7

#define MALE 0
#define FEMALE 1
#define GENDER_COUNT 2

#define BARD_SONG_LENGTH       6
#define NUM_STORYTELLER_TALES  4
#define NUM_TRADER_ITEMS       4
#define GIDDY_MAX_TALES       10
#define GIDDY_MAX_QUESTIONS    8

#define OPTIONS_BUTTON_MODE_NORMAL 0
#define OPTIONS_BUTTON_MODE_LR 1
#define OPTIONS_BUTTON_MODE_L_EQUALS_A 2

#define OPTIONS_TEXT_SPEED_SLOW 0
#define OPTIONS_TEXT_SPEED_MID 1
#define OPTIONS_TEXT_SPEED_FAST 2

#define OPTIONS_SOUND_MONO 0
#define OPTIONS_SOUND_STEREO 1

#define OPTIONS_BATTLE_STYLE_SHIFT 0
#define OPTIONS_BATTLE_STYLE_SET 1

#define DIR_NONE        0
#define DIR_SOUTH       1
#define DIR_NORTH       2
#define DIR_WEST        3
#define DIR_EAST        4
#define DIR_SOUTHWEST   5
#define DIR_SOUTHEAST   6
#define DIR_NORTHWEST   7
#define DIR_NORTHEAST   8

#define CONNECTION_INVALID -1
#define CONNECTION_NONE     0
#define CONNECTION_SOUTH    1
#define CONNECTION_NORTH    2
#define CONNECTION_WEST     3
#define CONNECTION_EAST     4
#define CONNECTION_DIVE     5
#define CONNECTION_EMERGE   6

#define STARTER_BULBASAUR 1
#define STARTER_IVYSAUR 2
#define STARTER_VENUSAUR 3
#define STARTER_CHARMANDER 4
#define STARTER_CHARMELEON 5
#define STARTER_CHARIZARD 6
#define STARTER_SQUIRTLE 7
#define STARTER_WARTORTLE 8
#define STARTER_BLASTOISE 9
#define STARTER_CATERPIE 10
#define STARTER_METAPOD 11
#define STARTER_BUTTERFREE 12
#define STARTER_WEEDLE 13
#define STARTER_KAKUNA 14
#define STARTER_BEEDRILL 15
#define STARTER_PIDGEY 16
#define STARTER_PIDGEOTTO 17
#define STARTER_PIDGEOT 18
#define STARTER_RATTATA 19
#define STARTER_RATICATE 20
#define STARTER_SPEAROW 21
#define STARTER_FEAROW 22
#define STARTER_EKANS 23
#define STARTER_ARBOK 24
#define STARTER_PIKACHU 25
#define STARTER_RAICHU 26
#define STARTER_SANDSHREW 27
#define STARTER_SANDSLASH 28
#define STARTER_NIDORAN_F 29
#define STARTER_NIDORINA 30
#define STARTER_NIDOQUEEN 31
#define STARTER_NIDORAN_M 32
#define STARTER_NIDORINO 33
#define STARTER_NIDOKING 34
#define STARTER_CLEFAIRY 35
#define STARTER_CLEFABLE 36
#define STARTER_VULPIX 37
#define STARTER_NINETALES 38
#define STARTER_JIGGLYPUFF 39
#define STARTER_WIGGLYTUFF 40
#define STARTER_ZUBAT 41
#define STARTER_GOLBAT 42
#define STARTER_ODDISH 43
#define STARTER_GLOOM 44
#define STARTER_VILEPLUME 45
#define STARTER_PARAS 46
#define STARTER_PARASECT 47
#define STARTER_VENONAT 48
#define STARTER_VENOMOTH 49
#define STARTER_DIGLETT 50
#define STARTER_DUGTRIO 51
#define STARTER_MEOWTH 52
#define STARTER_PERSIAN 53
#define STARTER_PSYDUCK 54
#define STARTER_GOLDUCK 55
#define STARTER_MANKEY 56
#define STARTER_PRIMEAPE 57
#define STARTER_GROWLITHE 58
#define STARTER_ARCANINE 59
#define STARTER_POLIWAG 60
#define STARTER_POLIWHIRL 61
#define STARTER_POLIWRATH 62
#define STARTER_ABRA 63
#define STARTER_KADABRA 64
#define STARTER_ALAKAZAM 65
#define STARTER_MACHOP 66
#define STARTER_MACHOKE 67
#define STARTER_MACHAMP 68
#define STARTER_BELLSPROUT 69
#define STARTER_WEEPINBELL 70
#define STARTER_VICTREEBEL 71
#define STARTER_TENTACOOL 72
#define STARTER_TENTACRUEL 73
#define STARTER_GEODUDE 74
#define STARTER_GRAVELER 75
#define STARTER_GOLEM 76
#define STARTER_PONYTA 77
#define STARTER_RAPIDASH 78
#define STARTER_SLOWPOKE 79
#define STARTER_SLOWBRO 80
#define STARTER_MAGNEMITE 81
#define STARTER_MAGNETON 82
#define STARTER_FARFETCHD 83
#define STARTER_DODUO 84
#define STARTER_DODRIO 85
#define STARTER_SEEL 86
#define STARTER_DEWGONG 87
#define STARTER_GRIMER 88
#define STARTER_MUK 89
#define STARTER_SHELLDER 90
#define STARTER_CLOYSTER 91
#define STARTER_GASTLY 92
#define STARTER_HAUNTER 93
#define STARTER_GENGAR 94
#define STARTER_ONIX 95
#define STARTER_DROWZEE 96
#define STARTER_HYPNO 97
#define STARTER_KRABBY 98
#define STARTER_KINGLER 99
#define STARTER_VOLTORB 100
#define STARTER_ELECTRODE 101
#define STARTER_EXEGGCUTE 102
#define STARTER_EXEGGUTOR 103
#define STARTER_CUBONE 104
#define STARTER_MAROWAK 105
#define STARTER_HITMONLEE 106
#define STARTER_HITMONCHAN 107
#define STARTER_LICKITUNG 108
#define STARTER_KOFFING 109
#define STARTER_WEEZING 110
#define STARTER_RHYHORN 111
#define STARTER_RHYDON 112
#define STARTER_CHANSEY 113
#define STARTER_TANGELA 114
#define STARTER_KANGASKHAN 115
#define STARTER_HORSEA 116
#define STARTER_SEADRA 117
#define STARTER_GOLDEEN 118
#define STARTER_SEAKING 119
#define STARTER_STARYU 120
#define STARTER_STARMIE 121
#define STARTER_MR_MIME 122
#define STARTER_SCYTHER 123
#define STARTER_JYNX 124
#define STARTER_ELECTABUZZ 125
#define STARTER_MAGMAR 126
#define STARTER_PINSIR 127
#define STARTER_TAUROS 128
#define STARTER_MAGIKARP 129
#define STARTER_GYARADOS 130
#define STARTER_LAPRAS 131
#define STARTER_DITTO 132
#define STARTER_EEVEE 133
#define STARTER_VAPOREON 134
#define STARTER_JOLTEON 135
#define STARTER_FLAREON 136
#define STARTER_PORYGON 137
#define STARTER_OMANYTE 138
#define STARTER_OMASTAR 139
#define STARTER_KABUTO 140
#define STARTER_KABUTOPS 141
#define STARTER_AERODACTYL 142
#define STARTER_SNORLAX 143
#define STARTER_ARTICUNO 144
#define STARTER_ZAPDOS 145
#define STARTER_MOLTRES 146
#define STARTER_DRATINI 147
#define STARTER_DRAGONAIR 148
#define STARTER_DRAGONITE 149
#define STARTER_MEWTWO 150
#define STARTER_MEW 151
#define STARTER_CHIKORITA 152
#define STARTER_BAYLEEF 153
#define STARTER_MEGANIUM 154
#define STARTER_CYNDAQUIL 155
#define STARTER_QUILAVA 156
#define STARTER_TYPHLOSION 157
#define STARTER_TOTODILE 158
#define STARTER_CROCONAW 159
#define STARTER_FERALIGATR 160
#define STARTER_SENTRET 161
#define STARTER_FURRET 162
#define STARTER_HOOTHOOT 163
#define STARTER_NOCTOWL 164
#define STARTER_LEDYBA 165
#define STARTER_LEDIAN 166
#define STARTER_SPINARAK 167
#define STARTER_ARIADOS 168
#define STARTER_CROBAT 169
#define STARTER_CHINCHOU 170
#define STARTER_LANTURN 171
#define STARTER_PICHU 172
#define STARTER_CLEFFA 173
#define STARTER_IGGLYBUFF 174
#define STARTER_TOGEPI 175
#define STARTER_TOGETIC 176
#define STARTER_NATU 177
#define STARTER_XATU 178
#define STARTER_MAREEP 179
#define STARTER_FLAAFFY 180
#define STARTER_AMPHAROS 181
#define STARTER_BELLOSSOM 182
#define STARTER_MARILL 183
#define STARTER_AZUMARILL 184
#define STARTER_SUDOWOODO 185
#define STARTER_POLITOED 186
#define STARTER_HOPPIP 187
#define STARTER_SKIPLOOM 188
#define STARTER_JUMPLUFF 189
#define STARTER_AIPOM 190
#define STARTER_SUNKERN 191
#define STARTER_SUNFLORA 192
#define STARTER_YANMA 193
#define STARTER_WOOPER 194
#define STARTER_QUAGSIRE 195
#define STARTER_ESPEON 196
#define STARTER_UMBREON 197
#define STARTER_MURKROW 198
#define STARTER_SLOWKING 199
#define STARTER_MISDREAVUS 200
#define STARTER_UNOWN 201
#define STARTER_WOBBUFFET 202
#define STARTER_GIRAFARIG 203
#define STARTER_PINECO 204
#define STARTER_FORRETRESS 205
#define STARTER_DUNSPARCE 206
#define STARTER_GLIGAR 207
#define STARTER_STEELIX 208
#define STARTER_SNUBBULL 209
#define STARTER_GRANBULL 210
#define STARTER_QWILFISH 211
#define STARTER_SCIZOR 212
#define STARTER_SHUCKLE 213
#define STARTER_HERACROSS 214
#define STARTER_SNEASEL 215
#define STARTER_TEDDIURSA 216
#define STARTER_URSARING 217
#define STARTER_SLUGMA 218
#define STARTER_MAGCARGO 219
#define STARTER_SWINUB 220
#define STARTER_PILOSWINE 221
#define STARTER_CORSOLA 222
#define STARTER_REMORAID 223
#define STARTER_OCTILLERY 224
#define STARTER_DELIBIRD 225
#define STARTER_MANTINE 226
#define STARTER_SKARMORY 227
#define STARTER_HOUNDOUR 228
#define STARTER_HOUNDOOM 229
#define STARTER_KINGDRA 230
#define STARTER_PHANPY 231
#define STARTER_DONPHAN 232
#define STARTER_PORYGON2 233
#define STARTER_STANTLER 234
#define STARTER_SMEARGLE 235
#define STARTER_TYROGUE 236
#define STARTER_HITMONTOP 237
#define STARTER_SMOOCHUM 238
#define STARTER_ELEKID 239
#define STARTER_MAGBY 240
#define STARTER_MILTANK 241
#define STARTER_BLISSEY 242
#define STARTER_RAIKOU 243
#define STARTER_ENTEI 244
#define STARTER_SUICUNE 245
#define STARTER_LARVITAR 246
#define STARTER_PUPITAR 247
#define STARTER_TYRANITAR 248
#define STARTER_LUGIA 249
#define STARTER_HO_OH 250
#define STARTER_CELEBI 251
#define STARTER_TREECKO 252
#define STARTER_GROVYLE 253
#define STARTER_SCEPTILE 254
#define STARTER_TORCHIC 255
#define STARTER_COMBUSKEN 256
#define STARTER_BLAZIKEN 257
#define STARTER_MUDKIP 258
#define STARTER_MARSHTOMP 259
#define STARTER_SWAMPERT 260
#define STARTER_POOCHYENA 261
#define STARTER_MIGHTYENA 262
#define STARTER_ZIGZAGOON 263
#define STARTER_LINOONE 264
#define STARTER_WURMPLE 265
#define STARTER_SILCOON 266
#define STARTER_BEAUTIFLY 267
#define STARTER_CASCOON 268
#define STARTER_DUSTOX 269
#define STARTER_LOTAD 270
#define STARTER_LOMBRE 271
#define STARTER_LUDICOLO 272
#define STARTER_SEEDOT 273
#define STARTER_NUZLEAF 274
#define STARTER_SHIFTRY 275
#define STARTER_TAILLOW 276
#define STARTER_SWELLOW 277
#define STARTER_WINGULL 278
#define STARTER_PELIPPER 279
#define STARTER_RALTS 280
#define STARTER_KIRLIA 281
#define STARTER_GARDEVOIR 282
#define STARTER_SURSKIT 283
#define STARTER_MASQUERAIN 284
#define STARTER_SHROOMISH 285
#define STARTER_BRELOOM 286
#define STARTER_SLAKOTH 287
#define STARTER_VIGOROTH 288
#define STARTER_SLAKING 289
#define STARTER_NINCADA 290
#define STARTER_NINJASK 291
#define STARTER_SHEDINJA 292
#define STARTER_WHISMUR 293
#define STARTER_LOUDRED 294
#define STARTER_EXPLOUD 295
#define STARTER_MAKUHITA 296
#define STARTER_HARIYAMA 297
#define STARTER_AZURILL 298
#define STARTER_NOSEPASS 299
#define STARTER_SKITTY 300
#define STARTER_DELCATTY 301
#define STARTER_SABLEYE 302
#define STARTER_MAWILE 303
#define STARTER_ARON 304
#define STARTER_LAIRON 305
#define STARTER_AGGRON 306
#define STARTER_MEDITITE 307
#define STARTER_MEDICHAM 308
#define STARTER_ELECTRIKE 309
#define STARTER_MANECTRIC 310
#define STARTER_PLUSLE 311
#define STARTER_MINUN 312
#define STARTER_VOLBEAT 313
#define STARTER_ILLUMISE 314
#define STARTER_ROSELIA 315
#define STARTER_GULPIN 316
#define STARTER_SWALOT 317
#define STARTER_CARVANHA 318
#define STARTER_SHARPEDO 319
#define STARTER_WAILMER 320
#define STARTER_WAILORD 321
#define STARTER_NUMEL 322
#define STARTER_CAMERUPT 323
#define STARTER_TORKOAL 324
#define STARTER_SPOINK 325
#define STARTER_GRUMPING 326
#define STARTER_SPINDA 327
#define STARTER_TRAPINCH 328
#define STARTER_VIBRAVA 329
#define STARTER_FLYGON 330
#define STARTER_CACNEA 331
#define STARTER_CACTURNE 332
#define STARTER_SWABLU 333
#define STARTER_ALTARIA 334
#define STARTER_ZANGOOSE 335
#define STARTER_SEVIPER 336
#define STARTER_LUNATONE 337
#define STARTER_SOLROCK 338
#define STARTER_BARBOACH 339
#define STARTER_WHISCASH 340
#define STARTER_CORPISH 341
#define STARTER_CRAWDAUNT 342
#define STARTER_BALTOY 343
#define STARTER_CLAYDOL 344
#define STARTER_LILEEP 345
#define STARTER_CRADILY 346
#define STARTER_ANORITH 347
#define STARTER_ARMALDO 348
#define STARTER_FEEBAS 349
#define STARTER_MILOTIC 350
#define STARTER_CASTFORM 351
#define STARTER_KECLEON 352
#define STARTER_SHUPPET 353
#define STARTER_BANETTE 354
#define STARTER_DUSKULL 355
#define STARTER_DUSCLOPS 356
#define STARTER_TROPIUS 357
#define STARTER_CHIMECHO 358
#define STARTER_ABSOL 359
#define STARTER_WYNAUT 360
#define STARTER_SNORUT 361
#define STARTER_GLALIE 362
#define STARTER_SPHEAL 363
#define STARTER_SEALEO 364
#define STARTER_WALREIN 365
#define STARTER_CLAMPERL 366
#define STARTER_HUNTAIL 367
#define STARTER_GOREBYSS 368
#define STARTER_RELICANTH 369
#define STARTER_LUVDISC 370
#define STARTER_BAGON 371
#define STARTER_SHELGON 372
#define STARTER_SALAMENCE 373
#define STARTER_BELDUM 374
#define STARTER_METANG 375
#define STARTER_METAGROSS 376
#define STARTER_REGIROCK 377
#define STARTER_REGICE 378
#define STARTER_REGISTEEL 379
#define STARTER_LATIAS 380
#define STARTER_LATIOS 381
#define STARTER_KYOGRE 382
#define STARTER_GROUDON 383
#define STARTER_RAYQUAZA 384
#define STARTER_JIRACHU 385
#define STARTER_DEOXYS 386

#endif // GUARD_CONSTANTS_GLOBAL_H
