/* PFPO.C       (c) Copyright Roger Bowler, 2009-2012                */
/*              Perform Floating Point Operation instruction         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*              (c) Copyright Bernard van der Helm, 2009-2011        */
/*              Noordwijkerhout, The Netherlands                     */

/*              (C) Copyright Bob Wood, 2018-2021                    */

/*-------------------------------------------------------------------*/
/* This module implements the Perform Floating Point Operation       */
/* instruction described in the manual SA22-7832-05.                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HENGINE_DLL_
#define _PFPO_C_

#include "hercules.h"
#include "opcode.h"

#include "decimal128.h"
#include "decimal64.h"
#include "decimal32.h"
#include "decPacked.h"

#if defined( FEATURE_044_PFPO_FACILITY )

#define GR0_IS(           _optbits )    ((_optbits) & 0x80)
#define GR0_AE(           _optbits )    ((_optbits) & 0x40)
#define GR0_TR_HFP_OVER(  _optbits )    ((_optbits) & 0x20)
#define GR0_TR_HFP_UNDER( _optbits )    ((_optbits) & 0x10)
#define GR0_TR_BFP_RSRVD( _optbits )    ((_optbits) & 0x30)
#define GR0_TR_DQPC(      _optbits )    ((_optbits) & 0x20)
#define GR0_TR_DPQC(      _optbits )    ((_optbits) & 0x10)
#define GR0_RM(           _optbits )    ((_optbits) & 0x0F)

const uint16_t DPD2BIN[1024]={    0,    1,    2,    3,    4,    5,    6,    7,
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
  998,  999};

const uint16_t BIN2DPD[1000]={
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
   158,  159,  414,  415,  670,  671,  926,  927,  254,  255};

const int  hflmaxdigit [5] = { 0,   6,   14, 0,    28 };
const int  dflmaxdigit [5] = { 0,   7,   16, 0,    34 };
const int  bflmaxdigit [5] = { 0,  23,   52, 0,   112 };
const int  dflsigbits  [5] = { 0,  20,   18, 0,    14 };
const int  dflrbebits  [5] = { 0,   6,    8, 0,    12 };
const int  dflexpmax   [5] = { 0, 101,  398, 0,  6176 };
const int  dflrbefac   [5] = { 0,  64,  256, 0,  4096 };
const int  bflexpbits  [5] = { 0,   9,   12, 0,    16 };
const int  bflexpbias  [5] = { 0, 127, 1023, 0, 16383 };
const int  bflexpmax   [5] = { 0, 255, 2047, 0, 32767 };

#define ARRAYMAX    7
#define ARRAYPAD    3

/***************************************************************/
/*   arraydiv:  Divide an array of integer values by a single  */
/*              integer value.  This is essentially using long */
/*              division where each integer is treated as a    */
/*              single digit.  The routine is needed because   */
/*              number longer than 64 bits are used to main-   */
/*              tain precision.  The integers are stored in    */
/*              an array of long long values to deal with      */
/*              carry issues.                                  */
/***************************************************************/
void arraydiv(unsigned int *ltab,int divisor,int ntab,unsigned int *rem)
{
  unsigned long long temp1 = 0;
  unsigned long long work1;
  unsigned long long divisort;
  unsigned long long dividend;
  int i;
  work1 = (unsigned long long)ltab[0];
  divisort = (unsigned long long)divisor;
  for (i = 0;i < ntab;i++)
  {
    dividend = work1 / divisort;
    ltab[i] = (unsigned int)(dividend & 0x00000000ffffffffll);
    temp1 = work1 % divisort;
    if ((i + 1 ) < ntab)
      work1 = (temp1 << 32) + (unsigned long long)ltab[i + 1];
  }
  *rem = (unsigned int)temp1;
  return;
}

/***************************************************************/
/*   arrayadd:  Add an array of integer values to another array*/
/*              of integer values.  The integers are stored as */
/*              long longs to avoid carry issues.              */
/***************************************************************/
void arrayadd(unsigned int *tab1,unsigned int *tab2,int ntab1, int ntab2)
{
  unsigned long long carry = 0;
  unsigned long long op1;
  unsigned long long op2;
  int i;
  int tab2ctr = ntab2;
  for (i = ntab1 - 1;i >= 0;i--)
  {
    op1 = (unsigned long long)tab1[i];
    if (tab2ctr < 1)
      op1 += carry;
    else
    {
      op2 = (unsigned long long)tab2[i];
      op1 += op2 + carry;
      tab2ctr--;
    }
    carry = op1 >> 32;
    tab1[i] = (unsigned int)(op1 & 0x00000000ffffffffll);
    if (carry == 0 && tab2ctr < 1)
      break;
  }
  tab1[0] += (unsigned int)carry;
  return;
}

/***************************************************************/
/*   arrayaddint:  Add an integer value to an array of integer */
/*                 values.  The array is stored as long longs  */
/*                 to simplify carry issues.                   */
/***************************************************************/
void arrayaddint(unsigned int *tab1,int incr,int ntab)
{
  unsigned long long carry = 0;
  unsigned long long op1;
  unsigned long long op2;
  int i;
  op2 = (unsigned long long)incr;
  op1 = (unsigned long long)tab1[ntab - 1];
  op1 += op2;
  carry = op1 >> 32;
  op1 &= 0x00000000ffffffffll;
  tab1[ntab - 1] = (unsigned int)op1;
  i = ntab - 2;
  while (carry && i > 0)
  {
    op1 = (unsigned long long)tab1[i];
    op1 += carry;
    carry = op1 >> 32;
    op1 &= 0x00000000ffffffffll;
    tab1[i] = (unsigned int)op1;
    i--;
  }
  tab1[0] += (unsigned int)carry;
  return;
}

/***************************************************************/
/*   arraymlt:  Multiply an array of integer values by a single*/
/*              integer value.  This is done using long        */
/*              multiplication where each integer is treated as*/
/*              a single digit.                                */
/***************************************************************/
void arraymlt(unsigned int *ltab,int mult,int ntab)
{
  int i;
  unsigned long long carry = 0;
  unsigned long long op1;
  unsigned long long op2 = (unsigned long long)mult;
  for (i = ntab - 1;i >= 0;i--)
  {
    op1 = (unsigned long long)ltab[i];
    op1 = (op1 * op2) + carry;
    carry = op1 >> 32;
    op1 &= 0x00000000ffffffffll;
    ltab[i] = (unsigned int)op1;
  }
  ltab[0] += (unsigned int)carry;
  return;
}

/***************************************************************/
/*   arrayshiftright:  Shift an array of integers a specified  */
/*              number of bits.                                */
/***************************************************************/
void arrayshiftright(unsigned int *ltab,int ntab,int shift, unsigned int *remtab)
{
  int i;
  int rx = ntab - 1;
  int shiftctr;
  int shiftword;
  unsigned int temp1;
  unsigned int temp2;
  shiftword = shift / 32;
  memset(remtab, 0x00, ntab * sizeof(int));
  if (shiftword > 0)
  {
    for (i = ntab - 1; i > ntab - shiftword - 1; i--)
    {
      remtab[rx] = ltab[i];
      rx--;
    }
    for (i = ntab - 1; i >= shiftword; i--)
      ltab[i] = ltab[i - shiftword];
    for (; i >= 0; i--)
      ltab[i] = 0;
    shift -= 32 * shiftword;
  }
  if (shift == 0)
    return;
  shiftctr = 32 - shift;
  temp1 = 0;
  for (i = 0; i < ntab; i++)
  {
    temp2 = ltab[i] << shiftctr;
    if (i == ntab - 1)
      remtab[rx] = temp2 >> shiftctr;
    ltab[i] >>= shift;
    ltab[i] += temp1;
    temp1 = temp2;
  }
  return;
}

/***************************************************************/
/*   arrayshiftleft:   Shift an array of integers a specified  */
/*              number of bits.                                */
/***************************************************************/
void arrayshiftleft(unsigned int *ltab,int ntab,int shift)
{
  int i;
  int shiftctr;
  int shiftword;
  int wordnum;
  unsigned int temp1;
  unsigned int temp2;
  shiftword = shift / 32;
  if (shiftword > 0)
  {
    wordnum = ntab - shiftword;
    for (i = 0; i < wordnum; i++)
      ltab[i] = ltab[i + shiftword];
    for (; i < ntab; i++)
      ltab[i] = 0;
    shift -= 32 * shiftword;
  }
  temp1 = 0;
  if (shift == 0)
    return;
  shiftctr = 32 - shift;
  for (i = ntab - 1; i >= 0; i--)
  {
    temp2 = ltab[i] >> shiftctr;
    ltab[i] <<= shift;
    ltab[i] += temp1;
    temp1 = temp2;
  }
  return;
}

/***************************************************************/
/*   dflexp:  Extract the exponent value for a decfloat number.*/
/***************************************************************/
int dflexp(int expword,int *lmdrtn,int dflwords)
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

int getlzerobits(unsigned int *ltab, int ntab)
{
  int bitctr = 0;
  int i;
  int bctr = 0;
  unsigned int temp1;
  for (i = 0; i < ntab; i++)
  {
    if (ltab[i] == 0)
    {
      bitctr += 32;
      continue;
    }
    temp1 = ltab[i];
    while (temp1 > 0)
    {
      bctr++;
      temp1 >>= 1;
    }
    bitctr += (32 - bctr);
    break;
  }
  return bitctr;
}

void roundarray(unsigned int *ntab, int nword, int roundrule, int rem, int base, int neg,int type, int mid)
{
  int half = base / 2;
  switch (roundrule)
  {
  case 0:
    if (rem > half)
      arrayaddint(ntab, 1, nword);
    else
      if (rem == half)
      {
        if (mid == 0)
          arrayaddint(ntab, 1, nword);
        else
        if (ntab[nword - 1] & 0x00000001)
          arrayaddint(ntab, 1, nword);
      }
    break;
  case 1:
    break;
  case 2:
    if (!neg)
      arrayaddint(ntab, 1, nword);
    break;
  case 3:
    if (neg)
      arrayaddint(ntab, 1, nword);
    break;
  case 4:
    if (rem >= half)
      arrayaddint(ntab, 1, nword);
    break;
  case 5:
    if (rem > half)
      arrayaddint(ntab, 1, nword);
    break;
  case 6:
    if (rem > 0)
      arrayaddint(ntab, 1, nword);
    break;
  case 7:
    switch (type)
    {
    case 0:
    case 1:
      if ((ntab[nword - 1] & 0x00000001) == 0x00)
        arrayaddint(ntab, 1, nword);
      break;
    case 2:
      if (rem == 0 || rem == 5)
        arrayaddint(ntab, 1, nword);
      break;
    }
    break;
  }
  return;
}

