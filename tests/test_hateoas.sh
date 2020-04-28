#!/bin/bash

#            ===Hateoas support test===
#
# This test allows to verify the correct functioning
# of the support for Hateoas in Polycube. Is compared
# the response json of a helloworld cube with the
# expected one so that this supports the Hateoas standard.
# The comparison is made only on the array containing
# the endpoint information and not on the entire response json.


source "${BASH_SOURCE%/*}/helpers.bash"

# the expected result
helloworld_result='[{"rel":"self","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/"},{"rel":"uuid","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/uuid/"},{"rel":"type","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/type/"},{"rel":"service-name","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/service-name/"},{"rel":"loglevel","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/loglevel/"},{"rel":"ports","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/ports/"},{"rel":"shadow","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/shadow/"},{"rel":"span","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/span/"},{"rel":"action","href":"//127.0.0.1:9000/polycube/v1/helloworld/hello/action/"}]'

set -e
set -x


function cleanup {
  set +e
  polycubectl del hello
  echo "FAIL"
}
trap cleanup EXIT

# adding a new helloworld cube
polycubectl helloworld add hello

# checking that the hateoas support is working properly
diff -q <(echo "$helloworld_result") <(echo "$(curl -s 'localhost:9000/polycube/v1/helloworld/hello' | jq -c '._links')")

# removing the helloworld cube
polycubectl del hello

set +x
trap - EXIT
echo "SUCCESS"