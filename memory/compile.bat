@ECHO OFF
PATH=I:\Apps\Programmer's Utilities\Inform 6.21;%PATH%
infrmw32 memory.inf memory.z5 $MAX_STATIC_DATA=655360 $MAX_EXPRESSION_NODES=512 $MAX_ZCODE_SIZE=320000 -~S
rem -a -j -n -o -s -z
