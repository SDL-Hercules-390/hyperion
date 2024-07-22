*Testcase TXFPER
# Test PER Tracing of TXF Transactions
sysclear
archlvl z/Arch
diag8cmd enable
loadcore "$(testpath)/TXFPER.core"
runtest .2
*Done
diag8cmd disable
