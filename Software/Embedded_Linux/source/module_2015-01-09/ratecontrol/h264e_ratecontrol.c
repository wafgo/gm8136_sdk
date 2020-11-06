#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>

#include "h264e_ratecontrol.h"
#ifdef VG_INTERFACE
#include "log.h"
#endif


//#define ABR_INIT_QP (param->rc_mode == RC_CRF ? param->rf_constant : 24 )
#define ABR_INIT_QP 24

#define FIX_POINT
//#define DUMP_RC_LOG
/*
#ifdef VG_INTERFACE
#define DEBUG_M(level, fmt, args...) { \
    if (log_level == LOG_DIRECT) \
        printk(fmt, ## args); \
    else if (log_level >= level) \
        printm(MODULE_NAME, fmt, ## args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (log_level >= level) \
        printk(fmt, ## args); }
#endif
*/

#ifdef FIX_POINT


#define SCALE_FACTOR    12
#define SCALE_BASE      (1<<SCALE_FACTOR)
#define HALF_SCALE_BASE (1<<(SCALE_FACTOR-1))

#define TABLE_BIT       4
#define TABLE_SIZE      (1<<TABLE_BIT)

static const unsigned int qp2qscale_tbl[52][TABLE_SIZE] = 
{
//	  0.0000  0.0625  0.1250  0.1875  0.2500  0.3125  0.3750  0.4375  0.5000  0.5625  0.6250  0.6875  0.7500  0.8125  0.8750  0.9375
//  ================================================================================================================================
	{    870,    877,    883,    889,    896,    902,    909,    916,    922,    929,    936,    942,    949,    956,    963,    970	},	// qp = 0
	{    977,    984,    991,    998,   1006,   1013,   1020,   1028,   1035,   1043,   1050,   1058,   1065,   1073,   1081,   1089	},	// qp = 1
	{   1097,   1105,   1113,   1121,   1129,   1137,   1145,   1153,   1162,   1170,   1179,   1187,   1196,   1205,   1213,   1222	},	// qp = 2
	{   1231,   1240,   1249,   1258,   1267,   1276,   1285,   1295,   1304,   1314,   1323,   1333,   1342,   1352,   1362,   1372	},	// qp = 3
	{   1382,   1392,   1402,   1412,   1422,   1432,   1443,   1453,   1464,   1474,   1485,   1496,   1507,   1518,   1529,   1540	},	// qp = 4
	{   1551,   1562,   1573,   1585,   1596,   1608,   1620,   1631,   1643,   1655,   1667,   1679,   1691,   1703,   1716,   1728	},	// qp = 5
	{   1741,   1753,   1766,   1779,   1792,   1805,   1818,   1831,   1844,   1858,   1871,   1885,   1898,   1912,   1926,   1940	},	// qp = 6
	{   1954,   1968,   1982,   1997,   2011,   2026,   2040,   2055,   2070,   2085,   2100,   2116,   2131,   2146,   2162,   2177	},	// qp = 7
	{   2193,   2209,   2225,   2241,   2258,   2274,   2290,   2307,   2324,   2341,   2357,   2375,   2392,   2409,   2427,   2444	},	// qp = 8
	{   2462,   2480,   2498,   2516,   2534,   2552,   2571,   2589,   2608,   2627,   2646,   2665,   2685,   2704,   2724,   2743	},	// qp = 9
	{   2763,   2783,   2804,   2824,   2844,   2865,   2886,   2907,   2928,   2949,   2970,   2992,   3013,   3035,   3057,   3079	},	// qp = 10
	{   3102,   3124,   3147,   3170,   3193,   3216,   3239,   3263,   3286,   3310,   3334,   3358,   3382,   3407,   3432,   3457	},	// qp = 11
	{   3482,   3507,   3532,   3558,   3584,   3610,   3636,   3662,   3689,   3715,   3742,   3769,   3797,   3824,   3852,   3880	},	// qp = 12
	{   3908,   3936,   3965,   3994,   4022,   4052,   4081,   4111,   4140,   4170,   4201,   4231,   4262,   4293,   4324,   4355	},	// qp = 13
	{   4387,   4418,   4450,   4483,   4515,   4548,   4581,   4614,   4647,   4681,   4715,   4749,   4784,   4818,   4853,   4888	},	// qp = 14
	{   4924,   4959,   4995,   5032,   5068,   5105,   5142,   5179,   5217,   5254,   5292,   5331,   5369,   5408,   5447,   5487	},	// qp = 15
	{   5527,   5567,   5607,   5648,   5689,   5730,   5771,   5813,   5855,   5898,   5940,   5984,   6027,   6071,   6115,   6159	},	// qp = 16
	{   6204,   6248,   6294,   6339,   6385,   6432,   6478,   6525,   6572,   6620,   6668,   6716,   6765,   6814,   6863,   6913	},	// qp = 17
	{   6963,   7014,   7064,   7116,   7167,   7219,   7271,   7324,   7377,   7431,   7485,   7539,   7593,   7648,   7704,   7760	},	// qp = 18
	{   7816,   7873,   7930,   7987,   8045,   8103,   8162,   8221,   8281,   8341,   8401,   8462,   8523,   8585,   8647,   8710	},	// qp = 19
	{   8773,   8837,   8901,   8965,   9030,   9096,   9161,   9228,   9295,   9362,   9430,   9498,   9567,   9636,   9706,   9777	},	// qp = 20
	{   9847,   9919,   9991,  10063,  10136,  10209,  10283,  10358,  10433,  10509,  10585,  10661,  10739,  10817,  10895,  10974	},	// qp = 21
	{  11053,  11133,  11214,  11295,  11377,  11460,  11543,  11626,  11711,  11796,  11881,  11967,  12054,  12141,  12229,  12318	},	// qp = 22
	{  12407,  12497,  12587,  12679,  12771,  12863,  12956,  13050,  13145,  13240,  13336,  13433,  13530,  13628,  13727,  13826	},	// qp = 23
	{  13926,  14027,  14129,  14231,  14334,  14438,  14543,  14648,  14755,  14861,  14969,  15078,  15187,  15297,  15408,  15519	},	// qp = 24
	{  15632,  15745,  15859,  15974,  16090,  16206,  16324,  16442,  16561,  16681,  16802,  16924,  17047,  17170,  17295,  17420	},	// qp = 25
	{  17546,  17673,  17801,  17930,  18060,  18191,  18323,  18456,  18590,  18724,  18860,  18997,  19134,  19273,  19413,  19553	},	// qp = 26
	{  19695,  19838,  19981,  20126,  20272,  20419,  20567,  20716,  20866,  21017,  21170,  21323,  21477,  21633,  21790,  21948	},	// qp = 27
	{  22107,  22267,  22428,  22591,  22755,  22919,  23086,  23253,  23421,  23591,  23762,  23934,  24108,  24282,  24458,  24636	},	// qp = 28
	{  24814,  24994,  25175,  25357,  25541,  25726,  25913,  26100,  26290,  26480,  26672,  26865,  27060,  27256,  27453,  27652	},	// qp = 29
	{  27853,  28055,  28258,  28463,  28669,  28877,  29086,  29297,  29509,  29723,  29938,  30155,  30374,  30594,  30815,  31039	},	// qp = 30
	{  31264,  31490,  31718,  31948,  32180,  32413,  32648,  32884,  33123,  33363,  33605,  33848,  34093,  34340,  34589,  34840	},	// qp = 31
	{  35092,  35347,  35603,  35861,  36121,  36382,  36646,  36912,  37179,  37448,  37720,  37993,  38268,  38546,  38825,  39106	},	// qp = 32
	{  39390,  39675,  39963,  40252,  40544,  40838,  41134,  41432,  41732,  42034,  42339,  42646,  42955,  43266,  43580,  43895	},	// qp = 33
	{  44214,  44534,  44857,  45182,  45509,  45839,  46171,  46506,  46843,  47182,  47524,  47868,  48215,  48565,  48917,  49271	},	// qp = 34
	{  49628,  49988,  50350,  50715,  51082,  51452,  51825,  52201,  52579,  52960,  53344,  53730,  54120,  54512,  54907,  55305	},	// qp = 35
	{  55706,  56109,  56516,  56925,  57338,  57753,  58172,  58593,  59018,  59446,  59876,  60310,  60747,  61188,  61631,  62078	},	// qp = 36
	{  62527,  62981,  63437,  63897,  64360,  64826,  65296,  65769,  66245,  66726,  67209,  67696,  68187,  68681,  69178,  69680	},	// qp = 37
	{  70185,  70693,  71206,  71722,  72241,  72765,  73292,  73823,  74358,  74897,  75440,  75986,  76537,  77092,  77650,  78213	},	// qp = 38
	{  78780,  79350,  79925,  80505,  81088,  81676,  82267,  82864,  83464,  84069,  84678,  85292,  85910,  86532,  87159,  87791	},	// qp = 39
	{  88427,  89068,  89713,  90363,  91018,  91678,  92342,  93011,  93685,  94364,  95048,  95737,  96430,  97129,  97833,  98542	},	// qp = 40
	{  99256,  99975, 100700, 101430, 102165, 102905, 103651, 104402, 105158, 105920, 106688, 107461, 108240, 109024, 109814, 110610	},	// qp = 41
	{ 111411, 112219, 113032, 113851, 114676, 115507, 116344, 117187, 118036, 118891, 119753, 120621, 121495, 122375, 123262, 124155	},	// qp = 42
	{ 125055, 125961, 126874, 127793, 128719, 129652, 130591, 131538, 132491, 133451, 134418, 135392, 136373, 137361, 138357, 139359	},	// qp = 43
	{ 140369, 141386, 142411, 143443, 144482, 145529, 146584, 147646, 148716, 149794, 150879, 151973, 153074, 154183, 155300, 156426	},	// qp = 44
	{ 157559, 158701, 159851, 161009, 162176, 163351, 164535, 165727, 166928, 168138, 169356, 170583, 171820, 173065, 174319, 175582	},	// qp = 45
	{ 176854, 178136, 179427, 180727, 182036, 183356, 184684, 186023, 187371, 188728, 190096, 191473, 192861, 194258, 195666, 197084	},	// qp = 46
	{ 198512, 199951, 201400, 202859, 204329, 205810, 207301, 208803, 210316, 211840, 213375, 214922, 216479, 218048, 219628, 221219	},	// qp = 47
	{ 222822, 224437, 226063, 227702, 229352, 231014, 232688, 234374, 236072, 237783, 239506, 241241, 242990, 244750, 246524, 248310	},	// qp = 48
	{ 250110, 251922, 253748, 255586, 257438, 259304, 261183, 263076, 264982, 266902, 268836, 270784, 272747, 274723, 276714, 278719	},	// qp = 49
	{ 280739, 282773, 284822, 286886, 288965, 291059, 293168, 295292, 297432, 299588, 301758, 303945, 306148, 308366, 310601, 312851	},	// qp = 50
	{ 315118, 317402, 319702, 322019, 324352, 326703, 329070, 331455, 333856, 336276, 338712, 341167, 343639, 346129, 348637, 351164	},	// qp = 51

};

