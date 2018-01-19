/* HCHAN.H       (C) Copyright Ivan Warren, 2004-2012                */
/* Based on work (c)Roger Bowler, Jan Jaeger & Others 1999-2010      */
/*              Generic channel device handler                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __HCHAN_H__
#define __HCHAN_H__
/*
 * Hercules Generic Channel internal definitions
 * (c) Ivan Scott Warren 2004-2006
 *     based on work
 * (c) Roger Bowler, Jan Jaeger and Others 1999-2006
 * This code is covered by the QPL Licence
 */

static  int     hchan_init_exec(DEVBLK *,int,char **);
static  int     hchan_init_connect(DEVBLK *,int,char **);
static  int     hchan_init_int(DEVBLK *,int,char **);

#endif
