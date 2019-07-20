/* ESA390IO.H   (C) Copyright "Fish" (David B. Trout), 2013          */
/*              Common I/O Device Commands structures                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Reference: SA22-7204-01 "ESA/390: Common I/O Device Commands"     */

#ifndef _ESA390IO_H
#define _ESA390IO_H

#undef ATTRIBUTE_PACKED
#if defined(_MSVC_)
 #pragma pack(push)
 #pragma pack(1)
 #define ATTRIBUTE_PACKED
#else
 #define ATTRIBUTE_PACKED __attribute__((packed))
#endif

/*-------------------------------------------------------------------*/
/* Sense-ID Command Information WORD (CIW)                           */
/*-------------------------------------------------------------------*/

// Byte 0: bits 0-1: constant zero and one, respectively
// Byte 0: bits 2-3: Reserved
// Byte 0: bits 4-7: Command type

#define CIW_TYP_RCD     0               /* Read configuration data   */
#define CIW_TYP_SII     1               /* Set interface identifier  */
#define CIW_TYP_RNI     2               /* Read node identifier      */
#define CIW_TYP_3       3               /* Reserved command type 3   */
#define CIW_TYP_4       4               /* Reserved command type 4   */
#define CIW_TYP_5       5               /* Reserved command type 5   */
#define CIW_TYP_6       6               /* Reserved command type 6   */
#define CIW_TYP_7       7               /* Reserved command type 7   */

// Byte  1:    CCW opcode
// Bytes 2-3:  CCW count

#define MAKE_CIW(_typ,_ccw,_len) \
    0x40 | (_typ), (_ccw), (_len >> 8) & 0xFF, (_len) & 0xFF

/*-------------------------------------------------------------------*/
/* Self-Describing Component (SDC), common to both NDs and NEDs      */
/*-------------------------------------------------------------------*/

struct SDC {
    BYTE    type[6];                    /* Component type            */
    BYTE    model[3];                   /* Component model           */
    BYTE    mfr[3];                     /* Component manufacturer    */
    BYTE    plant[2];                   /* Plant of manufacturer     */
    BYTE    serial[12];                 /* Sequence/serial number    */
}
ATTRIBUTE_PACKED; typedef struct SDC SDC;

#define  _SDC_TYP(_1,_2,_3,_4,_5,_6)                        { _1,_2,_3,_4,_5,_6 }
#define  _SDC_MOD(_1,_2,_3)                                 { _1,_2,_3 }
#define  _SDC_MFR(_1,_2,_3)                                 { _1,_2,_3 }
#define  _SDC_PLT(_1,_2)                                    { _1,_2 }
#define  _SDC_SER(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12)   { _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12 }
#define   MAKE_SDC( _typ, _mod, _mfr, _plt, _ser )          { _typ, _mod, _mfr, _plt, _ser }

/*-------------------------------------------------------------------*/
/* ND: Node Descriptor                                               */
/*-------------------------------------------------------------------*/

// Node Descriptors (ND), not be confused with Node-element
// Descriptors (NED), are returned to the program in response
// to the Read-Node-Identifier (RNI) CCW command.

// Byte 0: bits 0-2: Node validity

#define ND_VAL_VALID        0           /* Node is valid             */
#define ND_VAL_UNSURE       1           /* Node may not be current   */
#define ND_VAL_INVALID      2           /* Node is NOT valid         */

// Byte 0: bit    3: Node type

#define ND_TYP_DEVICE       0           /* Device node               */
#define ND_TYP_CSS          1           /* Channel-subsystem node    */

// Byte 0: bits 4-7: Reserved
// Byte 1: Reserved

// If byte 0 bit 3 = Device node:
// Byte 2: Device class
// Byte 3: Link address (if byte 2 class = protocol converter)

