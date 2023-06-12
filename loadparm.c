/* LOADPARM.C   (C) Copyright Jan Jaeger, 2004-2012                  */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2013-2021                             */
/*              SCLP / MSSF loadparm                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains functions which set, copy, and retrieve the  */
/* values of the LOADPARM and various other environmental parameters */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _LOADPARM_C_
#define _HENGINE_DLL_

#include "hercules.h"

                                          /*  H    E    R    C    U    L    E    S  */
static const BYTE dflt_lparname[8]      = { 0xC8,0xC5,0xD9,0xC3,0xE4,0xD3,0xC5,0xE2 };

                                         /*   E    M    U    L    A    T    O    R  */
static const BYTE dflt_model[16]        = { 0xC5,0xD4,0xE4,0xD3,0xC1,0xE3,0xD6,0xD9,
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
                                          /*  H    R    C  */
static const BYTE default_manufact[16]  = { 0xC8,0xD9,0xC3,0x40,0x40,0x40,0x40,0x40,
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
                                          /*  Z    Z           */
static const BYTE default_plant[4]      = { 0xE9,0xE9,0x40,0x40 };
                                         /*   H    E    R    C    U    L    E    S  */
static const BYTE dflt_cpid[16]         = { 0xC8,0xC5,0xD9,0xC3,0xE4,0xD3,0xC5,0xE2,
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 };
                                          /*  H    E    R    C    U    L    E    S  */
static const BYTE dflt_vmid[8]          = { 0xC8,0xC5,0xD9,0xC3,0xE4,0xD3,0xC5,0xE2 };

static int gsysinfo_init_flg = FALSE;

static GSYSINFO gsysinfo;
/*                                                          defaults
                                      = { { 0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 }, // loadparm
                                          { 0xC8,0xC5,0xD9,0xC3,0xE4,0xD3,0xC5,0xE2 }, // lparname
                                          { 0xC8,0xD9,0xC3,0x40,0x40,0x40,0x40,0x40,   // manufact
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 },
                                          { 0xE9,0xE9,0x40,0x40 },                     // plant
                                          { 0xC5,0xD4,0xE4,0xD3,0xC1,0xE3,0xD6,0xD9,   // model
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 },
                                          { 0xC5,0xD4,0xE4,0xD3,0xC1,0xE3,0xD6,0xD9,   // modelcapa
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 },
                                          { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // modelperm
                                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
                                          { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   // modeltemp
                                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
                                          { 0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 }, // systype
                                          { 0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 }, // sysname
                                          { 0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 }, // sysplex
                                          { 0xC8,0xC5,0xD9,0xC3,0xE4,0xD3,0xC5,0xE2,   // cpid
                                            0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40 },
                                          { 0xC8,0xC5,0xD9,0xC3,0xE4,0xD3,0xC5,0xE2 }  // vmid
                                        };
*/

// ebcdic_to_stringz_allow_return returns a null terminated string allowing
//                                embedded spaces
#define ebcdic_to_stringz_allow_return(_field) \
{ \
    static char result[sizeof(_field)+1]; \
    int i; \
    result[sizeof(_field)] = 0; \
    for (i = sizeof(_field) - 1; i >= 0 && _field[i] == 0x40; i--) \
        result[i] = 0; \
    for (; i >= 0; i--) \
        result[i] = guest_to_host((int)_field[i]); \
    return result; \
}


// ebcdic_to_stringz_return returns a null terminated string stopping
//                          at first embedded blank or end of field

#define ebcdic_to_stringz_return(_field) \
{ \
    static char result[sizeof(_field)+1]; \
    copy_ebcdic_to_stringz(result, sizeof(result), _field, sizeof(_field)); \
    return result; \
}


// set_static

#define set_static(_target,_source) \
{ \
    size_t i = 0; \
    size_t n = MIN(strlen(_source), sizeof(_target)); \
    if (_source) \
        { \
        for (; i < n; i++) \
             if (isprint((unsigned char)(_source)[i])) \
                 _target[i] = host_to_guest((int)toupper((unsigned char)(_source)[i])); \
             else \
                 _target[i] = 0x40; \
        } \
    for (; i < sizeof(_target); i++) \
        _target[i] = 0x40; \
}


// set_stsi_and_return

#define set_stsi_and_return(_target,_source,_default) \
{ \
    size_t  i; \
    int     n; \
    BYTE    temp[sizeof(_target)]; \
    memset(temp, 0x40, sizeof(temp) ); \
    for (i = 0, n = 0; (_source) && i < strlen(_source) && i < sizeof(temp); i++) \
        if (isalnum((unsigned char)(_source)[i])) \
        { \
            temp[i] = host_to_guest((int)toupper((unsigned char)(_source)[i])); \
            n++; \
        } \
        else \
            return -1;      /* 0-9, A-Z */ \
    if ( n > 0 )            /* valid if > 0 count */ \
        memcpy(_target,temp,sizeof(_target)); \
    else \
        memcpy(_target,_default,MIN(sizeof(_target),sizeof(_default))); \
    return n; \
}

/*-------------------------------------------------------------------*/
/* Subroutine to initialize (when NULL dest is passed) or retrieve   */
/* Guest System Information                                          */
/*-------------------------------------------------------------------*/

static void get_gsysinfo(GSYSINFO *dest)
{
    if ( dest == NULL )
    {
        gsysinfo_init_flg = TRUE;
        memset(&gsysinfo, 0x40, sizeof(GSYSINFO));

        memcpy(gsysinfo.lparname,  dflt_lparname,       sizeof(gsysinfo.lparname));
        memcpy(gsysinfo.manufact,  default_manufact,    sizeof(gsysinfo.manufact));
        memcpy(gsysinfo.plant,     default_plant,       sizeof(gsysinfo.plant));
        memcpy(gsysinfo.model,     dflt_model,          sizeof(gsysinfo.model));
        memcpy(gsysinfo.modelcapa, dflt_model,          sizeof(gsysinfo.modelcapa));
        memcpy(gsysinfo.cpid,      dflt_cpid,           sizeof(gsysinfo.cpid));
        memcpy(gsysinfo.vmid,      dflt_vmid,           sizeof(gsysinfo.vmid));

        memset(gsysinfo.modelperm, 0, sizeof(gsysinfo.modelperm));
        memset(gsysinfo.modeltemp, 0, sizeof(gsysinfo.modeltemp));
    }
    else
        memcpy( dest, &gsysinfo, sizeof(GSYSINFO) );
    return;
}


/*-------------------------------------------------------------------*/
/* SUBROUTINE TO COPY A STRINGZ TO A FIXED-LENGTH EBCDIC FIELD       */
/*-------------------------------------------------------------------*/
static int copy_stringz_to_ebcdic(BYTE* fld, size_t len, char *name)
{
    size_t  i;
    size_t  copylen;
    int     n;
    BYTE    *temp_fld;

    if ( name == NULL || strlen(name) == 0 )
    {
        memset(fld, 0, len);
        return 0;
    }

    temp_fld = (BYTE *)malloc(len+1);
    memset(temp_fld, 0x40, len);
    copylen = MIN(strlen(name), len);

    for ( i = 0, n = 0; i < copylen; i++ )
        if ( isalnum((unsigned char)name[i]) )
        {
            temp_fld[i] = host_to_guest((int)toupper((unsigned char)name[i]));
            n++;
        }
        else
        {
            n = -1;
            break;
        }

    if ( n > 0 )
        memcpy(fld,temp_fld,len);

    free(temp_fld);

    return n;
}

/*-------------------------------------------------------------------*/
/* SUBROUTINE TO COPY A FIXED-LENGTH EBCDIC FIELD TO A STRINGZ       */
/*-------------------------------------------------------------------*/
static int copy_ebcdic_to_stringz(char *name, size_t nlen, BYTE* fld, size_t flen)
{
    size_t  i;
    size_t  copylen;
    char    c;

    if ( name == NULL || nlen == 0 || flen == 0 ) return -1;

    memset(name, 0, nlen);
    copylen = MIN(nlen-1, flen);

    for ( i = 0; i < copylen; i++ )
    {
        c = guest_to_host(fld[i]);

        if ( c == SPACE || !isalnum((unsigned char)c) )
            break; /* there should not be any embedded blanks */

        name[i] = c;
    }

    return 0;

}
/*-------------------------------------------------------------------*/
/* LOAD PARAMETER                                                    */
/* Set by: LOADPARM configuration statement or panel command         */
/* Retrieved by: SERVC and MSSF_CALL instructions                    */
/*-------------------------------------------------------------------*/

void set_loadparm(char *name)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    set_static(gsysinfo.loadparm, name);
}


void get_loadparm(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.loadparm, sizeof(gsysinfo.loadparm));
}


