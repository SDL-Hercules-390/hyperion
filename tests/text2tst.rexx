/* Convert an object deck with PUNCH/REPRO annotation to .tst        */
/*                                John Hartmann 30 Nov 2016 12:31:23 */
Signal on novalue

/* This  file  is a variation of a file that was put into the public */
/* domain  2015-10-07  by  John  P.   Hartmann.   You can use it for */
/* anything you like, as long as this notice remains.                */

exitcode = 0

parse source .. myname .
call out ,
   '# This test file was generated from offline assembler source',,
   '# by' myname date() time(),,
   '# Treat as object code.  That is, modifications will be lost.',,
   '# assemble and listing files are provided for information only.'
do while lines() > 0
   l = charin(,, 80)

   Select
      When left(l, 4) == '02e3e7e3'x  /* TXT record                  */
         Then call txt
      When left(l, 1) == '02'x        /* Other object                */
         then nop
      otherwise
         call out l
   end
end
exit exitcode

/*********************************************************************/
/* Process text record into 16-byte stores.                          */
/*********************************************************************/

txt:
addr = c2d(substr(l, 6, 3))
len = c2d(substr(l, 11, 2))
data = substr(l, 17, 56)
do while len > 0
   toget = min(16, len)
   parse var data w +(toget) data
   call out 'r' right(d2x(addr), 6)'='c2x(w)
   addr = addr + 16
   len = len - 16
end
return

out:
do ? = 1 to arg()
   call lineout , arg(?)
end
return
