#!/bin/sh

echo
echo "Wrapper Build System"
echo "--------------------"

if [ "$WRAPPER_TOOLS" = "" ] ; then
    if [ -d tools ] ; then 
        WRAPPER_TOOLS=tools
    else
        echo "Unable to locate tools directory at tools/."
        echo "Aborting."
        exit 1
    fi
fi

chmod u+x $WRAPPER_TOOLS/bin/antRun
chmod u+x $WRAPPER_TOOLS/bin/ant

$WRAPPER_TOOLS/bin/ant -logger org.apache.tools.ant.NoBannerLogger -emacs -Dtools.dir=$WRAPPER_TOOLS $@ 
