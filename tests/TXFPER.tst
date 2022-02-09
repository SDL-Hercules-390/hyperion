*Testcase TXFPER: Test TXF PER Event-Suppression option
sysclear
archlvl z/Arch
diag8cmd enable
loadcore "$(testpath)/TXFPER.core"
runtest .2
*Done
diag8cmd disable
