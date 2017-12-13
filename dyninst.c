/* DYNINST.C    (c) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */

/* This module dynamically loads instructions.  Instruction routine  */
/* names must be registered under the name of s370_opcode_B220 for   */
/* example, where s370 may also be s390 or z900 for ESA/390 or ESAME */
/* mode respectively.  B220 is the opcode, and is depending on the   */
/* instruction 2 3 or 4 digits.                                      */

#include "hstdinc.h"
#include "hercules.h"

#if defined(OPTION_DYNAMIC_LOAD)

#include "opcode.h"
#include "inline.h"

#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "dyninst.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "dyninst.c"
#endif

typedef struct
{
  BYTE opcode1;
  BYTE opcode2;
  int arch;
  zz_func newinst;
  zz_func oldinst;
}
DYNINST;

#define MAXDYNINST 100
static DYNINST dyninst[MAXDYNINST];
static int dyninst_index = 0;

static char *prefix[] = {
#if defined(_370)
  "s370_dyninst_opcode_",
#endif
#if defined(_390)
  "s390_dyninst_opcode_",
#endif
#if defined(_900)
  "z900_dyninst_opcode_",
#endif
  NULL
};

static void init_dyninst()
{
  int i;

  logmsg("\ndyninst is deprecated, use HDL_DEF_INST to replace instructions\n");
  logmsg("Please refer to README.HDL for details on the use of HDL_DEF_INST\n\n");

  for(i = 0; i < MAXDYNINST; i++)
  {
    dyninst[i].opcode1 = 0;
    dyninst[i].opcode2 = 0;
    dyninst[i].arch = -1;
    dyninst[i].newinst = NULL;
    dyninst[i].oldinst = NULL;
  }
}

static void destroy_dyninst()
{
  int i;

  for(i = 0; i < dyninst_index; i++)
    replace_opcode(dyninst[i].arch, dyninst[i].oldinst, dyninst[i].opcode1, dyninst[i].opcode2);
}

static void update_dyninst()
{
  int arch;
  char name[64];
  zz_func newinst;
  zz_func oldinst;
  int opcode1;
  int opcode2;

  for(arch = 0; arch < GEN_ARCHCOUNT; arch++)
  {
    for(opcode1 = 0; opcode1 < 0x100 && dyninst_index < MAXDYNINST; opcode1++)
    {
      snprintf(name, sizeof(name), "%s%02X", prefix[arch], opcode1);
      newinst = HDL_FINDSYM(name);
      if(newinst)
      {
        oldinst = replace_opcode(arch, newinst, opcode1, -1);
        if(oldinst)
        {
          dyninst[dyninst_index].opcode1 = opcode1;
          dyninst[dyninst_index].opcode2 = -1;
          dyninst[dyninst_index].arch = arch;
          dyninst[dyninst_index].newinst = newinst;
          dyninst[dyninst_index].oldinst = oldinst;
          dyninst_index++;
        }
      }

      switch(opcode1) {
        case 0x01:
        case 0xa4:
        case 0xa6:
        case 0xb2:
        case 0xb3:
        case 0xb9:
        case 0xe4:
        case 0xe5:
        case 0xe6:
        case 0xa5:
        case 0xa7:
        case 0xc0:
        case 0xc2:
        case 0xc4:
        case 0xc6:
        case 0xc8:
        case 0xcc:
        case 0xe3:
        case 0xeb:
        case 0xec:
        case 0xed:
          for(opcode2 = 0; opcode2 < 0x100 && dyninst_index < MAXDYNINST; opcode2++)
          {
            snprintf(name, sizeof(name), "%s%02X%02X", prefix[arch], opcode1, opcode2);
            newinst = HDL_FINDSYM(name);
            if(newinst)
            {
              oldinst = replace_opcode(arch, newinst, opcode1, opcode2);
              if(oldinst)
              {
                dyninst[dyninst_index].opcode1 = opcode1;
                dyninst[dyninst_index].opcode2 = opcode2;
                dyninst[dyninst_index].arch = arch;
                dyninst[dyninst_index].newinst = newinst;
                dyninst[dyninst_index].oldinst = oldinst;
                dyninst_index++;
              }
            }
          }
       }
    }
  }
}


/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */
#if !defined(HDL_BUILD_SHARED) && defined(HDL_USE_LIBTOOL)
#define hdl_ddev dyninst_LTX_hdl_ddev
#define hdl_depc dyninst_LTX_hdl_depc
#define hdl_reso dyninst_LTX_hdl_reso
#define hdl_init dyninst_LTX_hdl_init
#define hdl_fini dyninst_LTX_hdl_fini
#endif


HDL_DEPENDENCY_SECTION;
{
  HDL_DEPENDENCY (HERCULES);
  HDL_DEPENDENCY (REGS);
  HDL_DEPENDENCY (DEVBLK);
  HDL_DEPENDENCY (SYSBLK);
}
END_DEPENDENCY_SECTION


HDL_REGISTER_SECTION;
{
  init_dyninst();
}
END_REGISTER_SECTION


HDL_RESOLVER_SECTION;
{
  HDL_RESOLVE ( replace_opcode );

  update_dyninst();
}
END_RESOLVER_SECTION

HDL_FINAL_SECTION;
{
  destroy_dyninst();
}
END_FINAL_SECTION


#endif /*!defined(_GEN_ARCH)*/

#endif /*defined(OPTION_DYNAMIC_LOAD)*/
