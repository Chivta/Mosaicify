#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <ctime>

#define COMP 3

static std::mt19937 rand_n(time(nullptr));

using ColorChannel = unsigned char;

struct __attribute__ ((packed)) Pixel{
    ColorChannel r,g,b;
    bool operator==(const Pixel& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
};

namespace std {
    template<>
    struct hash<Pixel> {
        size_t operator()(const Pixel& p) const noexcept {
            // Simple example combining channels:
            return ((size_t)p.r << 16) | ((size_t)p.g << 8) | (size_t)p.b;
        }
    };
}

// class IImage{
//     public:
//     // virtual void write_pixel(int x, int y, Pixel pixel);
//     virtual Pixel read_pixel(int x, int y) const = 0;
//     virtual Pixel getAvgColor() const = 0;
//     virtual int getHeight() const = 0;
//     virtual int getWidth() const = 0;
//     virtual int getStride() const = 0;
//     virtual void resize(int new_w, int new_h);

//     virtual ~IImage() = default; 
// };

// class OneColorImage : public IImage{
//     Pixel color_{};
//     int width_ {0};
//     int height_ {0};
//     int stride_ {0};

//     public:
//     OneColorImage(Pixel color)
//       : color_(color) {}


//     OneColorImage(int height, int width, Pixel color)
//       : color_(color), width_(width), height_(height), stride_(width_ * sizeof(Pixel)) {}

//     Pixel read_pixel(int x, int y) const override{
//         return color_;
//     }
//     Pixel getAvgColor() const override{
//         return color_;
//     }

//     void resize(int new_w, int new_h) override{
//         width_ = new_w;
//         height_ = new_h;
//     }


//     int getHeight() const override {return height_;}
//     int getWidth() const override {return width_;}
//     int getStride() const override {return stride_;}

// };

class Image
{
    struct Vec2{
        int x, y;
    };

    struct ColorFilter{
        Pixel filter {};
        double strength {};
    };

    int width_ {};
    int height_ {};
    int stride_ {};
    std::vector<Pixel> data_;
    Vec2 orientation {0, 1};

    ColorFilter color_filter_ {};

    bool isOneColor_ = false;
    Pixel color_{};

    void setDataPointer(const Pixel* new_data){
        data_.resize(height_*width_);
        std::memcpy(data_.data(),new_data,width_*height_*sizeof(Pixel));
    }

public:
    Image() {}

    Image(std::filesystem::path path){
        load(path);
    }
    
    Image(Pixel color): color_(color), isOneColor_(true) {}

    void set_color_filter(ColorFilter color_filter){
        color_filter_ = color_filter;
    }

    Pixel read_pixel(int x, int y) const {
        if(isOneColor_) { return color_; }
        
        if (orientation.x == 0 && orientation.y == 1) {
            // default, do nothing
        } else if (orientation.x == 1 && orientation.y == 0) {
            int temp = x;
            x = height_ - 1 - y;
            y = temp;
        } else if (orientation.x == 0 && orientation.y == -1) {
            x = width_ - 1 - x;
            y = height_ - 1 - y;
        } else if (orientation.x == -1 && orientation.y == 0) {
            int temp = x;
            x = y;
            y = width_ - 1 - temp;
        } else {
            throw std::logic_error("Invalid orientation");
        }
        
        if(x >= width_ || y >= height_){
            throw std::out_of_range("read_pixel: coordinates out of bounds");
        }
        
        Pixel target_pixel = data_[x + y * width_];

        target_pixel.r = target_pixel.r + (color_filter_.filter.r - target_pixel.r) * color_filter_.strength;
        target_pixel.g = target_pixel.g + (color_filter_.filter.g - target_pixel.g) * color_filter_.strength;
        target_pixel.b = target_pixel.b + (color_filter_.filter.b - target_pixel.b) * color_filter_.strength;
        
        return target_pixel;
    }

    void rotate_90_degrees(int times = 1){
        times = (times % 4 + 4) % 4; 
        while (times--) {
            int new_x = orientation.y;
            int new_y = -orientation.x;
            orientation.x = new_x;
            orientation.y = new_y;
        }
    }

    // void write_pixel(int x, int y, Pixel pixel){
    // if(x >= width_ || y >= height_){
    //     throw std::out_of_range("write_pixel: coordinates out of bounds");
    // }
    // data_[x + y * width_] = pixel;
    // }

    // Image& operator=(const Image& new_image){
    //     if (&new_image == this){
    //         return *this;
    //     }
    //     width_ = new_image.width_;
    //     height_ = new_image.height_;
    //     stride_ = width_ * sizeof(Pixel);

