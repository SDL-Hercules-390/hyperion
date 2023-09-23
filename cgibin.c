/* CGIBIN.C     (C) Copyright Jan Jaeger, 2002-2012                  */
/*                  HTTP cgi-bin routines                            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*                                                                   */
/* This file contains all cgi routines that may be executed on the   */
/* server (i.e. under control of a hercules thread).  All cgi-bin    */
/* routines are identified in the directory at the end of this file. */
/* cgi-bin routines may call the following HTTP service routines:    */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* char *cgi_variable(WEBBLK *webblk, char *name);                   */
/*                                                                   */
/*   This call returns a pointer to the cgi variable requested       */
/*   or a NULL pointer if the variable is not found                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* char *cgi_cookie(WEBBLK *webblk, char *name);                     */
/*                                                                   */
/*   This call returns a pointer to the cookie requested             */
/*   or a NULL pointer if the cookie is not found                    */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* char *cgi_username(WEBBLK *webblk);                               */
/*                                                                   */
/*   Returns the username for which the user has been authenticated  */
/*   or NULL if not authenticated (refer to auth/noauth parameter    */
/*   on the HTTPPORT configuration statement)                        */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* char *cgi_baseurl(WEBBLK *webblk);                                */
/*                                                                   */
/*   Returns the url as requested by the user                        */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* void html_header(WEBBLK *webblk);                                 */
/*                                                                   */
/*   Sets up the standard html header, and includes the              */
/*   html/header.htmlpart file.                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* void html_footer(WEBBLK *webblk);                                 */
/*                                                                   */
/*   Sets up the standard html footer, and includes the              */
/*   html/footer.htmlpart file.                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* int html_include(WEBBLK *webblk, char *filename);                 */
/*                                                                   */
/*   Includes an html file                                           */
/*                                                                   */
/*                                           Jan Jaeger - 28/03/2002 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _CGIBIN_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "httpmisc.h"

/*-------------------------------------------------------------------*/
/*                     cgibin_blinkenlights_cpu                      */
/*-------------------------------------------------------------------*/
/*     contributed by Doug Wegscheid [dwegscheid@sbcglobal.net]      */
/*                         tweaked by Fish                           */
/*-------------------------------------------------------------------*/
void cgibin_blinkenlights_cpu( WEBBLK* webblk )
{
    REGS*  regs;
    QWORD  psw;
    int    cpu, gpr;

    hprintf( webblk->sock, "Expires: 0\n" );
    hprintf( webblk->sock, "Content-type: text/csv;\n\n" );

    for (cpu=0; cpu < sysblk.maxcpu; cpu++)
    {
        if (!(regs = sysblk.regs[ cpu ]))
            regs = &sysblk.dummyregs;

        copy_psw( regs, psw );

        if (ARCH_900_IDX == regs->arch_mode)
        {
            hprintf( webblk->sock,
                "CPU%4.4d,PSW,%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\n"
                , cpu
                , psw[  0 ], psw[  1 ], psw[  2 ], psw[  3 ]
                , psw[  4 ], psw[  5 ], psw[  6 ], psw[  7 ]
                , psw[  8 ], psw[  9 ], psw[ 10 ], psw[ 11 ]
                , psw[ 12 ], psw[ 13 ], psw[ 14 ], psw[ 15 ]
            );
        }
        else
        {
            hprintf( webblk->sock,
                "CPU%4.4d,PSW,%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\n"
                , cpu
                , psw[0], psw[1], psw[2], psw[3]
                , psw[4], psw[5], psw[6], psw[7]
            );
        }

        if (ARCH_900_IDX == regs->arch_mode)
            for (gpr=0; gpr < 16; gpr++) hprintf( webblk->sock, "CPU%4.4d,GR%1.1X=%16.16"PRIX64"\n", cpu, gpr, (U64) regs->GR_G( gpr ));
        else
            for (gpr=0; gpr < 16; gpr++) hprintf( webblk->sock, "CPU%4.4d,GR%2.2d,%8.8"  PRIX32"\n", cpu, gpr, (U32) regs->GR_L( gpr ));
    }
}