int checkhfp(unsigned int *hfltab, int hflnum, int *hexpptr, BYTE optbits, int *fpc,int roundrule, int neg)
{
  int cc = 0;
  int rx;
  int i;
  int rbit;
  int mid;
  int rem;
  int hexp = *hexpptr;
  int shiftmax = 0;
  int shiftamt;
  unsigned int remtab[4];
  if (hexp > 127)
  {
    if (GR0_TR_HFP_OVER( optbits ))
    {
      if (*fpc & FPC_MASK_IMO)
      {
        *fpc &= ~(FPC_FLAG_SFX | FPC_DXC);
        *fpc |= DXC_IEEE_OF_INEX_TRUNC << FPC_DXC_SHIFT;
        hfltab[0] |= 0x41000000;
        if (GR0_AE( optbits ))
          cc = 2;
        else
          cc = -7;
      }
      else
      {
        memset(hfltab, 0xff, sizeof(int) * hflnum);
        hfltab[0] &= 0x00ffffff;
        hfltab[0] |= 0x7f000000;
        *fpc |= DXC_IEEE_OF_INEX_TRUNC << FPC_DXC_SHIFT;
        cc = 2;
      }
    }
    else
    {
      if (*fpc & FPC_MASK_IMI)
      {
        *fpc &= ~(FPC_FLAG_SFX | FPC_DXC);;
        *fpc |= FPC_DXC_I;
        if (GR0_AE( optbits ))
          cc = 2;
        else
          cc = -7;

      }
      else
      {
        memset(hfltab, 0xff, sizeof(int) * hflnum);
        hfltab[0] &= 0x00ffffff;
        hfltab[0] |= 0x7f000000;
        *fpc |= FPC_FLAG_SFI;
        cc = 2;
      }
    }
  }
  else
    if (hexp < 0)
    {
      switch (hflnum)
      {
      case 1:
        shiftmax = 20;
        break;
      case 2:
        shiftmax = 52;
        break;
      case 4:
        hfltab[2] = (hfltab[2] << 8) + ((hfltab[3] & 0xff000000) >> 24);
        hfltab[3] <<= 8;
        shiftmax = 108;
        break;
      }
      shiftamt = abs(hexp);
      if (shiftmax >= shiftamt)
      {
        if (hflnum == 4)
          shiftamt += 8;
        arrayshiftright(hfltab, hflnum, shiftamt, remtab);
        if (shiftamt % 32)
          rx = hflnum - (shiftamt / 32) - 1;
        else
          rx = hflnum - (shiftamt / 32);
        rbit = 32 - (shiftamt % 32);
        rem = remtab[rx] >> (28 - rbit);
        remtab[rx] = (remtab[rx] << (rbit + 4)) >> (rbit + 4);
        if (rem == 8)
        {
          mid = 1;
          for (i = rx; i < hflnum; i++)
          {
            if (remtab[i] != 0)
            {
              mid = 0;
              break;
            }
          }
        }
        else
          mid = 0;
        roundarray(hfltab, hflnum, roundrule, rem, 16, neg, 0, mid);
        *fpc |= FPC_FLAG_SFX;
        if (hflnum == 4)
        {
          hfltab[0] = (hfltab[0] << 8) | (hfltab[1] >> 24);
          hfltab[1] = (hfltab[1] << 8) | (hfltab[2] >> 24);
          hfltab[2] &= 0x00ffffff;
        }
        *hexpptr = 0;
        return 0;
      }
      if (GR0_TR_HFP_UNDER( optbits ))
      {
        if ((*fpc & FPC_MASK_IMU) && !GR0_AE( optbits ))
        {
          cc = -7;
          *fpc &= ~FPC_DXC;
          *fpc |= DXC_IEEE_INVALID_OP << FPC_DXC_SHIFT;
        }
        else
        {
          memset(hfltab, 0x00, sizeof(int) * hflnum);
          *fpc |= (FPC_FLAG_SFU | FPC_FLAG_SFX);
          cc = 2;
        }
      }
      else
      {
        if ((*fpc & FPC_MASK_IMI) && !GR0_AE( optbits ))
        {
          cc = -7;
          *fpc &= ~FPC_DXC;
          *fpc |= DXC_IEEE_INVALID_OP << FPC_DXC_SHIFT;
        }
        else
        {
          memset(hfltab, 0x00, sizeof(int) * hflnum);
          *fpc |= FPC_FLAG_SFX;
          cc = 2;
        }
      }
    }
  return cc;
}

int checkbfp(unsigned int *bfltab, int bflnum, int bexp, BYTE optbits, int *fpc,int roundrule,int neg)
{
  int cc = 0;
  int rx;
  int rbit;
  int mid;
  int rem;
  int i;
  int shiftamt;
  int shiftmax = 0;
  unsigned int remtab[4];
  int maxexp = bflexpmax[bflnum];
  if (bexp > maxexp)
  {
    if (*fpc & FPC_MASK_IMO)
    {
      *fpc &= ~(FPC_FLAG_SFX | FPC_DXC);;
      *fpc |= DXC_IEEE_OF_INEX_TRUNC << FPC_DXC_SHIFT;
      switch (bflnum)
      {
      case 1:
        bfltab[0] |= 0x3f800000;
        break;
      case 2:
        bfltab[0] |= 0x3ff80000;
        break;
      case 4:
        bfltab[0] |= 0x3fff8000;
        break;
      }
      if (GR0_AE( optbits ))
        cc = 2;
      else
        cc = -7;
    }
    else
    {
      memset(bfltab, 0xff, sizeof(int) * bflnum);
      switch (bflnum)
      {
      case 1:
        bfltab[0] = 0x7f800000;
        break;
      case 2:
        bfltab[0] = 0x7ff80000;
        bfltab[1] = 0;
        break;
      case 4:
        bfltab[0] = 0x7fff8000;
        bfltab[1] = 0;
        bfltab[2] = 0;
        bfltab[3] = 0;
        break;
      }

      *fpc |= FPC_FLAG_SFO;
      cc = 2;
    }
  }
  else
    if (bexp < 0)
    {
      switch (bflnum)
      {
      case 1:
        shiftmax = 22;
        break;
      case 2:
        shiftmax = 51;
        break;
      case 3:
        shiftmax = 111;
        break;
      }
      shiftamt = abs(bexp);
      if (shiftmax >= shiftamt)
      {
        arrayshiftright(bfltab, bflnum, shiftamt, remtab);
        if (shiftamt % 32)
          rx = bflnum - (shiftamt / 32) - 1;
        else
          rx = bflnum - (shiftamt / 32);
        rbit = 32 - (shiftamt % 32);
        rem = remtab[rx] >> (31 - rbit);
        remtab[rx] = (remtab[rx] << (rbit + 1)) >> (rbit + 1);
        if (rem == 1)
        {
          mid = 1;
          for (i = rx; i < bflnum; i++)
          {
            if (remtab[i] != 0)
            {
              mid = 0;
              break;
            }
          }
        }
        else
          mid = 0;
        roundarray(bfltab, bflnum , roundrule, rem, 2, neg, 1, mid);
        *fpc |= FPC_FLAG_SFX;
        return 0;
      }
      if (*fpc & FPC_MASK_IMU)
      {
        if (GR0_AE( optbits ))
        {
          cc = 2;
          *fpc &= ~FPC_DXC;
          *fpc |= DXC_IEEE_UF_INEX_INCR << FPC_DXC_SHIFT;
        }
        else
        {
          cc = -7;
          *fpc &= ~FPC_DXC;
          *fpc |= DXC_IEEE_INVALID_OP  << FPC_DXC_SHIFT;
        }
      }
      else
      {
        memset(bfltab, 0x00, sizeof(int) * bflnum);
        *fpc |= (FPC_FLAG_SFU | FPC_FLAG_SFX);
        cc = 2;
      }
    }
  return cc;
}

