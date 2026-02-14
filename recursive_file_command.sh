#!/usr/bin/bash

# THis simple script (AUg/1/25) recursively walks a directory and calls 
# the command passed with the "-c" switch for every file it finds
# Takes the first argument as the directory to start with when running the
# command passed in the "-c" 
#
# NOTE:
# Based on one of the comments in
#    https://stackoverflow.com/questions/1194882/how-to-generate-random-number-in-bash
#  bash 5.1 introduces a new variable, SRANDOM, which gets its random data from
#  the system's entropy engine and so is not linear and cannot be reseeded to
#  get an identical random sequence.
# HEnce I am using $SRANDOM to rename shreded files and directories

command_dir() {
    CURR_DIR=`pwd`
    if [ "${1}" != "${MYCOMMANDDIR}" ]; then
        echo -e "\e[32mGoing into ${CURR_DIR}/${1}/\e[0m"
    else
        echo -e "\e[32mGoing into ${1}/\e[0m"
    fi
    pushd ${1} > /dev/null # "pushd" prints out the directory that it goes into
    MYFILES=`ls ./`
    for i in ${MYFILES}; do
        if [ -d "$i" ]; then
            command_dir ${i}
        else
            if [ -e ${i} ]; then
                if [ ! -z "${VERBOSE}" ]; then
                    echo -e "\e[91m${COMMAND} \e[35m${i}\e[0m"
                fi
                ${COMMAND} ${i}
            else
                if [ ! -z "${VERBOSE}" ]; then
                    echo -e "\e[32mSkippinging non-file \e[35m${i}\e[0m\""
                fi
            fi
        fi
    done
    popd > /dev/null # "popd" prints out the directory that it goes into
}


print_usage() {
    echo -e "Usage: "
    echo -e "${0} [options] <starting-dir-to-run-command> "
    echo "Applies a command to every file under the directory <starting-dir-to-run-command>"
    echo -e "options:"
    echo -e "\t-t<seconds>  wait <seconds> before running the command passed in the '-c' option"
    echo -e "\t-v Verbose output"
    echo -e "\t-c\"<command>\"  Apply the command <command> instead of the  default '${COMMAND}' to every file. MANDATORY option"
}





# default is to rename
DONOTRENAME=false

# There is NO default command to run on every file. The user MUST supply a
# command
COMMAND="ls -l"


# Set default wait time before starting to run the command
WAITTIME=2

# Let's parse the options
while getopts ":vht:c:" Option
do
  case $Option in
    v     ) VERBOSE="true"; echo "Going to be VERBOSE [OPTIND=${OPTIND}]";;
    n     ) DONOTRENAME="true"; echo "Will NOT rename files or directories [DONOTRENAME=${DONOTRENAME} [OPTIND=${OPTIND}]";;
    t     ) WAITTIME=$OPTARG; echo "Going to wait $WAITTIME before shredding [OPTIND=${OPTIND}]";;
    c     ) echo "Going to apply '$OPTARG' instead of default command '${COMMAND}' [OPTIND=${OPTIND}]";COMMAND=$OPTARG;;
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
    echo -e "\e[91mMUST specify directory to start runing the command on !!!\e[0m"
    print_usage
    exit 1
else
    MYCOMMANDDIR=$1
fi
if [ ! -e ${MYCOMMANDDIR} ]; then
    echo -e "\e[91mCANNOT find  $MYCOMMANDDIR !!!\e[0m"
    exit 1
fi
if [ ! -d ${MYCOMMANDDIR} ]; then
    echo -e "\e[91m$MYCOMMANDDIR is NOT a directory!!!\e[0m"
    print_usage
    exit 1
fi
 
echo -e "In \e[1m\e[5m\e[91m\e[4m $WAITTIME SECONDS\e[0m \e[91mGoing to run \"$COMMAND\" on ALL files under \e[0m\"\e[1m\e[5m\e[91m$MYCOMMANDDIR\e[0m\""
sleep ${WAITTIME}
command_dir ${MYCOMMANDDIR}
exit $?
