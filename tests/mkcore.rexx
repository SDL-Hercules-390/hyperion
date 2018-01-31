/* Make core image from TEXT deck                                    */
/*                                John Hartmann 17 Aug 2016 11:29:36 */
Signal on novalue

/* This file was put into the public domain 2016-08-20               */
/* by John P. Hartmann.  You can use it for anything you like,       */
/* as long as this notice remains.                                   */

core = ''

do while lines() > 0
   ln = charin(, , 80)
   parse var ln two +1 tpe+3 +1 addr+3 +2 len+2 +2 id+2 data
   addr = c2d(addr)
   len = c2d(len)
   id = c2d(id)
   /* say d2x(addr) len id length(data) c2x(left(data, len))         */
   Select
      When two \== '02'x
         Then say 'Bad start char' c2x(two)
      When tpe = 'c5e2c4'x
         Then call esd
      When tpe = 'e3e7e3'x            /* TXT                         */
         then core = overlay(data, core, addr+1, len, '00'x)
      When tpe = 'd9d3c4'x
         Then nop
      When tpe = 'c5d5c4'x            /* END                         */
         Then leave
      Otherwise
         call err 'Bad record' c2x(tpe)
   end
end

/* Pad module to control section length                              */
call charout , left(core, modlen, '00'x)
exit

esd:
If len \= 16
   Then call err len 'bytes of ESD; 16 expecetd.'
parse var data name+8 t+1 addr+3 +2 len+2
If t \= '00'x
   Then call err 'ESD expected.  Got' c2x(t)
modlen = c2d(len)
say 'Section length:' c2x(len)
return

err:
say arg(1)
exit 17