/***************************************************************/
/*   dfl2hfl:  Convert a decfloat value to hexfloat            */
/***************************************************************/
int dfl2hflbfl(unsigned int * dfltab,unsigned int * hfltab,int dflwords,int hflwords,BYTE optbits,int binflg,int *fpc)
{
  int i;
  unsigned int dec[ARRAYMAX];
  unsigned int hfl[ARRAYMAX];
  int k;
  unsigned int wrk[ARRAYMAX];
  unsigned int remtab[ARRAYMAX];
  BYTE decwork[6210];
  unsigned int rem;
  BYTE binzero[ARRAYMAX * sizeof(int)];
  long long wk;
  int decnum;
  int cc = 0;
  int roundrule;
  int hexp;
  int rx;
  int rbit;
  int ndigit;
  int maxdigit = 0;
  int maxbits;
  int numbits;
  int expword2 = 0;
  int shiftamt;
  int shiftstd;
  int bitctr;
  int ndpd;
  int fac10;
  int neg = 0;
  int exact = 1;
  int exp;
  int bexp;
  int delta;
  int expword = 0;
  //  the following table is used to reverse the bits in a byte.  This is needed
  //  for nan processing
  unsigned int bittab1[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff };
  int power10tab[8] = { 1,10,100,1000,10000,100000,1000000, 10000000 };
  int power10;
  int pidx;
  unsigned int wk1;
  int lmd;
  unsigned int temp1;
  unsigned int temp2;
  int nan = 0;
  int tbits;
  int expbits;
  int lzero = 0;
  int lzerohex;
  int mid;
  int tradix;
  memset(binzero, 0x00, sizeof(binzero));
  temp1 = (int) GR0_RM( optbits );
  if (temp1 == 0)
    roundrule = (*fpc & FPC_DRM) >> 4;
  else
    if (temp1 == 1)
      roundrule = (*fpc & FPC_BRM_3BIT);
    else
      roundrule = temp1 - 8;
  tradix = (int)((optbits & 0x30) >> 4);
  if (binflg && tradix)
  {
    cc = -1;
    return cc;
  }
  wk1 = dfltab[0];
  wk1 = (wk1 << 1) >> 1;
  if (wk1 != dfltab[0])
    neg = 1;
  dfltab[0] = wk1;
  memset(dec,0x00,sizeof(dec));
/***************************************************************/
/*   extract the exponent portion based on the precision and   */
/*   then clear that portion from the number.                  */
/***************************************************************/
  switch (dflwords)
  {
  case 1:
    expword = dfltab[0] >> 20;;
    expword2 = expword >> 4;
    dec[ARRAYMAX - 1] = dfltab[0] & 0x000fffff;
    break;
  case 2:
    expword = dfltab[0] >> 18;
    dec[ARRAYMAX - 2] = dfltab[0] & 0x0003ffff;
    dec[ARRAYMAX - 1] = dfltab[1];
    expword2 = expword >> 6;
    break;
  case 4:
    expword = dfltab[0] >> 14;
    dec[ARRAYMAX - 4] = dfltab[0] & 0x00003fff;
    dec[ARRAYMAX - 3] = dfltab[1];
    dec[ARRAYMAX - 2] = dfltab[2];
    dec[ARRAYMAX - 1] = dfltab[3];
    expword2 = expword >> 10;
    break;
  }
  if (expword2 == 0x78)
  {
    if (binflg)
    {
      bexp = bflexpmax[hflwords];
      memset(hfltab, 0x00, 16);
      expbits = bflexpbits[hflwords];
      hfltab[0] = bexp << (32 - expbits);
      return 0;
    }
    else
    {
      if (*fpc & 0x80) // (invalid reserved bit that must be zero?)
      {
        *fpc &= ~FPC_DXC;
        *fpc |= DXC_IEEE_INVALID_OP << FPC_DXC_SHIFT;
        return -7;
      }
      memset(hfltab, 0xff, 4 * hflwords);
      hfltab[0] &= 0x7fffffff;
      return 2;
    }
  }
  if (expword2 == 0x7c)
  {
    nan = 1;
    if (binflg == 0)
    {
      if (*fpc & 0x80) // (invalid reserved bit that must be zero?)
      {
        *fpc &= ~FPC_DXC;
        *fpc |= DXC_IEEE_INVALID_OP << FPC_DXC_SHIFT;
        return -7;
      }
      memset(hfltab, 0xff, 4 * hflwords);
      hfltab[0] &= 0x7fffffff;
      return 2;
    }
    switch (dflwords)
    {
    case 1:
      if ((dfltab[0] & 0x00ffffff) == 0)
        nan = 2;
      break;
    case 2:
      if ((dfltab[0] & 0x00ffffff) == 0 && dfltab[1] == 0)
        nan = 2;
      break;
    case 4:
      if ((dfltab[0] & 0x00ffffff) == 0 && dfltab[1] == 0 &&
        dfltab[2] == 0 && dfltab[3] == 0)
        nan = 2;
      break;
    }
    if (nan == 2)
    {
      switch (hflwords)
      {
      case 1:
        hfltab[0] = 0x7fc00000;
        break;
      case 2:
        hfltab[0] = 0x7ff80000;
        hfltab[1] = 0;
        break;
      case 4:
        hfltab[0] = 0x7fff8000;
        hfltab[1] = 0;
        hfltab[2] = 0;
        hfltab[3] = 0;
        break;
      }
      if (neg)
        hfltab[0] |= 0x80000000;
      return 0;
    }
  }
/***************************************************************/
/*   convert the exponent bits to the acutal exponent value    */
/***************************************************************/
  if (nan)
  {
    exp = 0;
    lmd = 0;
  }
  else
  {
    exp = dflexp(expword, &lmd, dflwords);
    lmd = lmd * 1000;
  }
  lzero = getlzerobits(dec, ARRAYMAX);
  memset(hfl,0x00,sizeof(hfl));
  memset(decwork,0x00,sizeof(decwork));
  fac10 = 0;
  hexp = 0;
  bexp = 0;
  if (binflg)
    maxbits = bflmaxdigit[hflwords];
  else
  {
    maxdigit = hflmaxdigit[hflwords];
    maxbits = maxdigit * 4;
  }
/***************************************************************/
/*   convert the densely packed decimal values to an array of  */
/*   decimal number, one digit per byte.                       */
/***************************************************************/
  if (lzero < ARRAYMAX * 32)
  {
    tbits = (ARRAYMAX * 32) - lzero;
    ndpd = tbits / 10;
    if (tbits % 10)
      ndpd++;
/***************************************************************/
/*   handle each set of 10 bits that represent a densely       */
/*   packed decimal number from 0-999.  Convert from densely   */
/*   packed decimal to normal, then multiply by 10**n, where   */
/*   n starts at zero and increases by three for each pass.    */
/*   Then take the result and add it to what we have so far.   */
/***************************************************************/
    for (i = 0;i < ndpd;i++)
    {
      arraydiv(dec,1024,ARRAYMAX,&rem);
      decnum = DPD2BIN[rem];
      if (i == ndpd - 1) decnum += lmd;
      memset(wrk, 0x00, ARRAYMAX * 4);
      wrk[ARRAYMAX - 1] = decnum;
      k = fac10;
      while (k > 0)
      {
        pidx = min(k, 7);
        power10 = power10tab[pidx];
        k -= pidx;
        arraymlt(wrk, power10, ARRAYMAX);
      }
      fac10 += 3;
      arrayadd(hfl,wrk,ARRAYMAX,ARRAYMAX);
    }
  }
/***************************************************************/
/*   if the input is not a number (nan), format the binary     */
/*   output.  if the output is hex, we have alread done this   */
/***************************************************************/
  if (nan)
  {
    bexp = bflexpmax[hflwords];
    expbits = bflexpbits[hflwords];
    lzero = getlzerobits(hfltab, ARRAYMAX);
    if (lzero == ARRAYMAX * 32)
    {
      memset(hfltab, 0x00, ARRAYMAX * 4);
      hfltab[0] |= bexp << (32 - expbits);
      hfltab[0] += (1 << (31 - expbits));
      return 0;
    }
    memset(hfltab, 0x00, hflwords * sizeof(int));
    for (i = ARRAYMAX - 1; i >= ARRAYMAX - hflwords; i--)
    {
      for (k = 0; k < 4; k++)
      {
        temp1 = (hfl[i] >> (k * 8)) & 0x000000ff;
        temp2 = bittab1[temp1];
        hfltab[ARRAYMAX - i - 1] |= (temp2 << (24 - (k * 8)));
      }
    }
    switch (hflwords)
    {
    case 1:
      hfltab[0] >>= 10;
      hfltab[0] |= 0x7fc00000;
      break;
    case 2:
      temp1 = hfltab[0] & 0x00001fff;
      hfltab[0] >>= 13;
      hfltab[0] |= 0x7ff80000;
      hfltab[1] >>= 13;
      hfltab[1] |= (temp1 << 19);
      break;
    case 4:
      temp1 = hfltab[0] & 0x0001ffff;
      hfltab[0] >>= 17;
      hfltab[0] |= 0x7fff8000;
      temp2 = hfltab[1] & 0x0001ffff;
      hfltab[1] >>= 17;
      hfltab[1] |= (temp1 << 15);
      temp1 = temp2;
      temp2 = hfltab[2] & 0x0001ffff;
      hfltab[2] >>= 17;
      hfltab[2] |= (temp1 << 15);
      temp1 = temp2;
      hfltab[3] >>= 17;
      hfltab[3] |= (temp1 << 15);
      break;
    }
    if (neg)
      hfltab[0] |= 0x80000000;
    return 0;
  }
  /***************************************************************/
  /*   if a negative exponent, convert the decimal fraction to   */
  /*   hex by dividing by 10 for each value of the exponent.     */
  /*   logically multiply one by multiplying by 16 and           */
  /*   subtracting from the hex exponent.  This is done to       */
  /*   maintain precision.  If the hex number flows into the     */
  /*   first element in the array, divide until it does not.     */
  /***************************************************************/
  if (exp < 0)
  {
    exp = abs(exp);
    while (exp > 0)
    {
      if (hfl[0] == 0 && hfl[1] < 65536)
      {
        lzero = getlzerobits(hfl, ARRAYMAX);
        lzerohex = (lzero >> 2) << 2;
        shiftamt = lzerohex - 44;
        arrayshiftleft(hfl, ARRAYMAX, shiftamt);
        hexp -= (shiftamt / 4);
      }
      pidx = min(exp, 7);
      power10 = power10tab[pidx];
      arraydiv(hfl,power10, ARRAYMAX, &rem);
      if (rem > 0)
        exact = 0;
      exp -= pidx;
    }
  }
  else
    while (exp > 0)
    {
      pidx = min(exp, 7);
      power10 = power10tab[pidx];
      exp -= pidx;
      arraymlt(hfl, power10, ARRAYMAX);
      if (hfl[0] > 0)
      {
        lzero = getlzerobits(hfl, ARRAYMAX);
        lzerohex = (lzero >> 2) << 2;
        shiftamt = 64 - lzerohex;
        arrayshiftright(hfl, ARRAYMAX, shiftamt, remtab);
        if (memcmp(remtab, binzero, sizeof(remtab)) != 0)
          exact = 0;
        hexp += shiftamt / 4;
      }
    }
  lzero = getlzerobits(hfl, ARRAYMAX);
  /***************************************************************/
  /*   if too many digits, round it to the correct number and    */
  /*   refresh the digit counts.                                 */
  /***************************************************************/
  if (binflg)
  {
    bexp = hexp * 4;
    numbits = ARRAYMAX * 32 - lzero;
    if (bexp + numbits + bflexpbias[hflwords] > 0)
      bitctr = maxbits + 1;
    else
      bitctr = maxbits;
    if (numbits > bitctr)
    {
      shiftamt = numbits - bitctr;
      arrayshiftright(hfl, ARRAYMAX, shiftamt, remtab);
      if (memcmp(remtab, binzero, sizeof(remtab)) != 0)
        *fpc |= FPC_FLAG_SFX;
      if (shiftamt % 32)
      {
        rx = ARRAYMAX - (shiftamt / 32) - 1;
        rbit = 32 - (shiftamt % 32);
      }
      else
      {
        rx = ARRAYMAX - (shiftamt / 32);
        rbit = 0;
      }
      rem = remtab[rx] >> (31 - rbit);
      remtab[rx] = (remtab[rx] << (rbit + 1)) >> (rbit + 1);
      if (rem == 1)
      {
        mid = 1;
        for (i = rx; i < ARRAYMAX; i++)
        {
          if (remtab[i] != 0)
          {
            mid = 0;
            break;
          }
        }
      }
      else
        mid = 0;
      roundarray(hfl, ARRAYMAX, roundrule, rem, 2, neg, 1, mid);
      remtab[rx] = 0;
      bexp += shiftamt;
    }
    else
      if (numbits < bitctr)
      {
        delta = bitctr - numbits;
        arrayshiftleft(hfl, ARRAYMAX, delta);
        bexp -= delta;
      }
  }
  else
  //   converting to hex if here
  {
    ndigit = (ARRAYMAX * 8) - (lzero / 4);
    if (ndigit > maxdigit)
    {
      shiftamt = (ndigit - maxdigit) * 4;
      arrayshiftright(hfl, ARRAYMAX, shiftamt, remtab);
      if (memcmp(remtab, binzero, sizeof(remtab)) != 0)
        *fpc |= FPC_FLAG_SFX;
      if (shiftamt % 32)
        rx = ARRAYMAX - (shiftamt / 32) - 1;
      else
        rx = ARRAYMAX - (shiftamt / 32);
      rbit = 32 - (shiftamt % 32);
      rem = remtab[rx] >> (28 - rbit);
      remtab[rx] = (remtab[rx] << (rbit + 4)) >> (rbit + 4);
      if (rem == 8)
      {
        mid = 1;
        for (i = rx; i < ARRAYMAX; i++)
        {
          if (remtab[i] != 0)
          {
            mid = 0;
            break;
          }
        }
      }
      else
        mid = 0;
      roundarray(hfl, ARRAYMAX, roundrule, rem, 16, neg, 0, mid);
      remtab[rx] = 0;
      hexp += shiftamt / 4;
    }
    else
    if (ndigit < maxdigit)
    {
      shiftamt = maxdigit - ndigit;
      arrayshiftleft(hfl, ARRAYMAX, shiftamt * 4);
      hexp -= shiftamt;
    }
  }
  /***************************************************************/
  /*   now normalize it.                                         */
  /***************************************************************/
  lzero = getlzerobits(hfl, ARRAYMAX);
  shiftstd = 32 * (ARRAYPAD + 4 - hflwords);
  if (binflg)
  {
    expbits = bflexpbits[hflwords];
    wk1 = bexp + bflexpbias[hflwords];
    if (wk1 > 0)
      shiftamt = lzero - expbits + 1;
    else
      shiftamt = lzero - expbits;
    bexp -= (shiftamt - shiftstd);
    bexp += maxbits;
    bexp += bflexpbias[hflwords];
    if (bexp > 0)
    {
      arrayshiftleft(hfl, ARRAYMAX, shiftamt);
      if (bexp > 0)
      {
        hfl[0] <<= expbits + 32;
        hfl[0] >>= expbits + 32;
      }
      wk1 = bexp;
      if (bexp <= bflexpmax[hflwords] && bexp >= 0)
      {
        wk1 <<= (32 - expbits);
        hfl[0] += wk1;
      }
    }
  }
  else
  {
    lzerohex = (lzero >> 2) << 2;
    shiftamt = lzerohex - 8;
    if (hflwords == 4)
      hexp -= (shiftamt - shiftstd - 8) / 4;
    else
      hexp -= (shiftamt - shiftstd) / 4;
    arrayshiftleft(hfl, ARRAYMAX, shiftamt);
    hexp += maxdigit;
    hexp += 64;
    wk = hexp << 24;
    if (hexp <= 127 && hexp >= 0)
      hfl[0] += wk;

  }
  for (i = 0;i < hflwords;i++)
    hfltab[i] = (unsigned int)hfl[i];
  /***************************************************************/
  /*   if an extended hex float, add the low order exponent.     */
  /***************************************************************/
  if (hflwords == 4 && binflg == 0x00)
  {
    temp1 = hfltab[3] & 0x000000ff;
    arrayshiftright(hfltab, 4, 8, remtab);
    rem = temp1 >> 4;
    temp2 = temp1 & 0x0000000f;
    if (temp2 > 0)
      roundarray(hfltab, 4, roundrule, rem, 16, neg, 0, 0);
    else
      if (temp2 == 0)
        roundarray(hfltab, 4, roundrule, rem, 16, neg, 0, 1);
      else
        roundarray(hfltab, 4, roundrule, rem, 16, neg, 0, 0);
    temp1 = hfltab[2] >> 24;
    hfltab[2] &= 0x00ffffff;
    temp2 = hfltab[1] >> 24;
    hfltab[1] <<= 8;
    hfltab[1] += temp1;
    hfltab[0] <<= 8;
    hfltab[0] += temp2;
    wk1 = hexp - 14;
    if (wk1 > 0)
      hfltab[2] += (wk1 << 24);
  }
  if (!exact)
    *fpc |= FPC_FLAG_SFX;
  if (binflg)
    cc = checkbfp(hfltab, hflwords, bexp, optbits, fpc, roundrule, neg);
  else
    cc = checkhfp(hfltab, hflwords, &hexp, optbits, fpc, roundrule, neg);
  if (neg)
    hfltab[0] |= 0x80000000;
  return cc;
}

