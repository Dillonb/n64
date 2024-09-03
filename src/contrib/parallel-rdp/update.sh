#!/usr/bin/env bash
set -e
wget https://github.com/themaister/parallel-rdp-standalone/archive/refs/heads/master.tar.gz
tar xvf master.tar.gz
rsync -aP --delete parallel-rdp-standalone-master/* parallel-rdp-standalone/
rm master.tar.gz
rm -rf parallel-rdp-standalone-master
