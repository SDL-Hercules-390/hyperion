/* PFPO.C       (C) Copyright Roger Bowler, 2009-2012                */
/*              Perform Floating Point Operation instruction         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*              (C) Copyright Bernard van der Helm, 2009-2011        */
/*              Noordwijkerhout, The Netherlands                     */

/*              (C) Copyright Bob Wood, 2018-2019                    */

/*-------------------------------------------------------------------*/
/* This module implements the Perform Floating Point Operation       */
/* instruction described in the manual SA22-7832-05.                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _PFPO_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#include "decimal128.h"
#include "decimal64.h"
#include "decimal32.h"
#include "decPacked.h"

#if defined( FEATURE_044_PFPO_FACILITY )

#define CLASS_ZERO      1
#define CLASS_SUBNORMAL 2
#define CLASS_INFINITY  3
#define CLASS_QNAN      4
#define CLASS_SNAN      5
#define CLASS_NORMAL    6

#define INFINITYSTR     "Infinity"
#define QNANSTR         "NaN"
#define SNANSTR         "sNaN"

const uint16_t  DPD2BIN [1024] = {0,    1,    2,    3,    4,    5,    6,    7,
    8,    9,   80,   81,  800,  801,  880,  881,   10,   11,   12,   13,   14,
   15,   16,   17,   18,   19,   90,   91,  810,  811,  890,  891,   20,   21,
   22,   23,   24,   25,   26,   27,   28,   29,   82,   83,  820,  821,  808,
  809,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   92,   93,
  830,  831,  818,  819,   40,   41,   42,   43,   44,   45,   46,   47,   48,
   49,   84,   85,  840,  841,   88,   89,   50,   51,   52,   53,   54,   55,
   56,   57,   58,   59,   94,   95,  850,  851,   98,   99,   60,   61,   62,
   63,   64,   65,   66,   67,   68,   69,   86,   87,  860,  861,  888,  889,
   70,   71,   72,   73,   74,   75,   76,   77,   78,   79,   96,   97,  870,
  871,  898,  899,  100,  101,  102,  103,  104,  105,  106,  107,  108,  109,
  180,  181,  900,  901,  980,  981,  110,  111,  112,  113,  114,  115,  116,
  117,  118,  119,  190,  191,  910,  911,  990,  991,  120,  121,  122,  123,
  124,  125,  126,  127,  128,  129,  182,  183,  920,  921,  908,  909,  130,
  131,  132,  133,  134,  135,  136,  137,  138,  139,  192,  193,  930,  931,
  918,  919,  140,  141,  142,  143,  144,  145,  146,  147,  148,  149,  184,
  185,  940,  941,  188,  189,  150,  151,  152,  153,  154,  155,  156,  157,
  158,  159,  194,  195,  950,  951,  198,  199,  160,  161,  162,  163,  164,
  165,  166,  167,  168,  169,  186,  187,  960,  961,  988,  989,  170,  171,
  172,  173,  174,  175,  176,  177,  178,  179,  196,  197,  970,  971,  998,
  999,  200,  201,  202,  203,  204,  205,  206,  207,  208,  209,  280,  281,
  802,  803,  882,  883,  210,  211,  212,  213,  214,  215,  216,  217,  218,
  219,  290,  291,  812,  813,  892,  893,  220,  221,  222,  223,  224,  225,
  226,  227,  228,  229,  282,  283,  822,  823,  828,  829,  230,  231,  232,
  233,  234,  235,  236,  237,  238,  239,  292,  293,  832,  833,  838,  839,
  240,  241,  242,  243,  244,  245,  246,  247,  248,  249,  284,  285,  842,
  843,  288,  289,  250,  251,  252,  253,  254,  255,  256,  257,  258,  259,
  294,  295,  852,  853,  298,  299,  260,  261,  262,  263,  264,  265,  266,
  267,  268,  269,  286,  287,  862,  863,  888,  889,  270,  271,  272,  273,
  274,  275,  276,  277,  278,  279,  296,  297,  872,  873,  898,  899,  300,
  301,  302,  303,  304,  305,  306,  307,  308,  309,  380,  381,  902,  903,
  982,  983,  310,  311,  312,  313,  314,  315,  316,  317,  318,  319,  390,
  391,  912,  913,  992,  993,  320,  321,  322,  323,  324,  325,  326,  327,
  328,  329,  382,  383,  922,  923,  928,  929,  330,  331,  332,  333,  334,
  335,  336,  337,  338,  339,  392,  393,  932,  933,  938,  939,  340,  341,
  342,  343,  344,  345,  346,  347,  348,  349,  384,  385,  942,  943,  388,
  389,  350,  351,  352,  353,  354,  355,  356,  357,  358,  359,  394,  395,
  952,  953,  398,  399,  360,  361,  362,  363,  364,  365,  366,  367,  368,
  369,  386,  387,  962,  963,  988,  989,  370,  371,  372,  373,  374,  375,
  376,  377,  378,  379,  396,  397,  972,  973,  998,  999,  400,  401,  402,
  403,  404,  405,  406,  407,  408,  409,  480,  481,  804,  805,  884,  885,
  410,  411,  412,  413,  414,  415,  416,  417,  418,  419,  490,  491,  814,
  815,  894,  895,  420,  421,  422,  423,  424,  425,  426,  427,  428,  429,
  482,  483,  824,  825,  848,  849,  430,  431,  432,  433,  434,  435,  436,
  437,  438,  439,  492,  493,  834,  835,  858,  859,  440,  441,  442,  443,
  444,  445,  446,  447,  448,  449,  484,  485,  844,  845,  488,  489,  450,
  451,  452,  453,  454,  455,  456,  457,  458,  459,  494,  495,  854,  855,
  498,  499,  460,  461,  462,  463,  464,  465,  466,  467,  468,  469,  486,
  487,  864,  865,  888,  889,  470,  471,  472,  473,  474,  475,  476,  477,
  478,  479,  496,  497,  874,  875,  898,  899,  500,  501,  502,  503,  504,
  505,  506,  507,  508,  509,  580,  581,  904,  905,  984,  985,  510,  511,
  512,  513,  514,  515,  516,  517,  518,  519,  590,  591,  914,  915,  994,
  995,  520,  521,  522,  523,  524,  525,  526,  527,  528,  529,  582,  583,
  924,  925,  948,  949,  530,  531,  532,  533,  534,  535,  536,  537,  538,
  539,  592,  593,  934,  935,  958,  959,  540,  541,  542,  543,  544,  545,
  546,  547,  548,  549,  584,  585,  944,  945,  588,  589,  550,  551,  552,
  553,  554,  555,  556,  557,  558,  559,  594,  595,  954,  955,  598,  599,
  560,  561,  562,  563,  564,  565,  566,  567,  568,  569,  586,  587,  964,
  965,  988,  989,  570,  571,  572,  573,  574,  575,  576,  577,  578,  579,
  596,  597,  974,  975,  998,  999,  600,  601,  602,  603,  604,  605,  606,
  607,  608,  609,  680,  681,  806,  807,  886,  887,  610,  611,  612,  613,
  614,  615,  616,  617,  618,  619,  690,  691,  816,  817,  896,  897,  620,
  621,  622,  623,  624,  625,  626,  627,  628,  629,  682,  683,  826,  827,
  868,  869,  630,  631,  632,  633,  634,  635,  636,  637,  638,  639,  692,
  693,  836,  837,  878,  879,  640,  641,  642,  643,  644,  645,  646,  647,
  648,  649,  684,  685,  846,  847,  688,  689,  650,  651,  652,  653,  654,
  655,  656,  657,  658,  659,  694,  695,  856,  857,  698,  699,  660,  661,
  662,  663,  664,  665,  666,  667,  668,  669,  686,  687,  866,  867,  888,
  889,  670,  671,  672,  673,  674,  675,  676,  677,  678,  679,  696,  697,
  876,  877,  898,  899,  700,  701,  702,  703,  704,  705,  706,  707,  708,
  709,  780,  781,  906,  907,  986,  987,  710,  711,  712,  713,  714,  715,
  716,  717,  718,  719,  790,  791,  916,  917,  996,  997,  720,  721,  722,
  723,  724,  725,  726,  727,  728,  729,  782,  783,  926,  927,  968,  969,
  730,  731,  732,  733,  734,  735,  736,  737,  738,  739,  792,  793,  936,
  937,  978,  979,  740,  741,  742,  743,  744,  745,  746,  747,  748,  749,
  784,  785,  946,  947,  788,  789,  750,  751,  752,  753,  754,  755,  756,
  757,  758,  759,  794,  795,  956,  957,  798,  799,  760,  761,  762,  763,
  764,  765,  766,  767,  768,  769,  786,  787,  966,  967,  988,  989,  770,
  771,  772,  773,  774,  775,  776,  777,  778,  779,  796,  797,  976,  977,
  998,  999
};

const uint16_t  BIN2DPD [1000] =
{
     0,    1,    2,    3,    4,    5,    6,    7,    8,    9,
    16,   17,   18,   19,   20,   21,   22,   23,   24,   25,
    32,   33,   34,   35,   36,   37,   38,   39,   40,   41,
    48,   49,   50,   51,   52,   53,   54,   55,   56,   57,
    64,   65,   66,   67,   68,   69,   70,   71,   72,   73,
    80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
    96,   97,   98,   99,  100,  101,  102,  103,  104,  105,
   112,  113,  114,  115,  116,  117,  118,  119,  120,  121,
    10,   11,   42,   43,   74,   75,  106,  107,   78,   79,
    26,   27,   58,   59,   90,   91,  122,  123,   94,   95,
   128,  129,  130,  131,  132,  133,  134,  135,  136,  137,
   144,  145,  146,  147,  148,  149,  150,  151,  152,  153,
   160,  161,  162,  163,  164,  165,  166,  167,  168,  169,
   176,  177,  178,  179,  180,  181,  182,  183,  184,  185,
   192,  193,  194,  195,  196,  197,  198,  199,  200,  201,
   208,  209,  210,  211,  212,  213,  214,  215,  216,  217,
   224,  225,  226,  227,  228,  229,  230,  231,  232,  233,
   240,  241,  242,  243,  244,  245,  246,  247,  248,  249,
   138,  139,  170,  171,  202,  203,  234,  235,  206,  207,
   154,  155,  186,  187,  218,  219,  250,  251,  222,  223,
   256,  257,  258,  259,  260,  261,  262,  263,  264,  265,
   272,  273,  274,  275,  276,  277,  278,  279,  280,  281,
   288,  289,  290,  291,  292,  293,  294,  295,  296,  297,
   304,  305,  306,  307,  308,  309,  310,  311,  312,  313,
   320,  321,  322,  323,  324,  325,  326,  327,  328,  329,
   336,  337,  338,  339,  340,  341,  342,  343,  344,  345,
   352,  353,  354,  355,  356,  357,  358,  359,  360,  361,
   368,  369,  370,  371,  372,  373,  374,  375,  376,  377,
   266,  267,  298,  299,  330,  331,  362,  363,  334,  335,
   282,  283,  314,  315,  346,  347,  378,  379,  350,  351,
   384,  385,  386,  387,  388,  389,  390,  391,  392,  393,
   400,  401,  402,  403,  404,  405,  406,  407,  408,  409,
   416,  417,  418,  419,  420,  421,  422,  423,  424,  425,
   432,  433,  434,  435,  436,  437,  438,  439,  440,  441,
   448,  449,  450,  451,  452,  453,  454,  455,  456,  457,
   464,  465,  466,  467,  468,  469,  470,  471,  472,  473,
   480,  481,  482,  483,  484,  485,  486,  487,  488,  489,
   496,  497,  498,  499,  500,  501,  502,  503,  504,  505,
   394,  395,  426,  427,  458,  459,  490,  491,  462,  463,
   410,  411,  442,  443,  474,  475,  506,  507,  478,  479,
   512,  513,  514,  515,  516,  517,  518,  519,  520,  521,
   528,  529,  530,  531,  532,  533,  534,  535,  536,  537,
   544,  545,  546,  547,  548,  549,  550,  551,  552,  553,
   560,  561,  562,  563,  564,  565,  566,  567,  568,  569,
   576,  577,  578,  579,  580,  581,  582,  583,  584,  585,
   592,  593,  594,  595,  596,  597,  598,  599,  600,  601,
   608,  609,  610,  611,  612,  613,  614,  615,  616,  617,
   624,  625,  626,  627,  628,  629,  630,  631,  632,  633,
   522,  523,  554,  555,  586,  587,  618,  619,  590,  591,
   538,  539,  570,  571,  602,  603,  634,  635,  606,  607,
   640,  641,  642,  643,  644,  645,  646,  647,  648,  649,
   656,  657,  658,  659,  660,  661,  662,  663,  664,  665,
   672,  673,  674,  675,  676,  677,  678,  679,  680,  681,
   688,  689,  690,  691,  692,  693,  694,  695,  696,  697,
   704,  705,  706,  707,  708,  709,  710,  711,  712,  713,
   720,  721,  722,  723,  724,  725,  726,  727,  728,  729,
   736,  737,  738,  739,  740,  741,  742,  743,  744,  745,
   752,  753,  754,  755,  756,  757,  758,  759,  760,  761,
   650,  651,  682,  683,  714,  715,  746,  747,  718,  719,
   666,  667,  698,  699,  730,  731,  762,  763,  734,  735,
   768,  769,  770,  771,  772,  773,  774,  775,  776,  777,
   784,  785,  786,  787,  788,  789,  790,  791,  792,  793,
   800,  801,  802,  803,  804,  805,  806,  807,  808,  809,
   816,  817,  818,  819,  820,  821,  822,  823,  824,  825,
   832,  833,  834,  835,  836,  837,  838,  839,  840,  841,
   848,  849,  850,  851,  852,  853,  854,  855,  856,  857,
   864,  865,  866,  867,  868,  869,  870,  871,  872,  873,
   880,  881,  882,  883,  884,  885,  886,  887,  888,  889,
   778,  779,  810,  811,  842,  843,  874,  875,  846,  847,
   794,  795,  826,  827,  858,  859,  890,  891,  862,  863,
   896,  897,  898,  899,  900,  901,  902,  903,  904,  905,
   912,  913,  914,  915,  916,  917,  918,  919,  920,  921,
   928,  929,  930,  931,  932,  933,  934,  935,  936,  937,
   944,  945,  946,  947,  948,  949,  950,  951,  952,  953,
   960,  961,  962,  963,  964,  965,  966,  967,  968,  969,
   976,  977,  978,  979,  980,  981,  982,  983,  984,  985,
   992,  993,  994,  995,  996,  997,  998,  999, 1000, 1001,
  1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017,
   906,  907,  938,  939,  970,  971, 1002, 1003,  974,  975,
   922,  923,  954,  955,  986,  987, 1018, 1019,  990,  991,
    12,   13,  268,  269,  524,  525,  780,  781,   46,   47,
    28,   29,  284,  285,  540,  541,  796,  797,   62,   63,
    44,   45,  300,  301,  556,  557,  812,  813,  302,  303,
    60,   61,  316,  317,  572,  573,  828,  829,  318,  319,
    76,   77,  332,  333,  588,  589,  844,  845,  558,  559,
    92,   93,  348,  349,  604,  605,  860,  861,  574,  575,
   108,  109,  364,  365,  620,  621,  876,  877,  814,  815,
   124,  125,  380,  381,  636,  637,  892,  893,  830,  831,
    14,   15,  270,  271,  526,  527,  782,  783,  110,  111,
    30,   31,  286,  287,  542,  543,  798,  799,  126,  127,
   140,  141,  396,  397,  652,  653,  908,  909,  174,  175,
   156,  157,  412,  413,  668,  669,  924,  925,  190,  191,
   172,  173,  428,  429,  684,  685,  940,  941,  430,  431,
   188,  189,  444,  445,  700,  701,  956,  957,  446,  447,
   204,  205,  460,  461,  716,  717,  972,  973,  686,  687,
   220,  221,  476,  477,  732,  733,  988,  989,  702,  703,
   236,  237,  492,  493,  748,  749, 1004, 1005,  942,  943,
   252,  253,  508,  509,  764,  765, 1020, 1021,  958,  959,
   142,  143,  398,  399,  654,  655,  910,  911,  238,  239,
   158,  159,  414,  415,  670,  671,  926,  927,  254,  255
};

const unsigned char  DECCHAR2BIN [64] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char  DECBIN2CHAR [10] =
{
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39
};

const int hflmaxdigit [5] = { 0,   6,  14, 0,   28 };
const int dflmaxdigit [5] = { 0,   7,  18, 0,   34 };
const int dflsigbits  [5] = { 0,  20,  18, 0,   14 };
const int dflrbebits  [5] = { 0,   6,   8, 0,   12 };
const int dflexpmax   [5] = { 0, 101, 398, 0, 6176 };
const int dflrbefac   [5] = { 0,  64, 256, 0, 4096 };

#define MAXOVERFLOW     16777216LL      // (16M)
#define ARRAYMAX        8
#define ARRAYPAD        4

void arraydiv( unsigned long long *ltab, int divisor, int ntab, unsigned int *rem )
{
    long long temp1;
    long long work1;
    int i;

    work1 = ltab[0];

    for (i=0; i < ntab; i++)
    {
        ltab[i] = work1 / divisor;
        temp1   = work1 % divisor;

        if (i < ntab)
            work1 = (temp1 << 32) + ltab[i + 1];
    }

    *rem = (unsigned int)temp1;
}

void arrayadd( unsigned long long *tab1, unsigned long long *tab2, int ntab1, int ntab2 )
{
    int        i;
    int        tab2ctr  = ntab2;
    long long  carry    = 0;

    for (i = ntab1 - 1; i >= 0; i--)
    {
        if (tab2ctr < 1)
            tab1[i] = tab1[i] + carry;
        else
        {
            tab1[i] = tab1[i] + tab2[i] + carry;
            tab2ctr--;
        }

        carry = tab1[i] >> 32;

        if (carry)
            tab1[i] = (tab1[i] << 32) >> 32;
    }

    tab1[0] += carry;
}

void arrayaddint( unsigned long long *tab1, int incr, int ntab )
{
    int i;
    long long carry = 0;

    tab1[ntab - 1] += incr;
    carry = tab1[ntab - 1] >> 32;

    if (carry)
        tab1[ntab - 1] = (tab1[ntab - 1] << 32) >> 32;

    i = ntab - 2;

    while (carry && i > 0)
    {
        tab1[i] += carry;
        carry = tab1[i] >> 32;

        if (carry)
            tab1[i] = (tab1[i] << 32) >> 32;

        i--;
    }

    tab1[0] += carry;
}

void arraymlt( unsigned long long *ltab, int mult, int ntab )
{
    int i;
    long long carry = 0;

    for (i = ntab - 1; i >= 0; i--)
    {
        ltab[i] = ltab[i] * mult + carry;
        carry = ltab[i] >> 32;

        if (carry)
            ltab[i] = (ltab[i] << 32) >> 32;
    }

    ltab[0] += carry;
}

void arrayshiftright( unsigned long long *ltab, int ntab, int shift )
{
    int i;
    unsigned long long carry = 0;
    unsigned long long ncarry;

    while (shift > 32)
    {
        for (i = ntab - 1; i > 0; i--)
            ltab[i] = ltab[i - 1];

        shift -= 32;
        ltab[0] = 0;
    }

    for (i=0; i < ntab; i++)
    {
        ncarry = ((ltab[i] << (32 - shift)) << 32) >> 32;
        ltab[i] = (ltab[i] >> shift) + carry;
        carry = ncarry;
    }

    ltab[ntab - 1] += carry;
}

void arrayshiftleft( unsigned long long *ltab, int ntab, int shift )
{
    int i;
    int tabctr = ntab;
    unsigned long long carry = 0;
    unsigned long long ncarry;

    while (shift > 32)
    {
        for (i=0; i < ntab - 1; i++)
            ltab[i] = ltab[i + 1];

        shift -= 32;
        ltab[ntab - 1] = 0;
        tabctr--;
    }

    for (i = tabctr - 1; i >= 0; i--)
    {
        ncarry = ltab[i] >> (32 - shift);
        ltab[i] = (((ltab[i] << shift) << 32) >> 32) + carry;
        carry = ncarry;
    }

    ltab[0] += carry;
}

int dflexp( int expword, int *lmdrtn, int dflwords )
{
    int exp;
    int lmd;
    int exp1;
    int cbits;

    cbits = expword >> dflrbebits[dflwords];

    if (cbits < 24)
    {
        lmd = cbits % 8;
        exp1 = cbits / 8;
    }
    else
    {
        if (cbits % 2)
            lmd = 9;
        else
            lmd = 8;

        exp1 = (cbits >> 1) % 2;
    }

    exp = (exp1 << dflrbebits[dflwords]) + (expword % dflrbefac[dflwords]) - dflexpmax[dflwords];
    *lmdrtn = lmd;

    return exp;
}

void getdigitcounts( unsigned long long *ltab, int ntab,
                     int *lzrtn, int *tzrtn, int *ndrtn, int *bitrtn )
{
    int i;

    int lzctr = 0;
    int tzctr = 0;
    int ndctr = 0;
    int nbit  = 0;
    int nlen  = 4 * ntab;

    unsigned char numwork [ARRAYMAX * 4];
    unsigned char *cptr;
    unsigned char wchar;

    cptr = numwork;

    for (i = ntab - 1; i >= 0; i--)
    {
        memcpy(cptr,&ltab[i],4);
        cptr += 4;
    }

    for (i=0; i < nlen; i++)
    {
        if (numwork[i] == 0x00)
        {
            tzctr += 2;
            continue;
        }

        if ((numwork[i] & 0x0f) == 0x00)
            tzctr++;

        break;
    }

    for (i = nlen - 1; i >= 0; i--)
    {
        if (numwork[i] == 0x00)
        {
            lzctr += 2;
            continue;
        }

        if ((numwork[i] & 0xf0) == 0x00)
            lzctr++;

        wchar = numwork[i];

        while (wchar != 0x00)
        {
            wchar = wchar >> 1;
            nbit++;
        }

        break;
    }

    *bitrtn = nbit;
    *lzrtn  = lzctr;
    *tzrtn  = tzctr;
    *ndrtn  = ntab * 8 - lzctr - tzctr;
}

void dfl2hfl( unsigned int * dfltab, unsigned int * hfltab, int dflwords, int hflwords )
{
    int  i;
    int  j;
    int  k;
    int  decnum;
    int  decctr;
    int  hexp;
    int  ndigit;
    int  maxdigit;
    int  nbithfl;
    int  ndpd;
    int  neg      = 0;
    int  exp;
    int  dexp;
    int  expctr;
    int  delta;
    int  skip     = 1;
    int  expword;
    int  ndec;
    int  nbitdec;
    int  wk1;
    int  lmd;
    int  temp1;
    int  tbits;
    int  tzero    = 0;
    int  tflag    = 0;
    int  lzero    = 0;

    unsigned int  rem;
    long long     wk;

    unsigned long long  dec [ARRAYMAX];
    unsigned long long  hfl [ARRAYMAX];
    unsigned long long  wrk [ARRAYMAX];

    unsigned char  decwork [6210];
    unsigned char  decchar [6211];
    char           dectemp [5];

    wk1 = dfltab[0];
    wk1 = (wk1 << 1) >> 1;

    if (wk1 != dfltab[0])
        neg = 1;

    dfltab[0] = wk1;

    memset( dec, 0x00, sizeof( dec ));

    for (i=0; i < dflwords; i++)
        dec[i + ARRAYPAD] = (long long)dfltab[i];

    switch (dflwords)
    {
    case 1:

        expword = (int)(dec[ARRAYPAD] / (1024*1024));
        dec[ARRAYPAD] = dec[ARRAYPAD] % (1024*1024);
        break;

    case 2:

        expword = (int)(dec[ARRAYPAD] / (256*1024));
        dec[ARRAYPAD] = dec[ARRAYPAD] % (256*1024);
        break;

    case 4:

        expword = (int)(dec[ARRAYPAD] / (16*1024));
        dec[ARRAYPAD] = dec[ARRAYPAD] % (16*1024);
        break;
    }

    exp  = dflexp( expword, &lmd, dflwords );
    lmd  = lmd * 1000;
    ndec = ARRAYPAD + dflwords;

    getdigitcounts( dec, ndec, &lzero, &tzero, &ndigit, &nbitdec );

    memset( hfl,     0x00, sizeof( hfl     ));
    memset( decwork, 0x00, sizeof( decwork ));

    decctr = 0;

    if (ndigit > 0)
    {
        tbits = (ndigit + tzero - 1) * 4 + nbitdec;
        ndpd = tbits / 10;

        if (tbits % 10)
            ndpd++;

        if (exp > 0)
        {
            decctr = exp;
            exp = 0;
        }

        for (i=0; i < ndpd; i++)
        {
            arraydiv( dec, 1024, ndec, &rem );

            decnum = DPD2BIN[rem];
            sprintf( dectemp, "%03d", decnum );

            for (k=2; k >= 0; k--)
            {
                decwork[decctr] = DECCHAR2BIN[(unsigned int)dectemp[k]];
                decctr++;
            }
        }
    }

    if (decwork[decctr - 1] == 0x00)
    {
        decctr--;

        if (decwork[decctr - 1] = 0x00)
            decctr--;
    }

    hexp = 0;

    for (i = decctr - 1, j = 0; i >= 0; i--, j++)
        decchar[j] = DECBIN2CHAR[decwork[i]];

    decchar[j] = 0x00;
    dexp = exp;
    maxdigit = hflmaxdigit[hflwords];

    for (i=0; i < decctr; i++)
    {
        temp1 = (int)decwork[i];
        memset( wrk, 0x00, sizeof( wrk ));
        wrk[ARRAYMAX - 1] = temp1;

        for (j=0; j < i; j++)
            arraymlt( wrk, 10, ARRAYMAX );

        arrayadd( hfl, wrk, ARRAYMAX, ARRAYMAX );

        while (hfl[0] > MAXOVERFLOW)
        {
            arraydiv( hfl, 16, ARRAYMAX, &rem );

            if (rem > 7)
                arrayaddint( hfl, 1, ARRAYMAX );

            if (dexp > 0)
                hexp++;
        }
    }

    if (exp < 0)
    {
        exp    = abs( exp );
        expctr = exp;

        getdigitcounts( hfl, ARRAYMAX, &lzero, &tzero, &ndigit, &nbithfl );

        if (ndigit < maxdigit + 4)
        {
            delta = maxdigit + 4 - ndigit;
            arrayshiftleft( hfl, ARRAYMAX, delta * 4 );
            hexp -= delta;
        }

        for (i=0; i < expctr; i++)
        {
            arrayshiftleft( hfl, ARRAYMAX, 4 );
            hexp--;

            while (hfl[0] > MAXOVERFLOW)
            {
                arraydiv( hfl, 10, ARRAYMAX, &rem );

                if (rem > 4)
                    arrayaddint( hfl, 1, ARRAYMAX );

                exp--;
            }
        }

        for (i=0; i < exp; i++)
        {
            arraydiv( hfl, 10, ARRAYMAX, &rem );

            if (rem > 4)
                arrayaddint( hfl, 1, ARRAYMAX );
        }
    }

    getdigitcounts( hfl, ARRAYMAX, &lzero, &tzero, &ndigit, &nbithfl );

    if (ndigit > maxdigit)
    {
        delta = ndigit - maxdigit;

        for (i=0; i < delta; i++)
        {
            arraydiv( hfl, 16, ARRAYMAX, &rem );

            if (rem > 7)
                arrayaddint( hfl, 1, ARRAYMAX );

            ndigit--;
            hexp++;
        }

        getdigitcounts( hfl, ARRAYMAX, &lzero, &tzero, &ndigit, &nbithfl );
    }

    arrayshiftleft( hfl, ARRAYMAX, (lzero - 2) * 4 );

    hexp += 64;
    hexp += ndigit;

    wk = hexp << 24;
    hfl[0] += wk;

    if (neg)
    {
        wk = 1 << 31;
        hfl[0] += wk;
    }

    if (hflwords == 4)
    {
        temp1 = (hfl[2] << 32) >> 56;
        hfl[2] = hfl[2] >> 8;
        hfl[3] = (hfl[3] >> 8) + temp1;
    }

    for (i=0; i < hflwords; i++)
        hfltab[i] = (int)hfl[i];
}

void hfl2dfl( unsigned int *hfltab, unsigned int *dfltab, int hflwords, int dflwords )
{
    int  i;
    int  j;

    int  decctr;
    int  neg        = 0;
    int  fnzero;
    int  nhfl;
    int  ndec;
    int  nbithfl;
    int  ndpdctr;
    int  ndpd;
    int  ndigits;
    int  paddec;
    int  lzero;
    int  tzero;
    int  maxdigits;
    int  maxdigitdfl;
    int  temp1;
    int  temp2;
    int  delta;
    int  expctr;
    int  expword;
    int  cbits;
    int  exp;
    int  dexp;

    unsigned int  exp1;
    unsigned int  rem;
    unsigned int  rbe;
    unsigned int  lmd;

    unsigned long long  hfl [ARRAYMAX];
    unsigned long long  dec [ARRAYMAX];
    unsigned long long  wrk [ARRAYMAX];

    unsigned char decwork    [6210];
    unsigned char decworkout [6210];

    exp = hfltab[0] >> 24;
    hfltab[0] = (hfltab[0] << 8) >> 8;

    if (exp > 127)
        neg = 1;

    exp = exp % 128;
    exp -= 64;
    dexp = 0;

    memset( hfl, 0x00, sizeof( hfl ));

    for (i=0; i < hflwords; i++)
        hfl[i + ARRAYPAD] = (long long)hfltab[i];

    if (hflwords == 4)
    {
        temp1 = (hfl[ARRAYPAD] << 56) >> 32;

        hfl[ARRAYPAD] = hfl[ARRAYPAD] >> 8;

        temp2 = (hfl[ARRAYPAD + 1] << 56) >> 32;

        hfl[ARRAYPAD + 1] = (hfl[ARRAYPAD + 1] >> 8) + temp1;
        hfl[ARRAYPAD + 2] = (hfl[ARRAYPAD + 2] << 40) >> 40;
        hfl[ARRAYPAD + 2] = hfl[ARRAYPAD + 2] + temp2;
    }

    nhfl = hflwords + ARRAYPAD;
    getdigitcounts( hfl, nhfl, &lzero, &tzero, &ndigits, &nbithfl );

    if (tzero > 0)
    {
        arrayshiftright( hfl, nhfl, tzero * 4 );
        getdigitcounts ( hfl, nhfl, &lzero, &tzero, &ndigits, &nbithfl );
    }

    maxdigits   = hflmaxdigit[hflwords];
    maxdigitdfl = dflmaxdigit[dflwords];

    if (exp > ndigits)
    {
        if (maxdigits  < 4 + maxdigitdfl)
        {
            paddec = maxdigitdfl + 4 - maxdigits;

            for (i=0; i < paddec; i++)
                arraymlt( hfl, 10, nhfl );

            dexp -= paddec;
        }

        delta  = exp - ndigits;
        expctr = delta;

        for (i=0; i < delta; i++)
        {
            arrayshiftleft( hfl, nhfl, 4 );

            while (hfl[0] > MAXOVERFLOW)
            {
                arraydiv( hfl, 10, nhfl, &rem );

                if (rem > 4)
                    arrayaddint( hfl, 1, nhfl );

                dexp++;
                expctr--;
            }
        }

        for (i=0; i < expctr; i++)
        {
            arraydiv( hfl, 10, nhfl, &rem );

            if (rem > 4)
                arrayaddint( hfl, 1, nhfl );

            dexp++;
        }
    }
    else if (exp < ndigits)
    {
        if (maxdigits  < 4 + maxdigitdfl)
        {
            paddec = maxdigitdfl + 4 - maxdigits;

            for (i=0; i < paddec; i++)
                arraymlt( hfl, 10, nhfl );

            dexp -= paddec;
        }

        delta = ndigits - exp;
        expctr = delta;

        for (i=0; i < delta; i++)
        {
            arraymlt( hfl, 10, nhfl );

            while (hfl[0] > MAXOVERFLOW)
            {
                arraydiv( hfl, 16, nhfl, &rem );

                if (rem > 7)
                    arrayaddint( hfl, 1, nhfl );

                expctr--;
            }
            dexp--;
        }

        for (i=0; i < expctr; i++)
        {
            arraydiv( hfl, 16, nhfl, &rem );

            if (rem > 7)
                arrayaddint( hfl, 1, nhfl );
        }
    }

    getdigitcounts( hfl, nhfl, &lzero, &tzero, &ndigits, &nbithfl );
    decctr = 0;

    for (i=0; i < ndigits + tzero + paddec; i++)
    {
        arraydiv( hfl, 10, nhfl, &rem );
        decwork[decctr] = (unsigned char)rem;
        decctr++;
    }

    if (hfl[nhfl - 1] / 10)
    {
        decwork[decctr] = hfl[nhfl - 1] / 10;
        decctr++;
    }

    maxdigits = dflmaxdigit[dflwords];

    if (decctr > maxdigits)
    {
        delta = decctr - maxdigits;

        for (i=0; i < decctr - 1; i++)
            decwork[i] = decwork[i + delta];

        memset( decwork + maxdigits, 0x00, sizeof( decwork ) - decctr );
        dexp += delta;
    }

    j      = 0;
    fnzero = 0;

    for (i=0; i < decctr; i++)
    {
        if (decwork[i] == 0x00 && !fnzero)
        {
            dexp++;
            continue;
        }

        decworkout[j] = decwork[i];
        fnzero = 1;
        j++;
    }

    decctr = j;
    ndpd   = decctr / 3;
    delta  = decctr % 3;

    if (delta)
    {
        ndpd++;
        decworkout[decctr] = 0x00;

        if (delta == 2)
            decworkout[decctr] = 0x00;
    }

    memset( dec, 0x00, sizeof( dec ));
    ndec = dflwords + ARRAYPAD;

    for (ndpdctr = 0; ndpdctr < ndpd; ndpdctr++)
    {
        i = ndpdctr * 3;

        temp1  = ((unsigned int)decworkout[i + 0] *   1);
        temp1 += ((unsigned int)decworkout[i + 1] *  10);
        temp1 += ((unsigned int)decworkout[i + 2] * 100);

        temp2 = BIN2DPD[temp1];

        if (ndpdctr > 0)
        {
            memset( wrk, 0x00, sizeof( wrk ));
            wrk[ndec - 1] = temp2;
            arrayshiftleft( wrk, ndec, 10 * ndpdctr );
            arrayadd( dec, wrk, ndec, ndec );
        }
        else
            dec[ndec - 1] = temp2;
    }

    if (decctr < dflmaxdigit[dflwords])
        lmd = 0;
    else
        lmd = (unsigned int)decwork[decctr - 1];

    dexp += dflexpmax[dflwords];
    rbe = dexp % dflrbefac[dflwords];
    exp1 = dexp / dflrbefac[dflwords];

    if (lmd < 8)
        cbits = exp1 * 8 + lmd;
    else if (lmd == 8)
        cbits = 24 + exp1 * 2;
    else
        cbits = 24 + exp1 * 2 + 1;

    if (neg)
        cbits += 64;

    expword = rbe + (cbits   << dflrbebits[dflwords]);
    expword =       (expword << dflsigbits[dflwords]);
    dec[ARRAYPAD] += expword;

    for (i=0; i < dflwords; i++)
        dfltab[i] = (int)dec[i + ARRAYPAD];
}

/*-------------------------------------------------------------------*/
/* 010A PFPO  - Perform Floating Point Operation                 [E] */
/*-------------------------------------------------------------------*/
DEF_INST( perform_floating_point_operation )
{
    S64  gr0;

    char creg0[8];

    unsigned int  ftab   [4];
    unsigned int  tabout [4];

    int opcode;
    int numout = 0;
    int i0, i2, i4, i6;

    i0  = FPR2I( 0 );
    i2  = FPR2I( 2 );
    i4  = FPR2I( 4 );
    i6  = FPR2I( 6 );

    E( inst, regs );

    gr0 = (S64) regs->GR_G( 0 );

    ftab[0] = (unsigned int)(regs->fpr[i4 + 0]);
    ftab[1] = (unsigned int)(regs->fpr[i4 + 1]);
    ftab[2] = (unsigned int)(regs->fpr[i6 + 0]);
    ftab[3] = (unsigned int)(regs->fpr[i6 + 1]);

#if 0
    MSGBUF( msgbuff, "gr0 = %016llx, ftab[0] = %08x, ftab[1] = %08x, ftab[2] = %08x, ftab[3] = %08x",
                      gr0,           ftab[0],        ftab[1],        ftab[2],        ftab[3] );
    WRMSG( HHC90700, "I", msgbuff );
#endif

    memcpy( creg0, (void*) &gr0, 8 );

    switch (creg0[2])
    {
    case 0x00:  opcode =  1;  break;
    case 0x01:  opcode =  2;  break;
    case 0x02:  opcode =  3;  break;
    case 0x08:  opcode =  4;  break;
    case 0x09:  opcode =  5;  break;
    case 0x0a:  opcode =  6;  break;
    default:    opcode = -1;  break;
    }

    switch (creg0[1])
    {
    case 0x00:  opcode += 10; break;
    case 0x01:  opcode += 20; break;
    case 0x02:  opcode += 30; break;
    case 0x08:  opcode += 40; break;
    case 0x09:  opcode += 50; break;
    case 0x0a:  opcode += 60; break;
    default:    numout =  -1; break;
    }

    switch (opcode)
    {
    case 14: hfl2dfl( ftab, tabout, 1, 1 ); numout =  1; break;
    case 15: hfl2dfl( ftab, tabout, 1, 2 ); numout =  2; break;
    case 16: hfl2dfl( ftab, tabout, 1, 3 ); numout =  4; break;

    case 24: hfl2dfl( ftab, tabout, 2, 1 ); numout =  1; break;
    case 25: hfl2dfl( ftab, tabout, 2, 2 ); numout =  2; break;
    case 26: hfl2dfl( ftab, tabout, 2, 4 ); numout =  4; break;

    case 34: hfl2dfl( ftab, tabout, 4, 1 ); numout =  1; break;
    case 35: hfl2dfl( ftab, tabout, 4, 2 ); numout =  2; break;
    case 36: hfl2dfl( ftab, tabout, 4, 4 ); numout =  4; break;

    case 41: dfl2hfl( ftab, tabout, 1, 1 ); numout =  1; break;
    case 42: dfl2hfl( ftab, tabout, 1, 2 ); numout =  2; break;
    case 43: dfl2hfl( ftab, tabout, 1, 4 ); numout =  4; break;

    case 51: dfl2hfl( ftab, tabout, 2, 1 ); numout =  1; break;
    case 52: dfl2hfl( ftab, tabout, 2, 2 ); numout =  2; break;
    case 53: dfl2hfl( ftab, tabout, 2, 4 ); numout =  4; break;

    case 61: dfl2hfl( ftab, tabout, 4, 1 ); numout =  1; break;
    case 62: dfl2hfl( ftab, tabout, 4, 2 ); numout =  2; break;
    case 63: dfl2hfl( ftab, tabout, 4, 4 ); numout =  4; break;

    default:                                numout = -1; break;
    }

    if (numout > 0)
    {
        regs->fpr[i0] = tabout[0];

        if (numout == 1)
            regs->fpr[i0 + 1] = 0;
        else
        {
            regs->fpr[i0 + 1] = tabout[1];

            if (numout > 2)
            {
                regs->fpr[i2 + 0] = tabout[2];
                regs->fpr[i2 + 1] = tabout[3];
            }
        }
    }
    else
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

} /* end DEF_INST( perform_floating_point_operation ) */

#endif /* defined( FEATURE_044_PFPO_FACILITY ) */

#if !defined( _GEN_ARCH )
  #if defined(           _ARCH_NUM_1 )
    #define   _GEN_ARCH  _ARCH_NUM_1
    #include "pfpo.c"
  #endif
  #if defined(           _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH  _ARCH_NUM_2
    #include "pfpo.c"
  #endif
#endif /* !defined( _GEN_ARCH ) */
