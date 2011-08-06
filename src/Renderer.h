#pragma once
//void  cvSetData( void* arr, void* data, int step );
#include <cv.h>


namespace kinecap
{
class Renderer
{
public:
	virtual bool Update() =0;
	virtual void Render(uint8_t *rgb_front, uint8_t *depth_front) =0;
};
class OpenCVRenderer : public Renderer
{
public:
	OpenCVRenderer();
	~OpenCVRenderer();
	bool Update();
	void Render(uint8_t *rgb_front, uint8_t *depth_front);
	IplImage* rgbImg;
	IplImage* bgrImg ;
	IplImage* depthImg;
};

class OpenGLRenderer : public Renderer
{
public:
	OpenGLRenderer();
	~OpenGLRenderer();
	bool Update();
	void Render(uint8_t *rgb_front, uint8_t *depth_front);
	void RenderExtra(uint8_t *rgb_front, size_t width, size_t height, size_t depth);

	uint32_t gl_depth_tex;
	uint32_t gl_rgb_tex;
};
}
