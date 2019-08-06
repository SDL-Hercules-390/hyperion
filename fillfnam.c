/* FILLFNAM.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules filename completion functions               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _FILLFNAM_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "fillfnam.h"

/* On Solaris 2.9 (SunOS 5.9) and earlier, there is no scandir
   and alphasort function. In this case fillfnam does nothing
   and the tab command is effectively a no-operation */
#if !(defined(HAVE_SCANDIR) && defined(HAVE_ALPHASORT)) && !defined(_MSVC_)
int tab_pressed(char *cmdlinefull, size_t cmdlinelen, int *cmdoffset) {
  UNREFERENCED(cmdlinefull);
  UNREFERENCED(cmdoffset);
  UNREFERENCED(cmdlinelen);
  return 0;
}
#else

char *filterarray;

#ifdef SCANDIR_CONST_STRUCT_DIRENT
int filter(const struct dirent *ent)
#else
int filter(      struct dirent *ent)
#endif
{
  if (filterarray == NULL)
    return(1);
  if (strncmp(ent->d_name, filterarray, strlen(filterarray)) == 0)
    return(1);
  return(0);
}

/* tab can be pressed anywhere in the command line, so I have to split
   command line
   for example: attach 0a00 3390 volu_ sf=volume1.shadow
   will be divided
   part1 = "attach 0a00 "
   part2 = "volu"
   part3 = " sf=volume1.shadow"
   only part2 can be changed and it must be divided into path and filename
*/
int tab_pressed(char *cmdlinefull, size_t cmdlinelen, int *cmdoffset) {
  struct dirent **namelist;
  int   n, i, j, len, len1, len2;
  int   cmdoff = *(cmdoffset); /* for easy reading of source code */
  char *part1, *part2, *part3;
  char *buff;
  char *filename, *path, *tmp;
  char  result[1024];
  char  pathname[MAX_PATH];
  size_t    bl, pl;
#ifdef _MSVC_
  int within_quoted_string = 0;
  int quote_pos;
#endif

  /* part3 goes from cursor position to the end of line */
  part3 = cmdlinefull + cmdoff;
  /* looking for ' ','@' or '=' backward, starting from cursor offset
     I am not aware of any other delimiters which can be used in hercules */
  /* (except for '"' (double quote) for the MSVC version of herc - Fish) */
#ifdef _MSVC_
  /* determine if tab was pressed within a quoted string */
  for (i=0; i < cmdoff; i++)
      if ('\"' == cmdlinefull[i])
      {
          within_quoted_string = !within_quoted_string;
          quote_pos = i; /* (save position of quote
                         immediately preceding cmdoff) */
      }
  if (within_quoted_string)
    i = quote_pos; /* (the quote is our delimiter) */
  else
#endif
  for (i = cmdoff-1; i>=0; i--)
    if (cmdlinefull[i] == ' '
       || cmdlinefull[i] == '@'
       || cmdlinefull[i] == '=')
      break;
  /* part1 is from beginning to found delimiter (including delimiter itself) */
  part1 = (char*) malloc(i+2);
  strncpy(part1, cmdlinefull, i+1);
  part1[i+1]= '\0';

  /* part2 is the interesting one */
  part2 = (char*)malloc(cmdoff - i);
  strncpy(part2, cmdlinefull + i + 1, cmdoff - i - 1);
  part2[cmdoff - i - 1] = '\0';

  len = (int)strlen(part2);
  /* 3 characters minimum needed, for ./\0 in path. */
  /* (or 4 chars for MSVC if within quoted string, for \"./\0 - Fish) */
#ifdef _MSVC_
  if (within_quoted_string)
  {
    if (len < 3)
      len = 3;
  }
  else
#endif
  if (len < 2)
    len = 2;
  pl = len + 1;
  path = (char*)malloc(pl);
  *path = '\0';
  filename = part2;
  /* is it pure filename or is there whole path ? */
#ifdef _MSVC_
  tmp = strrchr(part2, '\\');
#else
  tmp = strrchr(part2, '/');
#endif
#ifdef _MSVC_
  if (!tmp) tmp = strrchr(part2, '\\');
#endif
  if (tmp != NULL) {
    filename = tmp + 1;
    memcpy(path, part2, strlen(part2)-strlen(filename));
    path[strlen(part2)-strlen(filename)] = '\0';
    tmp[0] = '\0';
  }
  else {
#ifdef _MSVC_
    if (within_quoted_string)
        strlcpy(path,"\".\\",pl);
    else
        strlcpy(path,".\\",pl);
#else
        strlcpy(path,"./",pl);
#endif

  }
  /* this is used in filter function to include only relevant filenames */
  filterarray = filename;

  n = scandir(path, &namelist, filter, alphasort);
  if (n > 0) {
    for (i=0; i<n; i++) {
      struct stat buf;
      char fullfilename[1+MAX_PATH+1];
      /* namelist[i]->d_name contains filtered filenames, check if they are
         directories with stat(), before that create whole path */
      if (tmp != NULL)
         sprintf(fullfilename, "%s%s", path, namelist[i]->d_name);
      else
         sprintf(fullfilename, "%s", namelist[i]->d_name);
#ifdef _MSVC_
      if (within_quoted_string)
        STRLCAT( fullfilename, "\"" );
#endif
      /* if it is a directory, add '/' to the end so it can be seen on screen*/
      hostpath(pathname, fullfilename, sizeof(pathname));
      if (stat(pathname,&buf) == 0)
         if (buf.st_mode & S_IFDIR) {
//          STRLCAT( namelist[i]->d_name, "/" );
// Don't write past end of d_name
// Problem debugged by bb5ch39t
            namelist[i] = realloc(namelist[i], sizeof(struct dirent)
                                + strlen(namelist[i]->d_name) + 2);
            if (namelist[i])
#ifdef _MSVC_
               STRLCAT( namelist[i]->d_name, "\\" );
#else
               STRLCAT( namelist[i]->d_name, "/" );
#endif
         }
    }
    /* now check, if filenames have something in common, after a cycle buff
       contains maximal intersection of names */
    bl = (size_t)(strlen(namelist[0]->d_name) + 1);
    buff = (char*)malloc(bl); /* first one */
    strlcpy(buff, namelist[0]->d_name,bl);
    for (i = 1; i < n; i++) {
      len1 = (int)strlen(buff);
      len2 = (int)strlen(namelist[i]->d_name);
      /* check number of characters in shorter one */
      len = len1 > len2 ? len2 : len1;
      for (j = 0; j < len; j++)
      if (buff[j] != namelist[i]->d_name[j]) {
        buff[j] = '\0'; /* end the string where characters are not equal */
        break;
      }
    }
    /* if maximal intersection of filenames is longer then original filename */
    if (strlen(buff) > strlen(filename)) {
      char *fullfilename;

      bl = (size_t)(strlen(path) + strlen(buff) + 2);
      fullfilename = (char*)calloc(1,bl);

      /* this test is not useless as path contains './' if there was no path
         in original filename. it is because of scandir function, which
         needs path portion */
      if (tmp != NULL)
         snprintf(fullfilename, bl, "%s%s", path, buff);
      else
         snprintf(fullfilename, bl, "%s", buff);
      /* construct command line */
      snprintf(result, sizeof(result), "%s%s%s", part1, fullfilename, part3);
      /* move cursor */
      *(cmdoffset) = (int)(strlen(part1) + strlen(fullfilename));
      strlcpy(cmdlinefull, result, cmdlinelen);
      free(fullfilename);
    }
    else
    {
      /* display all alternatives */
      for (i = 0; i< n; i++)
         logmsg("%s\n", namelist[i]->d_name);
    }
    /* free everything */
    free(buff);
    for (i = 0; i< n; i++)
      free(namelist[i]);
    free(namelist);

  }
  free(part1);
  free(part2);
  free(path);
  return(0);
}
#endif /*(HAVE_SCANDIR && HAVE_ALPHASORT) || _MSVC_*/