char *str_loadparm()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_allow_return(gsysinfo.loadparm);
}


/*-------------------------------------------------------------------*/
/* LOGICAL PARTITION NAME                                            */
/* Set by: LPARNAME configuration statement                          */
/* Retrieved by: STSI and MSSF_CALL instructions                     */
/*-------------------------------------------------------------------*/

void set_lparname(char *name)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    set_static(gsysinfo.lparname, name);
}


void get_lparname(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.lparname, sizeof(gsysinfo.lparname));
}


LOADPARM_DLL_IMPORT
char *str_lparname()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.lparname);
}


/*-------------------------------------------------------------------*/
/* MANUFACTURER NAME                                                 */
/* Set by: MANUFACTURER configuration statement                      */
/* Retrieved by: STSI instruction                                    */
/*-------------------------------------------------------------------*/

int set_manufacturer(char *name)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    set_stsi_and_return(gsysinfo.manufact, name, default_manufact);
}


void get_manufacturer(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.manufact, sizeof(gsysinfo.manufact));
}

LOADPARM_DLL_IMPORT
char *str_manufacturer()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.manufact);
}

/*-------------------------------------------------------------------*/
/* MANUFACTURING PLANT NAME                                          */
/* Set by: PLANT configuration statement                             */
/* Retrieved by: STSI instruction      A-Z, 0-9                      */
/*-------------------------------------------------------------------*/

