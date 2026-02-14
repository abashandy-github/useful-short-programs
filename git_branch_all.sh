#!/usr/bin/bash
#
# REMEMBER that the way building an image works in the repository
# hpc-hms_ec-hms-scimage is by cloning all repos that are needed inside the
# directory pointed to by the variable $WORKAREA
# Similarly for the simulation, the repositories needed to build the simulation
# containers are cloned under hpc-sshot-slingshot-fabric-simulation/switch/src
#
# The objective of this script is to go into all github repositories that
# were cloned and do "git log"
#
# The way this script runs is as follows
# - Run the script
#   - PAss the directory $WORKAREA as the first argument
#   OR
#   - without any arguments from the directory $WORKAREA
# - IT will look into every subdirectory under the current directory (if no
#    arguments passed), or in the directory passed as the first argument 
# - If it finds the subdiretory ".git/" under any directory, it will assume
#   that this subdirectory is a cloned repository
# - Hence it will go inside it and do "git pull origin"
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
         
         
         