static const unsigned int qscale2qp_tbl[52][TABLE_SIZE] =
{
//	  0.0313  0.0938  0.1563  0.2188  0.2813  0.3438  0.4063  0.4688  0.5313  0.5938  0.6563  0.7188  0.7813  0.8438  0.9063  0.9688
//  =====================================================================================================================================
	{    874,    880,    886,    893,    899,    906,    912,    919,    925,    932,    939,    946,    953,    960,    966,    973	},	// qp = 0
	{    981,    988,    995,   1002,   1009,   1017,   1024,   1031,   1039,   1046,   1054,   1062,   1069,   1077,   1085,   1093	},	// qp = 1
	{   1101,   1109,   1117,   1125,   1133,   1141,   1149,   1158,   1166,   1174,   1183,   1192,   1200,   1209,   1218,   1226	},	// qp = 2
	{   1235,   1244,   1253,   1262,   1272,   1281,   1290,   1299,   1309,   1318,   1328,   1338,   1347,   1357,   1367,   1377	},	// qp = 3
	{   1387,   1397,   1407,   1417,   1427,   1438,   1448,   1459,   1469,   1480,   1490,   1501,   1512,   1523,   1534,   1545	},	// qp = 4
	{   1556,   1568,   1579,   1591,   1602,   1614,   1625,   1637,   1649,   1661,   1673,   1685,   1697,   1710,   1722,   1735	},	// qp = 5
	{   1747,   1760,   1773,   1785,   1798,   1811,   1824,   1838,   1851,   1864,   1878,   1892,   1905,   1919,   1933,   1947	},	// qp = 6
	{   1961,   1975,   1990,   2004,   2019,   2033,   2048,   2063,   2078,   2093,   2108,   2123,   2139,   2154,   2170,   2185	},	// qp = 7
	{   2201,   2217,   2233,   2249,   2266,   2282,   2299,   2315,   2332,   2349,   2366,   2383,   2400,   2418,   2435,   2453	},	// qp = 8
	{   2471,   2489,   2507,   2525,   2543,   2562,   2580,   2599,   2618,   2637,   2656,   2675,   2694,   2714,   2734,   2753	},	// qp = 9
	{   2773,   2793,   2814,   2834,   2855,   2875,   2896,   2917,   2938,   2960,   2981,   3003,   3024,   3046,   3068,   3091	},	// qp = 10
	{   3113,   3136,   3158,   3181,   3204,   3227,   3251,   3274,   3298,   3322,   3346,   3370,   3395,   3419,   3444,   3469	},	// qp = 11
	{   3494,   3520,   3545,   3571,   3597,   3623,   3649,   3675,   3702,   3729,   3756,   3783,   3810,   3838,   3866,   3894	},	// qp = 12
	{   3922,   3951,   3979,   4008,   4037,   4066,   4096,   4125,   4155,   4185,   4216,   4246,   4277,   4308,   4339,   4371	},	// qp = 13
	{   4402,   4434,   4466,   4499,   4531,   4564,   4597,   4631,   4664,   4698,   4732,   4766,   4801,   4836,   4871,   4906	},	// qp = 14
	{   4942,   4977,   5013,   5050,   5086,   5123,   5160,   5198,   5235,   5273,   5312,   5350,   5389,   5428,   5467,   5507	},	// qp = 15
	{   5547,   5587,   5627,   5668,   5709,   5751,   5792,   5834,   5877,   5919,   5962,   6005,   6049,   6093,   6137,   6181	},	// qp = 16
	{   6226,   6271,   6317,   6362,   6408,   6455,   6502,   6549,   6596,   6644,   6692,   6741,   6789,   6839,   6888,   6938	},	// qp = 17
	{   6988,   7039,   7090,   7141,   7193,   7245,   7298,   7351,   7404,   7458,   7512,   7566,   7621,   7676,   7732,   7788	},	// qp = 18
	{   7844,   7901,   7958,   8016,   8074,   8133,   8191,   8251,   8311,   8371,   8432,   8493,   8554,   8616,   8679,   8741	},	// qp = 19
	{   8805,   8869,   8933,   8998,   9063,   9128,   9195,   9261,   9328,   9396,   9464,   9533,   9602,   9671,   9741,   9812	},	// qp = 20
	{   9883,   9955,  10027,  10099,  10173,  10246,  10321,  10395,  10471,  10547,  10623,  10700,  10778,  10856,  10934,  11014	},	// qp = 21
	{  11093,  11174,  11255,  11336,  11418,  11501,  11585,  11668,  11753,  11838,  11924,  12010,  12097,  12185,  12273,  12362	},	// qp = 22
	{  12452,  12542,  12633,  12725,  12817,  12910,  13003,  13097,  13192,  13288,  13384,  13481,  13579,  13677,  13776,  13876	},	// qp = 23
	{  13977,  14078,  14180,  14283,  14386,  14491,  14596,  14701,  14808,  14915,  15023,  15132,  15242,  15352,  15463,  15576	},	// qp = 24
	{  15688,  15802,  15917,  16032,  16148,  16265,  16383,  16502,  16621,  16742,  16863,  16985,  17108,  17232,  17357,  17483	},	// qp = 25
	{  17610,  17737,  17866,  17995,  18126,  18257,  18389,  18523,  18657,  18792,  18928,  19065,  19203,  19343,  19483,  19624	},	// qp = 26
	{  19766,  19909,  20054,  20199,  20345,  20493,  20641,  20791,  20941,  21093,  21246,  21400,  21555,  21711,  21869,  22027	},	// qp = 27
	{  22187,  22348,  22509,  22673,  22837,  23002,  23169,  23337,  23506,  23676,  23848,  24021,  24195,  24370,  24547,  24725	},	// qp = 28
	{  24904,  25084,  25266,  25449,  25634,  25819,  26006,  26195,  26385,  26576,  26768,  26962,  27158,  27355,  27553,  27752	},	// qp = 29
	{  27954,  28156,  28360,  28566,  28773,  28981,  29191,  29403,  29616,  29830,  30047,  30264,  30484,  30704,  30927,  31151	},	// qp = 30
	{  31377,  31604,  31833,  32064,  32296,  32530,  32766,  33003,  33243,  33483,  33726,  33970,  34217,  34465,  34714,  34966	},	// qp = 31
	{  35219,  35474,  35732,  35990,  36251,  36514,  36779,  37045,  37313,  37584,  37856,  38131,  38407,  38685,  38966,  39248	},	// qp = 32
	{  39532,  39819,  40107,  40398,  40691,  40986,  41283,  41582,  41883,  42186,  42492,  42800,  43110,  43423,  43737,  44054	},	// qp = 33
	{  44373,  44695,  45019,  45345,  45674,  46005,  46338,  46674,  47012,  47353,  47696,  48041,  48390,  48740,  49093,  49449	},	// qp = 34
	{  49808,  50168,  50532,  50898,  51267,  51639,  52013,  52390,  52769,  53152,  53537,  53925,  54316,  54709,  55106,  55505	},	// qp = 35
	{  55907,  56312,  56720,  57131,  57545,  57962,  58382,  58805,  59231,  59661,  60093,  60528,  60967,  61409,  61854,  62302	},	// qp = 36
	{  62754,  63208,  63666,  64128,  64592,  65060,  65532,  66007,  66485,  66967,  67452,  67941,  68433,  68929,  69429,  69932	},	// qp = 37
	{  70438,  70949,  71463,  71981,  72502,  73028,  73557,  74090,  74627,  75168,  75712,  76261,  76814,  77370,  77931,  78496	},	// qp = 38
	{  79065,  79637,  80215,  80796,  81381,  81971,  82565,  83163,  83766,  84373,  84984,  85600,  86220,  86845,  87475,  88108	},	// qp = 39
	{  88747,  89390,  90038,  90690,  91347,  92009,  92676,  93348,  94024,  94705,  95392,  96083,  96779,  97481,  98187,  98898	},	// qp = 40
	{  99615, 100337, 101064, 101796, 102534, 103277, 104025, 104779, 105538, 106303, 107074, 107849, 108631, 109418, 110211, 111010	},	// qp = 41
	{ 111814, 112624, 113441, 114263, 115091, 115925, 116765, 117611, 118463, 119321, 120186, 121057, 121934, 122818, 123708, 124604	},	// qp = 42
	{ 125507, 126417, 127333, 128255, 129185, 130121, 131064, 132014, 132970, 133934, 134904, 135882, 136866, 137858, 138857, 139863	},	// qp = 43
	{ 140877, 141898, 142926, 143962, 145005, 146056, 147114, 148180, 149254, 150336, 151425, 152522, 153627, 154741, 155862, 156991	},	// qp = 44
	{ 158129, 159275, 160429, 161592, 162763, 163942, 165130, 166327, 167532, 168746, 169969, 171200, 172441, 173691, 174949, 176217	},	// qp = 45
	{ 177494, 178780, 180076, 181380, 182695, 184019, 185352, 186695, 188048, 189411, 190783, 192166, 193558, 194961, 196374, 197797	},	// qp = 46
	{ 199230, 200674, 202128, 203593, 205068, 206554, 208051, 209558, 211077, 212607, 214147, 215699, 217262, 218836, 220422, 222019	},	// qp = 47
	{ 223628, 225249, 226881, 228525, 230181, 231849, 233529, 235221, 236926, 238643, 240372, 242114, 243868, 245636, 247416, 249208	},	// qp = 48
	{ 251014, 252833, 254665, 256511, 258370, 260242, 262128, 264027, 265940, 267867, 269809, 271764, 273733, 275717, 277715, 279727	},	// qp = 49
	{ 281754, 283796, 285852, 287924, 290010, 292112, 294228, 296360, 298508, 300671, 302850, 305044, 307255, 309481, 311724, 313983	},	// qp = 50
	{ 316258, 318550, 320858, 323183, 325525, 327884, 330260, 332653, 335064, 337492, 339937, 342401, 344882, 347381, 349898, 352434	},	// qp = 51

};