int set_plant(char *name)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    set_stsi_and_return(gsysinfo.plant, name, default_plant);
}

void get_plant(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.plant, sizeof(gsysinfo.plant));
}

LOADPARM_DLL_IMPORT
char *str_plant()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.plant);
}

/*-------------------------------------------------------------------*/
/* MODEL IDENTIFICATION                                              */
/* Set by: MODEL configuration statement                             */
/* Retrieved by: STSI instruction                                    */
/* Notes: Although model and modelcapa are initially set to EBCDIC
 * "EMULATOR" it is possible for model, modeltemp, modelperm each to
 * be binary zero filled fields. "modelcapa" may never be binary zero.
 * All blanks is invalid for any of the fields. An attempt to set any
 * field other than "modelcapa" to all blanks will result in the field
 * being set to binary zeros. "modelcapa" must have at least one
 * character in the field. Valid characters are 0-9,A-Z, right padded
 * with blanks. '*' means not specified (keep existing value/default),
 * '=' means use same value that was specified in the previous field,
 * empty string means field is undefined (set field to binary zeros).
 */
/*-------------------------------------------------------------------*/

int set_model(char *m1, char *m2, char *m3, char *m4)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    /* Model may be binary zero */
    if ( m1 != NULL && m1[0] != '*' )
    {
        if ( strlen(m1) == 0 )
            memset(gsysinfo.model, 0, sizeof(gsysinfo.model));
        else if ( strcmp(m1, "=") == 0)
            memcpy(gsysinfo.model, dflt_model, sizeof(gsysinfo.model));
        else if ( copy_stringz_to_ebcdic(gsysinfo.model, sizeof(gsysinfo.model), m1) <= 0 ) return 1;
    }
    else if ( m1 == NULL )
        return 0;

    /* model-capa may NEVER be binary zero, if "" then default to
       the same value as the Model field if not binary zero. Otherwise
       if the Model field is binary zero the value 'EMULATOR' is used */
    if ( m2 != NULL && m2[0] != '*' )
    {
        if ( strlen(m2) == 0 || strcmp(m2, "=") == 0)
            memcpy(gsysinfo.modelcapa, gsysinfo.model[0] ? gsysinfo.model : dflt_model, sizeof(gsysinfo.modelcapa));
        else if ( copy_stringz_to_ebcdic(gsysinfo.modelcapa, sizeof(gsysinfo.modelcapa), m2) <= 0) return 2;
    }
    else if ( m2 == NULL )
        return 0;

    /* model-perm may be binary zero */
    if ( m3 != NULL && m3[0] != '*' )
    {
        if ( strlen(m3) == 0 )
            memset(gsysinfo.modelperm, 0, sizeof(gsysinfo.modelperm));
        else if ( strcmp(m3, "=") == 0)
            memcpy(gsysinfo.modelperm, gsysinfo.modelcapa, sizeof(gsysinfo.modelperm));
        else if ( copy_stringz_to_ebcdic(gsysinfo.modelperm, sizeof(gsysinfo.modelperm), m3) <= 0 ) return 3;
    }
    else if ( m3 == NULL )
        return 0;

    /* model-temp may be binary zero */
    if ( m4 != NULL && m4[0] != '*' )
    {
        if ( strlen(m4) == 0 )
            memset(gsysinfo.modeltemp, 0, sizeof(gsysinfo.modeltemp));
        else if ( strcmp(m4, "=") == 0)
            memcpy(gsysinfo.modeltemp, gsysinfo.modelperm, sizeof(gsysinfo.modeltemp));
        else if ( copy_stringz_to_ebcdic(gsysinfo.modeltemp, sizeof(gsysinfo.modeltemp), m4) <= 0 ) return 4;
    }
