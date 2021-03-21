export LD_LIBRARY_PATH="$(pwd)/third_party/motr/motr/.libs/:"\
"$(pwd)/third_party/motr/helpers/.libs/:"\
"$(pwd)/third_party/motr/extra-libs/gf-complete/src/.libs/"


systemctl start haproxy
systemctl start s3authserver
systemctl start slapd


./third_party/motr/motr/st/utils/motr_services.sh start
./dev-starts3.sh

./runallsystest.sh --no_https

./third_party/motr/motr/st/utils/motr_services.sh stop
./dev-stops3.sh