/***************************************************************/
/*   hfldhfl:  Convert a hexfloat value to decfloat            */
/***************************************************************/
int hflbfl2dfl(unsigned int *hfltab, unsigned int *dfltab, int hflwords, int dflwords, BYTE optbits, int binflg, int *fpc)
{
  int exp;
  int dexp = 0;
  unsigned int exp1;
  unsigned int hfl[ARRAYMAX];
  unsigned int dec[ARRAYMAX];
  unsigned int wrk[ARRAYMAX];
  unsigned int remtab[ARRAYMAX];
  BYTE decwork[6210];
  BYTE binzero[6210];
  int i;
  int k;
  int cc = 0;
  int decctr;
  int neg = 0;
  unsigned int wk1;
  unsigned int rem;
  int ndpdctr;
  int ndpd;
  unsigned int rbe;
  int lzero;
  int lzerohex;
  int bexp = 0;
  int delta;
  int hexp = 0;
  int shiftamt;
  unsigned int maxexp;
  int maxdigits;
  int nan = 0;
  //  the following table is used to reverse the bits in a nibble.  This is needed
  //  for nan processing
  unsigned int bittab1[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff };
  int power10tab[8] = { 1,10,100,1000,10000,100000,1000000, 10000000};
  int hexdigittab[6] = { 0, 2, 3, 4, 5, 7 };
  int power10;
  int pidx;
  int hdigit;
  int rview = 1;
  unsigned int lmd;
  unsigned int temptab[4];
  int expword;
  int cbits;
  int maxbits = 0;
  unsigned int temp1;
  unsigned int temp2;
  memset(binzero, 0x00, sizeof(binzero));
  temp1 = (unsigned int) GR0_RM( optbits );
  if (hfltab[0] & 0x80000000)
  {
    neg = 1;
    hfltab[0] &= 0x7fffffff;
  }
  /***************************************************************/
  /*   get the biased exponent value, then shift it out          */
  /***************************************************************/
  if (binflg)
  {
    cbits = bflexpbits[hflwords];
    bexp = hfltab[0] >> (32 - cbits);
    maxexp = bflexpmax[hflwords];
    hfltab[0] = (hfltab[0] << cbits) >> cbits;
    wk1 = bexp;
    if (wk1 > 0 && wk1 < maxexp)
      hfltab[0] += (1 << (32 - cbits));
    bexp -= bflexpbias[hflwords];
    if (wk1 == maxexp)
    {
      memset(temptab, 0x00, sizeof(temptab));
      memcpy(temptab, hfltab, hflwords * 4);
      if (memcmp(temptab, binzero, 16) == 0)
      {
        memset(dfltab, 0x00, dflwords * 4);
        dfltab[0] |= 0x78000000;
        if (neg)
          dfltab[0] |= 0x80000000;
        return 0;
      }
      temptab[0] = (temptab[0] << (cbits + 1)) >> (cbits + 1);
      switch (hflwords)
      {
      case 1:
        if (temptab[0] == 0)
        {
          memset(dfltab, 0x00, sizeof(int) * dflwords);
          dfltab[0] = 0x7c000000;
          if (neg)
            dfltab[0] |= 0x80000000;
          return 0;
        }
        temptab[0] <<= 10;
        break;
      case 2:
        if (temptab[0] == 0 && temptab[1] == 0)
        {
          memset(dfltab, 0x00, sizeof(int) * dflwords);
          dfltab[0] = 0x7c000000;
          if (neg)
            dfltab[0] |= 0x80000000;
          return 0;
        }
        temptab[0] = (temptab[0] << 13) | ((temptab[1] & 0xfff80000) >> 19);
        temptab[1] <<= 13;
        break;
      case 4:
        if (temptab[0] == 0 && temptab[1] == 0 &&
          temptab[2] == 0 && temptab[3] == 0)
        {
          memset(dfltab, 0x00, sizeof(int) * dflwords);
          dfltab[0] = 0x7c000000;
          if (neg)
            dfltab[0] |= 0x80000000;
          return 0;
        }
        temptab[0] = (temptab[0] << 17) | ((temptab[1] & 0xffff8000) >> 15);
        temptab[1] = (temptab[1] << 17) | ((temptab[2] & 0xffff8000) >> 15);
        temptab[2] = (temptab[2] << 17) | ((temptab[3] & 0xffff8000) >> 15);
        temptab[3] <<= 17;
        break;
      }
      memset(hfl, 0x00, sizeof(hfl));
      for (i = 0; i < hflwords; i++)
      {
        for (k = 0; k < 4; k++)
        {
          temp1 = (temptab[i] >> (24 - (k * 8))) & 0x000000ff;
          temp2 = bittab1[temp1];
          hfl[ARRAYMAX - i - 1] |= (temp2 << (k * 8));
        }
      }
      nan = 1;
    }
  }
  else
  {
    hexp = hfltab[0] >> 24;
    hfltab[0] &= 0x00ffffff;
    hexp -= 64;
  }
  if (memcmp(hfltab, binzero, 4 * hflwords) == 0 && !nan)
  {
    memset(dfltab, 0x00, 4 * dflwords);
    return 0;
  }
  if (!nan)
  {
    dexp = 0;
    memset(hfl, 0x00, sizeof(hfl));
    /***************************************************************/
    /*   load into a long long array                               */
    /***************************************************************/
    switch (hflwords)
    {
    case 1:
      hfl[ARRAYMAX - 1] = hfltab[0];
      if (binflg)
        maxbits = 23;
      else
        maxbits = 24;
      break;
    case 2:
      hfl[ARRAYMAX - 2] = hfltab[0];
      hfl[ARRAYMAX - 1] = hfltab[1];
      if (binflg)
        maxbits = 52;
      else
        maxbits = 56;
      break;
    case 4:
      if (binflg)
      {
        hfl[ARRAYMAX - 4] = hfltab[0];
        hfl[ARRAYMAX - 3] = hfltab[1];
          hfl[ARRAYMAX - 2] = hfltab[2];
        hfl[ARRAYMAX - 1] = hfltab[3];
        maxbits = 112;
      }
      else
      {
        hfl[ARRAYMAX - 4] = hfltab[0] >> 8;
        hfl[ARRAYMAX - 3] = ((hfltab[0] & 0x000000ff) << 24) + (hfltab[1] >> 8);
        hfl[ARRAYMAX - 2] = (hfltab[2] & 0x00ffffff) + ((hfltab[1] & 0x000000ff) << 24);
        hfl[ARRAYMAX - 1] = hfltab[3];
        maxbits = 112;
      }
      break;
    }
    if (binflg)
      if (bexp + bflexpbias[hflwords] == 0)
        exp = bexp - maxbits + 1;
      else
        exp = bexp - maxbits;
    else
      exp = (hexp * 4) - maxbits;
    /***************************************************************/
    /*   We need to adjust the number by the difference between    */
    /*   the exponent (in bits) and the number of bits for the     */
    /*   fraction.  We need to multiply by 2**N, where N is        */
    /*   the difference.  Note that if N is negative, it is        */
    /*   actually a divide, and if zero multiply by one.           */
    /***************************************************************/
    dexp = 0;
    if (exp < 0)
    {
      exp = abs(exp);
      while (exp > 0)
      {
        while (hfl[0] == 0 && hfl[1] < 16777216)
        {
          lzero = getlzerobits(hfl, ARRAYMAX);
          lzerohex = (lzero >> 2) << 2;
          hdigit = (lzerohex - 32) / 4;
          hdigit = max(hdigit, 5);
          hdigit = min(hdigit, 1);
          pidx = hexdigittab[hdigit];
          power10 = power10tab[pidx];
          arraymlt(hfl, power10, ARRAYMAX);
          dexp -= pidx;
        }
        shiftamt = min(32, exp);
        arrayshiftright(hfl, ARRAYMAX, shiftamt, remtab);
        exp -= shiftamt;
      }
    }
    else
    /***************************************************************/
    /*   if the exponent is negaive, we need to convert the hex    */
    /*   fraction to decimal, so we need to divide by 16 for each  */
    /*   significant digit, minus the exponent value.  If the      */
    /*   exponent itself is negative, this means more divides,     */
    /*   if positive less divides.  To maintain precision, we will */
    /*   multiply by 10 and reduce the decimal exponent by 1 for   */
    /*   each pass.  Periodically, we will multiply by 100 and     */
    /*   reduce the exponent by two.                               */
    /***************************************************************/
    while (exp > 0)
    {
      shiftamt = min(32, exp);
      arrayshiftleft(hfl, ARRAYMAX, shiftamt);
      exp -= shiftamt;
      while (hfl[0] > 0 || (hfl[0] == 0 && hfl[1] >= 16777216))
      {
        lzero = getlzerobits(hfl, ARRAYMAX);
        lzerohex = (lzero >> 2) << 2;
        hdigit = (40 - lzerohex) / 4;
        hdigit = max(hdigit, 5);
        hdigit = min(hdigit, 1);
        pidx = hexdigittab[hdigit];
        power10 = power10tab[pidx];
        arraydiv(hfl, power10, ARRAYMAX, &rem);
        dexp += pidx;
      }
    }
  }
/***************************************************************/
/*   now we get number of digits.                              */
/***************************************************************/
  lzero = getlzerobits(hfl, ARRAYMAX);
  decctr = 0;
  memset(binzero, 0x00, sizeof(binzero));
  memset(decwork, 0x00, sizeof(decwork));
/***************************************************************/
/*   convert the hex number to an array of decimal digits, one */
/*   digit per byte.  Note that the array is in reverse order, */
/*   with the least significant digit first.                   */
/***************************************************************/
  for (;;)
  {
/***************************************************************/
/*   the remainder of the division below is the next decimal   */
/*   digit.                                                    */
/***************************************************************/
    arraydiv(hfl,10,ARRAYMAX,&rem);
    decwork[decctr] = (BYTE)rem;
    decctr++;
    if (memcmp(hfl, binzero, ARRAYMAX * 4) == 0)
      break;
  }
  maxdigits = dflmaxdigit[dflwords];
/***************************************************************/
/*   if too many digits, round it and adjust the exponent      */
/***************************************************************/
  if (decctr > maxdigits)
  {
    delta = decctr - maxdigits;
    if (memcmp(decwork, binzero, delta) != 0)
      rview = 0;
    wk1 = (unsigned int)decwork[delta - 1];
    rem = 0;
    if (wk1 > 5)
      rem = 1;
    else
      if (wk1 == 5)
      {
        if (memcmp(decwork,binzero,delta) != 0)
          rem = 1;
      }
    if (rem == 1)
    {
      for (i = delta; i < decctr; i++)
      {
        wk1 = (unsigned int)decwork[i] + 1;
        if (wk1 < 10)
        {
          decwork[i] = (BYTE)wk1;
          break;
        }
        decwork[i] = 0x00;
      }
      if (i == decctr)
      {
        dexp++;
        decwork[decctr - 1] = 0x01;
      }
    }
    for (i = 0;i < maxdigits;i++)
      decwork[i] = decwork[i + delta];
    for (; i < decctr; i++)
      decwork[i] = 0;
    decctr = maxdigits;
    dexp += delta;
  }
  /***************************************************************/
  /*   change to right view if we did not need to round.         */
  /***************************************************************/
  if (rview)
  {
    delta = 0;
    for (i = 0; i < decctr; i++)
    {
      if (decwork[i] != 0x00)
        break;
      delta++;
    }
    if (delta > 0)
    {
      for (i = 0; i < decctr - delta; i++)
        decwork[i] = decwork[i + delta];
      decwork[i] = 0x00;
      decwork[i + 1] = 0x00;
      decctr -= delta;
      dexp += delta;
    }
  }
  else
    *fpc |= FPC_FLAG_SFX;
  /***************************************************************/
  /*   figure out the lmd (left most digit)                      */
  /***************************************************************/
  if (decctr < dflmaxdigit[dflwords])
    lmd = 0;
  else
  {
    lmd = (unsigned int)decwork[decctr - 1];
    decwork[decctr - 1] = 0x00;
    decctr--;
  }
  ndpd = decctr / 3;
  delta = decctr % 3;
  if (delta)
    ndpd++;
  memset(dec,0x00,sizeof(dec));
/***************************************************************/
/*   convert decimal digits to densely packed decimal          */
/***************************************************************/
  for (ndpdctr = 0;ndpdctr < ndpd;ndpdctr++)
  {
    i = ndpdctr * 3;
    temp1 = (unsigned int)decwork[i];
    temp1 += ((unsigned int)decwork[i + 1] * 10);
    temp1 += ((unsigned int)decwork[i + 2] * 100);
    temp2 = BIN2DPD[temp1];
    if (ndpdctr > 0)
    {
      memset(wrk,0x00,sizeof(wrk));
      wrk[ARRAYMAX - 1] = temp2;
      arrayshiftleft(wrk,ARRAYMAX,10 * ndpdctr);
      arrayadd(dec,wrk,ARRAYMAX,ARRAYMAX);
    }
    else
      dec[ARRAYMAX - 1] = temp2;
  }
  if (!nan)
  {
    /***************************************************************/
    /*   compute the rbe (remaining bytes of exponent)             */
    /***************************************************************/
    dexp += dflexpmax[dflwords];
    rbe = dexp % dflrbefac[dflwords];
    exp1 = dexp / dflrbefac[dflwords];
    /***************************************************************/
    /*   calcuate the exponent control bits (bits 1-5)             */
    /***************************************************************/
    if (lmd < 8)
      cbits = exp1 * 8 + lmd;
    else
      if (lmd == 8)
        cbits = 24 + exp1 * 2;
      else
        cbits = 24 + exp1 * 2 + 1;
    if (neg)
      cbits += 64;
    /***************************************************************/
    /*   put it all together to form the exponent                  */
    /***************************************************************/
    expword = rbe + (cbits << dflrbebits[dflwords]);
    /***************************************************************/
    /*   shift it and merge in.                                    */
    /***************************************************************/
    expword = expword << dflsigbits[dflwords];
    dec[ARRAYMAX - dflwords] += expword;
  }
  switch (dflwords)
  {
  case 1:
    dfltab[0] = dec[ARRAYMAX - 1];
    break;
  case 2:
    dfltab[0] = dec[ARRAYMAX - 2];
    dfltab[1] = dec[ARRAYMAX - 1];
    break;
  case 4:
    dfltab[0] = dec[ARRAYMAX - 4];
    dfltab[1] = dec[ARRAYMAX - 3];
    dfltab[2] = dec[ARRAYMAX - 2];
    dfltab[3] = dec[ARRAYMAX - 1];
    break;
  }
  if (nan)
  {
    dfltab[0] |= 0x7c000000;
    if (neg)
      dfltab[0] |= 0x80000000;
  }
  return cc;
}

