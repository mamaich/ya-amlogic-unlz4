#include <stdio.h>
#include <stdlib.h>
#include <lz4.h>  // Подключи lz4.h из библиотеки LZ4

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    FILE* fp = fopen(input_file, "rb");
    if (!fp) {
        perror("Failed to open input file");
        return 1;
    }

    // Читаем весь файл в память (для простоты; в реальности используй mmap для больших файлов)
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* file_data = malloc(file_size);
    if (!file_data || fread(file_data, 1, file_size, fp) != file_size) {
        perror("Failed to read file");
        fclose(fp);
        free(file_data);
        return 1;
    }
    fclose(fp);

    // Парсим header по твоему описанию (адаптируй, если нужно)
    // Пропускаем magic "LZ4C" (байты 0-3)
    // Флаги/буфер: байты 4-7 (00 00 80 00)
    // unpacked_size: байты 8-11 (little-endian?)
    // packed_size: байты 12-15
    unsigned int unpacked_size = *(unsigned int*)(file_data + 8);  // Предполагаем little-endian
    unsigned int packed_size = *(unsigned int*)(file_data + 12);

    // Compressed data start at 0x80
    char* src = file_data + 0x80;
    int src_size = packed_size;  // Или file_size - 0x80, если header неверный

    // Выделяем буфер для вывода (больше unpacked_size на всякий случай)
    int dst_capacity = unpacked_size * 2;  // Safe margin
    char* dst = malloc(dst_capacity);
    if (!dst) {
        perror("Malloc failed");
        free(file_data);
        return 1;
    }

    // Сначала пробуем full decompress
    int ret = LZ4_decompress_safe(src, dst, src_size, dst_capacity);
    if (ret > 0) {
        printf("Full decompression success: %d bytes\n", ret);
        FILE* out_fp = fopen(output_file, "wb");
        if (out_fp) {
            fwrite(dst, 1, ret, out_fp);
            fclose(out_fp);
            printf("Output written to %s\n", output_file);
        } else {
            perror("Failed to open output file");
        }
    } else {
        printf("Full decompression failed (error %d)\n", ret);
        int error_pos = -ret;  // Позиция ошибки в src

        // Теперь partial на хорошей части (до error_pos)
        int partial_ret = LZ4_decompress_safe_partial(src, dst, error_pos, unpacked_size, dst_capacity);
        if (partial_ret > 0) {
            printf("Partial decompression success: extracted %d bytes\n", partial_ret);
            FILE* out_fp = fopen(output_file, "wb");
            if (out_fp) {
                fwrite(dst, 1, partial_ret, out_fp);
                fclose(out_fp);
                printf("Partial output written to %s\n", output_file);
            } else {
                perror("Failed to open output file");
            }
        } else {
            printf("Partial failed too (error %d)\n", partial_ret);
            // Если и это не сработало, возможно, нужно парсить блоки вручную (см. ниже)
        }
    }

    free(dst);
    free(file_data);
    return 0;
}