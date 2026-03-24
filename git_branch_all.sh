#!/usr/bin/bash
#
# 
print_usage() {
    echo -e "Usage: "
    echo -e "${0} [options] <starting-dir-to-do-git-branch> "
    echo "Runs 'git branch --show-current' on every subdirectory that is cloned repository and logs $LOGCOUNT commits"
}

COMMAND="git branch --show-current"

# Let's parse the options
while getopts ":h" Option
do
  case $Option in
    h     ) print_usage; exit 0;;
    #  Note that option 'q' must have an associated argument,
    #+ otherwise it falls through to the default.
  esac
done

shift $(($OPTIND - 1))
#  Decrements the argument pointer so it points to next argument.
#  $1 now references the first non-option item supplied on the command-line
#+ if one exists.

if [ -z ${1} ];  then
    MYWORKAREA=`pwd`
else
    MYWORKAREA=$1
fi

if [ ! -e ${MYWORKAREA} ]; then
    echo -e "\e[91mCANNOT find  $MYWORKAREA !!!\e[0m"
    exit 1
fi
pushd ${MYWORKAREA} > /dev/null
for i in $(find `pwd` -type d ); do
    if [ -d "${i}/.git" ]; then
        CURRDIR=`basename ${i}`
        # CURRDIR=${i}
        echo -e "\e[32m$CURRDIR\e[0m"
        cd $i
        git branch --show-current
        # The next format does NOT work and makes git log complain about ambiguous %cd
        # I do NOT know why !!!
#        ${COMMAND} -${LOGCOUNT}
    fi
done
popd
         
         
         
