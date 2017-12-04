/* CRYPTO.C     (c) Copyright Jan Jaeger, 2000-2012                  */
/*              Cryptographic instructions                           */

#include "hstdinc.h"
#include "hercules.h"

#define _CRYPTO_C_

#if defined(FEATURE_017_MSA_FACILITY)

/*----------------------------------------------------------------------------*/
/* Function: renew_wrapping_keys                                              */
/*                                                                            */
/* Each time a clear reset is performed, a new set of wrapping keys and their */
/* associated verification patterns are generated. The contents of the two    */
/* wrapping-key registers are kept internal to the model so that no program,  */
/* including the operating system, can directly observe their clear value.    */
/*----------------------------------------------------------------------------*/
void renew_wrapping_keys(void)
{
  int   i;
  BYTE  lparname[8];
  U64   cpuid;

  srandom((unsigned)(time(0)));

  for(i = 0; i < 0x100; i++)
    srandom((unsigned)((int)random() * (int)(host_tod()&0xFFFFFFFF))); /* Randomize related to time */
  obtain_wrlock(&sysblk.wklock);
  for(i = 0; i < 32; i++)
    sysblk.wkaes_reg[i] = random()&0xFF;
  for(i = 0; i < 24; i++)
    sysblk.wkdea_reg[i] = random()&0xFF;

  /* We set the verification pattern to */
  /* cpuid (8 bytes) */
  /* lpar name (8 bytes) */
  /* lparnum (1 byte) */
  /* random number 8 bytes at the end) */
  memset(sysblk.wkvpaes_reg, 0, 32);
  memset(sysblk.wkvpdea_reg, 0, 24);
  cpuid = sysblk.cpuid;
  for(i = 0; i < 8; i++)
  {
    sysblk.wkvpaes_reg[7 - i] = cpuid & 0xff;
    sysblk.wkvpdea_reg[7 - i] = cpuid & 0xff;
    cpuid >>= 8;
  }
  get_lparname(lparname);
  memcpy(&sysblk.wkvpaes_reg[8], lparname, 8);
  memcpy(&sysblk.wkvpdea_reg[8], lparname, 8);
  sysblk.wkvpaes_reg[16] = sysblk.lparnum;
  sysblk.wkvpdea_reg[16] = sysblk.lparnum;
  for(i = 0; i < 8; i++)
  {
    sysblk.wkvpaes_reg[31 - i] = sysblk.wkvpdea_reg[23 - i] = random() & 0xff;
  }
  release_rwlock(&sysblk.wklock);

#if 0
#define OPTION_WRAPPINGKEYS_DEBUG
#endif
#ifdef OPTION_WRAPPINGKEYS_DEBUG
  char buf[128];

  MSGBUF(buf, "AES wrapping key:    ");
  for(i = 0; i < 32; i++)
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", sysblk.wkaes_reg[i]);
  buf[sizeof(buf)-1] = '\0';
  WRMSG(HHC90190, "D", buf);
  snprintf(buf, sizeof(buf), "AES wrapping key vp: ");
  for(i = 0; i < 32; i++)
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", sysblk.wkvpaes_reg[i]);
  buf[sizeof(buf)-1] = '\0';
  WRMSG(HHC90190, "D", buf);
  snprintf(buf, sizeof(buf), "DEA wrapping key:    ");
  for(i = 0; i < 24; i++)
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", sysblk.wkdea_reg[i]);
  buf[sizeof(buf)-1] = '\0';
  WRMSG(HHC90190, "D", buf);
  snprintf(buf, sizeof(buf), "DEA wrapping key vp: ");
  for(i = 0; i < 24; i++)
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", sysblk.wkvpdea_reg[i]);
  buf[sizeof(buf)-1] = '\0';
  WRMSG(HHC90190, "D", buf);
#endif
}
#endif /* FEATURE_076_MSA_EXTENSION_FACILITY_3 */