//  else if ( m4 == NULL )      // uncomment test if anything else is done
//      return 0;

    return 0;
}

void get_model(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.model, sizeof(gsysinfo.model));
}

void get_modelcapa(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.modelcapa, sizeof(gsysinfo.modelcapa));
}

void get_modelperm(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.modelperm, sizeof(gsysinfo.modelperm));
}

void get_modeltemp(BYTE *dest)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dest, gsysinfo.modeltemp, sizeof(gsysinfo.modeltemp));
}

char *str_modelhard()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.model);
}

char *str_modelcapa()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.modelcapa);
}

char *str_modelperm()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.modelperm);
}

char *str_modeltemp()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.modeltemp);
}


/*-------------------------------------------------------------------*/
/* SYSTEM TYPE IDENTIFICATION                                        */
/* Set by: SERVC instruction                                         */
/* Retrieved by: DIAG204 instruction                                 */
/*-------------------------------------------------------------------*/

void set_systype(BYTE *src)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(gsysinfo.systype, src, sizeof(gsysinfo.systype));
}

void get_systype(BYTE *dst)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dst, gsysinfo.systype, sizeof(gsysinfo.systype));
}

LOADPARM_DLL_IMPORT
char *str_systype()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.systype);
}


/*-------------------------------------------------------------------*/
/* SYSTEM NAME                                                       */
/* Set by: SERVC instruction                                         */
/* Retrieved by: DIAG204 instruction                                 */
/*-------------------------------------------------------------------*/

void set_sysname(BYTE *src)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(gsysinfo.sysname, src, sizeof(gsysinfo.sysname));
}

void get_sysname(BYTE *dst)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dst, gsysinfo.sysname, sizeof(gsysinfo.sysname));
}

LOADPARM_DLL_IMPORT
char *str_sysname()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.sysname);
}


/*-------------------------------------------------------------------*/
/* SYSPLEX NAME                                                      */
/* Set by: SERVC instruction                                         */
/* Retrieved by: DIAG204 instruction                                 */
/*-------------------------------------------------------------------*/

