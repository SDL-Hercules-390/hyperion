/* vsvers.h     (C) "Fish" (David B. Trout), 2017                    */
/*              Visual Studio compiler version constants             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _VSVERS_H_
#define _VSVERS_H_

/*-------------------------------------------------------------------*/
/*  The following defines are to more easily test for known          */
/*  versions of Microsoft's Visual Studio compiler via e.g.          */
/*  #if (_MSC_VER >= VS2008), #if (_MSC_VER >= MSVC9), etc.          */
/*-------------------------------------------------------------------*/
/*    Add support for new Visual Studio versions here...             */
/*    Don't forget to update the 'VSVERS.msvc' file also!            */
/*    Don't forget to update the 'makefile.bat' file too!            */
/*-------------------------------------------------------------------*/

#define VS2022_9    1939                /* Visual Studio 2022 */
#define VS2022_8    1938                /* Visual Studio 2022 */
#define VS2022      1930                /* Visual Studio 2022 */
#define VS2019      1920                /* Visual Studio 2019 */
#define VS2017_5    1912                /* Visual Studio 2017 */
#define VS2017_4    1911                /* Visual Studio 2017 */
#define VS2017_3    1911                /* Visual Studio 2017 */
#define VS2017_2    1910                /* Visual Studio 2017 */
#define VS2017_1    1910                /* Visual Studio 2017 */
#define VS2017      1910                /* Visual Studio 2017 */
#define VS2015      1900                /* Visual Studio 2015 */
#define VS2013      1800                /* Visual Studio 2013 */
#define VS2012      1700                /* Visual Studio 2012 */
#define VS2010      1600                /* Visual Studio 2010 */
#define VS2008      1500                /* Visual Studio 2008 */
#define VS2005      1400                /* Visual Studio 2005 */
#define VS2003      1310                /* Visual Studio 2003 */
#define VS2002      1300                /* Visual Studio 2002 */

#define MSVC17      1930                /* Visual Studio 2022 */
#define MSVC16      1920                /* Visual Studio 2019 */
#define MSVC15_5    1912                /* Visual Studio 2017 */
#define MSVC15_4    1911                /* Visual Studio 2017 */
#define MSVC15_3    1911                /* Visual Studio 2017 */
#define MSVC15_2    1910                /* Visual Studio 2017 */
#define MSVC15_1    1910                /* Visual Studio 2017 */
#define MSVC15      1910                /* Visual Studio 2017 */
#define MSVC14      1900                /* Visual Studio 2015 */
#define MSVC12      1800                /* Visual Studio 2013 */
#define MSVC11      1700                /* Visual Studio 2012 */
#define MSVC10      1600                /* Visual Studio 2010 */
#define MSVC9       1500                /* Visual Studio 2008 */
#define MSVC8       1400                /* Visual Studio 2005 */
#define MSVC71      1310                /* Visual Studio 2003 */
#define MSVC7       1300                /* Visual Studio 2002 */

#endif /*_VSVERS_H_*/