static const unsigned int qp_factor_tbl[52] = {
    4096,    4598,    5161,    5793,    6502,    7298,    8192,    9195,   10321,   11585,	 // qp 0 ~ 9
   13004,   14596,   16384,   18390,   20643,   23170,   26008,   29193,   32768,   36781,	 // qp 10 ~ 19
   41285,   46341,   52016,   58386,   65536,   73562,   82570,   92682,  104032,  116772,	 // qp 20 ~ 29
  131072,  147123,  165140,  185364,  208064,  233544,  262144,  294247,  330281,  370728,	 // qp 30 ~ 39
  416128,  467088,  524288,  588493,  660561,  741455,  832255,  934175, 1048576, 1176987,	 // qp 40 ~ 49
 1321123, 1482910
};

static const unsigned int sqrt_factor_tbl[] = {
      4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
      4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
      4096,  4096,  4096,  4096,  4096,  4096,  4177,  4257,  4335,  4412,
      4487,  4561,  4634,  4706,  4777,  4846,  4915,  4983,  5050,  5116,
      5181,  5245,  5309,  5372,  5434,  5495,  5556,  5616,  5676,  5734,
      5793,  5850,  5907,  5964,  6020,  6075,  6130,  6185,  6239,  6292,
      6345,  6398,  6450,  6502,  6554,  6605,  6655,  6705,  6755,  6805,
      6854,  6903,  6951,  6999,  7047,  7094,  7142,  7188,  7235,  7281,
      7327,  7373,  7418,  7463,  7508,  7553,  7597,  7641,  7685,  7728,
      7772,  7815,  7857,  7900,  7942,  7985,  8026,  8068,  8110,  8151,
      8192,  8233,  8274,  8314,  8354,  8394,  8434,  8474,  8513,  8553,
      8592,  8631,  8670,  8708,  8747,  8785,  8823,  8861,  8899,  8936,
      8974,  9011,  9048,  9085,  9122,  9159,  9195,  9232,  9268,  9304,
      9340,  9376,  9412,  9447,  9483,  9518,  9553,  9588,  9623,  9658,
      9693,  9727,  9762,  9796,  9830,  9864,  9898,  9932,  9966, 10000,
     10033, 10066, 10100, 10133, 10166, 10199, 10232, 10265, 10297, 10330,
     10362, 10394, 10427, 10459, 10491, 10523, 10555, 10586, 10618, 10650,
     10681, 10712, 10744, 10775, 10806, 10837, 10868, 10899, 10929, 10960,
     10991, 11021, 11052, 11082, 11112, 11142, 11172, 11202, 11232, 11262,
     11292, 11322, 11351, 11381, 11410, 11440, 11469, 11498, 11527, 11556,
     11585, 11614, 11643, 11672, 11701, 11729, 11758, 11786, 11815, 11843,
     11871, 11900, 11928, 11956, 11984, 12012, 12040, 12068, 12095, 12123,
     12151, 12178, 12206, 12233, 12261, 12288, 12315, 12342, 12370, 12397,
     12424, 12451, 12478, 12505, 12531, 12558, 12585, 12611, 12638, 12665,
     12691, 12717, 12744, 12770, 12796, 12823, 12849, 12875, 12901, 12927,
     12953, 12979, 13004, 13030, 13056, 13082, 13107, 13133, 13158, 13184,
     13209, 13235, 13260, 13285, 13310, 13336, 13361, 13386, 13411, 13436,
     13461, 13486, 13511, 13535, 13560, 13585, 13610, 13634, 13659, 13683,
     13708, 13732, 13757, 13781, 13805, 13830, 13854, 13878, 13902, 13926,
     13950, 13975, 13998, 14022, 14046, 14070, 14094, 14118, 14142, 14165,
     14189, 14213, 14236, 14260, 14283, 14307, 14330, 14354, 14377, 14400,
     14424, 14447, 14470, 14493, 14516, 14539, 14562, 14585, 14608, 14631,
     14654, 14677, 14700, 14723, 14746, 14768, 14791, 14814, 14836, 14859,
     14882, 14904, 14927, 14949, 14971, 14994, 15016, 15039, 15061, 15083,
     15105, 15127, 15150, 15172, 15194, 15216, 15238, 15260, 15282, 15304,
     15326, 15348, 15370, 15391, 15413, 15435, 15457, 15478, 15500, 15522,
     15543, 15565, 15586, 15608, 15629, 15651, 15672, 15694, 15715, 15736,
     15758, 15779, 15800, 15821, 15843, 15864, 15885, 15906, 15927, 15948,
     15969, 15990, 16011, 16032, 16053, 16074, 16095, 16116, 16136, 16157,
     16178, 16199, 16219, 16240, 16261, 16281, 16302, 16322, 16343, 16364,
     16384,
};