void set_sysplex(BYTE *src)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(gsysinfo.sysplex, src, sizeof(gsysinfo.sysplex));
}

void get_sysplex(BYTE *dst)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dst, gsysinfo.sysplex, sizeof(gsysinfo.sysplex));
}

LOADPARM_DLL_IMPORT
char *str_sysplex()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.sysplex);
}


/*-------------------------------------------------------------------*/
/* VIRTUAL MACHINE ID                                                */
/* Set by: VMD configuration statement                               */
/* Retrieved by: STSI instruction                                    */
/*-------------------------------------------------------------------*/

void set_vmid(BYTE *src)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(gsysinfo.vmid, src, sizeof(gsysinfo.vmid));
}

void get_vmid(BYTE *dst)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dst, gsysinfo.vmid, sizeof(gsysinfo.vmid));
}

LOADPARM_DLL_IMPORT
char *str_vmid()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.vmid);
}


/*-------------------------------------------------------------------*/
/* CONTROL PROGRAM  ID                                               */
/* Set by: VMD configuration statement                               */
/* Retrieved by: STSI instruction                                    */
/*-------------------------------------------------------------------*/

void set_cpid(BYTE *src)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(gsysinfo.cpid, src, sizeof(gsysinfo.cpid));
}

void get_cpid(BYTE *dst)
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    memcpy(dst, gsysinfo.cpid, sizeof(gsysinfo.cpid));
}

LOADPARM_DLL_IMPORT
char *str_cpid()
{
    if (gsysinfo_init_flg == FALSE )
        get_gsysinfo(NULL);

    ebcdic_to_stringz_return(gsysinfo.cpid);
}


/*-------------------------------------------------------------------*/
/* Build a STSI System Information Block (SYSIB) Sequence value      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* PROGRAMMING NOTE: according to manual SA22-7832-10 zArchitecture  */
/* Principles of Operation, page 10-141 and 10-145:                  */
/*                                                                   */
/*    "Multiple CPUs in the same configuration have the              */
/*     same sequence code, and it is necessary to use other          */
/*     information, such as the CPU address, to establish            */
/*     a unique CPU identity.  The sequence code returned            */
/*     for a basic-machine CPU and a logical-partition CPU           */
/*     are identical and have the same value as the sequence         */
/*     code returned for the basic-machine configuration."           */
/*                                                                   */
/*-------------------------------------------------------------------*/
void bld_sysib_sequence( BYTE* seqc )
{
    static BYTE hexebcdic[16] = {
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,  // "01234567"
        0xF8, 0xF9, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6   // "89ABCDEF"
    };
    size_t  seqc_idx;
    BYTE    serial_shift, hexebcdic_idx, i;

    memset( seqc, 0xF0, 16 );

    for (i=0; i < 6; i++)
    {
        seqc_idx         = 16 - 6 + i;
        serial_shift     = (32 - (3 * 4)) - (i * 4);
        hexebcdic_idx    = (sysblk.cpuserial >> serial_shift) & 0x0F;
        seqc[ seqc_idx ] = hexebcdic[ hexebcdic_idx ];
    }
}


