#include <iostream>
#include <Windows.h>
#include <conio.h>
#include "ground_generate.h"
using namespace std;

bitmap_image::bitmap_image(const char *filename)
{
	bfSize = biWidth =biHeight = biBitCount =0;
	pImageData = NULL;
	pRgbQuad = NULL;
	ifstream in;
	
	in.open(filename, ios::in | ios::binary);
	if (!in.is_open()) {
		cout << "文件打开失败" << endl;
		return;
	}
	char data[54];
	in.read(data,54);
	bfSize = *(unsigned int*)(data + 2);
	biWidth = *(unsigned int*)(data + 18);
	biHeight = *(int*)(data + 22);
	biBitCount = *(unsigned short*)(data + 28);
	std::cout << biBitCount << std::endl;
	if (biBitCount != REALCOLORBITMAP && biBitCount != REALCOLORBITMAP_32) {
		int ColorCount =(int)pow(2, biBitCount);	
		//std::cout << ColorCount << std::endl;
		pRgbQuad = new COLORTABLE[ColorCount];
		//std::cout << "error";
		in.read((char*)pRgbQuad, ColorCount*4);
	}	
	pImageData = new char[bfSize];
	in.read(pImageData, bfSize);
	
	in.close();
}
unsigned int bitmap_image::get_pixel(int row, int col) const
{
	if (biBitCount == 32) {
		int blue = pImageData[(biHeight - 1 - row) * RowLine() + 4 * col];
		int green = pImageData[(biHeight - 1 - row) * RowLine() + 4 * col + 1];
		int red = pImageData[(biHeight - 1 - row) * RowLine() + 4 * col + 2];
		return RGB(red, green, blue);
	}
	else if (biBitCount == REALCOLORBITMAP) {
		int blue = pImageData[(biHeight-1 - row) * RowLine() + 3*col];
		int green = pImageData[(biHeight-1 - row) * RowLine() + 3*col+1];
		int red = pImageData[(biHeight-1 - row) * RowLine() + 3*col+2];
		return RGB(red, green, blue);
	}
	else {
		int pos=0;
		unsigned char c;
		if (biBitCount == 1) {
			c = pImageData[(biHeight - 1 - row) * RowLine() + col / 8];
			pos = (c >> (7-col % 8)) & 1;
		}
		else if (biBitCount == 4) {
			c = pImageData[(biHeight - 1 - row) * RowLine() + col / 2];
			pos = (c >> (1-col % 2)*4) & 0x0F;
		}
		else if (biBitCount == 8) {
			c = pImageData[(biHeight - 1 - row) * RowLine() + col];
			pos = int(c);
		}
		return RGB(pRgbQuad[pos].rgbRed,pRgbQuad[pos].rgbGreen,pRgbQuad[pos].rgbBlue);
	}
}
int bitmap_image::get_grey(int row, int col) const
{
	unsigned char result;
	if (biBitCount == REALCOLORBITMAP_32) {
		result = pImageData[(biHeight - 1 - row) * RowLine() + 4 * col];
		return result;
	}
	else if (biBitCount == REALCOLORBITMAP) {
		result = pImageData[(biHeight - 1 - row) * RowLine() + 3 * col];
		return result;
	}
	else {
		int pos = 0;
		unsigned char c;
		if (biBitCount == 1) {
			c = pImageData[(biHeight - 1 - row) * RowLine() + col / 8];
			pos = (c >> (7 - col % 8)) & 1;
		}
		else if (biBitCount == 4) {
			c = pImageData[(biHeight - 1 - row) * RowLine() + col / 2];
			pos = (c >> (1 - col % 2) * 4) & 0x0F;
		}
		else if (biBitCount == 8) {
			c = pImageData[(biHeight - 1 - row) * RowLine() + col];
			pos = int(c);
		}
		return pRgbQuad[pos].rgbRed;
	}
}
bitmap_image::~bitmap_image() 
{
	if (pRgbQuad)
		delete[]pRgbQuad;
	if (pImageData)
		delete[]pImageData;
}
int bitmap_image::RowLine() const
{
	return (biBitCount * biWidth + 31) / 32 * 4;
}
int bitmap_image::width() const
{
	return biWidth;
}
int bitmap_image::height() const
{
	return biHeight;
}

void get_height(float* height, string path, const int vex_count) {
	bitmap_image bmp(path.c_str());
	int pointer = 0;
	int x, y;

	for (int i = 0; i < vex_count; i++)
	{
		for (int j = 0; j < vex_count; j++)
		{
			x = i * bmp.height() / vex_count;
			y = j * bmp.width() / vex_count;
			height[pointer++] = (float)bmp.get_grey(x, y) / 10;
		}
	}
}

