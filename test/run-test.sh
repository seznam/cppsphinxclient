
#
#Test sphinxclient
#
#./run-test.sh
#
#Script sets up test environment and run tests. Sphinx indexer
#and searchd binaries must be put in directories in $PATH environment
#variable.
#
#Description of what run-test.sh does:
#
#* create database sphinxtest_cppsphinxclient_123456
#* fill database with test values
#* index with sphinx indexer command
#* starts searchd
#* run test programs
#* stop searchd


db=sphinxtest_cppsphinxclient_123456

(cd ../; make) || exit -1
# re-create testing database
echo "drop database if exists ${db}" | mysql -u root
echo "create database ${db}" | mysql -u root
# fill database
mysql -u root ${db} < testdata.sql
# index 
indexer --config sphinxtest.conf --all
# start searchd
searchd --config sphinxtest.conf
# run tests
../sphinxtest || (echo "./sphinxtest failed"; kill `cat searchd.pid`; exit -1) || exit -1
../sphinxtest2 || (echo "./sphinxtest2 failed"; kill `cat searchd.pid`; exit -1) || exit -1
../keywordstest || (echo "./keywordstest failed"; kill `cat searchd.pid`; exit -1) || exit -1
../sphinxtest3 || (echo "./sphinxtest3 failed"; kill `cat searchd.pid`; exit -1) || exit -1
#../mqtest


#kill searchd
kill `cat searchd.pid`

echo "all tests passed."