/*-------------------------------------------------------------------*/
/* Retrieve Multiprocessing CPU-Capability Adjustment Factors        */
/*                                                                   */
/* This function retrieves the Multiprocessing CPU-Capability        */
/* Adjustment Factor values for SYSIB (System Information Block)     */
/* 1.2.2 as described in the Principles of Operations manual for     */
/* the STORE SYSTEM INFORMATION instruction.                         */
/*                                                                   */
/* Input:                                                            */
/*      dest  Address of where to store the information.             */
/* Output:                                                           */
/*      The requested MP Factor values at the address specified.     */
/* Used by:                                                          */
/*      B27D STSI  Store System Information (Basic-machine All CPUs) */
/*      B220 SERVC Service Call             (read_scpinfo)           */
/*-------------------------------------------------------------------*/
void get_mpfactors(BYTE *dest)
{
/*-------------------------------------------------------------------*/
/* The new z10 machine will use a denominator of 65535 for better    */
/* granularity. But this will mess up old software. We will stick    */
/* to the old value of 100. Bernard Feb 26, 2010.                    */
/*                                                                   */
/* Calculations will be done in a higher precision to prevent early  */
/* truncation and degradation of the factors. To achieve this, the   */
/* factors are calculated * 256. When storing the values, the        */
/* individual values are rounded up by 128 and then divided by 256   */
/* for the store operation.                                          */
/*                                                                   */
/* Once the number of host logical CPUs is reached, subsequent MP    */
/* factors are no longer adjusted.                                   */
/*                                                                   */
/* MPFACTOR_DENOMINATOR must be 100, 255, or 65535.                  */
/*                                                                   */
/* It is recommended that MPFACTOR_PERCENT remain at 95 (95%).       */
/*-------------------------------------------------------------------*/
#define  MPFACTOR_DENOMINATOR     100
#define  MPFACTOR_PERCENT          95

    static U16 mpfactors[ MAX_CPU_ENGS - 1 ] = {0};
    static BYTE didthis = 0;

    if (!didthis)
    {
        /* First time: initialize array... */
        size_t  limit = get_RealCPCount();
        size_t  i;
        U32     mpfactor = MPFACTOR_DENOMINATOR << 8;
        U16     result = 0;
        U16     resmin = 1;

        switch (MPFACTOR_DENOMINATOR)
        {
            default: /* resmin =   1; break; */
            case   100: resmin =   1; break;
            case   255: resmin = 101; break;
            case 65535: resmin = 256; break;
        }

        for (i=0; i < _countof( mpfactors ); i++)
        {
            /* Calculate the value of each subsequent entry as a
             * percentage of the previous entry's value for the real
             * defined CPU entries.
             */
            if (i < limit)
            {
                mpfactor = (mpfactor * MPFACTOR_PERCENT) / 100;
                result = (mpfactor + 128) >> 8;
                result = MAX(result, resmin);
            }
            STORE_HW( &mpfactors[i], result );
        }
        didthis = 1;
    }

    /* Return the requested information... */
    memcpy( dest, &mpfactors[0], (MAX_CPU_ENGS - 1) * sizeof( U16 ));
}


/*-------------------------------------------------------------------*/
/* get_RealCPCount - Get the maximum count of possible concurrently  */
/*                   dispatchable CP engines on the host processor.  */
/*-------------------------------------------------------------------*/
unsigned int
get_RealCPCount (void)
{
    unsigned int    possible;
    unsigned int    reserved;
    unsigned int    cp = 0;
    unsigned int    cpu;
    unsigned int    result;

    /* Initialize the number of possible concurrently dispatchable CP
     * engines, taking into account that any of the hostinfo fields
     * may be zero when not reported by the host OS.
     */
    if (hostinfo.num_logical_cpu)
        possible = hostinfo.num_logical_cpu;
    else if (hostinfo.num_procs)
    {
        if (hostinfo.num_physical_cpu)
            possible = hostinfo.num_procs * hostinfo.num_physical_cpu;
        else
            possible = hostinfo.num_procs;
    }
    else
        possible = MAX_CPU_ENGS;

    /* Limit to the maximum number of Hercules CPU engines */
    if (possible > MAX_CPU_ENGS)
        possible = MAX_CPU_ENGS;

    /* Set number of reserved processors */
    reserved = possible - sysblk.cpus;

    /* Count the number of defined CP processors */
    for ( cpu = 0; cpu < (unsigned) sysblk.hicpu; ++cpu )
    {
        /* Loop through online CP CPUs */
        if ( IS_CPU_ONLINE(cpu) )
        {
            if ( sysblk.ptyp[cpu] == SCCB_PTYP_CP )
                ++cp;
        }
    }

    /* The result is the number of CP processors plus the number of
     * reserved processors that may become CP processors, limited by the
     * number of possible physical/logical processors.
     */
    result = MIN(cp + reserved, possible);

    return (result);
}