#define ND_DEV_UNKNOWN      0           /* Unspecified class         */
#define ND_DEV_DASD         1           /* DASD class                */
#define ND_DEV_TAPE         2           /* Magnetic tape class       */
#define ND_DEV_READER       3           /* Unit record input         */
#define ND_DEV_PUNCH        4           /* Unit record output        */
#define ND_DEV_PRINTER      5           /* Printer class             */
#define ND_DEV_COMM         6           /* Communications controller */
#define ND_DEV_DISPLAY      7           /* Full screen terminal      */
#define ND_DEV_CONSOLE      8           /* Line mode terminal        */
#define ND_DEV_CTCA         9           /* Standalone CTCA class     */
#define ND_DEV_SWITCH      10           /* Switch class              */
#define ND_DEV_PROTO       11           /* Protocol converter class
                                           (byte 3 = link address)   */
// If byte 0 bit 3 = Channel-subsystem node:
// Byte 2: Interface class
// Byte 3: CHPID

#define ND_CSS_UNKNOWN      0           /* Unspecified class         */
#define ND_CSS_CHPATH       1           /* Channel path class        */
#define ND_CSS_CTCA         2           /* Integrated CTCA class     */

// Bytes  4- 9: Type number               ( 6 bytes)
// Bytes 10-12: Model number              ( 3 bytes)
// Bytes 13-15: Manufacturer              ( 3 bytes)
// Bytes 16-17: Plant of Manufacture      ( 2 bytes)
// Bytes 18-29: Sequence Number           (12 bytes)
// Bytes 30-31: Tag (physical identifier) ( 2 bytes)

/*-------------------------------------------------------------------*/
/* Node Descriptor (ND) structure                                    */
/*-------------------------------------------------------------------*/

struct ND {
/*000*/ BYTE    flags;                  /* Node validity/type flags  */
/*001*/ BYTE    rsrvd;                  /* Reserved                  */
/*002*/ BYTE    cls;                    /* Node class                */
/*003*/ BYTE    ua;                     /* Link addr, CHPID or zero  */
/*004*/ SDC     info;                   /* SDC information           */
/*01E*/ BYTE    tag[2];                 /* Node tag  (physical id)   */
}
ATTRIBUTE_PACKED; typedef struct ND ND;

#define MAKE_ND(_typ,_cls,_by3,_sdc,_tag) \
    { (ND_VAL_VALID << 5) | (_typ << 4), 0, _cls, _by3, _sdc, _tag }

/*-------------------------------------------------------------------*/
/* NQ: Node Qualifier                                                */
/*-------------------------------------------------------------------*/

// Node Qualifiers (NQ), not be confused with Node-Element
// Qualifiers (NEQ), are returned to the program in response
// to the the Read-Node-Identifier (RNI) CCW command.

// Byte 0: bits 0-2: Node qualifier contents
// Byte 0: bits 3-7: Reserved

#define NQ_TYP_IIL          0           /* Interface ID list         */
#define NQ_TYP_MODEP        1           /* Model-dependent           */

// Bytes  1-3: Reserved
// Bytes  4-7: Reserved
// Bytes 8-31: Node qualifier information

/*-------------------------------------------------------------------*/
/* Node Qualifier (NQ) structure                                     */
/*-------------------------------------------------------------------*/

struct NQ {
/*000*/ BYTE    flags;                  /* Node contents flag        */
/*001*/ BYTE    rsrvd[7];               /* Reserved                  */
/*008*/ BYTE    info[24];               /* Model dependent info      */
}
ATTRIBUTE_PACKED; typedef struct NQ NQ;

#define MAKE_NQ( _typ, _info24 ) \
    { _typ << 5, {0,0,0,0,0,0,0}, _info24 }

/*-------------------------------------------------------------------*/
/* NED: Node-Element Descriptor                                      */
/*-------------------------------------------------------------------*/

// Node-Element Descriptors (NED), not be confused with Node
// Descriptors (ND), are returned to the program in response
// to the the Read-Configuration-Data (RCD) CCW command.