/***************************************************************/
/*   convert hex float to binary (IEEE) float.                 */
/***************************************************************/
int hfl2bfl(unsigned int *tab,unsigned int *tabout, int nwordin, int nwordout, BYTE optbits, int *fpc)
{
  unsigned int temptab1[6];
  int hexp;
  int bexp = 0;
  int neg = 0;
  int i;
  int mid;
  int cc = 0;
  int zeroctr;
  int bitshift;
  int roundrule;
  int shiftword;
  int maxword;
  int rem;
  unsigned int temp1;
  unsigned int temp2;
  temp1 = (unsigned int) GR0_RM( optbits );
  if (temp1 == 0)
    roundrule = (*fpc & FPC_DRM) >> 4;
  else
    if (temp1 == 1)
      roundrule = (*fpc & FPC_BRM_3BIT);
    else
      roundrule = (int)(temp1 - 8);
  memset(temptab1, 0x00,sizeof(temptab1));
  memcpy(temptab1, tab, nwordin * 4);
  neg = tab[0] >> 31;
  temptab1[0] &= 0x7fffffff;
  maxword = max(nwordin, nwordout) + 1;
/***************************************************************/
/*   extract the hex exponent and shift it out                 */
/***************************************************************/
  hexp = temptab1[0] >> 24;
  temptab1[0] &= 0x00ffffff;
  hexp -= 64;
/***************************************************************/
/*   if extended hex float, shift out the low order exponent   */
/***************************************************************/
  if (nwordin == 4)
  {
    temp1 = temptab1[3] >> 24;
    temptab1[3] <<= 8;
    temptab1[2] <<= 8;
    temptab1[2] += temp1;
  }
/***************************************************************/
/*   count leading bits                                        */
/***************************************************************/
  zeroctr = 0;
  for (i = 0; i < nwordin;i++)
  {
    if (temptab1[i] == 0)
    {
      zeroctr += 32;
      continue;
    }
    break;
  }
/***************************************************************/
/*  if all zeros, clear the output number and exit             */
/***************************************************************/
  if (i == nwordin)
  {
    memset(tabout, 0x00, nwordout * 4);
    return 0;
  }
  temp1 = temptab1[i];
  while (temp1 > 0)
  {
    if (temp1 & 0x80000000)
      break;
    zeroctr++;
    temp1 <<= 1;
  }
  zeroctr -= 8;     //   reduce for the exponent byte
/***************************************************************/
/*   The nummber of bits to shift is the difference between    */
/*   the number of bits for the hex exponent (always 8) minus  */
/*   the number of bits for the binary exponent. plus the      */
/*   number of leading zero bits (adjusted for the exponent    */
/*   yte.  Note that when going from hex to binary, the        */
/*   number will never be subnormal.  There is also an         */
/*   implied bit that is in the low order bit of the exponent  */
/*   that we must allow for in the shift calculation           */
/***************************************************************/
  bitshift = 9 - bflexpbits[nwordout] + zeroctr;
/***************************************************************/
/*   the binary exponent is the hex exponent times 4 minus     */
/*   the shift for normalization.                              */
/***************************************************************/
  bexp = (hexp * 4) - (zeroctr + 1);
/***************************************************************/
/*   do the shift                                              */
/***************************************************************/
  if (bitshift > 32)
  {
    shiftword = bitshift / 32;
    for (i = 0; i < maxword - shiftword; i++)
      temptab1[i] = temptab1[i + shiftword];
    for (; i < maxword; i++)
      temptab1[i] = 0;
    bitshift -= shiftword * 32;
  }
  if (bitshift < 0)
  {
    bitshift = abs(bitshift);
    temp1 = 0;
    for (i = 0; i <= maxword; i++)
    {
      temp2 = temptab1[i] << (32 - bitshift);
      temptab1[i] >>= bitshift;
      temptab1[i] += temp1;
      temp1 = temp2;
    }
  }
  else
    if (bitshift > 0)
    {
      temp1 = 0;
      for (i = maxword - 1; i >= 0; i--)
      {
        temp2 = temptab1[i] >> (32 - bitshift);
        temptab1[i] <<= bitshift;
        temptab1[i] += temp1;
        temp1 = temp2;
      }
    }
/***************************************************************/
/*   copy the number back from workarea                        */
/***************************************************************/
  memset(tabout, 0x00, 16);
  memcpy(tabout, temptab1, nwordout * sizeof(int));
/***************************************************************/
/* shift the binary exponent based on the size                 */
/***************************************************************/
  switch (nwordout)
  {
  case 1:
    bexp += 127;
    bexp <<= 23;
    rem = temptab1[1] >> 31;
    temptab1[1] &= 0x7fffffff;
    mid = 0;
    if (temptab1[1] == 0 && temptab1[2] == 0 &&
      temptab1[3] == 0 && temptab1[4] == 0)
      mid = 1;
    if (mid == 0 || rem != 0)
      *fpc |= FPC_FLAG_SFX;
    roundarray(tabout, 1, roundrule, rem, 2, neg, 1, mid);
    tabout[0] &= 0x007fffff;
    break;
  case 2:
    bexp += 1023;
    bexp <<= 20;
    rem = temptab1[2] >> 31;
    temptab1[2] &= 0x7fffffff;
    mid = 0;
    if (temptab1[2] == 0 && temptab1[3] == 0 &&
      temptab1[4] == 0)
      mid = 1;
    if (mid == 0 || rem != 0)
      *fpc |= FPC_FLAG_SFX;
    roundarray(tabout, 2, roundrule, rem, 2, neg, 1, mid);
    tabout[0] &= 0x000fffff;
    break;
  case 4:
    rem = temptab1[4] >> 31;
    temptab1[4] &= 0x7fffffff;
    mid = 0;
    if (temptab1[4] == 0)
      mid = 1;
    if (mid == 0 || rem != 0)
      *fpc |= FPC_FLAG_SFX;
    roundarray(tabout, 4, roundrule, rem, 2, neg, 1, mid);
    bexp += 16383;
    bexp <<= 16;
    tabout[0] &= 0x0000ffff;
    break;
  }
/***************************************************************/
/* merge it back in                                            */
/***************************************************************/
  tabout[0] += bexp;
  if (neg)
    tabout[0] |= 0x80000000;
  return cc;
}
/******************************************************************************/
/*         bfl2hfl  -  convert a binary (IEEE) floating point numeber to hex  */
/*                     float.  The binary float number must be shifted to     */
/*                     make the binary exponent a multiple of four, so that   */
/*                     it can be converted to a hex exponent.                 */
/*                                                                            */
/******************************************************************************/
int bfl2hfl(unsigned int *tab, unsigned int *tabout, int nwordin, int nwordout, BYTE optbits, int *fpc)
{
  unsigned int temptab1[6];
  unsigned int remtab[4];
  int hexp;
  int bexpbias = 0;
  int hexp2;
  int bexp = 0;
  int bexpround;
  int neg = 0;
  int mid;
  int i;
  int cc = 0;
  int bitctr;
    int rem;
  int shiftctr;
  int roundrule;
  int maxword;
  unsigned int temp1;
  unsigned int temp2;
  BYTE binzero[32];
  maxword = max(nwordin, nwordout) + 1;
  memset(binzero, 0x00, sizeof(binzero));
  temp1 = (unsigned int) GR0_RM( optbits );
  if (temp1 == 0)
    roundrule = (*fpc & FPC_DRM) >> 4;
  else
    if (temp1 == 1)
      roundrule = (*fpc & FPC_BRM_3BIT);
    else
      roundrule = (int)(temp1 - 8);
  memset(temptab1, 0x00, sizeof(temptab1));
  memcpy(temptab1, tab, sizeof(int) * nwordin);
  neg = tab[0] >> 31;
/***************************************************************/
/* isolate the binary exponent, then turn on the assumed bit   */
/* (first bit left of the fraction).                           */
/***************************************************************/
  temptab1[0] &= 0x7fffffff;
  bitctr = 0;
  switch (nwordin)
  {
  case 4:
    bexpbias = temptab1[0] >> 16;
    bexp = bexpbias - 16383;
    temptab1[0] &= 0x0000ffff;
    temptab1[0] |= 0x00010000;
    break;
  case 2:
    bexpbias = temptab1[0] >> 20;
    bexp = bexpbias -1023;
    temptab1[0] &= 0x000fffff;
    temptab1[0] |= 0x00100000;
    break;
  case 1:
    bexpbias = temptab1[0] >> 23;
    bexp = bexpbias - 127;
    bitctr = 1;
    temptab1[0] &= 0x007fffff;
    temptab1[0] |= 0x00800000;
    break;
  }
  if (bexpbias == bflexpmax[nwordin])
  {
    if (*fpc & 0x80) // (invalid reserved bit that must be zero?)
    {
      *fpc &= ~FPC_DXC;
      *fpc |= DXC_IEEE_INVALID_OP << FPC_DXC_SHIFT;
      return -7;
    }
    memset(tabout, 0xff, 16);
    tabout[0] &= 0x7fffffff;
    return 2;
  }
  if (bexp < -256)             //  too small for hex float
  {
    memset(tabout, 0x00, 16);
    return 2;
  }
/**************************************************************/
/*  add one to the exponent for the hidden bit                */
/**************************************************************/
  bexp++;
/***************************************************************/
/* the hex exponent is the binary exponent rounded up to a     */
/* a multiple of 4 divided by 4                                */
/***************************************************************/
  if (bexp < 0)
    bexpround = (bexp / 4) * 4;
  else
    bexpround = ((bexp + 3) / 4) * 4;
  hexp = bexpround / 4;
/***************************************************************/
/* the shift needed is the difference between the bits needed  */
/* for the binary exponent and the bits for the hex exponent   */
/* (8) plus the shift needed to round the exponent.  That is   */
/* hexp * 4 - bexp, or bexpround - bexp.  The shift is then    */
/* reduced by one for the hidden bit.                          */
/***************************************************************/
  bitctr = bflexpbits[nwordin] - 9 - (bexpround - bexp);
/***************************************************************/
/* for long and extented binary float, the net shift will be to*/
/* the left.  For short, it will be to the right.              */
/***************************************************************/
  if (bitctr > 0)
  {
    temp1 = 0;
    shiftctr = 32 - bitctr;
    for (i = maxword - 1; i >= 0; i--)
    {
      temp2 = temptab1[i] >> shiftctr;
      temptab1[i] <<= bitctr;
      temptab1[i] += temp1;
      temp1 = temp2;
    }
  }
  else
    if (bitctr < 0)
    {
      bitctr = abs(bitctr);
      shiftctr = 32 - bitctr;
      temp1 = 0;
      for (i = 0; i <= maxword; i++)
      {
        temp2 = (temptab1[i] & 0x000000ff) << shiftctr;
        temptab1[i] >>= bitctr;
        temptab1[i] += temp1;
        temp1 = temp2;
      }
    }
  hexp += 64;
  cc = checkhfp(temptab1, nwordout, &hexp, optbits, fpc, roundrule, neg);
  memcpy(tabout, temptab1, sizeof(int) * nwordout);
  switch (nwordout)
  {
  case 1:
    rem = temptab1[1] >> 28;
    mid = 0;
    temptab1[1] &= 0x0fffffff;
    if (temptab1[1] == 0 && temptab1[2] == 0 &&
      temptab1[3] == 0 && temptab1[4] == 0)
      mid = 1;
    if (mid == 0 || rem != 0)
      *fpc |= FPC_FLAG_SFX;
    roundarray(tabout, 1, roundrule, rem, 16, neg, 0, mid);
    tabout[1] = 0;
    tabout[2] = 0;
    tabout[3] = 0;
    break;
  case 2:
    rem = temptab1[2] >> 28;
    mid = 0;
    temptab1[2] &= 0x0fffffff;
    if (temptab1[2] == 0 && temptab1[3] == 0 &&
      temptab1[4] == 0)
      mid = 1;
    if (mid == 0 || rem != 0)
      *fpc |= FPC_FLAG_SFX;
    roundarray(tabout, 2, roundrule, rem, 16, neg, 0, mid);
    tabout[2] = 0;
    tabout[3] = 0;
    break;
  case 4:
    rem = (tabout[3] & 0x000000f0) >> 4;
    arrayshiftright(tabout, 4, 8, remtab);
    mid = 0;
    temptab1[3] &= 0x0000000f;
    if (temptab1[3] == 0 && temptab1[4] == 0)
      mid = 1;
    if (mid == 0 || rem != 0)
      *fpc |= FPC_FLAG_SFX;
    roundarray(tabout, 4, roundrule, rem, 16, neg, 0, mid);
    temp1 = tabout[2] >> 24;
    tabout[2] &= 0x00ffffff;
    temp2 = tabout[1] >> 24;
    tabout[1] <<= 8;
    tabout[1] += temp1;
    tabout[0] <<= 8;
    tabout[0] += temp2;
    hexp2 = (hexp - 14) << 24;
    tabout[2] += hexp2;
    break;
  }
  hexp <<= 24;
  tabout[0] += hexp;
  if (neg)
    tabout[0] |= 0x80000000;
  return cc;
}