/*-------------------------------------------------------------------*/
/*                       cgibin_reg_control                          */
/*-------------------------------------------------------------------*/
void cgibin_reg_control(WEBBLK *webblk)
{
int i;

    REGS *regs;

    regs = sysblk.regs[sysblk.pcpu];
    if (!regs) regs = &sysblk.dummyregs;

    html_header(webblk);

    hprintf(webblk->sock, "<H2>Control Registers</H2>\n");
    hprintf(webblk->sock, "<PRE>\n");
    if(regs->arch_mode != ARCH_900_IDX)
        for (i = 0; i < 16; i++)
            hprintf(webblk->sock, "CR%2.2d=%8.8X%s", i, regs->CR_L(i),
                ((i & 0x03) == 0x03) ? "\n" : "\t");
    else
        for (i = 0; i < 16; i++)
            hprintf(webblk->sock, "CR%1.1X=%16.16"PRIX64"%s", i,
                (U64)regs->CR_G(i), ((i & 0x03) == 0x03) ? "\n" : " ");

    hprintf(webblk->sock, "</PRE>\n");

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                      cgibin_reg_general                           */
/*-------------------------------------------------------------------*/
void cgibin_reg_general(WEBBLK *webblk)
{
int i;

    REGS *regs;

    regs = sysblk.regs[sysblk.pcpu];
    if (!regs) regs = &sysblk.dummyregs;

    html_header(webblk);

    hprintf(webblk->sock, "<H2>General Registers</H2>\n");
    hprintf(webblk->sock, "<PRE>\n");
    if(regs->arch_mode != ARCH_900_IDX)
        for (i = 0; i < 16; i++)
            hprintf(webblk->sock, "GR%2.2d=%8.8X%s", i, regs->GR_L(i),
                ((i & 0x03) == 0x03) ? "\n" : "\t");
    else
        for (i = 0; i < 16; i++)
            hprintf(webblk->sock, "GR%1.1X=%16.16"PRIX64"%s", i,
                (U64)regs->GR_G(i), ((i & 0x03) == 0x03) ? "\n" : " ");

    hprintf(webblk->sock, "</PRE>\n");

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                           cgibin_psw                              */
/*-------------------------------------------------------------------*/
void cgibin_psw(WEBBLK *webblk)
{
    REGS *regs;
    QWORD   qword;

    char *value;
    int autorefresh=0;
    int refresh_interval=5;

    regs = sysblk.regs[sysblk.pcpu];
    if (!regs) regs = &sysblk.dummyregs;

    html_header(webblk);


    if (cgi_variable(webblk,"autorefresh"))
        autorefresh = 1;
    else if (cgi_variable(webblk,"norefresh"))
        autorefresh = 0;
    else if (cgi_variable(webblk,"refresh"))
        autorefresh = 1;

    if ((value = cgi_variable(webblk,"refresh_interval")))
        refresh_interval = atoi(value);

    hprintf(webblk->sock, "<H2>Program Status Word</H2>\n");

    hprintf(webblk->sock, "<FORM method=post>\n");

    if (!autorefresh)
    {
        hprintf(webblk->sock, "<INPUT type=submit value=\"Auto Refresh\" name=autorefresh>\n");
        hprintf(webblk->sock, "Refresh Interval: ");
        hprintf(webblk->sock, "<INPUT type=text size=2 name=\"refresh_interval\" value=%d>\n",
           refresh_interval);
    }
    else
    {
        hprintf(webblk->sock, "<INPUT type=submit value=\"Stop Refreshing\" name=norefresh>\n");
        hprintf(webblk->sock, "Refresh Interval: %d\n", refresh_interval);
        hprintf(webblk->sock, "<INPUT type=hidden name=\"refresh_interval\" value=%d>\n",refresh_interval);
    }

    hprintf(webblk->sock, "</FORM>\n");

    hprintf(webblk->sock, "<P>\n");

    if( regs->arch_mode != ARCH_900_IDX )
    {
        copy_psw (regs, qword);
        hprintf(webblk->sock, "PSW=%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7]);
    }
    else
    {
        copy_psw (regs, qword);
        hprintf(webblk->sock, "PSW=%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\n",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7],
                qword[8], qword[9], qword[10], qword[11],
                qword[12], qword[13], qword[14], qword[15]);
    }

    if (autorefresh)
    {
        /* JavaScript to cause automatic page refresh */
        hprintf(webblk->sock, "<script language=\"JavaScript\">\n");
        hprintf(webblk->sock, "<!--\nsetTimeout('window.location.replace(\"%s?refresh_interval=%d&refresh=1\")', %d)\n",
               cgi_baseurl(webblk),
               refresh_interval,
               refresh_interval*1000);
        hprintf(webblk->sock, "//-->\n</script>\n");
    }

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                         cgibin_syslog                             */
/*-------------------------------------------------------------------*/
void cgibin_syslog(WEBBLK *webblk)
{
int     num_bytes;
int     logbuf_idx;
char   *logbuf_ptr;
char   *command;
char   *value;
int     autorefresh = 0;
int     refresh_interval = 5;
int     msgcount = 22;

    if ((command = cgi_variable(webblk,"command")))
    {
        panel_command(command);
        // Wait a bit before proceeding in case
        // the command issues a lot of messages
        USLEEP(50000);
    }

    if((value = cgi_variable(webblk,"msgcount")))
        msgcount = atoi(value);
    else
        if((value = cgi_cookie(webblk,"msgcount")))
            msgcount = atoi(value);

    if ((value = cgi_variable(webblk,"refresh_interval")))
        refresh_interval = atoi(value);

    if (cgi_variable(webblk,"autorefresh"))
        autorefresh = 1;
    else if (cgi_variable(webblk,"norefresh"))
        autorefresh = 0;
    else if (cgi_variable(webblk,"refresh"))
        autorefresh = 1;

    html_header(webblk);

    hprintf(webblk->sock,"<script language=\"JavaScript\">\n"
                          "<!--\n"
                          "document.cookie = \"msgcount=%d\";\n"
                          "//-->\n"
                          "</script>\n",
                          msgcount);

    hprintf(webblk->sock, "<H2>Hercules System Log</H2>\n");
    hprintf(webblk->sock, "<PRE>\n");

    // Get the index to our desired starting message...

    logbuf_idx = msgcount ? log_line( msgcount ) : -1;

    // Now read the logfile starting at that index. The return
    // value is the total #of bytes of messages data there is.

    if ((num_bytes = log_read( &logbuf_ptr, &logbuf_idx, LOG_NOBLOCK )) > 0)
    {
        // Copy the message data to a work buffer for processing.
        // This is to allow for the possibility, however remote,
        // that the logfile buffer actually wraps around and over-
        // lays the message data we were going to display (which
        // could happen if there's a sudden flood of messages)

        char   *wrk_bufptr      = malloc( num_bytes );
        char   *sav_wrk         = NULL;

        if (wrk_bufptr)
        {
            sav_wrk = wrk_bufptr;
            strncpy( wrk_bufptr,  logbuf_ptr, num_bytes );
        }
        else         wrk_bufptr = logbuf_ptr;


        // We need to convert certain characters that might
        // possibly be erroneously interpreted as HTML code

#define  AMP_LT    "&lt;"       // (HTML code for '<')
#define  AMP_GT    "&gt;"       // (HTML code for '>')
#define  AMP_AMP   "&amp;"      // (HTML code for '&')

        while ( num_bytes-- )
        {
            switch ( *wrk_bufptr )
            {
            case '<':
                hwrite( webblk->sock, AMP_LT     , sizeof(AMP_LT) );
                break;
            case '>':
                hwrite( webblk->sock, AMP_GT     , sizeof(AMP_GT) );
                break;
            case '&':
                hwrite( webblk->sock, AMP_AMP    , sizeof(AMP_AMP));
                break;
            default:
                hwrite( webblk->sock, wrk_bufptr , 1              );
                break;
            }

            wrk_bufptr++;
        }

        // (free our work buffer if it's really ours)

        if ( sav_wrk )
            free( sav_wrk );
    }

    hprintf(webblk->sock, "</PRE>\n");

    hprintf(webblk->sock, "<FORM method=post>Command:\n");
    hprintf(webblk->sock, "<INPUT type=text name=command size=80 autofocus>\n");
    hprintf(webblk->sock, "<INPUT type=submit name=send value=\"Send\">\n");
    hprintf(webblk->sock, "<INPUT type=hidden name=%srefresh value=1>\n",autorefresh ? "auto" : "no");
    hprintf(webblk->sock, "<INPUT type=hidden name=refresh_interval value=%d>\n",refresh_interval);
    hprintf(webblk->sock, "<INPUT type=hidden name=msgcount value=%d>\n",msgcount);
    hprintf(webblk->sock, "</FORM>\n<BR>\n");

    hprintf(webblk->sock, "<A name=bottom>\n");

    hprintf(webblk->sock, "<FORM method=post>\n");
    if(!autorefresh)
    {
        hprintf(webblk->sock, "<INPUT type=submit value=\"Auto Refresh\" name=autorefresh>\n");
        hprintf(webblk->sock, "Refresh Interval: ");
        hprintf(webblk->sock, "<INPUT type=text name=\"refresh_interval\" size=2 value=%d>\n",
           refresh_interval);
    }
    else
    {
        hprintf(webblk->sock, "<INPUT type=submit name=norefresh value=\"Stop Refreshing\">\n");
        hprintf(webblk->sock, "<INPUT type=hidden name=refresh_interval value=%d>\n",refresh_interval);
        hprintf(webblk->sock, " Refresh Interval: %2d \n", refresh_interval);
    }
    hprintf(webblk->sock, "<INPUT type=hidden name=msgcount value=%d>\n",msgcount);
    hprintf(webblk->sock, "</FORM>\n");

    hprintf(webblk->sock, "<FORM method=post>\n");
    hprintf(webblk->sock, "Only show last ");
    hprintf(webblk->sock, "<INPUT type=text name=msgcount size=3 value=%d>",msgcount);
    hprintf(webblk->sock, " lines (zero for all loglines)\n");
    hprintf(webblk->sock, "<INPUT type=hidden name=%srefresh value=1>\n",autorefresh ? "auto" : "no");
    hprintf(webblk->sock, "<INPUT type=hidden name=refresh_interval value=%d>\n",refresh_interval);
    hprintf(webblk->sock, "</FORM>\n");

    if (autorefresh)
    {
        /* JavaScript to cause automatic page refresh */
        hprintf(webblk->sock, "<script language=\"JavaScript\">\n");
        hprintf(webblk->sock, "<!--\nsetTimeout('window.location.replace(\"%s"
               "?refresh_interval=%d"
               "&refresh=1"
               "&msgcount=%d"
               "\")', %d)\n",
               cgi_baseurl(webblk),
               refresh_interval,
               msgcount,
               refresh_interval*1000);
        hprintf(webblk->sock, "//-->\n</script>\n");
    }

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                     cgibin_debug_registers                        */
/*-------------------------------------------------------------------*/
void cgibin_debug_registers(WEBBLK *webblk)
{
int i, cpu = 0;
int select_gr, select_cr, select_ar;
char *value;
REGS *regs;

    if((value = cgi_variable(webblk,"cpu")))
        cpu = atoi(value);

    if((value = cgi_variable(webblk,"select_gr")) && *value == 'S')
        select_gr = 1;
    else
        select_gr = 0;

    if((value = cgi_variable(webblk,"select_cr")) && *value == 'S')
        select_cr = 1;
    else
        select_cr = 0;

    if((value = cgi_variable(webblk,"select_ar")) && *value == 'S')
        select_ar = 1;
    else
        select_ar = 0;

    /* Validate cpu number */
    if (cpu < 0 || cpu >= sysblk.maxcpu || !IS_CPU_ONLINE(cpu))
        for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
            if(IS_CPU_ONLINE(cpu))
                break;

    if(cpu < sysblk.maxcpu)
        regs = sysblk.regs[cpu];
    else
        regs = sysblk.regs[sysblk.pcpu];

    if (!regs) regs = &sysblk.dummyregs;

    if((value = cgi_variable(webblk,"alter_gr")) && *value == 'A')
    {
        for(i = 0; i < 16; i++)
        {
        char regname[16];
            MSGBUF(regname,"alter_gr%d",i);
            if((value = cgi_variable(webblk,regname)))
            {
                if(regs->arch_mode != ARCH_900_IDX)
                    sscanf(value,"%"SCNx32,&(regs->GR_L(i)));
                else
                    sscanf(value,"%"SCNx64,&(regs->GR_G(i)));
            }
        }
    }

    if((value = cgi_variable(webblk,"alter_cr")) && *value == 'A')
    {
        for(i = 0; i < 16; i++)
        {
        char regname[16];
            MSGBUF(regname,"alter_cr%d",i);
            if((value = cgi_variable(webblk,regname)))
            {
                if(regs->arch_mode != ARCH_900_IDX)
                    sscanf(value,"%"SCNx32,&(regs->CR_L(i)));
                else
                    sscanf(value,"%"SCNx64,&(regs->CR_G(i)));
            }
        }
    }

    if((value = cgi_variable(webblk,"alter_ar")) && *value == 'A')
    {
        for(i = 0; i < 16; i++)
        {
        char regname[16];
            MSGBUF(regname,"alter_ar%d",i);
            if((value = cgi_variable(webblk,regname)))
                sscanf(value,"%x",&(regs->AR(i)));
        }
    }

    html_header(webblk);

    hprintf(webblk->sock,"<form method=post>\n"
                          "<select type=submit name=cpu>\n");

    for(i = 0; i < sysblk.maxcpu; i++)
        if(IS_CPU_ONLINE(i))
            hprintf(webblk->sock,"<option value=%d%s>CPU%4.4X</option>\n",
              i,i==cpu?" selected":"",i);

    hprintf(webblk->sock,"</select>\n"
                          "<input type=submit name=selcpu value=\"Select\">\n"
                          "<input type=hidden name=cpu value=%d>\n"
                          "<input type=hidden name=select_gr value=%c>\n"
                          "<input type=hidden name=select_cr value=%c>\n"
                          "<input type=hidden name=select_ar value=%c>\n",
                          cpu, select_gr?'S':'H',select_cr?'S':'H',select_ar?'S':'H');
    hprintf( webblk->sock, "Mode: %s\n", get_arch_name( regs ));
    hprintf(webblk->sock,"</form>\n");

    if(!select_gr)
    {
        hprintf(webblk->sock,"<form method=post>\n"
                              "<input type=submit name=select_gr "
                              "value=\"Select General Registers\">\n"
                              "<input type=hidden name=cpu value=%d>\n"
                              "<input type=hidden name=select_cr value=%c>\n"
                              "<input type=hidden name=select_ar value=%c>\n"
                              "</form>\n",cpu,select_cr?'S':'H',select_ar?'S':'H');
    }
    else
    {
        hprintf(webblk->sock,"<form method=post>\n"
                              "<input type=submit name=select_gr "
                              "value=\"Hide General Registers\">\n"
                              "<input type=hidden name=cpu value=%d>\n"
                              "<input type=hidden name=select_cr value=%c>\n"
                              "<input type=hidden name=select_ar value=%c>\n"
                              "</form>\n",cpu,select_cr?'S':'H',select_ar?'S':'H');

        hprintf(webblk->sock,"<form method=post>\n"
                              "<table>\n");
        for(i = 0; i < 16; i++)
        {
            if(regs->arch_mode != ARCH_900_IDX)
                hprintf(webblk->sock,"%s<td>GR%d</td><td><input type=text name=alter_gr%d size=8 "
                  "value=%8.8X></td>\n%s",
                  (i&3)==0?"<tr>\n":"",i,i,regs->GR_L(i),((i&3)==3)?"</tr>\n":"");
            else
                hprintf(webblk->sock,"%s<td>GR%d</td><td><input type=text name=alter_gr%d size=16 "
                  "value=%16.16"PRIX64"></td>\n%s",
                  (i&3)==0?"<tr>\n":"",i,i,(U64)regs->GR_G(i),((i&3)==3)?"</tr>\n":"");
        }
        hprintf(webblk->sock,"</table>\n"
                              "<input type=submit name=refresh value=\"Refresh\">\n"
                              "<input type=submit name=alter_gr value=\"Alter\">\n"
                              "<input type=hidden name=cpu value=%d>\n"
                              "<input type=hidden name=select_gr value=S>\n"
                              "<input type=hidden name=select_cr value=%c>\n"
                              "<input type=hidden name=select_ar value=%c>\n"
                              "</form>\n",cpu,select_cr?'S':'H',select_ar?'S':'H');
    }


    if(!select_cr)
    {
        hprintf(webblk->sock,"<form method=post>\n"
                              "<input type=submit name=select_cr "
                              "value=\"Select Control Registers\">\n"
                              "<input type=hidden name=cpu value=%d>\n"
                              "<input type=hidden name=select_gr value=%c>\n"
                              "<input type=hidden name=select_ar value=%c>\n"
                              "</form>\n",cpu,select_gr?'S':'H',select_ar?'S':'H');
    }
    else
    {
        hprintf(webblk->sock,"<form method=post>\n"
                              "<input type=submit name=select_cr "
                              "value=\"Hide Control Registers\">\n"
                              "<input type=hidden name=cpu value=%d>\n"
                              "<input type=hidden name=select_gr value=%c>\n"
                              "<input type=hidden name=select_ar value=%c>\n"
                              "</form>\n",cpu,select_gr?'S':'H',select_ar?'S':'H');

        hprintf(webblk->sock,"<form method=post>\n"
                              "<table>\n");
        for(i = 0; i < 16; i++)
        {
            if(regs->arch_mode != ARCH_900_IDX)
                hprintf(webblk->sock,"%s<td>CR%d</td><td><input type=text name=alter_cr%d size=8 "
                  "value=%8.8X></td>\n%s",
                  (i&3)==0?"<tr>\n":"",i,i,regs->CR_L(i),((i&3)==3)?"</tr>\n":"");
            else
                hprintf(webblk->sock,"%s<td>CR%d</td><td><input type=text name=alter_cr%d size=16 "
                  "value=%16.16"PRIX64"></td>\n%s",
                  (i&3)==0?"<tr>\n":"",i,i,(U64)regs->CR_G(i),((i&3)==3)?"</tr>\n":"");
        }
        hprintf(webblk->sock,"</table>\n"
                              "<input type=submit name=refresh value=\"Refresh\">\n"
                              "<input type=submit name=alter_cr value=\"Alter\">\n"
                              "<input type=hidden name=cpu value=%d>\n"
                              "<input type=hidden name=select_cr value=S>\n"
                              "<input type=hidden name=select_gr value=%c>\n"
                              "<input type=hidden name=select_ar value=%c>\n"
                              "</form>\n",cpu,select_gr?'S':'H',select_ar?'S':'H');
    }


    if(regs->arch_mode != ARCH_370_IDX)
    {
        if(!select_ar)
        {
            hprintf(webblk->sock,"<form method=post>\n"
                                  "<input type=submit name=select_ar "
                                  "value=\"Select Access Registers\">\n"
                                  "<input type=hidden name=cpu value=%d>\n"
                                  "<input type=hidden name=select_gr value=%c>\n"
                                  "<input type=hidden name=select_cr value=%c>\n"
                                  "</form>\n",cpu,select_gr?'S':'H',select_cr?'S':'H');
        }
        else
        {
            hprintf(webblk->sock,"<form method=post>\n"
                                  "<input type=submit name=select_ar "
                                  "value=\"Hide Access Registers\">\n"
                                  "<input type=hidden name=cpu value=%d>\n"
                                  "<input type=hidden name=select_gr value=%c>\n"
                                  "<input type=hidden name=select_cr value=%c>\n"
                                  "</form>\n",cpu,select_gr?'S':'H',select_cr?'S':'H');

            hprintf(webblk->sock,"<form method=post>\n"
                                  "<table>\n");
            for(i = 0; i < 16; i++)
            {
                hprintf(webblk->sock,"%s<td>AR%d</td><td><input type=text name=alter_ar%d size=8 "
                  "value=%8.8X></td>\n%s",
                  (i&3)==0?"<tr>\n":"",i,i,regs->AR(i),((i&3)==3)?"</tr>\n":"");
            }
            hprintf(webblk->sock,"</table>\n"
                                  "<input type=submit name=refresh value=\"Refresh\">\n"
                                  "<input type=submit name=alter_ar value=\"Alter\">\n"
                                  "<input type=hidden name=cpu value=%d>\n"
                                  "<input type=hidden name=select_gr value=%c>\n"
                                  "<input type=hidden name=select_cr value=%c>\n"
                                  "<input type=hidden name=select_ar value=S>\n"
                                  "</form>\n",cpu,select_gr?'S':'H',select_cr?'S':'H');
        }
    }

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                      cgibin_debug_storage                         */
/*-------------------------------------------------------------------*/
void cgibin_debug_storage(WEBBLK *webblk)
{
int i, j;
char *value;
U32 addr = 0;

    /* INCOMPLETE
     * no storage alter
     * no storage type (abs/real/prim virt/sec virt/access reg virt)
     * no cpu selection for storage other than abs
     */

    if((value = cgi_variable(webblk,"alter_a0")))
        sscanf(value,"%x",&addr);

    addr &= ~0x0F;

    html_header(webblk);


    hprintf(webblk->sock,"<form method=post>\n"
                          "<table>\n");

    if(addr > sysblk.mainsize || (addr + 128) > sysblk.mainsize)
        addr = sysblk.mainsize - 128;

    for(i = 0; i < 128;)
    {
        if(i == 0)
            hprintf(webblk->sock,"<tr>\n"
                                  "<td><input type=text name=alter_a0 size=8 value=%8.8X>"
                                  "<input type=hidden name=alter_a1 value=%8.8X></td>\n"
                                  "<td><input type=submit name=refresh value=\"Refresh\"></td>\n",
                                  i + addr, i + addr);
        else
            hprintf(webblk->sock,"<tr>\n"
                                  "<td align=center>%8.8X</td>\n"
                                  "<td></td>\n",
                                  i + addr);

    for(j = 0; j < 4; i += 4, j++)
        {
        U32 m;
            FETCH_FW(m,sysblk.mainstor + i + addr);
            hprintf(webblk->sock,"<td><input type=text name=alter_m%d size=8 value=%8.8X></td>\n",i,m);
        }

        hprintf(webblk->sock,"</tr>\n");
    }

    hprintf(webblk->sock,"</table>\n"
                          "</form>\n");
    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                           cgibin_ipl                              */
/*-------------------------------------------------------------------*/
void cgibin_ipl(WEBBLK *webblk)
{
int i;
char *value;
DEVBLK *dev;
U16 ipldev;
int iplcpu;
U32 doipl;

    html_header(webblk);

    hprintf(webblk->sock,"<h1>Perform Initial Program Load</h1>\n");

    if(cgi_variable(webblk,"doipl"))
        doipl = 1;
    else
        doipl = 0;

    if((value = cgi_variable(webblk,"device")))
        sscanf(value,"%hx",&ipldev);
    else
        ipldev = sysblk.ipldev;

    if((value = cgi_variable(webblk,"cpu")))
        sscanf(value,"%x",&iplcpu);
    else
        iplcpu = sysblk.iplcpu;

    if((value = cgi_variable(webblk,"loadparm")))
    set_loadparm(value);

    /* Validate CPU number */
    if(iplcpu >= sysblk.maxcpu)
        doipl = 0;

    if(!doipl)
    {
        /* Present IPL parameters */
        hprintf(webblk->sock,"<form method=post>\n"
                              "<select type=submit name=cpu>\n");

        for(i = 0; i < sysblk.maxcpu; i++)
            if(IS_CPU_ONLINE(i))
                hprintf(webblk->sock,"<option value=%4.4X%s>CPU%4.4X</option>\n",
                  i, ((sysblk.regs[i]->cpuad == iplcpu) ? " selected" : ""), i);

        hprintf(webblk->sock,"</select>\n"
                              "<select type=submit name=device>\n");

        for(dev = sysblk.firstdev; dev; dev = dev->nextdev)
            if(dev->pmcw.flag5 & PMCW5_V)
                hprintf(webblk->sock,"<option value=%4.4X%s>DEV%4.4X</option>\n",
                  dev->devnum, ((dev->devnum == ipldev) ? " selected" : ""), dev->devnum);

        hprintf(webblk->sock,"</select>\n");

        hprintf(webblk->sock,"Loadparm:<input type=text name=loadparm size=8 value=\"%s\">\n", str_loadparm());

        hprintf(webblk->sock,"<input type=submit name=doipl value=\"IPL\">\n"
                          "</form>\n");

    }
    else
    {
        OBTAIN_INTLOCK(NULL);
        /* Perform IPL function */
        if( load_ipl(0, ipldev, iplcpu,0) )
        {
            hprintf(webblk->sock,"<h3>IPL failed, see the "
                                  "<a href=\"syslog#bottom\">system log</a> "
                                  "for details</h3>\n");
        }
        else
        {
            hprintf(webblk->sock,"<h3>IPL completed</h3>\n");
        }
        RELEASE_INTLOCK(NULL);
    }

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                    cgibin_debug_device_list                       */
/*-------------------------------------------------------------------*/
void cgibin_debug_device_list(WEBBLK *webblk)
{
DEVBLK *dev;
char   *devclass;

    html_header(webblk);

    hprintf(webblk->sock,"<h2>Attached Device List</h2>\n"
                          "<table>\n"
                          "<tr><th>Number</th>"
                          "<th>Subchannel</th>"
                          "<th>Class</th>"
                          "<th>Type</th>"
                          "<th>Status</th></tr>\n");

    for(dev = sysblk.firstdev; dev; dev = dev->nextdev)
        if(dev->pmcw.flag5 & PMCW5_V)
        {
             (dev->hnd->query)(dev, &devclass, 0, NULL);

             hprintf(webblk->sock,"<tr>"
                                   "<td>%4.4X</td>"
                                   "<td><a href=\"detail?subchan=%4.4X\">%4.4X</a></td>"
                                   "<td>%s</td>"
                                   "<td>%4.4X</td>"
                                   "<td>%s%s%s</td>"
                                   "</tr>\n",
                                   dev->devnum,
                                   dev->subchan,dev->subchan,
                                   devclass,
                                   dev->devtype,
                                   (dev->fd >= 0   ? "open "    : ""),
                                   (dev->busy      ? "busy "    : ""),
                                   (IOPENDING(dev) ? "pending " : ""));
        }

    hprintf(webblk->sock,"</table>\n");

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                  cgibin_debug_device_detail                       */
/*-------------------------------------------------------------------*/
void cgibin_debug_device_detail(WEBBLK *webblk)
{
DEVBLK *sel, *dev = NULL;
char *value;
int subchan;

    html_header(webblk);

    if((value = cgi_variable(webblk,"subchan"))
      && sscanf(value,"%x",&subchan) == 1)
        for(dev = sysblk.firstdev; dev; dev = dev->nextdev)
            if(dev->subchan == subchan)
                break;

    hprintf(webblk->sock,"<h3>Subchannel Details</h3>\n");

    hprintf(webblk->sock,"<form method=post>\n"
                          "<select type=submit name=subchan>\n");

    for(sel = sysblk.firstdev; sel; sel = sel->nextdev)
    {
        hprintf(webblk->sock,"<option value=%4.4X%s>Subchannel %4.4X",
          sel->subchan, ((sel == dev) ? " selected" : ""), sel->subchan);
        if(sel->pmcw.flag5 & PMCW5_V)
            hprintf(webblk->sock," Device %4.4X</option>\n",sel->devnum);
        else
            hprintf(webblk->sock,"</option>\n");
    }

    hprintf(webblk->sock,"</select>\n"
                          "<input type=submit value=\"Select / Refresh\">\n"
                          "</form>\n");

    if(dev)
    {

        hprintf(webblk->sock,"<table border>\n"
                              "<caption align=left>"
                              "<h3>Path Management Control Word</h3>"
                              "</caption>\n");

        hprintf(webblk->sock,"<tr><th colspan=32>Interruption Parameter</th></tr>\n");

        hprintf(webblk->sock,"<tr><td colspan=32>%2.2X%2.2X%2.2X%2.2X</td></tr>\n",
                              dev->pmcw.intparm[0], dev->pmcw.intparm[1],
                              dev->pmcw.intparm[2], dev->pmcw.intparm[3]);

        hprintf(webblk->sock,"<tr><th>Q</th>"
                              "<th>0</th>"
                              "<th colspan=3>ISC</th>"
                              "<th colspan=2>00</th>"
                              "<th>A</th>"
                              "<th>E</th>"
                              "<th colspan=2>LM</th>"
                              "<th colspan=2>MM</th>"
                              "<th>D</th>"
                              "<th>T</th>"
                              "<th>V</th>"
                              "<th colspan=16>DEVNUM</th></tr>\n");

        hprintf(webblk->sock,"<tr><td>%d</td>"
                              "<td></td>"
                              "<td colspan=3>%d</td>"
                              "<td colspan=2></td>"
                              "<td>%d</td>"
                              "<td>%d</td>"
                              "<td colspan=2>%d%d</td>"
                              "<td colspan=2>%d%d</td>"
                              "<td>%d</td>"
                              "<td>%d</td>"
                              "<td>%d</td>"
                              "<td colspan=16>%2.2X%2.2X</td></tr>\n",
                              ((dev->pmcw.flag4 & PMCW4_Q) >> 7),
                              ((dev->pmcw.flag4 & PMCW4_ISC) >> 3),
                              (dev->pmcw.flag4 & 1),
                              ((dev->pmcw.flag5 >> 7) & 1),
                              ((dev->pmcw.flag5 >> 6) & 1),
                              ((dev->pmcw.flag5 >> 5) & 1),
                              ((dev->pmcw.flag5 >> 4) & 1),
                              ((dev->pmcw.flag5 >> 3) & 1),
                              ((dev->pmcw.flag5 >> 2) & 1),
                              ((dev->pmcw.flag5 >> 1) & 1),
                              (dev->pmcw.flag5 & 1),
                              dev->pmcw.devnum[0],
                              dev->pmcw.devnum[1]);

        hprintf(webblk->sock,"<tr><th colspan=8>LPM</th>"
                              "<th colspan=8>PNOM</th>"
                              "<th colspan=8>LPUM</th>"
                              "<th colspan=8>PIM</th></tr>\n");

        hprintf(webblk->sock,"<tr><td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td></tr>\n",
                              dev->pmcw.lpm,
                              dev->pmcw.pnom,
                              dev->pmcw.lpum,
                              dev->pmcw.pim);

        hprintf(webblk->sock,"<tr><th colspan=16>MBI</th>"
                              "<th colspan=8>POM</th>"
                              "<th colspan=8>PAM</th></tr>\n");

        hprintf(webblk->sock,"<tr><td colspan=16>%2.2X%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td></tr>\n",
                              dev->pmcw.mbi[0],
                              dev->pmcw.mbi[1],
                              dev->pmcw.pom,
                              dev->pmcw.pam);

        hprintf(webblk->sock,"<tr><th colspan=8>CHPID=0</th>"
                              "<th colspan=8>CHPID=1</th>"
                              "<th colspan=8>CHPID=2</th>"
                              "<th colspan=8>CHPID=3</th></tr>\n");

        hprintf(webblk->sock,"<tr><td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td></tr>\n",
                              dev->pmcw.chpid[0],
                              dev->pmcw.chpid[1],
                              dev->pmcw.chpid[2],
                              dev->pmcw.chpid[3]);

        hprintf(webblk->sock,"<tr><th colspan=8>CHPID=4</th>"
                              "<th colspan=8>CHPID=5</th>"
                              "<th colspan=8>CHPID=6</th>"
                              "<th colspan=8>CHPID=7</th></tr>\n");

        hprintf(webblk->sock,"<tr><td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td>"
                              "<td colspan=8>%2.2X</td></tr>\n",
                              dev->pmcw.chpid[4],
                              dev->pmcw.chpid[5],
                              dev->pmcw.chpid[6],
                              dev->pmcw.chpid[7]);

        hprintf(webblk->sock,"<tr><th colspan=8>ZONE</th>"
                              "<th colspan=5>00000</th>"
                              "<th colspan=3>VISC</th>"
                              "<th colspan=8>00000000</th>"
                              "<th>I</th>"
                              "<th colspan=6>000000</th>"
                              "<th>S</th></tr>\n");

        hprintf(webblk->sock,"<tr><td colspan=8>%2.2X</td>"
                              "<td colspan=5></td>"
                              "<td colspan=3>%d</td>"
                              "<td colspan=8></td>"
                              "<td>%d</td>"
                              "<td colspan=6></td>"
                              "<td>%d</td></tr>\n",
                              dev->pmcw.zone,
                              (dev->pmcw.flag25 & PMCW25_VISC),
                              (dev->pmcw.flag27 & PMCW27_I) >> 7,
                              (dev->pmcw.flag27 & PMCW27_S));

        hprintf(webblk->sock,"</table>\n");

    }

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                      cgibin_debug_misc                            */
/*-------------------------------------------------------------------*/
void cgibin_debug_misc(WEBBLK *webblk)
{
int zone;

    html_header(webblk);

    hprintf(webblk->sock,"<h2>Miscellaneous Registers<h2>\n");


    hprintf(webblk->sock,"<table border>\n"
                          "<caption align=left>"
                          "<h3>Zone related Registers</h3>"
                          "</caption>\n");

    hprintf(webblk->sock,"<tr><th>Zone</th>"
                          "<th>CS Origin</th>"
                          "<th>CS Limit</th>"
                          "<th>ES Origin</th>"
                          "<th>ES Limit</th>"
                          "<th>Measurement Block</th>"
                          "<th>Key</th></tr>\n");

    for(zone = 0; zone < FEATURE_SIE_MAXZONES; zone++)
    {
        hprintf(webblk->sock,"<tr><td>%2.2X</td>"
                              "<td>%8.8X</td>"
                              "<td>%8.8X</td>"
                              "<td>%8.8X</td>"
                              "<td>%8.8X</td>"
                              "<td>%8.8X</td>"
                              "<td>%2.2X</td></tr>\n",
                              zone,
#if defined(_FEATURE_SIE)
                              (U32)sysblk.zpb[zone].mso << 20,
                              ((U32)sysblk.zpb[zone].msl << 20) | 0xFFFFF,
                              (U32)sysblk.zpb[zone].eso << 20,
                              ((U32)sysblk.zpb[zone].esl << 20) | 0xFFFFF,
                              (U32)sysblk.zpb[zone].mbo,
                              sysblk.zpb[zone].mbk
#else
                              0, 0, 0, 0, 0, 0
#endif
               );
    }

    hprintf(webblk->sock,"</table>\n");


    hprintf(webblk->sock,"<table border>\n"
                          "<caption align=left>"
                          "<h3>Alternate Measurement</h3>"
                          "</caption>\n");

    hprintf(webblk->sock,"<tr><th>Measurement Block</th>"
                          "<th>Key</th></tr>\n");

    hprintf(webblk->sock,"<tr><td>%8.8X</td>"
                          "<td>%2.2X</td></tr>\n",
                          (U32)sysblk.mbo,
                          sysblk.mbk);

    hprintf(webblk->sock,"</table>\n");


    hprintf(webblk->sock,"<table border>\n"
                          "<caption align=left>"
                          "<h3>Address Limit Register</h3>"
                          "</caption>\n");

    hprintf(webblk->sock,"<tr><td>%8.8X</td></tr>\n",
                              (U32)sysblk.addrlimval);

    hprintf(webblk->sock,"</table>\n");

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                     cgibin_configure_cpu                          */
/*-------------------------------------------------------------------*/
void cgibin_configure_cpu(WEBBLK *webblk)
{
int i,j;

    html_header(webblk);

    hprintf(webblk->sock,"<h1>Configure CPU</h1>\n");

    for(i = 0; i < sysblk.maxcpu; i++)
    {
    char cpuname[8], *cpustate;
    int  cpuonline = -1;

        MSGBUF(cpuname,"cpu%d",i);
        if((cpustate = cgi_variable(webblk,cpuname)))
            sscanf(cpustate,"%d",&cpuonline);

        OBTAIN_INTLOCK(NULL);

        switch(cpuonline) {

        case 0:
            if(IS_CPU_ONLINE(i))
                deconfigure_cpu(i);
            break;

        case 1:
            if(!IS_CPU_ONLINE(i))
                configure_cpu(i);
            break;
        }

        RELEASE_INTLOCK(NULL);
    }

    for(i = 0; i < sysblk.maxcpu; i++)
    {
        hprintf(webblk->sock,"<p>CPU%4.4X\n"
                              "<form method=post>\n"
                              "<select type=submit name=cpu%d>\n",i,i);

        for(j = 0; j < 2; j++)
            hprintf(webblk->sock,"<option value=%d%s>%sline</option>\n",
              j, ((j!=0) == (IS_CPU_ONLINE(i)!=0)) ? " selected" : "", (j) ? "On" : "Off");

        hprintf(webblk->sock,"</select>\n"
                              "<input type=submit value=Update>\n"
                              "</form>\n");
    }

    html_footer(webblk);

}

/*-------------------------------------------------------------------*/
/*                 cgibin_debug_version_info                         */
/*-------------------------------------------------------------------*/
void cgibin_debug_version_info( WEBBLK* webblk )
{
    html_header( webblk );
    hprintf( webblk->sock, "<h1>Hercules Version Information</h1>\n<pre>\n" );
    display_version( NULL, webblk->sock, "Hercules HTTP Server" );
    hprintf( webblk->sock, "</pre>\n" );
    html_footer( webblk );
}

/*-------------------------------------------------------------------*/
/*                      cgibin_xml_rates_info                        */
/*-------------------------------------------------------------------*/
/*       contributed by Tim Pinkawa [timpinkawa@gmail.com]           */
/*-------------------------------------------------------------------*/
void cgibin_xml_rates_info(WEBBLK *webblk)
{
    hprintf(webblk->sock,"Expires: 0\n");
    hprintf(webblk->sock,"Content-type: text/xml;\n\n");   /* XML document */

    hprintf(webblk->sock,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    hprintf(webblk->sock,"<hercules>\n");
    hprintf(webblk->sock,"\t<arch>%d</arch>\n", sysblk.arch_mode);
    hprintf(webblk->sock,"\t<mips>%.1d.%.2d</mips>\n",
        sysblk.mipsrate / 1000000, (sysblk.mipsrate % 1000000) / 10000);
    hprintf(webblk->sock,"\t<siosrate>%d</siosrate>\n", sysblk.siosrate);
    hprintf(webblk->sock,"</hercules>\n");
}


/*-------------------------------------------------------------------*/
/*                           JSON API                                */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1                                */
/*-------------------------------------------------------------------*/
/* Returns a JSON file of all the available APIs:                    */
/* Example:                                                          */
/* {                                                                 */
/* "desc": "lists and describes available APIs. The below APIs are   */
/*          under cgibin/api/v1/",                                   */
/* "APIs": {                                                         */
/*     "cpus": "Returns information about the configured CPUs",      */
/*     "psw": "Returns the PSW", ...                                 */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1(WEBBLK *webblk)
{
    json_header( webblk );
    hprintf(webblk->sock,"{"
    "\"desc\":\"lists and describes available APIs. The below APIs are under cgibin/api/v1/\","
    "\"APIs\":{"
    "\"cpus\":\"Returns information about the configured CPUs\","
    "\"psw\":\"Returns the PSW\","
    "\"version\":\"Returns version and build information\","
    "\"syslog\":\"Returns the hercules log as an array in `syslog:`. Can take two optional parameters:\\n"
                "- command: the command submitted will be interepreted by hercules"
                "similar to the hercules console, the default is an empty string\\n"
                "- msgcount: is a variable and can be changed via get/post/cookie"
                "and controls the number of lines returned the default is 22 lines\","
    "\"devices\":\"Returns configured device information\","
    "\"maxrates\":\"Returns maxrates\","
    "\"rates\":\"Returns current MIPS and IO rates\","
    "\"storage\":\"Returns fullwords from memory. Takes two parameters:\\n"
                "- address: the starting address, default 0x00\\n"
                "- fullwords: the number of fullwords to return, default 32\""
    "}}");
}

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_version                        */
/*-------------------------------------------------------------------*/
/* Returns a JSON file formatted with the following:                 */
/* {                                                                 */
/*  "hercules_version:": "4.7.0.10964-SDL-DEV-g5e214634-modified",   */
/*  "hercules_copyright:": "(C) Copyright 1999-2023 by Roger Bowler, */
/*   Jan Jaeger, and others",                                        */
/*  "build_date": "Jun 25 2023",                                     */
/*  "build_time": "17:21:26",                                        */
/*  "modes": ["S/370", "ESA/390", "z/Arch"],                         */
/*  "max_cpu_engines": 128,                                          */
/*  "build_info": "Built with: GCC 11.3.0\n ... Using ...\n ..."     */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_version(WEBBLK *webblk)
{
    unsigned int num_strs;
    const char** ppszBldInfoStr = NULL;
    json_header( webblk );
    hprintf(webblk->sock,"{\"hercules_version\": \"%s\",",VERSION);
    hprintf(webblk->sock,"\"hercules_copyright\": \"%s\",",HERCULES_COPYRIGHT);
    hprintf(webblk->sock,"\"build_date\": \"%s\",", __DATE__);
    hprintf(webblk->sock,"\"build_time\": \"%s\",", __TIME__);
    hprintf(webblk->sock,"\"modes\" :[");
#if defined(_370)
    hprintf(webblk->sock,"\"%s\",",_ARCH_370_NAME);
#endif
#if defined(_390)
    hprintf(webblk->sock,"\"%s\",",_ARCH_390_NAME);
#endif
#if defined(_900)
    hprintf(webblk->sock,"\"%s\"",_ARCH_900_NAME);
#endif
    hprintf(webblk->sock,"], \"max_cpu_engines\":%d,",  MAX_CPU_ENGS );

    hprintf(webblk->sock,"\"build_info\":\"");

    num_strs = get_buildinfo_strings( &ppszBldInfoStr );
    for (; num_strs; num_strs--, ppszBldInfoStr++ )
    {
        if ( strchr (*ppszBldInfoStr, '"') == NULL ) // We need to skip any entries with "
            hprintf(webblk->sock,"%s\\n",*ppszBldInfoStr);
    }
    hprintf(webblk->sock,"\"");

    hprintf(webblk->sock,"}\n\n");

}
/*-------------------------------------------------------------------*/
/*                           JSON APIs                               */
/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_cpus                           */
/*-------------------------------------------------------------------*/
/* Returns the cpus as an array under "cpus":                        */
/* Example:                                                          */
/* {                                                                 */
/*  "cpus":                                                          */
/*     [                                                             */
/*       {                                                           */
/*      "cpuid": "CPU0000",                                          */
/*      "online": true,                                              */
/*      "mode": "ESA/390",                                           */
/*      "percent": 0,                                                */
/*      "PSW": "030A0000800078DC",                                   */
/*      "general_registers": {                                       */
/*          "GR0": "00000000",                                       */
/*          ...                                                      */
/*          "GR15": "00000000"                                       */
/*      },                                                           */
/*      "control_registers": {                                       */
/*          "CR0": "00000840",                                       */
/*          ...                                                      */
/*          "CR15": "00000000"                                       */
/*      },                                                           */
/*      "access_registers": {                                        */
/*          "AR0": "00000000",                                       */
/*          ...                                                      */
/*          "AR15": "00000000"                                       */
/*      }                                                            */
/*    }, {                                                           */
/*      "cpuid": "CPU0001",                                          */
/*      "online": false,                                             */
/*      "mode": "z/Arch",                                            */
/*      "percent": 0,                                                */
/*      "PSW": "00000000000000000000000000000000",                   */
/*      "general_registers": {                                       */
/*          "GR0": "0000000000000000", ...                           */
/*      },                                                           */
/*      "control_registers": {                                       */
/*          "CR0": "0000000000000060", ...                           */
/*      },                                                           */
/*      "access_registers": {                                        */
/*          "AR0": "00000000", ...                                   */
/*      }                                                            */
/*    }                                                              */
/*     ]                                                             */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_cpus(WEBBLK *webblk)
{
    REGS*  regs;
    QWORD  psw;
    int    cpu, gpr, cr, ar;

    json_header( webblk );
    hprintf( webblk->sock,"{\"cpus\":[");

    for (cpu=0; cpu < sysblk.maxcpu; cpu++)
    {
        if (!(regs = sysblk.regs[ cpu ]))
            regs = &sysblk.dummyregs;

        copy_psw( regs, psw );

        hprintf( webblk->sock, "{\"cpuid\":\"CPU%4.4d\",", cpu );

        if(IS_CPU_ONLINE(cpu))
            hprintf( webblk->sock, "\"online\": true," );
        else
            hprintf( webblk->sock, "\"online\": false," );

        hprintf( webblk->sock, "\"mode\": \"%s\",",get_arch_name( regs ));

        hprintf( webblk->sock, "\"percent\": %d,", regs->cpupct);

        if (ARCH_900_IDX == regs->arch_mode)
        {
            hprintf( webblk->sock, "\"PSW\": \"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\","
                , psw[  0 ], psw[  1 ], psw[  2 ], psw[  3 ]
                , psw[  4 ], psw[  5 ], psw[  6 ], psw[  7 ]
                , psw[  8 ], psw[  9 ], psw[ 10 ], psw[ 11 ]
                , psw[ 12 ], psw[ 13 ], psw[ 14 ], psw[ 15 ]
                );
        }
        else
        {
            hprintf( webblk->sock, "\"PSW\": \"%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\","
                , psw[  0 ], psw[  1 ], psw[  2 ], psw[  3 ]
                , psw[  4 ], psw[  5 ], psw[  6 ], psw[  7 ]
                );
        }

        hprintf( webblk->sock, "\"general_registers\":{");
        for (gpr=0; gpr < 16; gpr++)
        {

            if (ARCH_900_IDX == regs->arch_mode)
                hprintf( webblk->sock, "\"GR%d\": \"%16.16"PRIX64"\"", gpr, (U64) regs->GR_G( gpr ));
            else
                hprintf( webblk->sock, "\"GR%d\": \"%8.8"PRIX32"\"", gpr, (U32) regs->GR_L( gpr ));

            if (gpr < 15)
                hprintf( webblk->sock,","); //  JSON doesn't allow trailing commas
        }
        hprintf( webblk->sock, "},");

        hprintf( webblk->sock, "\"control_registers\":{");
        for (cr=0; cr < 16; cr++)
        {
            if (ARCH_900_IDX == regs->arch_mode)
                hprintf( webblk->sock, "\"CR%d\": \"%16.16"PRIX64"\"", cr, (U64) regs->CR_G( cr ));
            else
                hprintf( webblk->sock, "\"CR%d\": \"%8.8"PRIX32"\"", cr, (U32) regs->CR_L( cr ));

            if (cr < 15)
                hprintf( webblk->sock,","); //  JSON doesn't allow trailing commas
        }
        hprintf( webblk->sock, "},");

        hprintf( webblk->sock, "\"access_registers\":{");
        for (ar=0; ar < 16; ar++)
        {

            hprintf( webblk->sock, "\"AR%d\": \"%8.8X\"", ar, regs->AR( ar ));

            if (ar < 15)
                hprintf( webblk->sock,","); //  JSON doesn't allow trailing commas
        }
        hprintf( webblk->sock, "}");


        if (cpu < sysblk.maxcpu - 1)
            hprintf(webblk->sock,"},"); // End of a CPU
        else
            hprintf(webblk->sock,"}"); // End of a CPU
    }

    hprintf(webblk->sock,"]}"); // End of JSON

}

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_maxrates                       */
/*-------------------------------------------------------------------*/
/* Returns maxrates information similar to console command:          */
/* Example:                                                          */
/* {                                                                 */
/*    "from": "Mon Jun 26 00:00:00 2023",                            */
/*    "to": "Mon Jun 26 14:31:49 2023",                              */
/*    "MIPS": 0.26,                                                  */
/*    "IO": 0,                                                       */
/*    "current_interval": 1440                                       */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_maxrates(WEBBLK *webblk)
{
    char*   pszPrevIntervalStartDateTime = NULL;
    char*   pszCurrIntervalStartDateTime = NULL;
    char*   pszCurrentDateTime           = NULL;
    time_t  current_time    = 0;
    int     rc              = TRUE;
    size_t  len             = 0;

    current_time = time( NULL );

    json_header(webblk);

    hprintf(webblk->sock,"{");

    do {
            pszPrevIntervalStartDateTime = strdup( ctime( &prev_int_start_time ) );
            len = strlen(pszPrevIntervalStartDateTime);
            if ( pszPrevIntervalStartDateTime != NULL && len > 0 )
            {
                pszPrevIntervalStartDateTime[len - 1] = 0;
            }
            else
            {
                rc = FALSE;
                break;
            }

            pszCurrIntervalStartDateTime = strdup( ctime( &curr_int_start_time ) );
            len = strlen(pszCurrIntervalStartDateTime);
            if ( pszCurrIntervalStartDateTime != NULL && len > 0 )
            {
                pszCurrIntervalStartDateTime[len - 1] = 0;
            }
            else
            {
                rc = FALSE;
                break;
            }

            pszCurrentDateTime           = strdup( ctime(    &current_time     ) );
            len = strlen(pszCurrentDateTime);
            if ( pszCurrentDateTime != NULL && len > 0 )
            {
                pszCurrentDateTime[len - 1] = 0;
            }
            else
            {
                rc = FALSE;
                break;
            }

            break;

        } while(0);

    if ( rc )
    {

        if ( prev_int_start_time != curr_int_start_time )
        {
            hprintf(webblk->sock,"\"from\":\"%s\",\"to\":\"%s\",",
                        pszPrevIntervalStartDateTime,
                        pszCurrIntervalStartDateTime);
            hprintf(webblk->sock, "\"MIPS\": %d.%02d,",
                        prev_high_mips_rate / 1000000,
                        prev_high_mips_rate % 1000000);
            hprintf(webblk->sock,  "\"IO\": %d,", prev_high_sios_rate);
        }
        hprintf(webblk->sock,"\"from\":\"%s\",\"to\":\"%s\",",
                    pszCurrIntervalStartDateTime,
                    pszCurrentDateTime);
        hprintf(webblk->sock, "\"MIPS\": %d.%02d,",
                    curr_high_mips_rate / 1000000,
                    curr_high_mips_rate % 1000000);
       hprintf(webblk->sock,  "\"IO\": %d,", curr_high_sios_rate);
       hprintf(webblk->sock,  "\"current_interval\": %d", maxrates_rpt_intvl);
    }
    else
    {
        hprintf(webblk->sock,"\"error\":\"strdup() zero length\"");
    }

    hprintf(webblk->sock,"}");
    free( pszPrevIntervalStartDateTime );
    free( pszCurrIntervalStartDateTime );
    free( pszCurrentDateTime           );
}

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_rates                          */
/*-------------------------------------------------------------------*/
/* Returns MIPS and IO rates                                         */
/* Example:                                                          */
/* {                                                                 */
/*     "mipsrate":0,                                                 */
/*     "siosrate":0                                                  */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_rates(WEBBLK *webblk)
{
    json_header(webblk);
    hprintf(webblk->sock, "{\"mipsrate\":%d,\"siosrate\":%d}",
        sysblk.mipsrate, sysblk.siosrate );
}

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_psw                            */
/*-------------------------------------------------------------------*/
/* Returns the PSW                                                   */
/* Example:                                                          */
/* {                                                                 */
/*    "psw":"030A0000 800078DC"                                      */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_psw(WEBBLK *webblk)
{
    REGS *regs;
    QWORD   qword;

    regs = sysblk.regs[sysblk.pcpu];
    if (!regs) regs = &sysblk.dummyregs;

    json_header( webblk );

    if( regs->arch_mode != ARCH_900_IDX )
    {
        copy_psw (regs, qword);
        hprintf(webblk->sock, "{\"psw\":\"%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\"}",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7]);
    }
    else
    {
        copy_psw (regs, qword);
        hprintf(webblk->sock, "{\"psw\":\"%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\"}",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7],
                qword[8], qword[9], qword[10], qword[11],
                qword[12], qword[13], qword[14], qword[15]);
    }


}

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_syslog                         */
/*-------------------------------------------------------------------*/
/* Mostly based off of cgibin_syslog                                 */
/* Returns a JSON with the following format:                         */
/* {                                                                 */
/*     "command": "",                                                */
/*     "msgcount": 22,                                               */
/*     "syslog": "[<array of lines from syslog based on msgcount>]"  */
/* }                                                                 */
/*-------------------------------------------------------------------*/
/* Notes:                                                            */
/* - command is a variable and can be changed via get/post/cookie,   */
/*   the command submitted will be interepreted by hercules          */
/*   similar to the hercules console, the default is an empty string */
/* - msgcount is a variable and can be changed via get/post/cookie   */
/*   and controls the number of lines returned in "syslog",          */
/*   the default is 22 lines                                         */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_syslog(WEBBLK *webblk)
{

    int     num_bytes;
    int     logbuf_idx;
    char   *logbuf_ptr;
    char   *command;
    char   *value;
    int     msgcount = 22;

    json_header( webblk );

    if ((command = cgi_variable(webblk,"command")))
    {
        panel_command(command);
        // Wait a bit before proceeding in case
        // the command issues a lot of messages
        USLEEP(50000);
        hprintf(webblk->sock,"{\"command\": \"%s\",",command);
    }
    else
        hprintf(webblk->sock,"{\"command\": \"\",");

    if((value = cgi_variable(webblk,"msgcount")))
        msgcount = atoi(value);

    hprintf(webblk->sock,"\"msgcount\": %d,",msgcount);
    hprintf(webblk->sock,"\"syslog\": [\"");


    logbuf_idx = msgcount ? log_line( msgcount ) : -1;

    if ((num_bytes = log_read( &logbuf_ptr, &logbuf_idx, LOG_NOBLOCK )) > 0)
    {
        char   *wrk_bufptr      = malloc( num_bytes );
        char   *sav_wrk         = NULL;

        if (wrk_bufptr)
        {
            sav_wrk = wrk_bufptr;
            strncpy( wrk_bufptr,  logbuf_ptr, num_bytes );
        }
        else         wrk_bufptr = logbuf_ptr;


        // We need to escape certain characters that are
        // not supported in JSON, namely '"', '\n' and '\'"

        while ( num_bytes-- )
        {
            switch ( *wrk_bufptr )
            {
            case '\\':
                hwrite( webblk->sock, "\\\\"    , 2);
                break;
            case '\n':
                //hwrite( webblk->sock, "\\n"    , 2                );
        hprintf(webblk->sock,"\",\"");
                break;
            case '"':
                hwrite( webblk->sock, "\\\""    , 2  );
                break;
            default:
                hwrite( webblk->sock, wrk_bufptr , 1              );
                break;
            }

            wrk_bufptr++;
        }

        // (free our work buffer if it's really ours)

        if ( sav_wrk )
            free( sav_wrk );
    }

    hprintf(webblk->sock,"\"]}");
}

/*-------------------------------------------------------------------*/
/*                     cgibin_api_v1_storage                         */
/*-------------------------------------------------------------------*/
/* Copied from cgibin_debug_storage                                  */
/* Returns the number of fullwords located at address. Can take two  */
/* parameters: address, the starting address and fullwords, the      */
/*             number of fullwords to return                         */
/*                                                                   */
/* {                                                                 */
/*    "address":"00000000",                                          */
/*    "fullwords":"32",                                              */
/*    "mainsize": 67108864,                                          */
/*    "storage":"00080000...800003D0"                                */
/* }                                                                 */
/*-------------------------------------------------------------------*/

void cgibin_api_v1_storage(WEBBLK *webblk)
{
int i,size;
int fw = 32;
char *value;
U32 addr = 0;

    addr &= ~0x0F;

    json_header(webblk);
    hprintf(webblk->sock,"{");

    if((value = cgi_variable(webblk,"fullwords"))) // number of fullwords to fetch
        fw = atoi(value);

    if((value = cgi_variable(webblk,"address"))) // starting address
        sscanf(value,"%x",&addr);

    hprintf(webblk->sock,"\"address\":\"%8.8X\",",addr);
    hprintf(webblk->sock,"\"fullwords\":\"%d\",",fw);
    hprintf(webblk->sock,"\"mainsize\": %"PRId64",",sysblk.mainsize);

    size = fw * 4;

    if(addr > sysblk.mainsize)
        addr = sysblk.mainsize - size;
    if((addr + size) > sysblk.mainsize)
        size = sysblk.mainsize;

    hprintf(webblk->sock,"\"storage\":\"");

    for(i = 0; i < size; i += 4)
    {
        U32 m;
        FETCH_FW(m,sysblk.mainstor + i + addr);
        hprintf(webblk->sock,"%8.8X",m);
    }

    hprintf(webblk->sock,"\"}");
}

/*-------------------------------------------------------------------*/
/*                      cgibin_api_v1_devices                        */
/*-------------------------------------------------------------------*/
/* Return an array of all the devices and information about them     */
/* with the following format:                                        */
/*{                                                                  */
/*    "devices": [                                                   */
/*        {                                                          */
/*        "devnum": "0009",                                          */
/*        "subchannel": "0000",                                      */
/*        "devclass": "CON",                                         */
/*        "devtype": "3215",                                         */
/*        "status": "open ",                                         */
/*        "assignment": "*syscons cmdpref(/) noprompt IO[0]"         */
/*        }, {                                                       */
/*        "devnum": "000C",                                          */
/*        "subchannel": "0001",                                      */
/*        "devclass": "RDR",                                         */
/*        "devtype": "3505",                                         */
/*        "status": "",                                              */
/*        "assignment": "./util/zzsacard.bin intrq IO[0]"            */
/*        }, {                                                       */
/*        "devnum": "000D",                                          */
/*        "subchannel": "0002",                                      */
/*        "devclass": "PCH",                                         */
/*        "devtype": "3525",                                         */
/*        "status": "open ",                                         */
/*        "assignment": "punch00d.txt ascii IO[0]"                   */
/*        }                                                          */
/*    ]                                                              */
/* }                                                                 */
/*-------------------------------------------------------------------*/
void cgibin_api_v1_devices(WEBBLK *webblk)
{
    DEVBLK *dev;
    char   *devclass;
    char    devnam[MAX_PATH];
    int     count=0;
    int     total=0;
    int     l, p;

    json_header(webblk);
    hprintf(webblk->sock,"{\"devices\":[");
    for(dev = sysblk.firstdev; dev; dev = dev->nextdev)
        total++;

    for(dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        if(dev->pmcw.flag5 & PMCW5_V)
        {
            (dev->hnd->query)(dev, &devclass, sizeof(devnam), devnam);

            l = (int)strlen(devnam);
            for ( p = 0; p < l; p++ )
            {
                if ( !isprint((unsigned char)devnam[p]) )
                {
                    devnam[p] = '\0';
                    break;
                }
            }

             hprintf(webblk->sock,"{\"devnum\":\"%4.4X\","
                                   "\"subchannel\":\"%4.4X\","
                                   "\"devclass\": \"%s\","
                                   "\"devtype\": \"%4.4X\","
                                   "\"status\": \"%s%s%s\","
                                   "\"assignment\": \"%s\"}"
                                   "%s",
                                   dev->devnum,dev->subchan,
                                   devclass,
                                   dev->devtype,
                                   (dev->fd >= 0       ? "open "    : ""),
                                   (dev->busy          ? "busy "    : ""),
                                   (IOPENDING(dev)     ? "pending " : ""),
                                   devnam,
                                   (count == total - 1 ? ""         : "," ));
        }
        count++;
    }

    hprintf(webblk->sock,"]}");
}
/*-------------------------------------------------------------------*/
/*                      END JSON APIs                                */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*   cgibin_hwrite      --      helper function to output HTML       */
/*-------------------------------------------------------------------*/
void cgibin_hwrite(WEBBLK *webblk, char *msg, int msg_len)
{
    char buffer[1024];
    char *new_str;

    unsigned buf_used = 0;
    unsigned new_len;
    int i;


    if ((msg == NULL) || (msg_len < 1))
        return;

    // Output message, accounting for http special characters.

    // Method: rather than doing an hwrite for every character,
    // buffer the message, expanding special characters.
    // Output the buffer when full, then reuse it.

    // Note that sizeof(X) where X is a #define string literal is 1 greater
    // than strlen(X).

    for (i = 0; i < msg_len; i++)
    {
        switch (msg[i])
        {
        case '<':
            new_len = sizeof(AMP_LT) - 1;
            new_str = AMP_LT;
            break;
        case '>':
            new_len = sizeof(AMP_GT) - 1;
            new_str = AMP_GT;
            break;
        case '&':
            new_len = sizeof(AMP_AMP) - 1;
            new_str = AMP_AMP;
            break;
        default:
            new_len = 1;
            new_str = &(msg[i]);
            break;
        }

        if ((buf_used + new_len) > sizeof(buffer))
        {
            // new piece won't fit, write forced
            hwrite(webblk->sock, buffer,  buf_used);
            buf_used = 0;
        }

        while (new_len > 0)
        {
            buffer[buf_used++] = *new_str++;
            new_len--;
        }
    }

    if (buf_used > 0)
    {
        // write out final/partial buffer
        hwrite(webblk->sock, buffer,  buf_used);
    }
}

/*-------------------------------------------------------------------*/
/* issue panel command and return response only, without the rest    */
/* of the syslog, and without most of the HTML wrapping              */
/*-------------------------------------------------------------------*/
void cgibin_cmd_cmd(WEBBLK *webblk, char *command)
{
    char * response;

    while (isspace((unsigned char)*command))
        command++;

    if (*command == 0)
    {
        return;             /* command is all blank, ignore */
    }

    panel_command_capture( command, &response, false );

    if (response == NULL)
    {
        return;             /* command failed to execute */
    }

    html_header(webblk);
    hprintf(webblk->sock, "<PRE>\n");

    cgibin_hwrite(webblk, response, (int)strlen (response));

    hprintf(webblk->sock, "</PRE>\n");
    html_footer(webblk);

    // Ensure command and response is visible on Hercules console panel

    logmsg( "%s", response );

    free (response);
}

/*-------------------------------------------------------------------*/
/* handle http requests of the form:                                 */
/*     http://localhost:8081/cgi-bin/tasks/cmd?cmd=qcpuid            */
/*-------------------------------------------------------------------*/
void cgibin_cmd(WEBBLK *webblk)
{
char   *command;

    if ((command = cgi_variable(webblk,"cmd")))
    {
        /* "cmd" issues a single command */
        cgibin_cmd_cmd (webblk, command);
        return;
    }
}

/*-------------------------------------------------------------------*/
/* The following table is the cgi-bin directory, which associates    */
/* directory filenames with cgibin routines.                         */
/*-------------------------------------------------------------------*/
CGITAB cgidir[] =
{
    { "blinkenlights/cpu",   &cgibin_blinkenlights_cpu   },

    { "configure/cpu",       &cgibin_configure_cpu       },

    { "debug/registers",     &cgibin_debug_registers     },
    { "debug/storage",       &cgibin_debug_storage       },
    { "debug/misc",          &cgibin_debug_misc          },
    { "debug/version_info",  &cgibin_debug_version_info  },
    { "debug/device/list",   &cgibin_debug_device_list   },
    { "debug/device/detail", &cgibin_debug_device_detail },

    { "tasks/cmd",           &cgibin_cmd                 },
    { "tasks/syslog",        &cgibin_syslog              },
    { "tasks/ipl",           &cgibin_ipl                 },

    { "registers/general",   &cgibin_reg_general         },
    { "registers/control",   &cgibin_reg_control         },
    { "registers/psw",       &cgibin_psw                 },

    { "xml/rates",           &cgibin_xml_rates_info      },

    /*---------------------------------------------------*/
    /*   Adding JSON APIs                                */

    { "api/v1",             &cgibin_api_v1              },
    { "api/v1/cpus",        &cgibin_api_v1_cpus         },
    { "api/v1/psw",         &cgibin_api_v1_psw          },
    { "api/v1/version",     &cgibin_api_v1_version      },
    { "api/v1/syslog",      &cgibin_api_v1_syslog       },
    { "api/v1/devices",     &cgibin_api_v1_devices      },
    { "api/v1/maxrates",    &cgibin_api_v1_maxrates     },
    { "api/v1/storage",     &cgibin_api_v1_storage      },
    { "api/v1/rates",       &cgibin_api_v1_rates        },

    { NULL, NULL }
};

/*-------------------------------------------------------------------*/