    //     setDataPointer(new_image.data_.data());

    //     return *this;
    // };

    int getWidth() const{ return width_; }

    int getHeight() const { return height_; }

    int getStride() const { return stride_; }

    Pixel getAvgColor() const {
        if(isOneColor_) { return color_; }

        long r = 0, g = 0, b = 0;
        for(int row = 0; row<height_;row++){
            for(int colomn = 0; colomn<width_;colomn++){
                Pixel pixel = data_[colomn + row * width_];
                r+=pixel.r;
                g+=pixel.g;
                b+=pixel.b;
            }
        }
        if(width_ == 0 || height_ == 0){
            throw std::runtime_error("getAvgColor: zero division exception");
        }
        r /= width_*height_;
        g /= width_*height_;;
        b /= width_*height_;;
        
        return {static_cast<ColorChannel>(r),static_cast<ColorChannel>(g),static_cast<ColorChannel>(b)};
    }

    void load(std::filesystem::path file_path){
        int n;
        ColorChannel* data = stbi_load(file_path.c_str(), &width_, &height_, &n, sizeof(Pixel));
        if(data==nullptr){
            std::cout << file_path << '\n'; 
            std::cout << stbi_failure_reason() << '\n'; 
            throw std::bad_alloc();
        }
        stride_ = sizeof(Pixel) * width_;
        setDataPointer(reinterpret_cast<Pixel*>(data));
        stbi_image_free(data);
    }
    
    void resize(int new_w, int new_h) {
        if(isOneColor_) { 
            width_ = new_w;
            height_ = new_h;
            stride_ = sizeof(Pixel) * width_;

            return; 
        }


        ColorChannel* new_data = stbir_resize_uint8_srgb((ColorChannel*)data_.data(), width_, height_, width_ * sizeof(Pixel),
                                nullptr, new_w, new_h, new_w * sizeof(Pixel),
                            STBIR_RGB);
        

        if(new_data==nullptr){
            std::cout << stbi_failure_reason() << '\n'; 
            throw std::bad_alloc();
        }

        width_ = new_w;
        height_ = new_h;
        stride_ = sizeof(Pixel) * width_;

        setDataPointer(reinterpret_cast<Pixel*>(new_data));

        stbi_image_free(new_data);
    }

    void save(std::filesystem::path filename) const {
        std::string ext = filename.extension().string();
        if (ext.empty()){
            throw std::invalid_argument("save: couldn't get file extention");
        }
        if (ext == ".png"){
            stbi_write_png(
                filename.c_str(),
                width_,
                height_,
                sizeof(Pixel),
                (ColorChannel*)data_.data(),
                width_*sizeof(Pixel));
        } 
        else if(ext == ".jpg"){
            stbi_write_jpg(
                filename.c_str(),
                width_,
                height_,
                sizeof(Pixel),
                (ColorChannel*)data_.data(),
                width_*sizeof(Pixel));
        } 
    }
};


class Canvas
{
    std::vector<Pixel> data_;
    int width_{0};
    int height_{0};

    void updateCanvas(){
        data_.resize(sizeof(Pixel)*width_*height_);
    }

    public:
    Canvas(int width, int height)
        : width_(width), height_(height), data_(width * height) {}

    inline int getWidth() const {
        return width_;
    }
    inline int getHeight() const {
        return height_;
    }

    inline int getStride(){
        return width_ * sizeof(Pixel);
    }

    inline ColorChannel* getData(){
        return (ColorChannel*) data_.data();
    }

    void resize(int width, int height){
        width_ = width;
        height_ = height;
        updateCanvas();
    }

    inline void write_pixel(int x, int y, const Pixel& pixel){
        if(x >= width_ || y >= height_){
            throw std::out_of_range("write_pixel: coordinates out of bounds");
        }
        data_[x + y * width_] = pixel;
    }
    
    Pixel read_pixel(int x, int y) const {
        if(x >= width_ || y >= height_){
            throw std::out_of_range("read_pixel: coordinates out of bounds");
        }
        return data_[x + y * width_];
    }

};

void printPixel(Pixel pixel){
    printf("%x%x%x\n",(int)pixel.r,(int)pixel.g,(int)pixel.b);
}

struct FileColor{
    Pixel avgColor;
    std::filesystem::path path;
    bool operator==(const FileColor& fc) const {
        return fc.avgColor.r == avgColor.r &&
        fc.avgColor.g == avgColor.g &&
        fc.avgColor.b == avgColor.b &&
        fc.path == path;
    }
};

class ImageCatalog{
    int block_size_ {16};
    int blocks_per_axis_ {16}; 
    std::vector<std::vector<FileColor>> color_matrix_;
    int index_cap_{0};
    int max_checks_{3}; // max amount of images checked in a block to find best

