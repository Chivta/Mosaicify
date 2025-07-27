# Mosaicify
A tool that generates a mosaic image by replacing each region of a source image with the closest-matching image from a catalog based on average color.

This project uses the public-domain [stb libraries](https://github.com/nothings/stb) for image loading, resizing, and saving: `stb_image.h` for reading image files, `stb_image_resize2.h` for resizing images with high-quality filters, and `stb_image_write.h` for writing output images in common formats like PNG and JPEG. These headers are single-file libraries by Sean Barrett and are included directly in the source for simplicity. Attribution is appreciated by the author but not required.
