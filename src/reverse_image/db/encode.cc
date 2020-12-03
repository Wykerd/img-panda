#include "img-panda/reverse_image/db.hpp"
#include <stdlib.h>

char *imp_ri_mat_to_blob (cv::Mat &src, size_t &len) {
    int type = src.type();
    int channels = src.channels();

    len = MAT_BLOB_HEADER_LEN;
    len += CV_ELEM_SIZE(type) * src.cols * src.rows;
    
    char *ret = (char *)malloc(len);
    if (ret == NULL) return NULL;
    
    size_t cursor = 0;

    // Header
    memcpy(ret + cursor, &src.rows, sizeof(int));
    cursor += sizeof(int);
    memcpy(ret + cursor, &src.cols, sizeof(int));
    cursor += sizeof(int);
    memcpy(ret + cursor, &type, sizeof(int));
    cursor += sizeof(int);
    memcpy(ret + cursor, &channels, sizeof(int)); 
    cursor += sizeof(int);

    if (src.isContinuous()) {
        memcpy(ret + cursor, src.ptr<char>(0), (src.dataend - src.datastart));
    } else {
        int rowsz = CV_ELEM_SIZE(type) * src.cols;
        for (int r = 0; r < src.rows; ++r) {
            memcpy(ret + cursor, src.ptr<char>(r), rowsz);
            cursor += rowsz;
        }
    }
    
    return ret;
};

cv::Mat imp_ri_blob_to_mat (const void* src) {
    int rows, cols, type, channels;

    size_t cursor = 0;

    memcpy(&rows, (const void*)((volatile uint8_t *)src + cursor), sizeof(int));
    cursor += sizeof(int);
    memcpy(&cols, (const void*)((volatile uint8_t *)src + cursor), sizeof(int));
    cursor += sizeof(int);
    memcpy(&type, (const void*)((volatile uint8_t *)src + cursor), sizeof(int));
    cursor += sizeof(int);
    memcpy(&channels, (const void*)((volatile uint8_t *)src + cursor), sizeof(int));
    cursor += sizeof(int);

    cv::Mat mat(rows, cols, type);

    memcpy(mat.data, (const void*)((volatile uint8_t *)src + cursor), CV_ELEM_SIZE(type) * rows * cols);

    return mat;
}
