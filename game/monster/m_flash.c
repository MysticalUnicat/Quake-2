/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// m_flash.c

#include "../q_shared.h"

// this file is included in both the game dll and quake2,
// the game needs it to source shot locations, the client
// needs it to position muzzle flashes
vec3_t monster_flash_offset[] = {
    // flash 0 is not used
    0.0, 0.0, 0.0,

    // MZ2_TANK_BLASTER_1				1
    20.7, -18.5, 28.7,
    // MZ2_TANK_BLASTER_2				2
    16.6, -21.5, 30.1,
    // MZ2_TANK_BLASTER_3				3
    11.8, -23.9, 32.1,
    // MZ2_TANK_MACHINEGUN_1			4
    22.9, -0.7, 25.3,
    // MZ2_TANK_MACHINEGUN_2			5
    22.2, 6.2, 22.3,
    // MZ2_TANK_MACHINEGUN_3			6
    19.4, 13.1, 18.6,
    // MZ2_TANK_MACHINEGUN_4			7
    19.4, 18.8, 18.6,
    // MZ2_TANK_MACHINEGUN_5			8
    17.9, 25.0, 18.6,
    // MZ2_TANK_MACHINEGUN_6			9
    14.1, 30.5, 20.6,
    // MZ2_TANK_MACHINEGUN_7			10
    9.3, 35.3, 22.1,
    // MZ2_TANK_MACHINEGUN_8			11
    4.7, 38.4, 22.1,
    // MZ2_TANK_MACHINEGUN_9			12
    -1.1, 40.4, 24.1,
    // MZ2_TANK_MACHINEGUN_10			13
    -6.5, 41.2, 24.1,
    // MZ2_TANK_MACHINEGUN_11			14
    3.2, 40.1, 24.7,
    // MZ2_TANK_MACHINEGUN_12			15
    11.7, 36.7, 26.0,
    // MZ2_TANK_MACHINEGUN_13			16
    18.9, 31.3, 26.0,
    // MZ2_TANK_MACHINEGUN_14			17
    24.4, 24.4, 26.4,
    // MZ2_TANK_MACHINEGUN_15			18
    27.1, 17.1, 27.2,
    // MZ2_TANK_MACHINEGUN_16			19
    28.5, 9.1, 28.0,
    // MZ2_TANK_MACHINEGUN_17			20
    27.1, 2.2, 28.0,
    // MZ2_TANK_MACHINEGUN_18			21
    24.9, -2.8, 28.0,
    // MZ2_TANK_MACHINEGUN_19			22
    21.6, -7.0, 26.4,
    // MZ2_TANK_ROCKET_1				23
    6.2, 29.1, 49.1,
    // MZ2_TANK_ROCKET_2				24
    6.9, 23.8, 49.1,
    // MZ2_TANK_ROCKET_3				25
    8.3, 17.8, 49.5,

    // MZ2_INFANTRY_MACHINEGUN_1		26
    26.6, 7.1, 13.1,
    // MZ2_INFANTRY_MACHINEGUN_2		27
    18.2, 7.5, 15.4,
    // MZ2_INFANTRY_MACHINEGUN_3		28
    17.2, 10.3, 17.9,
    // MZ2_INFANTRY_MACHINEGUN_4		29
    17.0, 12.8, 20.1,
    // MZ2_INFANTRY_MACHINEGUN_5		30
    15.1, 14.1, 21.8,
    // MZ2_INFANTRY_MACHINEGUN_6		31
    11.8, 17.2, 23.1,
    // MZ2_INFANTRY_MACHINEGUN_7		32
    11.4, 20.2, 21.0,
    // MZ2_INFANTRY_MACHINEGUN_8		33
    9.0, 23.0, 18.9,
    // MZ2_INFANTRY_MACHINEGUN_9		34
    13.9, 18.6, 17.7,
    // MZ2_INFANTRY_MACHINEGUN_10		35
    15.4, 15.6, 15.8,
    // MZ2_INFANTRY_MACHINEGUN_11		36
    10.2, 15.2, 25.1,
    // MZ2_INFANTRY_MACHINEGUN_12		37
    -1.9, 15.1, 28.2,
    // MZ2_INFANTRY_MACHINEGUN_13		38
    -12.4, 13.0, 20.2,

    // MZ2_SOLDIER_BLASTER_1			39
    10.6 * 1.2, 7.7 * 1.2, 7.8 * 1.2,
    // MZ2_SOLDIER_BLASTER_2			40
    21.1 * 1.2, 3.6 * 1.2, 19.0 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_1			41
    10.6 * 1.2, 7.7 * 1.2, 7.8 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_2			42
    21.1 * 1.2, 3.6 * 1.2, 19.0 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_1			43
    10.6 * 1.2, 7.7 * 1.2, 7.8 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_2			44
    21.1 * 1.2, 3.6 * 1.2, 19.0 * 1.2,

    // MZ2_GUNNER_MACHINEGUN_1			45
    30.1 * 1.15, 3.9 * 1.15, 19.6 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_2			46
    29.1 * 1.15, 2.5 * 1.15, 20.7 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_3			47
    28.2 * 1.15, 2.5 * 1.15, 22.2 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_4			48
    28.2 * 1.15, 3.6 * 1.15, 22.0 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_5			49
    26.9 * 1.15, 2.0 * 1.15, 23.4 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_6			50
    26.5 * 1.15, 0.6 * 1.15, 20.8 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_7			51
    26.9 * 1.15, 0.5 * 1.15, 21.5 * 1.15,
    // MZ2_GUNNER_MACHINEGUN_8			52
    29.0 * 1.15, 2.4 * 1.15, 19.5 * 1.15,
    // MZ2_GUNNER_GRENADE_1				53
    4.6 * 1.15, -16.8 * 1.15, 7.3 * 1.15,
    // MZ2_GUNNER_GRENADE_2				54
    4.6 * 1.15, -16.8 * 1.15, 7.3 * 1.15,
    // MZ2_GUNNER_GRENADE_3				55
    4.6 * 1.15, -16.8 * 1.15, 7.3 * 1.15,
    // MZ2_GUNNER_GRENADE_4				56
    4.6 * 1.15, -16.8 * 1.15, 7.3 * 1.15,

    // MZ2_CHICK_ROCKET_1				57
    //	-24.8, -9.0, 39.0,
    24.8, -9.0, 39.0, // PGM - this was incorrect in Q2

    // MZ2_FLYER_BLASTER_1				58
    12.1, 13.4, -14.5,
    // MZ2_FLYER_BLASTER_2				59
    12.1, -7.4, -14.5,

    // MZ2_MEDIC_BLASTER_1				60
    12.1, 5.4, 16.5,

    // MZ2_GLADIATOR_RAILGUN_1			61
    30.0, 18.0, 28.0,

    // MZ2_HOVER_BLASTER_1				62
    32.5, -0.8, 10.0,

    // MZ2_ACTOR_MACHINEGUN_1			63
    18.4, 7.4, 9.6,

    // MZ2_SUPERTANK_MACHINEGUN_1		64
    30.0, 30.0, 88.5,
    // MZ2_SUPERTANK_MACHINEGUN_2		65
    30.0, 30.0, 88.5,
    // MZ2_SUPERTANK_MACHINEGUN_3		66
    30.0, 30.0, 88.5,
    // MZ2_SUPERTANK_MACHINEGUN_4		67
    30.0, 30.0, 88.5,
    // MZ2_SUPERTANK_MACHINEGUN_5		68
    30.0, 30.0, 88.5,
    // MZ2_SUPERTANK_MACHINEGUN_6		69
    30.0, 30.0, 88.5,
    // MZ2_SUPERTANK_ROCKET_1			70
    16.0, -22.5, 91.2,
    // MZ2_SUPERTANK_ROCKET_2			71
    16.0, -33.4, 86.7,
    // MZ2_SUPERTANK_ROCKET_3			72
    16.0, -42.8, 83.3,

    // --- Start Xian Stuff ---
    // MZ2_BOSS2_MACHINEGUN_L1			73
    32, -40, 70,
    // MZ2_BOSS2_MACHINEGUN_L2			74
    32, -40, 70,
    // MZ2_BOSS2_MACHINEGUN_L3			75
    32, -40, 70,
    // MZ2_BOSS2_MACHINEGUN_L4			76
    32, -40, 70,
    // MZ2_BOSS2_MACHINEGUN_L5			77
    32, -40, 70,
    // --- End Xian Stuff

    // MZ2_BOSS2_ROCKET_1				78
    22.0, 16.0, 10.0,
    // MZ2_BOSS2_ROCKET_2				79
    22.0, 8.0, 10.0,
    // MZ2_BOSS2_ROCKET_3				80
    22.0, -8.0, 10.0,
    // MZ2_BOSS2_ROCKET_4				81
    22.0, -16.0, 10.0,

    // MZ2_FLOAT_BLASTER_1				82
    32.5, -0.8, 10,

    // MZ2_SOLDIER_BLASTER_3			83
    20.8 * 1.2, 10.1 * 1.2, -2.7 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_3			84
    20.8 * 1.2, 10.1 * 1.2, -2.7 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_3			85
    20.8 * 1.2, 10.1 * 1.2, -2.7 * 1.2,
    // MZ2_SOLDIER_BLASTER_4			86
    7.6 * 1.2, 9.3 * 1.2, 0.8 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_4			87
    7.6 * 1.2, 9.3 * 1.2, 0.8 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_4			88
    7.6 * 1.2, 9.3 * 1.2, 0.8 * 1.2,
    // MZ2_SOLDIER_BLASTER_5			89
    30.5 * 1.2, 9.9 * 1.2, -18.7 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_5			90
    30.5 * 1.2, 9.9 * 1.2, -18.7 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_5			91
    30.5 * 1.2, 9.9 * 1.2, -18.7 * 1.2,
    // MZ2_SOLDIER_BLASTER_6			92
    27.6 * 1.2, 3.4 * 1.2, -10.4 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_6			93
    27.6 * 1.2, 3.4 * 1.2, -10.4 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_6			94
    27.6 * 1.2, 3.4 * 1.2, -10.4 * 1.2,
    // MZ2_SOLDIER_BLASTER_7			95
    28.9 * 1.2, 4.6 * 1.2, -8.1 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_7			96
    28.9 * 1.2, 4.6 * 1.2, -8.1 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_7			97
    28.9 * 1.2, 4.6 * 1.2, -8.1 * 1.2,
    // MZ2_SOLDIER_BLASTER_8			98
    //	34.5 * 1.2, 9.6 * 1.2, 6.1 * 1.2,
    31.5 * 1.2, 9.6 * 1.2, 10.1 * 1.2,
    // MZ2_SOLDIER_SHOTGUN_8			99
    34.5 * 1.2, 9.6 * 1.2, 6.1 * 1.2,
    // MZ2_SOLDIER_MACHINEGUN_8			100
    34.5 * 1.2, 9.6 * 1.2, 6.1 * 1.2,

    // --- Xian shit below ---
    // MZ2_MAKRON_BFG					101
    17, -19.5, 62.9,
    // MZ2_MAKRON_BLASTER_1				102
    -3.6, -24.1, 59.5,
    // MZ2_MAKRON_BLASTER_2				103
    -1.6, -19.3, 59.5,
    // MZ2_MAKRON_BLASTER_3				104
    -0.1, -14.4, 59.5,
    // MZ2_MAKRON_BLASTER_4				105
    2.0, -7.6, 59.5,
    // MZ2_MAKRON_BLASTER_5				106
    3.4, 1.3, 59.5,
    // MZ2_MAKRON_BLASTER_6				107
    3.7, 11.1, 59.5,
    // MZ2_MAKRON_BLASTER_7				108
    -0.3, 22.3, 59.5,
    // MZ2_MAKRON_BLASTER_8				109
    -6, 33, 59.5,
    // MZ2_MAKRON_BLASTER_9				110
    -9.3, 36.4, 59.5,
    // MZ2_MAKRON_BLASTER_10			111
    -7, 35, 59.5,
    // MZ2_MAKRON_BLASTER_11			112
    -2.1, 29, 59.5,
    // MZ2_MAKRON_BLASTER_12			113
    3.9, 17.3, 59.5,
    // MZ2_MAKRON_BLASTER_13			114
    6.1, 5.8, 59.5,
    // MZ2_MAKRON_BLASTER_14			115
    5.9, -4.4, 59.5,
    // MZ2_MAKRON_BLASTER_15			116
    4.2, -14.1, 59.5,
    // MZ2_MAKRON_BLASTER_16			117
    2.4, -18.8, 59.5,
    // MZ2_MAKRON_BLASTER_17			118
    -1.8, -25.5, 59.5,
    // MZ2_MAKRON_RAILGUN_1				119
    -17.3, 7.8, 72.4,

    // MZ2_JORG_MACHINEGUN_L1			120
    78.5, -47.1, 96,
    // MZ2_JORG_MACHINEGUN_L2			121
    78.5, -47.1, 96,
    // MZ2_JORG_MACHINEGUN_L3			122
    78.5, -47.1, 96,
    // MZ2_JORG_MACHINEGUN_L4			123
    78.5, -47.1, 96,
    // MZ2_JORG_MACHINEGUN_L5			124
    78.5, -47.1, 96,
    // MZ2_JORG_MACHINEGUN_L6			125
    78.5, -47.1, 96,
    // MZ2_JORG_MACHINEGUN_R1			126
    78.5, 46.7, 96,
    // MZ2_JORG_MACHINEGUN_R2			127
    78.5, 46.7, 96,
    // MZ2_JORG_MACHINEGUN_R3			128
    78.5, 46.7, 96,
    // MZ2_JORG_MACHINEGUN_R4			129
    78.5, 46.7, 96,
    // MZ2_JORG_MACHINEGUN_R5			130
    78.5, 46.7, 96,
    // MZ2_JORG_MACHINEGUN_R6			131
    78.5, 46.7, 96,
    // MZ2_JORG_BFG_1					132
    6.3, -9, 111.2,

    // MZ2_BOSS2_MACHINEGUN_R1			73
    32, 40, 70,
    // MZ2_BOSS2_MACHINEGUN_R2			74
    32, 40, 70,
    // MZ2_BOSS2_MACHINEGUN_R3			75
    32, 40, 70,
    // MZ2_BOSS2_MACHINEGUN_R4			76
    32, 40, 70,
    // MZ2_BOSS2_MACHINEGUN_R5			77
    32, 40, 70,

    // --- End Xian Shit ---

    // ROGUE
    // note that the above really ends at 137
    // carrier machineguns
    // MZ2_CARRIER_MACHINEGUN_L1
    56, -32, 32,
    // MZ2_CARRIER_MACHINEGUN_R1
    56, 32, 32,
    // MZ2_CARRIER_GRENADE
    42, 24, 50,
    // MZ2_TURRET_MACHINEGUN			141
    16, 0, 0,
    // MZ2_TURRET_ROCKET				142
    16, 0, 0,
    // MZ2_TURRET_BLASTER				143
    16, 0, 0,
    // MZ2_STALKER_BLASTER				144
    24, 0, 6,
    // MZ2_DAEDALUS_BLASTER				145
    32.5, -0.8, 10.0,
    // MZ2_MEDIC_BLASTER_2				146
    12.1, 5.4, 16.5,
    // MZ2_CARRIER_RAILGUN				147
    32, 0, 6,
    // MZ2_WIDOW_DISRUPTOR				148
    57.72, 14.50, 88.81,
    // MZ2_WIDOW_BLASTER				149
    56, 32, 32,
    // MZ2_WIDOW_RAIL					150
    62, -20, 84,
    // MZ2_WIDOW_PLASMABEAM				151		// PMM - not used!
    32, 0, 6,
    // MZ2_CARRIER_MACHINEGUN_L2		152
    61, -32, 12,
    // MZ2_CARRIER_MACHINEGUN_R2		153
    61, 32, 12,
    // MZ2_WIDOW_RAIL_LEFT				154
    17, -62, 91,
    // MZ2_WIDOW_RAIL_RIGHT				155
    68, 12, 86,
    // MZ2_WIDOW_BLASTER_SWEEP1			156			pmm - the sweeps need to be in sequential order
    47.5, 56, 89,
    // MZ2_WIDOW_BLASTER_SWEEP2			157
    54, 52, 91,
    // MZ2_WIDOW_BLASTER_SWEEP3			158
    58, 40, 91,
    // MZ2_WIDOW_BLASTER_SWEEP4			159
    68, 30, 88,
    // MZ2_WIDOW_BLASTER_SWEEP5			160
    74, 20, 88,
    // MZ2_WIDOW_BLASTER_SWEEP6			161
    73, 11, 87,
    // MZ2_WIDOW_BLASTER_SWEEP7			162
    73, 3, 87,
    // MZ2_WIDOW_BLASTER_SWEEP8			163
    70, -12, 87,
    // MZ2_WIDOW_BLASTER_SWEEP9			164
    67, -20, 90,
    // MZ2_WIDOW_BLASTER_100			165
    -20, 76, 90,
    // MZ2_WIDOW_BLASTER_90				166
    -8, 74, 90,
    // MZ2_WIDOW_BLASTER_80				167
    0, 72, 90,
    // MZ2_WIDOW_BLASTER_70				168		d06
    10, 71, 89,
    // MZ2_WIDOW_BLASTER_60				169		d07
    23, 70, 87,
    // MZ2_WIDOW_BLASTER_50				170		d08
    32, 64, 85,
    // MZ2_WIDOW_BLASTER_40				171
    40, 58, 84,
    // MZ2_WIDOW_BLASTER_30				172		d10
    48, 50, 83,
    // MZ2_WIDOW_BLASTER_20				173
    54, 42, 82,
    // MZ2_WIDOW_BLASTER_10				174		d12
    56, 34, 82,
    // MZ2_WIDOW_BLASTER_0				175
    58, 26, 82,
    // MZ2_WIDOW_BLASTER_10L			176		d14
    60, 16, 82,
    // MZ2_WIDOW_BLASTER_20L			177
    59, 6, 81,
    // MZ2_WIDOW_BLASTER_30L			178		d16
    58, -2, 80,
    // MZ2_WIDOW_BLASTER_40L			179
    57, -10, 79,
    // MZ2_WIDOW_BLASTER_50L			180		d18
    54, -18, 78,
    // MZ2_WIDOW_BLASTER_60L			181
    42, -32, 80,
    // MZ2_WIDOW_BLASTER_70L			182		d20
    36, -40, 78,
    // MZ2_WIDOW_RUN_1					183
    68.4, 10.88, 82.08,
    // MZ2_WIDOW_RUN_2					184
    68.51, 8.64, 85.14,
    // MZ2_WIDOW_RUN_3					185
    68.66, 6.38, 88.78,
    // MZ2_WIDOW_RUN_4					186
    68.73, 5.1, 84.47,
    // MZ2_WIDOW_RUN_5					187
    68.82, 4.79, 80.52,
    // MZ2_WIDOW_RUN_6					188
    68.77, 6.11, 85.37,
    // MZ2_WIDOW_RUN_7					189
    68.67, 7.99, 90.24,
    // MZ2_WIDOW_RUN_8					190
    68.55, 9.54, 87.36,
    // MZ2_CARRIER_ROCKET_1				191
    0, 0, -5,
    // MZ2_CARRIER_ROCKET_2				192
    0, 0, -5,
    // MZ2_CARRIER_ROCKET_3				193
    0, 0, -5,
    // MZ2_CARRIER_ROCKET_4				194
    0, 0, -5,
    // MZ2_WIDOW2_BEAMER_1				195
    //	72.13, -17.63, 93.77,
    69.00, -17.63, 93.77,
    // MZ2_WIDOW2_BEAMER_2				196
    //	71.46, -17.08, 89.82,
    69.00, -17.08, 89.82,
    // MZ2_WIDOW2_BEAMER_3				197
    //	71.47, -18.40, 90.70,
    69.00, -18.40, 90.70,
    // MZ2_WIDOW2_BEAMER_4				198
    //	71.96, -18.34, 94.32,
    69.00, -18.34, 94.32,
    // MZ2_WIDOW2_BEAMER_5				199
    //	72.25, -18.30, 97.98,
    69.00, -18.30, 97.98,
    // MZ2_WIDOW2_BEAM_SWEEP_1			200
    45.04, -59.02, 92.24,
    // MZ2_WIDOW2_BEAM_SWEEP_2			201
    50.68, -54.70, 91.96,
    // MZ2_WIDOW2_BEAM_SWEEP_3			202
    56.57, -47.72, 91.65,
    // MZ2_WIDOW2_BEAM_SWEEP_4			203
    61.75, -38.75, 91.38,
    // MZ2_WIDOW2_BEAM_SWEEP_5			204
    65.55, -28.76, 91.24,
    // MZ2_WIDOW2_BEAM_SWEEP_6			205
    67.79, -18.90, 91.22,
    // MZ2_WIDOW2_BEAM_SWEEP_7			206
    68.60, -9.52, 91.23,
    // MZ2_WIDOW2_BEAM_SWEEP_8			207
    68.08, 0.18, 91.32,
    // MZ2_WIDOW2_BEAM_SWEEP_9			208
    66.14, 9.79, 91.44,
    // MZ2_WIDOW2_BEAM_SWEEP_10			209
    62.77, 18.91, 91.65,
    // MZ2_WIDOW2_BEAM_SWEEP_11			210
    58.29, 27.11, 92.00,

    // end of table
    0.0, 0.0, 0.0};
