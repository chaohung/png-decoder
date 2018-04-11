#include "png_decoder.hpp"

#include <png.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace hsu {

typedef struct png_buf {
    uint8_t const* data;
    size_t offset;
} png_buf;

static void user_read_data(png_struct* png_ptr, png_byte* data, png_size_t length) {
    png_buf* read_io_ptr = (png_buf*)png_get_io_ptr(png_ptr);
    memcpy(data, read_io_ptr->data + read_io_ptr->offset, length);
    read_io_ptr->offset += length;
}

static void decode_png(png_struct* png_ptr, png_info* info_ptr, png_info* end_info, rgba_img_data& img) {
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);
    printf("width: %d\n", width);
    printf("height: %d\n", height);
    printf("bit_depth: %d\n", bit_depth);
    printf("color_type: %d\n", color_type);
    printf("interlace_type: %d\n", interlace_type);
    printf("compression_type: %d\n", compression_type);
    printf("filter_method: %d\n", filter_method);

    int channels = png_get_channels(png_ptr, info_ptr);
    printf("channels: %d\n", channels);
    printf("\n");

    // transform
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY) {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
        png_set_gray_1_2_4_to_8(png_ptr);
#endif
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (bit_depth < 8)
        png_set_packing(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    png_read_update_info(png_ptr, info_ptr);

    // allocate buf
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    img.buf.resize(rowbytes * height);
    png_byte** row_pointers = (png_byte**)malloc(sizeof(png_byte*) * height);
    for (int i = 0; i < height; i++) {
        row_pointers[i] = &img.buf[rowbytes * i];
    }
    img.width = png_get_image_width(png_ptr, info_ptr);
    img.height = png_get_image_height(png_ptr, info_ptr);

    // decode png image
    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    free(row_pointers);
}

rgba_img_data read_png(uint8_t const* data, size_t size) {
    rgba_img_data img;

    if (png_sig_cmp(data, 0, 8)) {
        throw std::runtime_error("png_sig_cmp failed.");
    }

    png_struct* png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        throw std::runtime_error("png_create_read_struct failed.");
    }

    png_info* info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        throw std::runtime_error("png_create_read_struct failed.");
    }

    png_info* end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        throw std::runtime_error("png_create_info_struct failed.");
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        throw std::runtime_error("png decode failed.");
    }

    png_buf pbuf = {data, 0};
    png_set_read_fn(png_ptr, &pbuf, user_read_data);

    decode_png(png_ptr, info_ptr, end_info, img);

    return img;
}

rgba_img_data read_png(char const* path) {
    rgba_img_data img;
    char header[8];

    FILE* fp = fopen(path, "rb");
    fread(header, 1, 8, fp);
    if (png_sig_cmp((png_const_bytep)header, 0, 8)) {
        fclose(fp);
        throw std::runtime_error("png_sig_cmp failed.");
    }

    png_struct* png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        throw std::runtime_error("png_create_read_struct failed.");
    }

    png_info* info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        throw std::runtime_error("png_create_read_struct failed.");
    }

    png_info* end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        throw std::runtime_error("png_create_info_struct failed.");
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        throw std::runtime_error("png decode failed.");
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    decode_png(png_ptr, info_ptr, end_info, img);

    return img;
}

} // end of namespace hsu