// base 256, not 4096
static const unsigned int short_term_tbl[] = 
{
#if (SHORT_TERM_LEARNING_RATE==16)      // SHORT_TERM_LEARNING_RATE = 16
     256,
     256,   496,   721,   932,  1130,   // 1~5
    1315,  1489,  1652,  1805,  1948,   // 6~10
    2082,  2208,  2326,  2437,  2540,   // 11~15
    2638,  2729,  2814,  2894,  2969,   // 16~20
    3040,  3106,  3168,  3226,  3280,   // 21~25
    3331,  3379,  3424,  3466,  3505,   // 26~30
    3542,  3577,  3609,  3640,  3668,   // 31~35
    3695,  3720,  3743,  3765,  3786,   // 36~40
    3805,  3824,  3841,  3857,  3872,   // 41~45
    3886,  3899,  3911,  3923,  3933,   // 46~50
    3944,  3953,  3962,  3970,  3978,   // 51~55
    3986,  3993,  3999,  4005,  4011,   // 56~60
    4016,  4021,  4026,  4030,  4034,   // 61~65
    4038,  4042,  4045,  4048,  4051,   // 66~70
    4054,  4057,  4059,  4061,  4064,   // 71~75
    4066,  4068,  4069,  4071,  4073,   // 76~80
    4074,  4075,  4077,  4078,  4079,   // 81~85
    4080,  4081,  4082,  4083,  4084,   // 86~90
    4084,  4085,  4086,  4087,  4087,   // 91~95
    4088,  4088,  4089,  4089,  4090,   // 96~100
    4090,  4090,  4091,  4091,  4091,   // 101~105
    4092,  4092,  4092,  4092,  4093,   // 106~110
    4093,  4093,  4093,  4093,  4094,   // 111~115
    4094,  4094,  4094,  4094,  4094,   // 116~120
    4094,  4094,  4095,  4095,  4095,   // 121~125
    4095,  4095,  4095,  4095,  4095,   // 126~130
    4095,  4095,  4095,  4095,  4095,   // 131~135
    4095,  4095,  4095,  4095,  4096,   // 136~140
    4096,  4096,  4096,  4096,  4096,   // 141~145
    4096,  4096,  4096,  4096,  4096,   // 146~150
#elif (SHORT_TERM_LEARNING_RATE==8)     // SHORT_TERM_LEARNING_RATE = 8
      256,
      256,   480,   676,   848,   998, 	// 1~5
     1129,  1244,  1344,  1432,  1509, 	// 6~10
     1577,  1635,  1687,  1732,  1772, 	// 11~15
     1806,  1836,  1863,  1886,  1906, 	// 16~20
     1924,  1939,  1953,  1965,  1975, 	// 21~25
     1984,  1992,  1999,  2005,  2011, 	// 26~30
     2015,  2019,  2023,  2026,  2029, 	// 31~35
     2031,  2033,  2035,  2037,  2038, 	// 36~40
     2039,  2040,  2041,  2042,  2043, 	// 41~45
     2044,  2044,  2045,  2045,  2045, 	// 46~50
     2046,  2046,  2046,  2046,  2047, 	// 51~55
     2047,  2047,  2047,  2047,  2047, 	// 56~60
     2047,  2047,  2048,  2048,  2048, 	// 61~65
     2048,  2048,  2048,  2048,  2048, 	// 66~70
     2048,  2048,  2048,  2048,  2048, 	// 71~75
     2048,  2048,  2048,  2048,  2048, 	// 76~80
     2048,  2048,  2048,  2048,  2048, 	// 81~85
     2048,  2048,  2048,  2048,  2048, 	// 86~90
     2048,  2048,  2048,  2048,  2048, 	// 91~95
     2048,  2048,  2048,  2048,  2048, 	// 96~100
#elif (SHORT_TERM_LEARNING_RATE==4)     // SHORT_TERM_LEARNING_RATE = 4
      256,
      256,   448,   592,   700,   781, 	// 1~5
      842,   887,   921,   947,   966, 	// 6~10
      981,   992,  1000,  1006,  1010, 	// 11~15
     1014,  1016,  1018,  1020,  1021, 	// 16~20
     1022,  1022,  1023,  1023,  1023, 	// 21~25
     1023,  1024,  1024,  1024,  1024, 	// 26~30
     1024,  1024,  1024,  1024,  1024, 	// 31~35
     1024,  1024,  1024,  1024,  1024, 	// 36~40
     1024,  1024,  1024,  1024,  1024, 	// 41~45
     1024,  1024,  1024,  1024,  1024, 	// 46~50
     1024,  1024,  1024,  1024,  1024, 	// 51~55
     1024,  1024,  1024,  1024,  1024, 	// 56~60
     1024,  1024,  1024,  1024,  1024, 	// 61~65
     1024,  1024,  1024,  1024,  1024, 	// 66~70
     1024,  1024,  1024,  1024,  1024, 	// 71~75
     1024,  1024,  1024,  1024,  1024, 	// 76~80
     1024,  1024,  1024,  1024,  1024, 	// 81~85
     1024,  1024,  1024,  1024,  1024, 	// 86~90
     1024,  1024,  1024,  1024,  1024, 	// 91~95
     1024,  1024,  1024,  1024,  1024, 	// 96~100
#elif (SHORT_TERM_LEARNING_RATE==2)     // SHORT_TERM_LEARNING_RATE
      256,
      256,   384,   448,   480,   496,     // 1~5
      504,   508,   510,   511,   512,     // 6~10
      512,   512,   512,   512,   512,     // 11~15
      512,   512,   512,   512,   512,     // 16~20
      512,   512,   512,   512,   512,     // 21~25
      512,   512,   512,   512,   512,     // 26~30
      512,   512,   512,   512,   512,     // 31~35
      512,   512,   512,   512,   512,     // 36~40
      512,   512,   512,   512,   512,     // 41~45
      512,   512,   512,   512,   512,     // 46~50
      512,   512,   512,   512,   512,     // 51~55
      512,   512,   512,   512,   512,     // 56~60
      512,   512,   512,   512,   512,     // 61~65
      512,   512,   512,   512,   512,     // 66~70
      512,   512,   512,   512,   512,     // 71~75
      512,   512,   512,   512,   512,     // 76~80
      512,   512,   512,   512,   512,     // 81~85
      512,   512,   512,   512,   512,     // 86~90
      512,   512,   512,   512,   512,     // 91~95
      512,   512,   512,   512,   512,     // 96~100
#endif
};