void get_normal(float* vertices, unsigned int* indices,float* normal) {
	//遍历索引三角
	for (int i = 0; i < (VERTEX_COUNT - 1) * (VERTEX_COUNT - 1) * 2; i++)
	{
		unsigned int pIndex1 = indices[i * 3];
		unsigned int pIndex2 = indices[i * 3 + 1];
		unsigned int pIndex3 = indices[i * 3 + 2];
		float x1 = vertices[pIndex1 * 3];
		float y1 = vertices[pIndex1 * 3 + 1];
		float z1 = vertices[pIndex1 * 3 + 2];
		float x2 = vertices[pIndex2 * 3];
		float y2 = vertices[pIndex2 * 3 + 1];
		float z2 = vertices[pIndex2 * 3 + 2];
		float x3 = vertices[pIndex3 * 3];
		float y3 = vertices[pIndex3 * 3 + 1];
		float z3 = vertices[pIndex3 * 3 + 2];
		//求边
		float vx1 = x2 - x1;
		float vy1 = y2 - y1;
		float vz1 = z2 - z1;
		float vx2 = x3 - x1;
		float vy2 = y3 - y1;
		float vz2 = z3 - z1;
		//叉乘求三角形法线
		float xN = vy1 * vz2 - vz1 * vy2;
		float yN = vz1 * vx2 - vx1 * vz2;
		float zN = vx1 * vy2 - vy1 * vx2;
		float Length = sqrtf(xN * xN + yN * yN + zN * zN);
		xN /= Length;
		yN /= Length;
		zN /= Length;
		//顶点法线更新
		normal[pIndex1 * 3 + 0] += xN;
		normal[pIndex1 * 3 + 1] += yN;
		normal[pIndex1 * 3 + 2] += zN;
		normal[pIndex2 * 3 + 0] += xN;
		normal[pIndex2 * 3 + 1] += yN;
		normal[pIndex2 * 3 + 2] += zN;
		normal[pIndex3 * 3 + 0] += xN;
		normal[pIndex3 * 3 + 1] += yN;
		normal[pIndex3 * 3 + 2] += zN;
	}
}

void create_obj(string obj_path) {
	ofstream of;
	of.open(obj_path.c_str(), ios::out);
	if (!of.is_open()) {
		std::cout << "打开文件失败" << std::endl;
		return;
	}
	of << "mtllib Ground.mtl" << endl;
	of << "g default" << endl;
 
	int count = VERTEX_COUNT * VERTEX_COUNT;
	float* height = new float[count];
	float* pos = new float[count * 3];
	float* normal = new float[count * 3];
	unsigned int* indices = new unsigned int[6 * (VERTEX_COUNT - 1) * (VERTEX_COUNT - 1)];

	get_height(height, high_path, VERTEX_COUNT);

	int pointer = 0;
	for (int i = 0; i < VERTEX_COUNT; i++) {
		for (int j = 0; j < VERTEX_COUNT; j++) {
			// 生成顶点数据
			pos[pointer * 3] = (float)j / ((float)VERTEX_COUNT - 1) * GRASS_SIZE - GRASS_SIZE / 2;
			pos[pointer * 3 + 1] = (float)height[pointer];
			pos[pointer * 3 + 2] = (float)i / ((float)VERTEX_COUNT - 1) * GRASS_SIZE - GRASS_SIZE / 2;
			of << "v ";
			of << pos[pointer * 3] << " ";
			of << pos[pointer * 3 + 1] << " ";
			of << pos[pointer * 3 + 2] << endl;

			normal[pointer * 3] = 0;
			normal[pointer * 3 + 1] = 0;
			normal[pointer * 3 + 2] = 0;
			
			pointer++;
		}
	}

	for (int i = 0; i < VERTEX_COUNT; i++) {
		for (int j = 0; j < VERTEX_COUNT; j++) {
			// 生成纹理数据
			of << "vt ";
			of << (float)j / 10 << " ";
			of << (float)i / 10 << " " << endl;
		}
	}

	pointer = 0;
	for (int gz = 0; gz < VERTEX_COUNT - 1; gz++) {
		for (int gx = 0; gx < VERTEX_COUNT - 1; gx++) {
			int topLeft = (gz * VERTEX_COUNT) + gx;
			int topRight = topLeft + 1;
			int bottomLeft = ((gz + 1) * VERTEX_COUNT) + gx;
			int bottomRight = bottomLeft + 1;
			indices[pointer++] = topLeft;
			indices[pointer++] = bottomLeft;
			indices[pointer++] = topRight;

			indices[pointer++] = topRight;
			indices[pointer++] = bottomLeft;
			indices[pointer++] = bottomRight;
		}
	}

	get_normal(pos, indices, normal);

	pointer = 0;
	for (int i = 0; i < VERTEX_COUNT; i++) {
		for (int j = 0; j < VERTEX_COUNT; j++) {
			// 生成法线数据
			of << "vn ";
			of << normal[pointer * 3] << " ";
			of << normal[pointer * 3 + 1] << " ";
			of << normal[pointer * 3 + 2] << endl;
			pointer++;
		}
	}

	of << "g Ground" << endl << "usemtl Ground" << endl;

	for (int i = 0; i < (VERTEX_COUNT - 1) * (VERTEX_COUNT - 1) * 2; i++) {
		of << "f ";
		of << indices[i * 3] + 1 << "/" << indices[i * 3] + 1 << "/" << indices[i * 3] + 1 << " ";
		of << indices[i * 3 + 1] + 1 << "/" << indices[i * 3 + 1] + 1 << "/" << indices[i * 3 + 1] + 1 << " ";
		of << indices[i * 3 + 2] + 1 << "/" << indices[i * 3 + 2] + 1 << "/" << indices[i * 3 + 2] + 1 << endl;
	}

	of.close();
}

int main()
{
	create_obj(obj_path);

	return 0;
}