/*-------------------------------------------------------------------*/
/* 010A PFPO  - Perform Floating Point Operation                 [E] */
/*-------------------------------------------------------------------*/
DEF_INST( perform_floating_point_operation )
{
    unsigned int  ftab[4];   // Input Floating-Point value to be converted
    unsigned int  tabout[4]; // Output converted Floating-Point value result

    int numout = 0;     // Output length in words
    int opcode;         // Conversion Operation Code (NOT instruction opcode!)
    int cc = 0;         // Condition Code??
    int fpc;            // Floating-Point Control Register value
    int dxc;            // Decimal Exception Code value
    int i0, i2, i4, i6; // Floating-Point register array indexes

    /* Extract fields from General Register 0 */
    bool test_mode = GR0_T( regs );
    BYTE otc       = GR0_OTC( regs );
    BYTE ofc1      = GR0_OFC1( regs );
    BYTE ofc2      = GR0_OFC2( regs );
    BYTE optbits   = regs->GR_LHLCL(0);
    BYTE rm        = GR0_RM( optbits );

    E( inst, regs );
    TXFC_INSTR_CHECK( regs );

    /* Get fpr array indexes to source and destination registers */

    i0 = FPR2I(0);      // (op1 dst)
    i2 = FPR2I(2);      // (op1 dst)
    i4 = FPR2I(4);      // (op2 src)
    i6 = FPR2I(6);      // (op2 src)

    fpc = regs->fpc;    // FPC value...
    fpc &= ~FPC_DXC;    // ... without DXC

    /* Retrieve souce floating-point value to be converted */

    ftab[0] = (unsigned int) (regs->fpr[ i4 + 0 ]);
    ftab[1] = (unsigned int) (regs->fpr[ i4 + 1 ]);
    ftab[2] = (unsigned int) (regs->fpr[ i6 + 0 ]);
    ftab[3] = (unsigned int) (regs->fpr[ i6 + 1 ]);

    /* Check for Reserved/Invalid Operation-Type Code */
    if (otc != 1)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Check for Reserved/Invalid Rounding Method */
    if (rm >= 2 && rm <= 7)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* DFP DQPC only valid if FP Extension Facility installed */
    if (1
        && GR0_TR_DQPC( optbits )
        && !FACILITY_ENABLED( 037_FP_EXTENSION, regs )
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Check for Reserved/Invalid Operand-Format Code */
    switch (ofc1)
    {
    case 0x00: opcode =  1; break;  // HFP Short        target
    case 0x01: opcode =  2; break;  // HFP Long         target
    case 0x02: opcode =  3; break;  // HFP Extended     target

    case 0x05: opcode =  4; break;  // BFP Short        target
    case 0x06: opcode =  5; break;  // BFP Long         target
    case 0x07: opcode =  6; break;  // BFP Extended     target

    case 0x08: opcode =  7; break;  // DFP Short        target
    case 0x09: opcode =  8; break;  // DFP Long         target
    case 0x0A: opcode =  9; break;  // DFP Extended     target

    default:   opcode = -1; break;  // Reserved/Invalid target
    }
    if (0
        || opcode == -1
        || (1
            && (0
                || opcode == 4
                || opcode == 5
                || opcode == 6
               )
            && GR0_TR_BFP_RSRVD( optbits )
           )
    )
    {
        if (!test_mode)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

        regs->psw.cc = 3;
        regs->GR_L( 1 ) = 0;  //  *** See PROGRAMMING NOTE further below! ***
        return;
    }
    switch (ofc2)
    {
    case 0x00: opcode += 10; break;  // HFP Short        source
    case 0x01: opcode += 20; break;  // HFP Long         source
    case 0x02: opcode += 30; break;  // HFP Extended     source

    case 0x05: opcode += 40; break;  // BFP Short        source
    case 0x06: opcode += 50; break;  // BFP Long         source
    case 0x07: opcode += 60; break;  // BFP Extended     source

    case 0x08: opcode += 70; break;  // DFP Short        source
    case 0x09: opcode += 80; break;  // DFP Long         source
    case 0x0A: opcode += 90; break;  // DFP Extended     source

    default:   numout = -1;  break;  // Reserved/Invalid source
    }
    if (numout == -1)
    {
        if (!test_mode)
            ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

        regs->psw.cc = 3;
        regs->GR_L( 1 ) = 0;  //  *** See PROGRAMMING NOTE further below! ***
        return;
    }

    /* If test mode, validate the request itself */
    if (test_mode)
    {
        switch (opcode)
        {
        case 71: case 72: case 73:  // DFP Short     ==>  HFP Short/Long/Extended
        case 81: case 82: case 83:  // DFP Long      ==>  HFP Short/Long/Extended
        case 91: case 92: case 93:  // DFP Extended  ==>  HFP Short/Long/Extended

        case 17: case 18: case 19:  // HFP Short     ==>  DFP Short/Long/Extended
        case 27: case 28: case 29:  // HFP Long      ==>  DFP Short/Long/Extended
        case 37: case 38: case 39:  // HFP Extended  ==>  DFP Short/Long/Extended

        case 41: case 42: case 43:  // BFP Short     ==>  HFP Short/Long/Extended
        case 51: case 52: case 53:  // BFP Long      ==>  HFP Short/Long/Extended
        case 61: case 62: case 63:  // BFP Extended  ==>  HFP Short/Long/Extended

        case 14: case 15: case 16:  // HFP Short     ==>  BFP Short/Long/Extended
        case 24: case 25: case 26:  // HFP Long      ==>  BFP Short/Long/Extended
        case 34: case 35: case 36:  // HFP Extended  ==>  BFP Short/Long/Extended

        case 74: case 75: case 76:  // DFP Short     ==>  BFP Short/Long/Extended
        case 84: case 85: case 86:  // DFP Long      ==>  BFP Short/Long/Extended
        case 94: case 95: case 96:  // DFP Extended  ==>  BFP Short/Long/Extended

        case 47: case 48: case 49:  // BFP Short     ==>  DFP Short/Long/Extended
        case 57: case 58: case 59:  // BFP Long      ==>  DFP Short/Long/Extended
        case 67: case 68: case 69:  // BFP Extended  ==>  DFP Short/Long/Extended
            regs->psw.cc = 0; break;
        default:
            regs->psw.cc = 3; break;
        }
        regs->GR_L( 1 ) = 0;  //  *** See PROGRAMMING NOTE further below! ***
        return;
    }

    /* NOT test mode: Process their request... */
    switch (opcode)
    {
        // DFP Short     ==>  HFP Short/Long/Extended
    case 71: cc = dfl2hflbfl( ftab, tabout, 1, 1, optbits, 0, &fpc ); numout = 1; break;
    case 72: cc = dfl2hflbfl( ftab, tabout, 1, 2, optbits, 0, &fpc ); numout = 2; break;
    case 73: cc = dfl2hflbfl( ftab, tabout, 1, 4, optbits, 0, &fpc ); numout = 4; break;

        // DFP Long      ==>  HFP Short/Long/Extended
    case 81: cc = dfl2hflbfl( ftab, tabout, 2, 1, optbits, 0, &fpc ); numout = 1; break;
    case 82: cc = dfl2hflbfl( ftab, tabout, 2, 2, optbits, 0, &fpc ); numout = 2; break;
    case 83: cc = dfl2hflbfl( ftab, tabout, 2, 4, optbits, 0, &fpc ); numout = 4; break;

        // DFP Extended  ==>  HFP Short/Long/Extended
    case 91: cc = dfl2hflbfl( ftab, tabout, 4, 1, optbits, 0, &fpc ); numout = 1; break;
    case 92: cc = dfl2hflbfl( ftab, tabout, 4, 2, optbits, 0, &fpc ); numout = 2; break;
    case 93: cc = dfl2hflbfl( ftab, tabout, 4, 4, optbits, 0, &fpc ); numout = 4; break;

    //---------------------------------------------------------------------------------

        // HFP Short     ==>  DFP Short/Long/Extended
    case 17: cc = hflbfl2dfl( ftab, tabout, 1, 1, optbits, 0, &fpc ); numout = 1; break;
    case 18: cc = hflbfl2dfl( ftab, tabout, 1, 2, optbits, 0, &fpc ); numout = 2; break;
    case 19: cc = hflbfl2dfl( ftab, tabout, 1, 4, optbits, 0, &fpc ); numout = 4; break;

        // HFP Long      ==>  DFP Short/Long/Extended
    case 27: cc = hflbfl2dfl( ftab, tabout, 2, 1, optbits, 0, &fpc ); numout = 1; break;
    case 28: cc = hflbfl2dfl( ftab, tabout, 2, 2, optbits, 0, &fpc ); numout = 2; break;
    case 29: cc = hflbfl2dfl( ftab, tabout, 2, 4, optbits, 0, &fpc ); numout = 4; break;

        // HFP Extended  ==>  DFP Short/Long/Extended
    case 37: cc = hflbfl2dfl( ftab, tabout, 4, 1, optbits, 0, &fpc ); numout = 1; break;
    case 38: cc = hflbfl2dfl( ftab, tabout, 4, 2, optbits, 0, &fpc ); numout = 2; break;
    case 39: cc = hflbfl2dfl( ftab, tabout, 4, 4, optbits, 0, &fpc ); numout = 4; break;

    //---------------------------------------------------------------------------------

        // BFP Short     ==>  HFP Short/Long/Extended
    case 41: cc = bfl2hfl(    ftab, tabout, 1, 1, optbits,    &fpc ); numout = 1; break;
    case 42: cc = bfl2hfl(    ftab, tabout, 1, 2, optbits,    &fpc ); numout = 2; break;
    case 43: cc = bfl2hfl(    ftab, tabout, 1, 4, optbits,    &fpc ); numout = 4; break;

        // BFP Long      ==>  HFP Short/Long/Extended
    case 51: cc = bfl2hfl(    ftab, tabout, 2, 1, optbits,    &fpc ); numout = 1; break;
    case 52: cc = bfl2hfl(    ftab, tabout, 2, 2, optbits,    &fpc ); numout = 2; break;
    case 53: cc = bfl2hfl(    ftab, tabout, 2, 4, optbits,    &fpc ); numout = 4; break;

        // BFP Extended  ==>  HFP Short/Long/Extended
    case 61: cc = bfl2hfl(    ftab, tabout, 4, 1, optbits,    &fpc ); numout = 1; break;
    case 62: cc = bfl2hfl(    ftab, tabout, 4, 2, optbits,    &fpc ); numout = 2; break;
    case 63: cc = bfl2hfl(    ftab, tabout, 4, 4, optbits,    &fpc ); numout = 4; break;

    //---------------------------------------------------------------------------------

        // HFP Short     ==>  BFP Short/Long/Extended
    case 14: cc = hfl2bfl(    ftab, tabout, 1, 1, optbits,    &fpc ); numout = 1; break;
    case 15: cc = hfl2bfl(    ftab, tabout, 1, 2, optbits,    &fpc ); numout = 2; break;
    case 16: cc = hfl2bfl(    ftab, tabout, 1, 4, optbits,    &fpc ); numout = 4; break;

        // HFP Long      ==>  BFP Short/Long/Extended
    case 24: cc = hfl2bfl(    ftab, tabout, 2, 1, optbits,    &fpc ); numout = 1; break;
    case 25: cc = hfl2bfl(    ftab, tabout, 2, 2, optbits,    &fpc ); numout = 2; break;
    case 26: cc = hfl2bfl(    ftab, tabout, 2, 4, optbits,    &fpc ); numout = 4; break;

        // HFP Extended  ==>  BFP Short/Long/Extended
    case 34: cc = hfl2bfl(    ftab, tabout, 4, 1, optbits,    &fpc ); numout = 1; break;
    case 35: cc = hfl2bfl(    ftab, tabout, 4, 2, optbits,    &fpc ); numout = 2; break;
    case 36: cc = hfl2bfl(    ftab, tabout, 4, 4, optbits,    &fpc ); numout = 4; break;

    //---------------------------------------------------------------------------------

        // DFP Short     ==>  BFP Short/Long/Extended
    case 74: cc = dfl2hflbfl( ftab, tabout, 1, 1, optbits, 1, &fpc ); numout = 1; break;
    case 75: cc = dfl2hflbfl( ftab, tabout, 1, 2, optbits, 1, &fpc ); numout = 2; break;
    case 76: cc = dfl2hflbfl( ftab, tabout, 1, 4, optbits, 1, &fpc ); numout = 4; break;

        // DFP Long      ==>  BFP Short/Long/Extended
    case 84: cc = dfl2hflbfl( ftab, tabout, 2, 1, optbits, 1, &fpc ); numout = 1; break;
    case 85: cc = dfl2hflbfl( ftab, tabout, 2, 2, optbits, 1, &fpc ); numout = 2; break;
    case 86: cc = dfl2hflbfl( ftab, tabout, 2, 4, optbits, 1, &fpc ); numout = 4; break;

        // DFP Extended  ==>  BFP Short/Long/Extended
    case 94: cc = dfl2hflbfl( ftab, tabout, 4, 1, optbits, 1, &fpc ); numout = 1; break;
    case 95: cc = dfl2hflbfl( ftab, tabout, 4, 2, optbits, 1, &fpc ); numout = 2; break;
    case 96: cc = dfl2hflbfl( ftab, tabout, 4, 4, optbits, 1, &fpc ); numout = 4; break;

    //---------------------------------------------------------------------------------

        // BFP Short     ==>  DFP Short/Long/Extended
    case 47: cc = hflbfl2dfl( ftab, tabout, 1, 1, optbits, 1, &fpc ); numout = 1; break;
    case 48: cc = hflbfl2dfl( ftab, tabout, 1, 2, optbits, 1, &fpc ); numout = 2; break;
    case 49: cc = hflbfl2dfl( ftab, tabout, 1, 4, optbits, 1, &fpc ); numout = 4; break;

        // BFP Long      ==>  DFP Short/Long/Extended
    case 57: cc = hflbfl2dfl( ftab, tabout, 2, 1, optbits, 1, &fpc ); numout = 1; break;
    case 58: cc = hflbfl2dfl( ftab, tabout, 2, 2, optbits, 1, &fpc ); numout = 2; break;
    case 59: cc = hflbfl2dfl( ftab, tabout, 2, 4, optbits, 1, &fpc ); numout = 4; break;

        // BFP Extended  ==>  DFP Short/Long/Extended
    case 67: cc = hflbfl2dfl( ftab, tabout, 4, 1, optbits, 1, &fpc ); numout = 1; break;
    case 68: cc = hflbfl2dfl( ftab, tabout, 4, 2, optbits, 1, &fpc ); numout = 2; break;
    case 69: cc = hflbfl2dfl( ftab, tabout, 4, 4, optbits, 1, &fpc ); numout = 4; break;

    //---------------------------------------------------------------------------------

    default: numout = -1; break;
    }


    /*****************************************************************/
    /*                    PROGRAMMING NOTE!                          */
    /*****************************************************************/
    /*                                                               */
    /*  Set Return Code value in GR1 to zero. FIXME: we know this    */
    /*  isn't right, but it's the best we can do for now. We will    */
    /*  fix it later. Besides, it's very likely zero IS correct!     */
    /*                                                               */
    /*****************************************************************/
    regs->GR_L( 1 ) = 0;


    /* Specification Exception if Invalid Operation Code */
    if (numout < 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Retrieve/Set DXC/FPC results */
    dxc = (fpc & FPC_DXC) >> FPC_DXC_SHIFT;
    regs->dxc = dxc;
    regs->fpc = fpc;

    /* If not Invalid Operation, update registers with the results */
    if (!(fpc & FPC_DXC_I))
    {
        regs->fpr[ i0 + 0 ] = tabout[0];            // (short/long/extended)

        if (numout == 1)                            // (short?)
            regs->fpr[ i0 + 1 ] = 0;                // (short)
        else
        {
            regs->fpr[ i0 + 1 ] = tabout[1];        // (long/extended)

            if (numout > 2)                         // (extended?)
            {
                regs->fpr[ i2 + 0 ] = tabout[2];    // (extended)
                regs->fpr[ i2 + 1 ] = tabout[3];    // (extended)
            }
        }
    }

    if (cc >= 0)
    {
        // FIXME: currently, CC1 is never returned because cc=1 is
        // never returned by any of the above conversion functions,
        // but it's the best we can do for now. We'll fix it later.
        regs->psw.cc = cc;

        /* If GR0 says suppress inexact errors, then turn off the FPC bit */
        if (fpc & FPC_FLAG_SFX) // inexact
        {
            /* Generate a Program Interuption if the mask is in place
               and the error is not being purposely suppressed.
            */
            if ((fpc & FPC_MASK_IMX) && !GR0_IS( optbits ))
            {
                fpc &= ~FPC_DXC;
                fpc |= DXC_IEEE_INEXACT_INCR << FPC_DXC_SHIFT;

                regs->fpc = fpc;
                regs->dxc = DXC_DECIMAL;

                ARCH_DEP( program_interrupt )( regs, PGM_DATA_EXCEPTION );
            }

            /* Otherwise if inexact suppression was requested, turn off
               that bit in the FPC.
            */
            if (GR0_IS( optbits ))
                regs->fpc &= ~FPC_FLAG_SFX;
        }
    }
    else // (cc < 0)
    {
        regs->fpc = fpc;
        ARCH_DEP( program_interrupt )( regs, PGM_DATA_EXCEPTION );
    }

} /* end DEF_INST( perform_floating_point_operation ) */

#endif /* defined( FEATURE_044_PFPO_FACILITY ) */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "pfpo.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "pfpo.c"
  #endif

#endif /* !defined( _GEN_ARCH ) */