int log_level = LOG_WARNING;    //larger level, more message
#ifdef VG_INTERFACE
#define DEBUG_M(level, fmt, args...) { \
    if (log_level == LOG_DIRECT) \
        printk(fmt, ## args); \
    else if (log_level >= level) \
        printm(MODULE_NAME, fmt, ## args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (log_level >= level) \
        printk(fmt, ## args); }
#endif

#if 1
#define dmp_rc(fmt, args...) { \
        if (str) \
            len += sprintf(str+len, fmt, ## args); \
        else \
            printm(MODULE_NAME,  fmt, ## args); }
#else
#define dmp_rc(fmt, args...) { \
        if (str) \
            len += sprintf(str+len, fmt, ## args); \
        else \
            printk(fmt, ## args); }
#endif



static unsigned int qp2qscale_fix(unsigned int qp)
{
    int qp_int = qp >> SCALE_FACTOR;
    int qp_fract = qp - (qp_int<<SCALE_FACTOR);
    qp_fract = (qp_fract + (1<<(SCALE_FACTOR - TABLE_BIT-1)))>>(SCALE_FACTOR - TABLE_BIT);
    return qp2qscale_tbl[qp_int][qp_fract];
}

static unsigned int look_table_int(unsigned int qscale, int lb, int ub)
{
    int mid = (lb + ub)/2;
    if (mid == lb || mid == ub) {
        if (iDiff(qp2qscale_tbl[lb][TABLE_SIZE>>1], qscale) < 
            iDiff(qp2qscale_tbl[ub][TABLE_SIZE>>1], qscale))
            return lb;
        else
            return ub;
    }
    if (qscale < qp2qscale_tbl[mid][TABLE_BIT>>1])
        return look_table_int(qscale, lb, mid);
    else
        return look_table_int(qscale, mid, ub);
}

static unsigned int qscale2qp_fix(unsigned int qscale)
{
    int qp_int;
    int qp_fract = TABLE_SIZE - 1;

    qp_int = look_table_int(qscale, 0, 51);
    if (51 == qp_int)
        return (qp_int<<SCALE_FACTOR);
    for (qp_fract = 0; qp_fract < TABLE_SIZE - 1; qp_fract++) {
        if (qscale < qscale2qp_tbl[qp_int][qp_fract])
            break;
    }
    return (qp_int<<SCALE_FACTOR) + (qp_fract<<(SCALE_FACTOR - TABLE_BIT));
}

#else   // FIX_POINT

static __inline double qp2qscale(double qp)
{
    return 0.85 * pow(2.0, ( qp - 12.0 ) / 6.0);
}
static __inline double qscale2qp(double qscale)
{
    return 12.0 + 6.0 * log(qscale/0.85) / log(2.0);
}
#endif

static const char slice_type_name[3] = {'P', 'B', 'I'};
/*********************************************
 * #define ACCUM_P_QP_RATE         0.95
 * #define OVERFLOW_BITRATE_BOUND  1.5
 * #define UNDERFLOW_BITRATE_BOUND 0.5
 *********************************************/
static const unsigned int accum_p_qp_rate = 3891;
//static const unsigned int overflow_bitrate_bound = 6144;
//static const unsigned int underflow_bitrate_bound = 2048;
extern unsigned int overflow_bitrate_bound;
extern unsigned int underflow_bitrate_bound;
extern unsigned int rate_tolerance;

#define RC_INIT_IDX     0
#define RC_START_IDX    1
#define RC_END_IDX      2
#define RC_EXIT_IDX     3
static int dump_rc_info(struct rc_private_data_t *rc, int frame_num, int slice_type, int bits, int pos)
{
#ifdef DUMP_RC_LOG
    if (RC_INIT_IDX == pos)
        printk("============ init rc ============\n");
    else if (RC_START_IDX == pos)
        printk("====== %c frame %d start rc ======\n", slice_type_name[slice_type], frame_num);
    else if (RC_END_IDX == pos)
        printk("======= %c frame %d end rc =======\n", slice_type_name[slice_type], frame_num);
    else
        printk("============ exit rc ============\n");

    if (RC_INIT_IDX == pos) {
        printk("rc_mode = %d\n", rc->rc_mode);
        printk("b_abr = %d\n", rc->b_abr);
        printk("fbase = %d\n", rc->fbase);
        printk("fincr = %d\n", rc->fincr);
        //printk("qcompress = %d\n", rc->qcompress);
        printk("bitrate = %d\n", rc->bitrate);
        printk("rate_tolerance = %d\n", rc->rate_factor_constant);
        printk("mb_count = %d\n", rc->mb_count);
        printk("cbr_decay = %d\n", rc->cbr_decay);
        printk("ip_offset = %d\n", rc->ip_offset);
        printk("pb_offset = %d\n", rc->pb_offset);
        printk("ip_factor = %d\n", rc->ip_factor);
        printk("pb_factor = %d\n", rc->pb_factor);
        printk("lstep = %d\n", rc->lstep);
        printk("qp_constant[0] = %d\n", rc->qp_constant[0]);
        printk("qp_constant[1] = %d\n", rc->qp_constant[1]);
        printk("qp_constant[1] = %d\n", rc->qp_constant[2]);
        printk("min_quant[0] = %d\n", rc->min_quant[0]);
        printk("min_quant[1] = %d\n", rc->min_quant[1]);
        printk("min_quant[2] = %d\n", rc->min_quant[2]);
        printk("max_quant[0] = %d\n", rc->max_quant[0]);
        printk("max_quant[1] = %d\n", rc->max_quant[1]);
        printk("max_quant[2] = %d\n", rc->max_quant[2]);
        printk("bframes = %d\n", rc->bframes);
    }
    printk("last_non_b_pict_type = %d\n", rc->last_non_b_pict_type);
    printk("accum_p_norm = %d\n", rc->accum_p_norm);
    printk("accum_p_qp = %lld\n", rc->accum_p_qp);
    printk("cplxr_sum = %lld\n", rc->cplxr_sum);
    printk("wanted_bits_window = %d\n", rc->wanted_bits_window);
    printk("last_qscale = %d\n", rc->last_qscale);
    printk("last_qscale_for[0] = %d\n", rc->last_qscale_for[0]);
    printk("last_qscale_for[1] = %d\n", rc->last_qscale_for[1]);
    printk("last_qscale_for[2] = %d\n", rc->last_qscale_for[2]);
    printk("total_bits = %d (0x%x)\n", rc->total_bits, rc->total_bits/8);
    printk("last_satd = %d\n", rc->last_satd);
    printk("last_rceq = %d\n", rc->last_rceq);
    printk("avg_qp = %d\n", rc->avg_qp);
    printk("qp = %d\n", (int)((rc->qp + HALF_SCALE_BASE) / SCALE_BASE));

    if (RC_END_IDX == pos) {
        printk("bits = %d\n", bits);
        //printk("pred size = %d\n", rc->frame_size_pred);
    }
#endif
    return 0;
}

static void dump_rc_param(struct rc_frame_info_t *param)
{
#ifdef DUMP_RC_LOG
    printk("------- param -------\n");
    printk("force qp   = %d\n", param->force_qp);
    printk("slice type = %d\n", param->slice_type);
    printk("last satd  = %d\n", param->last_satd);
    printk("frame size = %d\n", param->frame_size);
    printk("avg_qp     = %d\n", param->avg_qp);
#endif
}

static __inline unsigned int div32(unsigned int numer, unsigned int denom)
{
    if (denom == 0) {
        DEBUG_M(LOG_ERROR, "div 32: %d / %d\n", numer, denom);
    #ifdef VG_INTERFACE
        //damnit(MODULE_NAME);
    #endif
        return numer;
    }
    return (numer + denom/2) / denom;
}

static __inline unsigned int div64(unsigned long long numer, unsigned int denom, char *src_str)
{
#ifdef LINUX
    unsigned long long tmp = numer + denom/2;
    if (denom == 0) {
        DEBUG_M(LOG_ERROR, "%s: div 64: %llu / %d\n", src_str, numer, denom);
    #ifdef VG_INTERFACE
        //damnit(MODULE_NAME);
    #endif
        return numer;
    }
    do_div(tmp, denom);
    return (unsigned int)tmp;
#else
    if (denom == 0)
        return numer;
    return (unsigned int)((numer + denom/2) / denom);
#endif
}

static __inline int div64sign(long long numer, unsigned int denom)
{
#ifdef LINUX
    int sign;
    unsigned long long tmp;
    if (denom == 0) {
        DEBUG_M(LOG_ERROR, "div sign 64: %lld / %d\n", numer, denom);
    #ifdef VG_INTERFACE
        //damnit(MODULE_NAME);
    #endif
        return numer;
    }
    if (numer < 0) {
        sign = -1;
        tmp = (-numer) + denom/2;
    }
    else {
        sign = 1;
        tmp = numer + denom/2;
    }
    do_div(tmp, denom);
    return (int)(sign*tmp);
#else    
    return (int)((numer + denom/2) / denom);
#endif
}

static int getInitialQP(unsigned int bitrate, int mb_count)
{
#if 1
    unsigned int bpmb = bitrate / mb_count; 
    unsigned int L1, L2, L3;

    if (mb_count <= 99) {
        L1 = 26;    // 0.1*256
        L2 = 77;    // 0.3*256
        L3 = 154;   // 0.4*256
    }
    else if (mb_count <= 396) {
        L1 = 51;    // 0.2*256
        L2 = 154;   // 0.6*256
        L3 = 307;   // 1.2*256
    }
    else {
        L1 = 154;   // 0.6*256
        L2 = 358;   // 1.4*256
        L3 = 614;   // 2.4*256
    }
    if (bpmb <= L1)
        return 35;
    else if (bpmb <= L2)
        return 25;
    else if (bpmb <= L3)
        return 20;
    else 
        return 10;
#else
    return ABR_INIT_QP;
#endif
}

static unsigned int get_init_cplx_sum(unsigned int mb_count)
{
    if (mb_count <= 99)
        return 69649;
    else if (mb_count <= 396)
        return 139298;
    else if (mb_count <= 1350)
        return 257196;
    else if (mb_count <= 3600)
        return 420000;
    else if (mb_count <= 8160)
        return 632329;
    else
        return 1260000;
}

static unsigned int get_qscale(struct rc_private_data_t *rc, unsigned int rate_factor_numer, 
                               unsigned long long rate_factor_denom, int frame_num, int pict_type)
{
    unsigned int q;
#ifdef USE_SHORT_TERN_CPLX
    if (0 == rc->last_satd) {
        rc->last_rceq = 
        q = rc->last_qscale_for[pict_type];
    }
    else {
        unsigned long long temp;
        q = rc->last_rceq = rc->short_term_cplxsum * 256 / short_term_tbl[rc->short_term_cplxcount];
        temp = q * rate_factor_denom;
        q = div64(temp, rate_factor_numer, "get_scale");
        rc->last_qscale = q;
    }
#else
    if (0 == frame_num) {
        q = rc->last_qscale_for[pict_type];
        rc->last_rceq = q;
        rc->last_qscale = q;
    }
    else {
        unsigned long long temp;
        q = rc->last_qscale_for[pict_type];
        rc->last_rceq = q;
        temp = q * rate_factor_denom;
        q = div64(temp, rate_factor_numer, "get_scale");
        //q = (unsigned int)(temp / rate_factor_numer);
        rc->last_qscale = q;
    }
#endif
    return q;
}

static void accum_p_qp_update(struct rc_private_data_t *rc, unsigned int qp, int slice_type)
{
/*
    rc->accum_p_qp   *= .95;
    rc->accum_p_norm *= .95;
    rc->accum_p_norm += 1;
    if (SLICE_TYPE_I == slice_type)
        rc->accum_p_qp += qp + rc->ip_offset;
    else
        rc->accum_p_qp += qp;
*/
    if (SLICE_TYPE_I == slice_type)
        rc->accum_p_qp = div64(rc->accum_p_qp * accum_p_qp_rate, SCALE_BASE, "accum_I") + qp + rc->ip_offset;
    else
        rc->accum_p_qp = div64(rc->accum_p_qp * accum_p_qp_rate, SCALE_BASE, "accum_P") + qp;

    rc->accum_p_norm = div32(rc->accum_p_norm * accum_p_qp_rate, SCALE_BASE) + SCALE_BASE;
}

static unsigned int clip_qscale(struct rc_private_data_t *rc, int pict_type, unsigned int q)
{
    unsigned int lmin = rc->min_qscale[pict_type];
    unsigned int lmax = rc->max_qscale[pict_type];

    if (lmin == lmax)
        return lmin;
    return RC_CLIP3(q, lmin, lmax);
}

static unsigned int rate_estimate_qscale(struct rc_private_data_t *rc, struct rc_frame_info_t *data)
{
    unsigned int q;
    int pict_type = data->slice_type;
    unsigned int lmin = rc->min_qscale[pict_type];
    unsigned int lmax = rc->max_qscale[pict_type];
    unsigned int abr_buffer;// = 2 * rc->rate_tolerance * rc->bitrate / SCALE_BASE;
    //rc_entry_t rce;
    unsigned int wanted_bits, overflow = SCALE_BASE;
    //unsigned int pred_size;

    // last not define
    rc->last_satd = data->last_satd * SCALE_BASE / rc->mb_count / 256;

    if (rc->last_satd > 0) {
        rc->short_term_cplxsum = rc->last_satd + (SHORT_TERM_LEARNING_RATE - 1) * rc->short_term_cplxsum / SHORT_TERM_LEARNING_RATE;
        if (rc->short_term_cplxcount < MAX_SHORTTERM_CNT_TABLE)
            rc->short_term_cplxcount++;
    }
    /*
    if (RC_CRF == rc->rc_mode)
        q = get_qscale(rc, rc->rate_factor_constant, SCALE_BASE, data->frame_num, pict_type);
    else 
    */
    {
        //int frame_done = data->frame_num;
        int frame_done = rc->encoded_frame;
        long long residual;

        q = get_qscale(rc, rc->wanted_bits_window, rc->cplxr_sum, rc->encoded_frame, pict_type);

        //if (data->frame_num > MAX_BUFFERED_BUDGET)
        //    frame_done = MAX_BUFFERED_BUDGET;
        //wanted_bits = frame_done * rc->bitrate * rc->fincr / rc->fbase;
        wanted_bits = frame_done * rc->budget_buffer;
        if (wanted_bits > 0) {
            //int remain = (rc->total_bits - wanted_bits)*SCALE_BASE;
            if (frame_done <= MAX_BUFFERED_BUDGET)
                abr_buffer = div64((unsigned long long)rc->abr_buffer * sqrt_factor_tbl[frame_done], SCALE_BASE, "req_abr");
            else
                abr_buffer = div64((unsigned long long)rc->abr_buffer * sqrt_factor_tbl[400], SCALE_BASE, "req_abr");
            residual = (int)rc->total_bits - (int)wanted_bits;
            overflow = RC_CLIP3(SCALE_BASE + div64sign(residual*SCALE_BASE, (int)abr_buffer), HALF_SCALE_BASE, SCALE_BASE<<1);
            q = div64((unsigned long long)q * overflow, SCALE_BASE, "req_q");
        }
    }

    //if (SLICE_TYPE_I == pict_type && data->idr_period && 
    //    SLICE_TYPE_I != rc->last_non_b_pict_type) {
    if (SLICE_TYPE_I == pict_type && rc->idr_period && 
        SLICE_TYPE_I != rc->last_non_b_pict_type) {
        #ifdef FIX_POINT
            q = qp2qscale_fix(div64(rc->accum_p_qp * SCALE_BASE, rc->accum_p_norm, "req_accum"));
            q = div64((unsigned long long)q * SCALE_BASE, rc->ip_factor, "req_ip");
        #else
            q = qp2qscale(rc->accum_p_qp / rc->accum_p_norm);
            q /= RC_ABS(rc->ip_factor);
        #endif
            
    }
    //else if (data->frame_num > 0) {
    else if (rc->encoded_frame) {
        lmin = rc->last_qscale_for[pict_type] * SCALE_BASE / rc->lstep;
        lmax = div64((unsigned long)rc->last_qscale_for[pict_type] * rc->lstep, SCALE_BASE, "req_max");
        //if (overflow > overflow_bitrate_bound && data->frame_num > 3) // > 1.1
        if (overflow > overflow_bitrate_bound && rc->encoded_frame > 3) // > 1.1
            lmax = lmax * rc->lstep / SCALE_BASE;
        else if (overflow < underflow_bitrate_bound)   // < 0.9
            lmin = lmin * SCALE_BASE / rc->lstep;
        q = RC_CLIP3(q, lmin, lmax);
    }
    /*
    else if (RC_CRF == rc->rc_mode && rc->qcompress != 1)
        q = qp2qscale(ABR_INIT_QP) / RC_ABS(rc->ip_factor);
    */

    q = clip_qscale(rc, pict_type, q);

    rc->last_qscale_for[pict_type] =
    rc->last_qscale = q;

    //if (0 == data->frame_num)
    if (0 == rc->encoded_frame)
        rc->last_qscale_for[SLICE_TYPE_P] = div64((unsigned long long)q * rc->ip_factor, SCALE_BASE, "req_P");
#ifdef MB_LEVEL_RATE_CONTROL
    rc->frame_size_pred = predict_size(&rc->rc_pred[data->slice_type], (double)q/SCALE_BASE, rc->last_satd);
    pred_size = predict_size_fix(&rc->pred[data->slice_type], q, rc->last_satd);
    /*
    if (overflow > 4506)
        rc->frame_size_pred = rc->frame_size_pred * 3686 / SCALE_BASE;
    else if (overflow < 3686)
        rc->frame_size_pred = rc->frame_size_pred * 4506 / SCALE_BASE;
    */
#endif
    //rc->frame_size_planned = predict_size(&rc->pred[data->slice_type], q, rc->last_satd);
    //rc->frame_size_estimated = rc->frame_size_planned;
    return q;
}

//int rc_init_fix(void *handle, void *in_param)
int rc_init_fix(struct rc_private_data_t *rc, struct rc_init_param_t *param)
{
    //struct rc_private_data_t *rc = (struct rc_private_data_t *)handle;
    //struct rc_init_param_t *param = (struct rc_init_param_t *)in_param;
    int i;
    int initialQP;

    memset(rc, 0 , sizeof(struct rc_private_data_t));
    rc->rc_mode = param->rc_mode;
    rc->b_abr = (param->rc_mode != EM_VRC_VBR);

    if (param->fbase > 0 && param->fincr > 0) {
        rc->fbase = param->fbase;
        rc->fincr = param->fincr;
    }
    else {
        rc->fbase = 30;
        rc->fincr = 1;
    }
    if (param->qp_constant < 0 || rc->b_abr) {
        initialQP = getInitialQP(div64((unsigned long long)param->bitrate * 1000 * rc->fincr, rc->fbase, "init_qp"), param->mb_count);
    }
    else
        initialQP = param->qp_constant;
    rc->qcompress = 1;

    rc->bitrate = param->bitrate * 1000;
    rc->max_bitrate = param->max_bitrate * 1000;
    rc->budget_buffer = div64((unsigned long long)rc->bitrate * rc->fincr, rc->fbase, "budget"); 
    DEBUG_M(LOG_INFO, "rc->budget_buffer = %d\n", rc->budget_buffer);
#if 0
    if (param->rate_tolerance < 0.01) {
        printf("bitrate tolerance too samll, force to 0.01\n");
        param->rate_tolerance = 0.01;
    }
    rc->rate_tolerance = (unsigned int)(param->rate_tolerance*SCALE_BASE + 0.5); // SCALE_BASE
#else
    //rc->rate_tolerance = 41;    // 0.01*SCALE_BASE
    rc->rate_tolerance = param->rate_tolerance_fix;
#endif
    rc->abr_buffer = div32(2 * rc->rate_tolerance * rc->bitrate, SCALE_BASE);
    rc->mb_count = param->mb_count;
    rc->idr_period = param->idr_period;
    rc->last_non_b_pict_type = -1;
#if 0
    if (param->cbr_decay < 0.0 || param->cbr_decay > 1.0)
        param->cbr_decay = 0.95;
    rc->cbr_decay = (unsigned int)(param->cbr_decay*SCALE_BASE + 0.5);
#else
    rc->cbr_decay = 3891;   // 0.95 * SCALE_BASE;
#endif
    //rc->mb_var_qp = param->mb_var_qp;

    if (rc->b_abr) {
        rc->accum_p_norm = 41;  //(unsigned int)(0.01*SCALE_BASE + 0.5);
        rc->accum_p_qp = (unsigned int)(initialQP * rc->accum_p_norm);
        rc->cplxr_sum = get_init_cplx_sum(rc->mb_count);
        /*
        if (1 == rc->qcompress)
            rc->cplxr_sum = (unsigned int)(0.01 * 7.0e5 * pow(rc->mb_count, 0.5));
        else
            rc->cplxr_sum = (unsigned int)(0.01 * pow(rc->mb_count, 0.5));
        */
        //rc->cplxr_sum = 0.01 * pow( 7.0e5, rc->qcompress ) * pow( rc->mb_count, 0.5 );
        //rc->wanted_bits_window = (unsigned int)(rc->bitrate * rc->fincr / rc->fbase);
        rc->wanted_bits_window = rc->budget_buffer;
        rc->last_non_b_pict_type = SLICE_TYPE_I;
    }

    rc->ip_offset = (param->ip_offset << SCALE_FACTOR);
    rc->pb_offset = (param->pb_offset << SCALE_FACTOR);
    rc->ip_factor = qp_factor_tbl[param->ip_offset];
    rc->pb_factor = qp_factor_tbl[param->pb_offset];

    rc->qp_constant[SLICE_TYPE_P] = (param->qp_constant * SCALE_BASE);
    rc->qp_constant[SLICE_TYPE_I] = RC_CLIP3(rc->qp_constant[SLICE_TYPE_P] - rc->ip_offset, 0, 51<<SCALE_FACTOR);
    rc->qp_constant[SLICE_TYPE_B] = RC_CLIP3(rc->qp_constant[SLICE_TYPE_P] + rc->pb_offset, 0, 51<<SCALE_FACTOR);
#ifdef SUPPORT_EVBR
    rc->qscale_constant[SLICE_TYPE_P] = qp2qscale_fix(rc->qp_constant[SLICE_TYPE_P]);
    rc->qscale_constant[SLICE_TYPE_B] = qp2qscale_fix(rc->qp_constant[SLICE_TYPE_B]);
    rc->qscale_constant[SLICE_TYPE_I] = qp2qscale_fix(rc->qp_constant[SLICE_TYPE_I]);
#endif
    //rc->lstep = pow( 2, param->qp_step / 6.0 );
    rc->lstep = qp_factor_tbl[param->qp_step];
#ifdef FIX_POINT
    rc->last_qscale = qp2qscale_fix(26 << SCALE_FACTOR);
#else
    rc->last_qscale = qp2qscale(26);
#endif

    for (i = 0; i < 3; i++) {
    #ifdef FIX_POINT
        rc->last_qscale_for[i] = qp2qscale_fix(initialQP * SCALE_BASE);
        rc->min_qscale[i] = qp2qscale_fix(param->min_quant << SCALE_FACTOR);
        rc->max_qscale[i] = qp2qscale_fix(param->max_quant << SCALE_FACTOR);
    #else
        rc->last_qscale_for[i] = qp2qscale(ABR_INIT_QP);
        rc->min_qscale[i] = qp2qscale(param->min_quant);
        rc->max_qscale[i] = qp2qscale(param->max_quant);
    #endif
    }
    rc->max_quant = param->max_quant << SCALE_FACTOR;
    rc->min_quant = param->min_quant << SCALE_FACTOR;

    rc->bframes = param->bframe;

    rc->encoded_frame = 0;
    rc->state = NORMAL_STATE;

    dump_rc_info(rc, 0, 0, 0, RC_INIT_IDX);

    return 0;
}

int rc_exit_fix(struct rc_private_data_t *rc)
{
    return 0;
}

//int rc_start_fix(void *handle, void *in_data)
int rc_start_fix(struct rc_private_data_t *rc, struct rc_frame_info_t *data)
{
    //struct rc_private_data_t *rc = (struct rc_private_data_t *)handle;
    //struct rc_frame_info_t *data = (struct rc_frame_info_t *)in_data;
    unsigned int qp;
    //unsigned int pred_size;

    dump_rc_param(data);

    if (data->force_qp > 0)
        qp = data->force_qp * SCALE_BASE;
    else if (rc->b_abr) {
        if (SLICE_TYPE_B == data->slice_type) {
            int i0 = IsIFrame(data->list[0].slice_type);
            int i1 = IsIFrame(data->list[1].slice_type);
            int dt0 = RC_ABS(data->cur_frame.poc - data->list[0].poc);
            int dt1 = RC_ABS(data->cur_frame.poc - data->list[1].poc);
            int q0 = data->list[0].avg_qp;
            int q1 = data->list[1].avg_qp;

            if (i0 && i1)
                qp = (((q0 + q1 + 1) / 2)<<SCALE_FACTOR) + rc->ip_offset;
            else if (i0)
                qp = (q1 << SCALE_FACTOR);
            else if (i1)
                qp = (q0 << SCALE_FACTOR);
            else
                //qp = (((q0*dt1 + q1*dt0) + (dt0 + dt1)/2)<<SCALE_FACTOR) / (dt0 + dt1);
                qp = ((q0*dt1 + q1*dt0) << SCALE_FACTOR) / (dt0 + dt1);
    
            qp += rc->pb_offset;
        #ifdef MB_LEVEL_RATE_CONTROL
            rc->frame_size_pred = predict_size(&rc->rc_pred[data->slice_type], (double)qp2qscale_fix(qp)/SCALE_BASE, rc->last_satd);
            pred_size = predict_size_fix(&rc->pred[data->slice_type], qp2qscale_fix(qp), rc->last_satd);
        #endif
        }
        else
            qp = qscale2qp_fix(rate_estimate_qscale(rc, data));
    #ifdef SUPPORT_EVBR
        if (EM_VRC_EVBR == rc->rc_mode) {
            if (NORMAL_STATE == rc->state || qp < rc->qp_constant[data->slice_type]) {
                qp = rc->qp_constant[data->slice_type];
                rc->last_qscale_for[data->slice_type] = rc->qscale_constant[data->slice_type];
            }
        }
    #endif
    }
    else {    /* VBR(CQP) */
        qp = rc->qp_constant[data->slice_type];
    }

    //q = RC_CLIP3(q, h->param.rc.i_qp_min, h->param.rc.i_qp_max );

    //rc->qp = RC_CLIP3(qp, 0, 51<<SCALE_FACTOR);
    rc->qp = RC_CLIP3(qp, rc->min_quant, rc->max_quant);

    accum_p_qp_update(rc, rc->qp, data->slice_type);

    if (SLICE_TYPE_B != data->slice_type)
        rc->last_non_b_pict_type = data->slice_type;

    dump_rc_info(rc, rc->encoded_frame, data->slice_type, 0, RC_START_IDX);
#ifdef MB_LEVEL_RATE_CONTROL
    data->frame_size_estimate = rc->frame_size_pred;
#endif


    return (rc->qp + HALF_SCALE_BASE) / SCALE_BASE;
}

//int rc_end_fix(void *handle, void *in_data)
int rc_end_fix(struct rc_private_data_t *rc, struct rc_frame_info_t *data)
{
    //struct rc_private_data_t *rc = (struct rc_private_data_t *)handle;
    //struct rc_frame_info_t *data = (struct rc_frame_info_t *)in_data;
    unsigned int bits = data->frame_size * 8;
    //int max_buffer_budget = 0;
    unsigned long long temp;

    dump_rc_param(data);
    if (rc->encoded_frame >= MAX_BUFFERED_BUDGET) {
        rc->encoded_frame = MAX_BUFFERED_BUDGET;
        //max_buffer_budget = 1;
    }
    else 
        rc->encoded_frame++;

    rc->avg_qp = data->avg_qp;
    rc->avg_qp_act = data->avg_qp_act;

    //if (data->frame_num > MAX_BUFFERED_BUDGET)
    if (rc->encoded_frame >= MAX_BUFFERED_BUDGET)
    //if (max_buffer_budget)
        rc->total_bits -= rc->total_bits/MAX_BUFFERED_BUDGET;
    rc->total_bits += bits;

    if (rc->b_abr) {
        unsigned int qscale;
        qscale = qp2qscale_fix(data->avg_qp * SCALE_BASE);
        if (SLICE_TYPE_B != data->slice_type) {
            rc->cplxr_sum += div64((unsigned long long)bits * qscale, rc->last_rceq, "cplxr_sum");
        }
        else {
            rc->cplxr_sum += div64((unsigned long long)bits * qscale, rc->last_rceq * rc->pb_factor / SCALE_BASE, "cplxr_sum");
        }
        rc->cplxr_sum = (rc->cplxr_sum * rc->cbr_decay + HALF_SCALE_BASE) / SCALE_BASE;
        temp = ((unsigned long long)rc->wanted_bits_window + rc->budget_buffer) * rc->cbr_decay / SCALE_BASE;
        rc->wanted_bits_window = (unsigned int)temp;
    }
#ifdef SUPPORT_EVBR
    if (EM_VRC_EVBR == rc->rc_mode) {
        unsigned int wanted_bits = rc->encoded_frame * rc->budget_buffer;
        if (rc->total_bits > wanted_bits) {
            rc->state = LIMIT_STATE;
        }
        else if (LIMIT_STATE == rc->state && rc->qp <= rc->qp_constant[data->slice_type])
            rc->state = NORMAL_STATE;
    }
#endif
    rc->last_satd = data->last_satd * SCALE_BASE / rc->mb_count / 256;
    dump_rc_info(rc, rc->encoded_frame, data->slice_type, data->frame_size*8, RC_END_IDX);
    return 0;
}

//int rc_reset_param_fix(struct rc_private_data_t *rc, struct rc_init_param_t *param, unsigned int reset_flag)
int rc_increase_qp_fix(struct rc_private_data_t *rc, unsigned int inc_qp)
{
    unsigned int inc_qp_fix = inc_qp * SCALE_BASE;
    unsigned int inc_qscale_fix = qp_factor_tbl[inc_qp];
    rc->qp_constant[SLICE_TYPE_P] = RC_CLIP3(rc->qp_constant[SLICE_TYPE_P] + inc_qp_fix, 0, 51<<SCALE_FACTOR);
    rc->qp_constant[SLICE_TYPE_I] = RC_CLIP3(rc->qp_constant[SLICE_TYPE_I] + inc_qp_fix, 0, 51<<SCALE_FACTOR);
    rc->qp_constant[SLICE_TYPE_B] = RC_CLIP3(rc->qp_constant[SLICE_TYPE_B] + inc_qp_fix, 0, 51<<SCALE_FACTOR);
#ifdef SUPPORT_EVBR
    rc->qscale_constant[SLICE_TYPE_P] = qp2qscale_fix(rc->qp_constant[SLICE_TYPE_P]);
    rc->qscale_constant[SLICE_TYPE_B] = qp2qscale_fix(rc->qp_constant[SLICE_TYPE_B]);
    rc->qscale_constant[SLICE_TYPE_I] = qp2qscale_fix(rc->qp_constant[SLICE_TYPE_I]);
#endif
    rc->last_qscale_for[SLICE_TYPE_P] = rc->last_qscale_for[SLICE_TYPE_P] * inc_qscale_fix / SCALE_BASE;
    rc->last_qscale_for[SLICE_TYPE_I] = rc->last_qscale_for[SLICE_TYPE_I] * inc_qscale_fix / SCALE_BASE;
    rc->last_qscale_for[SLICE_TYPE_B] = rc->last_qscale_for[SLICE_TYPE_B] * inc_qscale_fix / SCALE_BASE;
    return 0;
}

