#include <fstream>
#include <string>
#define REALCOLORBITMAP 24
#define REALCOLORBITMAP_32 32

#pragma once

const int VERTEX_COUNT = 250, GRASS_SIZE = 100;
std::string high_path = "3.bmp";
std::string obj_path = "ground.obj";
void get_height(float* height, std::string path, const int vex_count);
void get_normal(float* vertices, unsigned int* indices, float* normal);
void create_obj(std::string obj_path);

struct COLORTABLE {
    //3．彩色表/调色板（color table）
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    unsigned char rgbReserved; //保留，总为0
};

class bitmap_image
{
protected:
    unsigned int bfSize;    //大小
    unsigned int biWidth;   //宽度
    int biHeight;           //高度
    unsigned short biBitCount;//位数
    COLORTABLE* pRgbQuad;   //调色板数据
    char* pImageData;       //图像数据
    int RowLine() const;
public:
    int width() const;	//返回图片的宽度
    int height() const; //返回图片的高度
    unsigned int get_pixel(int row, int col) const; //返回指定点的RGB颜色
    int get_grey(int row, int col) const;

    /* 根据需要加入自己的定义 */
    bitmap_image(const char*filename);
    ~bitmap_image();
};