// Byte 0: bits 0-1: Field identifier

#define FIELD_IS_UNUSED     0           /* Field is unused           */
#define FIELD_IS_NEQ        1           /* Field is a specific NEQ   */
#define FIELD_IS_GENEQ      2           /* Field is a general NEQ    */
#define FIELD_IS_NED        3           /* Field is a NED            */

// Byte 0: bit 2: Token indicator

#define NED_NORMAL_NED      0           /* NED is not a token NED    */
#define NED_TOKEN_NED       1           /* NED is a token NED        */

// Byte 0: bits 3-4: Serial-number indicator

#define NED_SN_NEXT         0           /* Use next non-0 indicator  */
#define NED_SN_UNIQUE       1           /* Installation-unique       */
#define NED_SN_NODE         2           /* Normal node element       */
#define NED_SN_CODE3        3           /* (same meaning as code 2)  */

// Byte 0: bit 5: Reserved
// Byte 0: bit 6: Emulation NED

#define NED_REAL            0           /* Normal node element       */
#define NED_EMULATED        1           /* Emulated node element     */

// Byte 0: bit 7: Reserved
// Byte 1: NED type

#define NED_TYP_UNSPEC      0           /* Unspecified node type     */
#define NED_TYP_DEVICE      1           /* I/O device node type      */
#define NED_TYP_CTLUNIT     2           /* Control unit node type    */

// Byte 2: Device class (if byte 1 = device)

#define NED_DEV_UNSPEC      0           /* Unspecified class         */
#define NED_DEV_DASD        1           /* DASD class                */
#define NED_DEV_TAPE        2           /* Magnetic tape class       */
#define NED_DEV_READER      3           /* Unit record input         */
#define NED_DEV_PUNCH       4           /* Unit record output        */
#define NED_DEV_PRINTER     5           /* Printer class             */
#define NED_DEV_COMM        6           /* Communications controller */
#define NED_DEV_DISPLAY     7           /* Full screen terminal      */
#define NED_DEV_CONSOLE     8           /* Line mode terminal        */
#define NED_DEV_CTCA        9           /* Standalone CTCA class     */
#define NED_DEV_SWITCH     10           /* Switch class              */
#define NED_DEV_PROTO      11           /* Protocol converter class  */

// Byte 3: bits 0-6: Reserved
// Byte 3: bit    7: Node level

#define NED_RELATED         0           /* Node accessed after next
                                           node during normal i/o    */
#define NED_UNRELATED       1           /* Node unrelated to next    */

// Bytes  4- 9: Type number               ( 6 bytes)
// Bytes 10-12: Model number              ( 3 bytes)
// Bytes 13-15: Manufacturer              ( 3 bytes)
// Bytes 16-17: Plant of Manufacture      ( 2 bytes)
// Bytes 18-29: Sequence Number           (12 bytes)
// Bytes 30-31: Tag (physical identifier) ( 2 bytes)

/*-------------------------------------------------------------------*/
/* Node-Element Descriptor (NED) structure                           */
/*-------------------------------------------------------------------*/

struct NED {
/*000*/ BYTE    flags;                  /* Field identifier/flags    */
/*001*/ BYTE    type;                   /* Node type                 */
/*002*/ BYTE    cls;                    /* Node class                */
/*003*/ BYTE    lvl;                    /* Node level flag           */
/*004*/ SDC     info;                   /* SDC information           */
/*01E*/ BYTE    tag[2];                 /* Node tag (for uniqueness) */
}
ATTRIBUTE_PACKED; typedef struct NED NED;

#define _NED_FLAGS(_fib,_tkn,_sni,_emu)  \
    (_fib << 6) | (_tkn << 5) | (_sni << 3) | (_emu << 1)
#define MAKE_NED(_fib,_tkn,_sni,_emu,_typ,_cls,_lvl,_sdc,_tag) \
    { _NED_FLAGS(_fib,_tkn,_sni,_emu), _typ, _cls, _lvl, _sdc, _tag }

