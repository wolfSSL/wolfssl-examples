#!/bin/bash -
# arg1 is the build directory
# arg2 the input binary file
# arg3 output binary anme
projectdir="${1}"
infile="${2}"
outfile="${3}"
binary_type="${4}"
entry_point="${5}"

echo ${projectdir}
SOURCE="Projects"
slash=""

if echo "${projectdir}" | grep -q '/'; then
  slash="/"
elif echo "${projectdir}" | grep -q '\\'; then
  slash="\\"
else
  echo "Invalid Project Directory"
  exit 1
fi

count=$(echo "${projectdir}" | tr "${slash}" '\n' | grep -n "${SOURCE}" | cut -d':' -f1)
firmwaredir=$(echo "${projectdir}" | cut -d"${slash}" -f1-${count})
echo "Projects Directory: ${firmwaredir}"
basedir="${firmwaredir}/../STM32CubeMP13/Utilities/ImageHeader"

case "$(uname -s)" in
   Linux)
      #line for python
      echo Postbuild with python script
      imgtool="${basedir}/Python3/Stm32ImageAddHeader.py"
      cmd="python"
      ;;
   *)
      #line for window executeable
      echo Postbuild with windows executable
      imgtool="${basedir}/exe.win-amd64-2.7/Stm32ImageAddHeader.exe"
      cmd=""
      ;;
esac

message="${infile}'s signature done. Output file: ${outfile}"
command="${cmd} ${imgtool} ${infile} ${outfile} ${binary_type} ${entry_point}"

${command}  > "${projectdir}/output_log.txt"
ret=$?

if [ ${ret} == 0 ]; then
echo ${message}
else
echo "postbuild.sh failed"
fi
