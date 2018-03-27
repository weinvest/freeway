#bin/bash
ls | egrep -v '\.sh' | xargs -n 1 -I ff rm -rf ff
