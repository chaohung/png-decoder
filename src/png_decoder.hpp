#ifndef HSU_PNG_DECODER_HPP
#define HSU_PNG_DECODER_HPP

#include <vector>

namespace hsu {

typedef struct rgba_img_data {
    int width;
    int height;
    std::vector<uint8_t> buf;
} rgba_img_data;

rgba_img_data read_png(uint8_t const* data, size_t size);
rgba_img_data read_png(char const* path);

} // end of namespace hsu

#endif
