# !/bin/sh

kbshell_cmds=""
newline=$'\n'
kbshell_enums=""
cnt=0

while read line; do
    col1=`echo $line | cut -d "," -f1`
    col2=`echo $line | cut -d "," -f1 | tr '[:lower:]' '[:upper:]' | sed -e "s/ /_/g" -e "s/<//g"  -e "s/>//g" -e "s/\"//g" -e "s/^/KBSHELL_CLI_/g" -e "s/-/_/g"`
    col3=`echo $line | cut -d "," -f2 `
    kbshell_cmds="$kbshell_cmds    { $col1 , $col2 , $col3 },${newline}"
    kbshell_enums="$kbshell_enums    $col2,${newline}"
    cnt=$(( $cnt + 1))
done < "kbshell.cmd"

echo "//               kbshell_cmds.c" > kbshell_cmds.c
echo "//        Command entries for kbshell" >> kbshell_cmds.c
echo "//               Author: Kiran Kotla" >> kbshell_cmds.c
echo "// Auto generated file, Please do not edit" >> kbshell_cmds.c
echo "// Copyright 2017 MIT License${newline}" >> kbshell_cmds.c
echo "#include \"kbshell_cli.h\"${newline}" >> kbshell_cmds.c
echo "kbshell_cmd_t kbshell_cmds[] =${newline}{${newline}$kbshell_cmds};" >> kbshell_cmds.c
echo "//               kbshell_cmds.h" > include/kbshell_cmds.h
echo "//        Command enums for KB shell" >> include/kbshell_cmds.h
echo "//               Author: Kiran Kotla" >> include/kbshell_cmds.h
echo "// Auto generated file, Please do not edit" >> include/kbshell_cmds.h
echo "// Copyright 2017 MIT License${newline}" >> include/kbshell_cmds.h
echo "#define KBSHELL_NUM_CMDS $cnt${newline}" >> include/kbshell_cmds.h
echo "typedef enum${newline}{${newline}$kbshell_enums}kbshell_cli_type_t;" >> include/kbshell_cmds.h
