#!/usr/bin/bash

# THis simple script (AUg/1/25) recursively walks a directory and calls 
# "shred" for every file it finds then renames it to a random number
# Takes the first argument as the directory to shred and optional second
# argument to wait for this number of seconds before shredding to allow the
# user to press CTRL^C. If no second argument, it wait 2 seconds
# After renaming all files in a directory, it will rename the directory itself
# It does NOT delete any directory or file but just renames them
# If you want, you can delete it yourself using rm -rf
#
# NOTE:
# Based on one of the comments in
#    https://stackoverflow.com/questions/1194882/how-to-generate-random-number-in-bash
#  bash 5.1 introduces a new variable, SRANDOM, which gets its random data from
#  the system's entropy engine and so is not linear and cannot be reseeded to
#  get an identical random sequence.
# HEnce I am using $SRANDOM to rename shreded files and directories

shred_dir() {
    CURR_DIR=`pwd`
    if [ "${1}" != "${MYSHREDDIR}" ]; then
        echo -e "\e[32mGoing into ${CURR_DIR}/${1}/\e[0m"
    else
        echo -e "\e[32mGoing into ${1}/\e[0m"
    fi
    pushd ${1} > /dev/null # "pushd" prints out the directory that it goes into
    MYFILES=`ls ./`
    for i in ${MYFILES}; do
        if [ -d "$i" ]; then
            shred_dir ${i}
        else
            if [ -e ${i} ]; then
                if [ ! -z "${VERBOSE}" ]; then
                    echo -e "\e[91m${SHRED} \e[35m${i}\e[0m"
                fi
                ${SHRED} ${i}
                RENAMED=`echo -e $SRANDOM`
                if [ ! -z "${RENAMED}" -a "${DONOTRENAME}" = false ]; then 
                    if [ ! -z "${VERBOSE}" ]; then
                        echo -e "Renaming file ${i} to ${RENAMED}"
                    fi
                    mv ${i} ${RENAMED}
                fi
            else
                if [ ! -z "${VERBOSE}" ]; then
                    echo -e "\e[32mSkippinging non-file \e[35m${i}\e[0m\""
                fi
            fi
        fi
    done
    popd > /dev/null # "popd" prints out the directory that it goes into
    RENAMED=`echo -e $SRANDOM`
    if [ ! -z "${RENAMED}" -a "${DONOTRENAME}" = false ]; then
        if [ ! -z "${VERBOSE}" ]; then
            CURR_DIR=`pwd`
            if [ "${1}" != "${MYSHREDDIR}" ]; then
                echo -e "Renaming directory ${CURR_DIR}/${1}/ to ${CURR_DIR}/${RENAMED}/"     
            else
                echo -e "Renaming directory ${1}/ to ${CURR_DIR}/${RENAMED}/"     
            fi
        fi
        mv ${1} ${RENAMED}
        MY_RENAMED_DIR="${CURR_DIR}/${RENAMED}"
    fi
}


print_usage() {
    echo -e "Usage: "
    echo -e "${0} [options] <dir-to-shred> "
    echo "Applies a command to every file under a directory. Default is '${SHRED}' "
    echo -e "options:"
    echo -e "\t-t<seconds>  wait <seconds> before shredding"
    echo -e "\t-v Verbose output"
    echo -e "\t-n do NOT rename files or directories"
    echo -e "\t-d Delete the entire directory at the end. Default is do NOT delete "
    echo -e "\t-c\"<command>\"  Apply the command <command> instead of the default '${SHRED}' to every file"
}





# Default is NOT to delete the directory
DELETE=false

# default is to rename
DONOTRENAME=false

# Set default shred command
SHRED="shred -f"


# Set default wait time
WAITTIME=2

# Let's parse the options
while getopts ":vnhdt:c:" Option
do
  case $Option in
    d     ) DELETE="true"; echo "Going to be DELETE directory  [OPTIND=${OPTIND}]";;
    v     ) VERBOSE="true"; echo "Going to be VERBOSE [OPTIND=${OPTIND}]";;
    n     ) DONOTRENAME="true"; echo "Will NOT rename files or directories [DONOTRENAME=${DONOTRENAME} [OPTIND=${OPTIND}]";;
    t     ) WAITTIME=$OPTARG; echo "Going to wait $WAITTIME before shredding [OPTIND=${OPTIND}]";;
    c     ) echo "Going to apply '$OPTARG' instead of default command '${SHRED}' [OPTIND=${OPTIND}]";SHRED=$OPTARG;;
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
    echo -e "\e[91mMUST specify directory to shred !!!\e[0m"
    print_usage
    exit 1
else
    MYSHREDDIR=$1
fi
if [ ! -e ${MYSHREDDIR} ]; then
    echo -e "\e[91mCANNOT find  $MYSHREDDIR !!!\e[0m"
    exit 1
fi
if [ ! -d ${MYSHREDDIR} ]; then
    echo -e "\e[91m$MYSHREDDIR is NOT a directory!!!\e[0m"
    print_usage
    exit 1
fi
 
echo -e "In \e[1m\e[5m\e[91m\e[4m $WAITTIME SECONDS\e[0m \e[91mGoing to shred ALL files under \e[0m\"\e[1m\e[5m\e[91m$MYSHREDDIR\e[0m\""
sleep ${WAITTIME}
MY_RENAMED_DIR="${MYSHREDDIR}"
shred_dir ${MYSHREDDIR}
if [ "${DELETE}" = true ]; then
    echo -e "Deleting '${MY_RENAMED_DIR}/', which was '${MYSHREDDIR}'"
    rm -rf ${MY_RENAMED_DIR}
fi
exit $?