    void write_to_appropriate_block(FileColor fc){
        auto& block = get_appropriate_block(fc.avgColor);
        if (std::find(block.begin(),block.end(),fc) == block.end()){
            block.push_back(fc);
            images_ctr_++;
        }
    }

    public:
    int images_ctr_{0};

    ImageCatalog(int block_size)
        : block_size_(block_size), blocks_per_axis_(256/block_size), 
        color_matrix_(blocks_per_axis_*blocks_per_axis_*blocks_per_axis_),
        index_cap_(blocks_per_axis_*blocks_per_axis_*blocks_per_axis_) {}

    void save_cache(std::filesystem::path path){
        if(color_matrix_.size()>0){
            std::ofstream ofs;
            
            ofs.open(path, std::ios::out);
            
            
            for(auto& block : color_matrix_){
                for(auto& fc : block){
                    ofs.write(reinterpret_cast<const char*>(&fc.avgColor), sizeof(Pixel));
                    const char* file_path = fc.path.c_str(); 
                    int32_t size = fc.path.string().size();
                    ofs.write(reinterpret_cast<const char*>(&size),sizeof(int32_t));

                    ofs.write(file_path,size);
                }
            }
        }
    }

    void load_cache(std::filesystem::path path){
        if(std::filesystem::exists(path)){
            std::ifstream ifs {path,std::ios::binary};
            int32_t size {0};
            ifs.seekg(0,ifs.beg);
            FileColor fc {};
            while (ifs.peek() != EOF){
                ifs.read(reinterpret_cast<char*>(&fc.avgColor),sizeof(fc.avgColor));
                ifs.read(reinterpret_cast<char*>(&size),sizeof(int32_t));
                std::string path (size,'\0');
                ifs.read(path.data(), size);
                fc.path = std::move(path);
                write_to_appropriate_block(fc);
            }   
        }
        else{
            throw std::invalid_argument("Unexisting file");
        }
    }


    
    void ScanDir(std::filesystem::path directory){
        Image img {};
        for(auto entry : std::filesystem::directory_iterator(directory)){
            try{
                img.load(entry.path());
                Pixel avg = img.getAvgColor();
                write_to_appropriate_block({avg,entry.path()});
            }
            catch (const std::bad_alloc&){
                std::cout<<"Bad image: " << entry.path() << '\n';
                continue;
            }
            catch (...){
                std::cout << "Exception\n"; 
                continue;
            }
        }
    }

    int calculate_squared_distance(const Pixel& color1, const Pixel& color2) const{
        int dr = static_cast<int>(color1.r) - static_cast<int>(color2.r);
        int dg = static_cast<int>(color1.g) - static_cast<int>(color2.g);
        int db = static_cast<int>(color1.b) - static_cast<int>(color2.b);
        return dr * dr + dg * dg + db * db;
    }

    std::vector<FileColor> get_expanded_block(const Pixel& target_pixel) const {
        std::vector<FileColor> res {};

        for (int i = -1; i <= 1; i++){
            for (int j = -1; j <= 1; j++){
                for (int k = -1; k <= 1; k++){

                    int new_r, new_g, new_b;

                    new_r = target_pixel.r - block_size_ * i;
                    new_g = target_pixel.g - block_size_ * i;
                    new_b = target_pixel.b - block_size_ * i;

                    if (new_r >= 0 && new_r < 256 &&
                        new_g >= 0 && new_g < 256 &&
                        new_b >= 0 && new_b < 256){
                        auto& block = get_appropriate_block({
                            static_cast<ColorChannel>(new_r),
                            static_cast<ColorChannel>(new_g),
                            static_cast<ColorChannel>(new_b),
                        });
                        
                        res.insert(res.end(),block.begin(),block.end());
                    }

                }
            }    
        }
        return res;
    }


