/* Testcase 3211 printer */

if arg() \= 1 then
do
    say "ERR: arg() \= 1"
    exit 0
end

filename = strip( arg(1) )

if length( filename ) = 0 then
do
    say "ERR: length( filename ) = 0"
    exit 0
end

      rc = stream( filename, "command", "open read" )
filesize = stream( filename, "command", "query Size" )
      rc = stream( filename, "command", "close" )

if length( filesize ) = 0 then
do
    say "ERR: length( filesize ) = 0"
    exit 0
end

say filesize
exit filesize