/*-------------------------------------------------------------------*/
/* NEQ: Node-Element Qualifier                                       */
/*-------------------------------------------------------------------*/

// Node-Element Qualifiers (NEQ), not be confused with Node
// Qualifiers (NQ), are returned to the program in response
// to the the Read-Configuration-Data (RCD) CCW command.

// Byte 0: bits 0-1: Field identifier
// Byte 0: bits 2-7: Reserved

//efine FIELD_IS_UNUSED     0           /* Field is unused           */
//efine FIELD_IS_NEQ        1           /* Field is a specific NEQ   */
//efine FIELD_IS_GENEQ      2           /* Field is a general NEQ    */
//efine FIELD_IS_NED        3           /* Field is a NED            */

// Byte 1: Reserved

// Bytes 2-3: Interface ID              (General NEQ only, else Reserved)
// Byte    4: Device-Dependent Timeout  (General NEQ only, else Reserved)

// Bytes 5-7: Reserved

// Bytes 8-32: Extended information

/*-------------------------------------------------------------------*/
/* Node-Element Qualifier (NEQ) structure                            */
/*-------------------------------------------------------------------*/

struct NEQ {
/*000*/ BYTE    flags;                  /* Node contents flag        */
/*001*/ BYTE    rsrvd;                  /* Reserved                  */
/*002*/ HWORD   iid;                    /* Interface-ID       (GENEQ)*/
/*004*/ BYTE    ddto;                   /* Device Timeout     (GENEQ)*/
/*005*/ BYTE    rsrvd2[3];              /* Reserved                  */
/*008*/ BYTE    info[24];               /* Extended information      */
}
ATTRIBUTE_PACKED; typedef struct NEQ NEQ;

#define MAKE_NEQ( _typ, _dto, _info24 ) \
    { _typ << 6, 0, { 0,0 }, _dto, {0,0,0}, _info24 }

/*-------------------------------------------------------------------*/
/* Some commonly used values                                         */
/*-------------------------------------------------------------------*/

#define  MODEL_001          _SDC_MOD( 0xF0, 0xF0, 0xF1 )
#define  MODEL_002          _SDC_MOD( 0xF0, 0xF0, 0xF2 )
#define  MODEL_003          _SDC_MOD( 0xF0, 0xF0, 0xF3 )
#define  MODEL_004          _SDC_MOD( 0xF0, 0xF0, 0xF4 )
#define  MODEL_005          _SDC_MOD( 0xF0, 0xF0, 0xF5 )

#define  MFR_HRC            _SDC_MFR( 0xC8, 0xD9, 0xC3 )
#define  PLANT_ZZ           _SDC_PLT( 0xE9, 0xE9 )

#define  SER_000000000000   _SDC_SER( 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0 )
#define  SER_000000000001   _SDC_SER( 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF1 )
#define  SER_000000000002   _SDC_SER( 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF2 )
#define  SER_000000000003   _SDC_SER( 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF3 )

#define  NULL_INFO_24   { 0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0,0,0 }

#define  TAG_00         { 0x00, 0x00 }
#define  TAG_01         { 0x00, 0x01 }
#define  TAG_02         { 0x00, 0x02 }
#define  TAG_03         { 0x00, 0x03 }

#define  ND_CHPID_00    0x00
#define  ND_CHPID_FF    0xFF

#define  NULL_MODEP_NQ  MAKE_NQ( NQ_TYP_MODEP, NULL_INFO_24 )
#define  NULL_NEQ       MAKE_NEQ( FIELD_IS_NEQ,   0, NULL_INFO_24 )
#define  NULL_GENEQ     MAKE_NEQ( FIELD_IS_GENEQ, 0, NULL_INFO_24 )

#if defined(_MSVC_)
 #pragma pack(pop)
#endif

#endif /* _ESA390IO_H */