    Image get_closest_image(const Pixel& target_pixel) const {
        std::vector<FileColor> block = get_appropriate_block(target_pixel);
        int block_size = block.size();

        if(block_size == 0){ 
            // return Image{target_pixel};

            block = get_expanded_block(target_pixel);
            block_size = block.size();
            if (block_size == 0){
                return Image{target_pixel};
            }
            // printPixel(target_pixel);
            // throw std::runtime_error("No images found in the block."); /* TODO: add some handling here */ 
        }

        int cur_smallest_distance = INT_MAX;
        const FileColor* most_fitting_image = nullptr;
        
        int images_to_check = max_checks_;

        if(images_to_check > block_size){
            images_to_check = block_size;
        }

        for(int i = 0; i < images_to_check; i++){
            // getting image at pseudo-random index
            int index = rand_n() % block_size;
            auto& cur_image = block.at(index);

            int square_distance = calculate_squared_distance(cur_image.avgColor,target_pixel);
            if(square_distance < cur_smallest_distance){
                cur_smallest_distance = square_distance;
                most_fitting_image = &cur_image;
            }
        }

        assert(most_fitting_image != nullptr);

        Image res {most_fitting_image->path};
        res.set_color_filter({target_pixel,0.5});
        return res;
    }

    private:
    std::vector<FileColor>& get_appropriate_block(Pixel avgPixel) {
        // each channel can have 256 values, so we divide it by block_size_ to get the index of a block
        int xi = avgPixel.r / block_size_; 
        int yi = avgPixel.g / block_size_;
        int zi = avgPixel.b / block_size_;

        // multiplying by amout of blocks per axis each coordinate to get value from 1d vector 
        int index = xi * blocks_per_axis_ * blocks_per_axis_ + yi * blocks_per_axis_ + zi;
        if(index >= index_cap_){
            throw std::runtime_error("std::vector<FileColor>& get_appropriate_block: index is too big");
        }

        return color_matrix_[index];
    }

    const std::vector<FileColor>& get_appropriate_block(Pixel avgPixel) const {
        // each channel can have 256 values, so we divide it by block_size_ to get the index of a block
        int xi = avgPixel.r / block_size_;
        int yi = avgPixel.g / block_size_;
        int zi = avgPixel.b / block_size_;

        // multiplying by amout of blocks per axis each coordinate to get 3d value from 1d vector 
        int index = xi * blocks_per_axis_ * blocks_per_axis_ + yi * blocks_per_axis_ + zi;
        if(index >= index_cap_){
            throw std::runtime_error("const std::vector<FileColor>& get_appropriate_block: index is too big");
        }

        return color_matrix_[index];
    }
};



class ImageGrid{
    int tile_width_ {0};
    int tile_height_ {0};
    int rounding_coef_{4};
    bool allow_rotation_{true};
    Canvas canvas;
    ImageCatalog image_catalog_;

    void fill_with_black(){
        int height = canvas.getHeight();
        int width = canvas.getWidth();

        for (int row = 0; row < height; row++){
            for (int column = 0; column < width; column++){
                    canvas.write_pixel(column,row,{0,0,0});
                }
            }
    }

    void place_tile(int x_offset, int y_offset, const Image& img){        
        int canvas_width = canvas.getWidth();
        int canvas_height = canvas.getHeight();
        if(x_offset>=canvas_width || y_offset >= canvas_height){
            throw std::invalid_argument("place_tile: offset is bigger or equal to canvas size");
        }
        
        int tile_height = img.getHeight();
        int tile_width = img.getWidth();

        // making sure tile doesn't go out of canvas boundaries
        if(tile_width + x_offset > canvas_width){
            tile_width = canvas_width - x_offset;
        }
        if(tile_height + y_offset > canvas_height){
            tile_height = canvas_height - y_offset;
        }

        for (int y = 0; y < tile_height; y++){
            for (int x = 0; x < tile_width; x++){
                canvas.write_pixel(x_offset + x, y_offset + y,img.read_pixel(x,y));
            }
        }
    }

    public:

        ImageGrid(int width, int height, int tile_width, int tile_height, ImageCatalog image_catalog)
        : canvas(width, height), tile_width_(tile_width), tile_height_(tile_height), image_catalog_(image_catalog) {}
        
        bool getAllowRotation() const { return allow_rotation_; }
        void setAllowRotation(bool val){ allow_rotation_ = val; }

        void set_target_image(const Image& image){
            canvas.resize(image.getWidth(),image.getHeight());
            place_tile(0,0,image);
        }


