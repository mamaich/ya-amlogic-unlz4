# ya-amlogic-unlz4
Распаковка u-boot сжатого LZ4  

Компиляция:  
gcc unpack_lz4c.c -o unpack_lz4c -llz4   
Требуется библиотека LZ4 (liblz4-dev)

Использование:  
./unpack_lz4c ./bl33.lz4 u-boot.bin    