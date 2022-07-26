![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)


# Adding New z/Architecture Facilities to Hercules


## Contents

1. [Introduction](#1-Introduction)
2. [Defining the STFLE feature bits](#2-Defining-the-STFLE-feature-bits)
3. [Defining the feat900 `FEATURE` build macros](#3-Defining-the-feat900-FEATURE-build-macros)
4. [Verifying `FEATURE` build macro sanity](#4-Verifying-FEATURE-build-macro-sanity)
5. [Defining the facility.c run-time tables and functions](#5-Defining-the-facility-c-run-time-tables-and-functions)<br>
   5a. [The `FT` table](#5a-The-FT-table)<br>
   5b. [The `FT2` table](#5b-The-FT2-table)<br>
   5c. [`modnnn` Modification Check functions](#5c-modnnn-Modification-Check-functions)<br>
   5d. [`instrxxx` Update Opcode Table functions](#5d-instrxxx-Update-Opcode-Table-functions)<br>
6. [Coding the facility instructions themselves](#6-Coding-the-facility-instructions-themselves)<br>
   6a. [`DEF_INST` in `opcode.h`](#6a-DEF_INST-in-opcode-h)<br>
   6b. [`UNDEF_INST` and `GENx___x___x900` in `opcode.c`](#6b-UNDEF_INST-and-GENx___x___x900-in-opcode-c)<br>
   6c. [The actual `esame.c` instruction function itself](#6c-The-actual-esame-c-instruction-function-itself)<br>



## 1. Introduction

IBM's z/Architecture supports the concept of Facilities, many of which have a corresponding
facility list bit assigned to them, available to the program via the STORE FACILITY LIST
EXTENDED (STFLE) instruction.

Hercules's support of z/Architecture facilities is controlled
by the `facility.c`, `stfl.h`, `feat900.h` and `featchk.h` source files:

  - &nbsp; [stfl.h](../stfl.h) &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; defines the actual STFLE facility list bits themselves
  - &nbsp; [feat900.h](../feat900.h) &nbsp; &nbsp; &nbsp; defines the `FEATURE...` build macros
  - &nbsp; [featchk.h](../featchk.h) &nbsp; &nbsp; &nbsp; enforces `FEATURE...` build macro sanity
  - &nbsp; [facility.c](../facility.c) &nbsp; &nbsp; &nbsp; &nbsp; implements the `facility` command and table-based control of facility bits and instructions

&nbsp;



## 2. Defining the STFLE feature bits

The [stfl.h](../stfl.h) header defines the actual STFLE feature bits themselves. The `#define`
macros contain the bit numbers in their name since many of the defined facilities have very
similar names. This reduces the likelihood of using the wrong macro name in the code.

The [stfl.h](../stfl.h) header looks like this:

```C
      #define STFL_000_N3_INSTR              0    /* Instructions marked N3
                                                     are installed             */
      #define STFL_001_ZARCH_INSTALLED       1    /* z/Arch mode is available on
                                                     this processor            */
      #define STFL_002_ZARCH_ACTIVE          2    /* z/Architecture architecural
                                                     mode active. When bit 2 and
                                                     168 are both zero, ESA/390
                                                     mode is active. When bit 2
                                                     is zero and bit 168 is one,
                                                     ESA/390-compatibility mode
                                                     is active.                */
      #define STFL_003_DAT_ENHANCE_1         3    /* DAT-Enhancement Facility 1
                                                     is installed.             */
      #define STFL_004_IDTE_SC_SEGTAB        4    /* IDTE selective clearing
                                                     when segtab invalidated. Bit
                                                     3 is one if bit 4 is one. */
      ...etc...

      #define STFL_153_UNASSIGNED          153    /* Unassigned                */

      #define STFL_154_UNASSIGNED          154    /* Unassigned                */

      #define STFL_155_MSA_EXTENSION_9     155    /* Message-security-assist-
                                                     extension-9 installed.
                                                     Bits 76 and 77 are one
                                                     when bit 155 is one.      */

      #define STFL_156_IBM_INTERNAL        156    /* IBM internal use          */

      //efine STFL_nnn_UNASSIGNED      157-167    /* Unassigned or IBM internal*/

      #define STFL_168_ESA390_COMPAT_MODE  168    /* ESA/390-compatibility-mode.
                                                     Bit 168 can only be 1 when
                                                     bit 2 is zero.            */

      #define STFL_IBM_LAST_BIT            168    /* Last defined IBM facility */

      #define STFL_IBM_BY_SIZE        (ROUND_UP( STFL_IBM_LAST_BIT, 8 ) / 8)
      #define STFL_IBM_DW_SIZE        (ROUND_UP( STFL_IBM_BY_SIZE, sizeof( DW )) / sizeof( DW ))
```

&nbsp;

_**Please note**_ the comments on some of the definitions (e.g.
`"Bits 76 and 77 are one when bit 155 is one"`). It is _**very important**_ to
document such conditions as their enforcement is one of the primary purposes
of the `facility` command, controlled via the [`FT2` table](#5b-The-FT2-table)
and corresponding [`modnnn` functions](#5c-modnnn-Modification-Check-Functions).

&nbsp;



## 3. Defining the feat900 `FEATURE` build macros

Our `FEATURE...` build macros control actual code generation for a given build
architecture according to whether the macro is defined or not in each architecture's
"feature" header (e.g. [feat370.h](../feat370.h) for System/370, [feat390.h](../feat390.h)
for ESA/390 or [feat900.h](../feat900.h) for z/Architecture).

The `FEATURE_xxx...` #defines for facilities contain the bit numbers in their name,
just like the `STFL_xxx...` #defines in header [stfl.h](../stfl.h) do:

```C
      ...

      #define FEATURE_037_FP_EXTENSION_FACILITY                       /*@SRO*/
      //efine FEATURE_038_OP_CMPSC_FACILITY
      #define FEATURE_040_LOAD_PROG_PARAM_FACILITY
      #define FEATURE_041_DFP_ROUNDING_FACILITY
      #define FEATURE_041_FPR_GR_TRANSFER_FACILITY
      #define FEATURE_041_FPS_ENHANCEMENT_FACILITY
      #define FEATURE_041_FPS_SIGN_HANDLING_FACILITY
      #define FEATURE_041_IEEE_EXCEPT_SIM_FACILITY
      #define FEATURE_042_DFP_FACILITY                                /*DFP*/
      #define FEATURE_043_DFP_HPERF_FACILITY

      ...etc...
```

&nbsp;



## 4. Verifying `FEATURE` build macro sanity

The [featchk.h](../featchk.h) header not only #defines our all-important
`_FEATURE_xxx...` _**underscore**_ macros (depending on whether or not the given
feature is #defined for _any_ of the build architectures), but also enforces
feature definition sanity.

For example, it checks to make sure that if the
Constrained-Transactional-Execution Facility FEATURE is #defined,
that the Transactional-Execution Facility FEATURE is also #defined:

```C
      #if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY )
       #define    _FEATURE_050_CONSTR_TRANSACT_FACILITY
      #endif

      ...

      #if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
       #define    _FEATURE_073_TRANSACT_EXEC_FACILITY
      #endif

      ...

      #if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY ) && !defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
       #error Constrained-transactional-execution facility requires Transactional-execution facility
      #endif
```

The same facility dependency concept (one facility being dependent on, or implying, another)
is also enforced at runtime (but accomplished differently of course) by the
[`modnnn`](#5c-modnnn-Modification-Check-Functions) function declared in the
facility's [`FT2`](#5b-The-FT2-table) table entry.

&nbsp;



## 5. Defining the facility-c run-time tables and functions

The code in [facility.c](../facility.c) controls virtually all aspects of Hercules's
facility support, creating (initializing) the facility list bit strings in SYSBLK,
allowing user control over the setting or clearing (enabling or disabling) of any
given facility via the `facility` command, as well disabling or enabling instructions
associated with a given facility.


### 5a. The `FT` table

The `FT` table is an architecture _dependent_ table that gets built differently
for each #defined build architecture (`OPTION_370_MODE`, `OPTION_390_MODE` and
`OPTION_900_MODE`) depending on which `FEATURE_999_XXX...` facilities are #defined
for each build architecture.

During Hercules startup and initialization, [bldcfg.c](../bldcfg.c)'s `build_config`
function calls into [facility.c](../facility.c)'s `init_facilities_lists` function
to initialize the `sysblk.facility_list` variable in `SYSBLK`. It first merges the
three separate architecture _dependent_ `FT` tables into one master internal
architecture _independent_ table called `factab`
(the [`FT2` table](#5b-The-FT2-table) controls this merging),
and it is this master `factab` table
that is then used to initialize each architecture's `sysblk.facility_list` variable
in `SYSBLK` depending on whether the given facility is enabled or not for that
particular architecture or not.

The format of the `FT` table is quite simple:

* **Supported:**   &nbsp; &nbsp; which architecture(s) the given facility applies to
* **Default:**     &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; which architecture(s) have the facility enabled by default
* **Required:**    &nbsp; &nbsp; &nbsp; which architecture(s) REQUIRE the facility (which prevents it from being disabled)
* **Short name:**  &nbsp; the abbreviated "name" of the facility as used by the `FACILITY_ENABLED` macro (which is just the [stfl.h](../stfl.h) header #define name without the "STFL_"):
 


```C
      /*-------------------------------------------------------------------*/
      /*              Temporary ARCH_DEP Facility Table                    */
      /*-------------------------------------------------------------------*/

      static FACTAB ARCH_DEP( facs_tab )[] =      /* Arch-DEPENDENT table  */
      {
      /*-------------------------------------------------------------------*/
      /*  Sup   Def   Req   Short Name...                                  */
      /*-------------------------------------------------------------------*/

      ...

      #if defined(  FEATURE_018_LONG_DISPL_INST_FACILITY )
      FT( Z90X, Z900, Z900, 018_LONG_DISPL_INST )
      #endif

      ...etc...
```

The **Sup** (Supported), **Def** (Default) and **Req** (Required) parameters use
one of eight defined values #defined at the very beginning of [facility.c](../facility.c):

* &nbsp; **NONE** &nbsp; &nbsp; (no architectures or facility disabled)
* &nbsp; **S370** &nbsp; &nbsp; &nbsp; (S/370 only)
* &nbsp; **E390** &nbsp; &nbsp; &nbsp; (ESA/390 only)
* &nbsp; **Z900** &nbsp; &nbsp; &nbsp; (z/Arch only)
* &nbsp; **Z390** &nbsp; &nbsp; &nbsp; (both ESA/390 and z/Arch)
* &nbsp; **Z39X** &nbsp; &nbsp; &nbsp; (E390 + Z900 + optionally S370)
* &nbsp; **Z90X** &nbsp; &nbsp; &nbsp; (Z900 + optionally S370)
* &nbsp; **MALL** &nbsp;&nbsp; &nbsp; (all architectures)



### 5b. The `FT2` table

The `FT2` table defines additional information for each facility defined to the
system, such as the name of the facility's [`modnnn`](#5c-modnnn-Modification-Check-Functions)
Modification Check function, the name of the facility's 
[`instrxxx`](#5d-instrxxx-Update-Opcode-Table-functions) Update Opcode Table function
and the facility's "Long" name (description).

The information in the `FT2` table is "merged" with each architecture's
[`FT`](#5a-The-FT-table) table (by the `init_facilities_lists` function
called by [bldcfg.c](../bldcfg.c)'s `build_config` function during Hercules
startup and initialization) to create the master `factab` table used to
initialize the `sysblk.facility_list` variable in `SYSBLK`.

The [`modnnn`](#5c-modnnn-Modification-Check-Functions) parameter defines the
name of the facility's Modification Check function which defines the function
that controls the enabling and disabling of that particular facility bit when
the given facility requires or implies one or more other facility bits also
being set. Refer to the next section just below for more information about the
[`modnnn`](#5c-modnnn-Modification-Check-Functions) Modification Check function.

The [`instrxxx`](#5d-instrxxx-Update-Opcode-Table-functions) Update Opcode Table function
parameter defines the function which controls the enabling or disabling of the actual
instructions themselves defined by the facility. That is to say, certain facilities
define new z/Architecture instructions which only exist if that given facility exists
(i.e. if that particular facility list bit is one). If the facility doesn't exist (i.e.
if the facility list bit is off or zero), then the instructions that facility introduced
do not exist, and attempts to execute such instructions cause an immediate "Operation
Exception" Program Check interruption.


```C
      /*-------------------------------------------------------------------*/
      /* The ACTUAL facilities table, initialized by init_facilities_lists */
      /*-------------------------------------------------------------------*/
      /*  The individual ARCH_DEP( facs_tab ) tables are merged into this  */
      /*  table to yield the actual facilities table the system will use.  */
      /*  Refer to init_facilities_lists() function for how this is done.  */
      /*-------------------------------------------------------------------*/

      static FACTAB factab[] =
      {
      /*----------------------------------------------------------------------------*/
      /*   (func)   (func)    Short Name...          Long Description...            */
      /*----------------------------------------------------------------------------*/

      ...

      FT2( mod018,  instr18,  018_LONG_DISPL_INST,   "Long-Displacement Facility" )
      FT2( mod019,  NULL,     019_LONG_DISPL_HPERF,  "Long-Displacement Facility Has High Performance" )

      ...etc...

      FT2( NULL,    instr21,  021_EXTENDED_IMMED,    "Extended-Immediate Facility" )

      ...etc...
```

The "Long name" is simply the official descriptive name of the given facility and is used
by the `facility` command when a display of the available facilities is requested.
(The `facility` command supports listing available facilities by either SHORT or LONG name.)





### 5c. `modnnn` Modification Check functions

The `modnnn` Modification Check functions are defined in the [`FT2`](#5b-The-FT2-table)
table entries and control the enabling or disabling of a given facility for those
facilities which are dependent on one or more other facilities.

For example, facility 18 is the "Long-Displacement Facility" and facility 19 is
the "Long-Displacement Facility Has High Performance" facility. If the "Long-Displacement
Facility Has High Performance" (bit 19) is enabled then it follows that the "Long-Displacement
Facility" (bit 18) must necessarily also be enabled. That is to say, you cannot have
facility 19 enabled without facility 18 also being enabled.

On the other hand, you _may_ have facility 18 enabled but _not_ facility 19. That is
allowed. But having 19 enabled without _also_ having 18 enabled too, is _**invalid**_.

It is the `modnnn` Modification Check function's job to enforce such restrictions,
and such functions are defined in the first parameter of the [`FT2`](#5b-The-FT2-table)
table. The actual function itself that does the enforcement looks like this:


```C
      static  bool  mod018    ( bool enable, int bitno, int archnum, ...
      static  bool  mod019    ( bool enable, int bitno, int archnum, ...

      ...

      /*-------------------------------------------------------------------*/
      /*                          mod018                                   */
      /*-------------------------------------------------------------------*/
      /*                       required by 19                              */
      /*-------------------------------------------------------------------*/
      FAC_MOD_OK_FUNC           ( mod018 )
      {
          if (!enable) // disabling
          {
              if (FACILITY_ENABLED_ARCH( 019_LONG_DISPL_HPERF, archnum ))
                  return HHC00890E( STFL_019_LONG_DISPL_HPERF );
          }
          return true;
      }

      /*-------------------------------------------------------------------*/
      /*                          mod019                                   */
      /*-------------------------------------------------------------------*/
      /*                     also requires 18                              */
      /*-------------------------------------------------------------------*/
      FAC_MOD_OK_FUNC           ( mod019 )
      {
          if (enable)
          {
              if (!FACILITY_ENABLED_ARCH( 018_LONG_DISPL_INST, archnum ))
                  return HHC00890E(  STFL_018_LONG_DISPL_INST );
          }
          return true;
      }
```

**Please note** that the above functions not only prevent enabling bit 19 unless
bit 18 is first enabled, but also prevents bit 18 from being _disabled_ as well,
unless bit 19 is first disabled beforehand.

_**For reference**_, I have manually created the following tables which documents
the various interfacility dependencies according to the _May 2022_ version of manual
SA22-7832-_**13** "z/Architecture Principles of Operation"_:

<center>

### Facility Dependencies

  Bit  |  Requires ...                  |  Required _by_ ...
------:|:-------------------------------|:--------------------------------------
  000  |                                |  007
  003  |                                |  004, 005
  004  |  003                           |  005
  005  |  003, 004                      |
  007  |  000                           |
  008  |                                |  078
  014  |                                |  149
  018  |                                |  019
  019  |  018                           |
  025  |                                |  139
  028  |                                |  139
  037  |  042                           |
`+` 040|                                |  068
  042  |                                |  037, 043
  043  |  042                           |
  045  |                                |  061
  048  |  042                           |
  049  |                                |  073, 081
  050  |  073                           |
  051  |                                |  194
  061  |  045                           |
`+` 067|                                |  068, 142
`+` 068|  040, 067                      |
  073  |  049                           |  050
  076  |                                |  146, 155
  077  |                                |  155
  078  |  008                           |
  080  |  042                           |
  081  |  049                           |
  129  |                                |  134, 135, 148, 152, 165, 192
  134  |  129                           |  152, 192
  135  |  129                           |  148
  139  |  025, 028                      |
`+` 142|  067                           |
  146  |  076                           |
  148  |  129, 135                      |
  149  |  014                           |
  152  |  129, 134                      |  192
  155  |  076, 077                      |
  165  |  129                           |
  192  |  129, 134, 152                 |
  193  | (PER-3)                        |
  194  |  051                           |
  196  |                                |  197
  197  |  196                           |


</center>

    +    For facility bits 40, 67, 68: see pages vii and 2-1, and reference 7 on page viii
         of manual SA23-2260-05 "Load-Program-Parameter and CPU-Measurement Facilities".

         For facility bits 67, 142: see reference 10 on page xxxii of manual SA22-7832-13
         "z/Architecture Principles of Operation".

<center>

### Facility Incompatibilities

  Bit  | Incompatible with ...
------:|:--------------------------
`+` 002|  168
  010  |  169
  014  |  169
  066  |  169
  145  |  169
  149  |  169
`+` 168|  002
  169  |  010, 014, 066, 145, 149

</center>

      +    For facility bits 002 and 168: either bit may be on, or neither bit may be on,
           but both bits can never be on at the same time.

### 5d. `instrxxx` Update Opcode Table functions

For those facilities which introduce new z/Architecture instructions to go along
with the facility, the `instrxxx` function (defined as the second parameter of
the [`FT2`](#5b-The-FT2-table) table) defines the list of instructions that only
exist when the given facility is enabled.

The function is called by the `init_facilities_lists` function at Hercules startup
(as well as by the `facility` command too whenever a facility is manually enabled
or disabled) to patch (update) the `opcode.c` instruction table to either enable or
disable the given set of instructions depending on whether the given facility is
enabled or disabled for that architecture.

This eliminates the need for each individual instruction from having to manually
check whether the given facility is enabled or not (via the `FACILITY_ENABLED( ... )`
macro) and then having to manually call the `program_interrupt` function to throw
an operation exception if it's not. Instead, this is all handled automatically
by each facility's defined `instrxxx` function, which looks like this:


```C
      static void instr21  ( int arch, bool enable );

      ...

      BEG_DIS_FAC_INS_FUNC( instr21 )
      {
          DIS_FAC_INS( C208, "AGFI    C208  ADD IMMEDIATE (64 <- 32)" );
          DIS_FAC_INS( C209, "AFI     C209  ADD IMMEDIATE (32)" );

          ...etc...

          DIS_FAC_INS( B907, "LGHR    B907  LOAD HALFWORD (64 <- 16)" );
          DIS_FAC_INS( B927, "LHR     B927  LOAD HALFWORD (32 <- 16)" );

          ...etc...

          DIS_FAC_INS( C204, "SLGFI   C204  SUBTRACT LOGICAL IMMEDIATE (64 <- 32)" );
          DIS_FAC_INS( C205, "SLFI    C205  SUBTRACT LOGICAL IMMEDIATE (32)" );
      }
      END_DIS_FAC_INS_FUNC()
```

The first parameter of the `DIS_FAC_INS` macro is obviously the instruction's
hexadecimal opcode, and the second parameter is simply a unique descriptive name
for that particular instruction.

&nbsp;





## 6. Coding the facility instructions themselves

Implementing a new instruction in Hercules involves updating three source files:
the [opcode.h](../opcode.h) header (which declares its existence), the
[opcode.c](../opcode.c) instruction dispatch table (directing the `run_cpu`
instruction execution loop in [cpu.c](../cpu.c) to jump to the
actual instruction function itself), and of course the actual instruction
function itself (which does not necessarily have to be in source file `esame.c`
but may instead be in a completely different source file, possibly its own).


### 6a. `DEF_INST` in `opcode-h`

Within header file [opcode.h](../opcode.h), simply insert a new `DEF_INST` macro
for your new instruction, guarded with the appropriate
`#if defined( FEATURE_999_xxxx...)` statement (where `_999_xxx...` is of course
the named of the `FEATURE` macro you defined in your [`feat900.h`](../feat900.h)
header):

```C
      #if defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
      DEF_INST( perform_processor_assist );
      #endif
```


### 6b. `UNDEF_INST` and `GENx___x___x900` in `opcode-c`

Within the [opcode.c](../opcode.c) source file, insert a `UNDEF_INST` macro for
your new instruction guarded with an appropriate
<i>#if <u><b>!</b></u>defined( FEATURE_999_xxxx...)</i> statement:

```C
      #if !defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
       UNDEF_INST( perform_processor_assist );
      #endif
```

Then about halfway down, update the appropriate `GENx___x___x900` macro statement
for your instruction's opcode, defining the name of your instruction function,
the instruction's decoder format and its mnemonic:


```C
       /*B2E8*/ GENx___x___x900 (perform_processor_assist,RRF_M,"PPA"),
```

Note that each `x___` spot in the macro's name corresponds to a given build architecture.
The first `x___` being replaced with `x370` if the given instruction is defined to the
System/370 architecture, the second being replaced with `x390` if the instruction is
defined to the ESA/390 architecture and the third spot being replaced with `x900` if the
instruction is defined to z/Architecture. The `/*B2E8*/` is of course just a helpful
comment documenting the instruction's opcode.




### 6c. The actual `esame-c` instruction function itself

Depending on its complexity, this is perhaps the easiest part: coding the actual
instruction function itself.

All you need to be careful to do is to wrap (guard) your function with the
appropriate `#if defined( FEATURE_999_xxx...)` and `#endif` statements so that
it is only compiled if that particular FEATURE is #defined for the given build
architecture:


```C
      #if defined( FEATURE_021_EXTENDED_IMMED_FACILITY )

      /*-------------------------------------------------------------------*/
      /* B907 LGHR  - Load Long Halfword Register                    [RRE] */
      /*-------------------------------------------------------------------*/
      DEF_INST( load_long_halfword_register )
      {
      int     r1, r2;                         /* Values of R fields        */

          RRE( inst, regs, r1, r2 );

          /* Load sign-extended halfword from second operand register */
          regs->GR_G( r1 ) = (S64)(S16)(regs->GR_LHL( r2 ));

      } /* end DEF_INST( load_long_halfword_register ) */

      #endif /* defined( FEATURE_021_EXTENDED_IMMED_FACILITY ) */
```


**Please note** that under normal circumstances there is no need to code any
`if (FACILITY_ENABLED( ... ))` test anywhere in your instruction if your
instruction is only defined when the given facility is enabled, as this is
handled automatically by the associated `facility.c` [`BEG_DIS_FAC_INS_FUNC`
function](#5d-instrxxx-Update-Opcode-Table-functions) (controlled by the
[`instrxxx`](#5d-instrxxx-Update-Opcode-Table-functions) second parameter of
the [`FT2`](#5b-The-FT2-table) table.)

When the facility is enabled, the instruction is defined and will be called.
When the facility is _not_ enabled, the instruction is _not_ defined and will
automatically program-check if the guest attempts to execute it. That's one
of the primary purposes of the code in [`facility.c`](../facility.c).

Thus you can be assured that if your instruction is called, the corresponding
facility is indeed enabled. Otherwise your instruction function would never
have been called! Thus any use of `if (FACILITY_ENABLED( ... ))` statement is
completely unnecessary.

_The **only** time you **might** need to_ code a `if (FACILITY_ENABLED( ... ))`
statement is if the instruction in question is defined to behave _differently_
depending on whether a given facility is enabled (installed) or not. For example,
take a look at the `IPTE` (Invalidate Page Table Entry) and `SSKE` (Set Storage
Key extended) instructions in `control.c`.

The `IPTE` instruction contains a `if (FACILITY_ENABLED( ... ))` check for
each of the `051_LOCAL_TLB_CLEARING` and `013_IPTE_RANGE` facilities because
it behaves differently depending on whether either of those facilities is
enabled or not.

Similarly, the `SSKE` instruction contains a `if (FACILITY_ENABLED( ... ))`
check for the `008_EDAT_1` facility because it too behaves differently depending
on whether that particular facility is enabled or not.

These are likely the _only_ times an instruction function might actually need
to use an `if (FACILITY_ENABLED( ... ))` statement.

_**But the key point is,**_ you _don't_ need to do any `if (FACILITY_ENABLED( ... ))`
test simply to check whether or not your _INSTRUCTION EXISTS_ due to whether
or not your facility is enabled or not. _That_ type of check (test) is handled
automatically by the `FT2` table's second parameter and corresponding `instrxxx`
function.

&nbsp;