        Pixel getAvgColorOfTile(int x_offset, int y_offset) const{
            int canvas_width = canvas.getWidth();
            int canvas_height = canvas.getHeight();
            if(x_offset>=canvas_width || y_offset >= canvas_height){
                throw std::invalid_argument("getAvgColorOfTile: offset is bigger or equal to canvas size");
            }

            int tile_width = tile_width_;
            int tile_height = tile_height_;

            // making sure tile doesn't go out of canvas boundaries
            if(tile_width + x_offset > canvas_width){
                tile_width = canvas_width - x_offset;
            }
            if(tile_height + y_offset > canvas_height){
                tile_height = canvas_height - y_offset;
            }

            
            long r = 0, g = 0, b = 0;
            for(int row = 0; row < tile_height; row++){
                for(int colomn = 0; colomn < tile_width; colomn++){
                    Pixel pixel = canvas.read_pixel(x_offset + colomn, y_offset + row);
                    r+=pixel.r;
                    g+=pixel.g;
                    b+=pixel.b;
                }
            }
            if(tile_width_ == 0 || tile_height_ == 0){
                throw std::runtime_error("getAvgColorOfTile: zero division exception");
            }
            r /= tile_width_*tile_height_;
            g /= tile_width_*tile_height_;
            b /= tile_width_*tile_height_;
            
            return {static_cast<ColorChannel>(r),static_cast<ColorChannel>(g),static_cast<ColorChannel>(b)};
        }

        void apply_mosaic(){
            int width = canvas.getWidth();
            int height = canvas.getHeight();
            
            SimilarTiles tiles = gather_similar_tiles();

            for(auto& t : tiles){
                Image img = image_catalog_.get_closest_image(t.first);
                img.resize(tile_width_,tile_height_);
                for(auto& pos : t.second){
                    img.rotate_90_degrees(rand_n() % 4);
                    place_tile(pos.first,pos.second,img);
                }
            }


            // img = image_catalog_.get_closest_image(avg);
            // img.resize(tile_width_,tile_height_);
            // img.rotate_90_degrees(rand_n() % 4);
            // place_tile(x_offset,y_offset,img);
                
        }

        using SimilarTiles = std::unordered_map<Pixel, std::vector<std::pair<int, int>>>;
        
        SimilarTiles gather_similar_tiles(){
            SimilarTiles result;
            int width = canvas.getWidth();
            int height = canvas.getHeight();
            for (int x_offset = 0; x_offset < width; x_offset+=tile_width_){
                for (int y_offset = 0; y_offset < height; y_offset+=tile_height_){
                    Pixel rounded_avg = roundPixel(getAvgColorOfTile(x_offset,y_offset));
                    result[rounded_avg].push_back(std::pair(x_offset,y_offset));
                }
            }

            return result;
        }

        Pixel roundPixel(Pixel pixel){
            return {static_cast<ColorChannel>(pixel.r / rounding_coef_ * rounding_coef_),
                    static_cast<ColorChannel>(pixel.g / rounding_coef_ * rounding_coef_),
                    static_cast<ColorChannel>(pixel.b / rounding_coef_ * rounding_coef_)};
        }

        void save(const std::filesystem::path& filename){
            int width = canvas.getWidth();
            int height = canvas.getHeight();
            int stride = canvas.getStride();
            ColorChannel* data = canvas.getData();
            std::string ext = filename.extension().string();


            int (*saver)(char const *filename, int x, int y, int comp, const void *data, int stride_bytes) = nullptr;

            if(ext == ".png"){
                saver = stbi_write_png;
            }
            else if(ext == ".jpg"){
                saver = stbi_write_jpg;
            }
            else {
                throw std::invalid_argument("Unsupported extention");
            }

            int result = saver(filename.c_str(),width,height,sizeof(Pixel),data,stride);

            if(!result){
                throw std::runtime_error(stbi_failure_reason());
            }
        }
};


int main(){

    Image img {"/home/arvlas/Projects/Mosaicify/hitler.png"};
    std::cout << "initialized image" << '\n';

    std::cout << "initializing catalog\n";
    ImageCatalog catalog(32);

    std::cout<< "loading cache\n";
    catalog.load_cache("/home/arvlas/Projects/Mosaicify/cache");

    // std::cout << "scanning dir" << '\n';
    // catalog.ScanDir("/home/arvlas/Projects/Mosaicify/images/");

    // std::cout<< "saving cache\n";
    // catalog.save_cache("/home/arvlas/Projects/Mosaicify/cache");

    std::cout << "initializing imGrid\n";
    ImageGrid imGrid(256,256,64,64,catalog);

    std::cout << "resizing image" << '\n';
    img.resize(img.getWidth()*10,img.getHeight()*10);
    
    std::cout << "setting target image" << '\n';
    imGrid.set_target_image(img);

    std::cout << "applying mosaic" << '\n';
    imGrid.apply_mosaic();

    std::cout << "saving" << '\n';
    imGrid.save("/home/arvlas/Projects/Mosaicify/OUT.png");

    return 0;
}

