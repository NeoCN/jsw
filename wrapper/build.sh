#!/bin/sh

echo
echo "Aigent Build System"
echo "-------------------"

if [ "$AIGENT_TOOLS" = "" ] ; then
    if [ -d tools ] ; then 
        AIGENT_TOOLS=tools
    else
        echo "Unable to locate tools directory at tools/."
        echo "Aborting."
        exit 1
    fi
fi

chmod u+x $AIGENT_TOOLS/bin/antRun
chmod u+x $AIGENT_TOOLS/bin/ant

$AIGENT_TOOLS/bin/ant -logger org.apache.tools.ant.NoBannerLogger -emacs -Dtools.dir=$AIGENT_TOOLS $@ 
