#!/bin/bash

set -e -v

VERSION="0.3.5-atlassian-$1"

git tag $VERSION
git push --tags

mvn3 deploy:deploy-file -Durl=https://maven.atlassian.com/3rdparty \
                       -DrepositoryId=atlassian-3rdparty \
                       -Dfile=target/javasysmon.jar \
                       -DgroupId=com.jezhumble.javasysmon \
                       -DartifactId=javasysmon \
                       -Dversion=$VERSION \
                       -Dpackaging=jar \
                       -DgeneratePom=true 
