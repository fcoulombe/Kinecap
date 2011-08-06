#include "Renderer.h"

#include <SDL.h>
#include <SDL_video.h>

#include <SDL_opengl.h>

/* If using GLEW */
//#include <GL/glew.h>
#include <GL/gl.h>


#include <math.h>
#include <highgui.h>
#include <libfreenect.h>

using namespace kinecap;
//using namespace cv;


OpenCVRenderer::OpenCVRenderer()
{
	rgbImg =cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);
	bgrImg = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);
	depthImg = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 1);

	cvNamedWindow("rgb",CV_WINDOW_AUTOSIZE);
	cvNamedWindow("depth",CV_WINDOW_AUTOSIZE);
	//moveWindow("rgb", 0, 0);
	//moveWindow("depth", 640, 0);

}
OpenCVRenderer::~OpenCVRenderer()
{
	cvDestroyWindow("rgb");
	cvDestroyWindow("depth");
	cvReleaseImage(&rgbImg);
	cvReleaseImage(&depthImg);
}
bool OpenCVRenderer::Update()
{
	if( (char)27==cv::waitKey(1) )
		return false;
	return true;

}
void OpenCVRenderer::Render(uint8_t *rgb_front, uint8_t *depth_front)
{
	cvSetData(rgbImg, rgb_front, 640*3);
	cvCvtColor(rgbImg,bgrImg, CV_RGB2BGR );

	cvSetData(depthImg, depth_front, 640*3);
	cvShowImage("rgb", bgrImg);
	cvShowImage("depth", depthImg);

}


#include "KinectDriver.h"
OpenGLRenderer::OpenGLRenderer()
{
	int sdlInitSuccessful = SDL_Init(SDL_INIT_VIDEO);
	assert(sdlInitSuccessful>= 0);

	const SDL_VideoInfo* info = NULL;
	int width = 1280;
	int height = 480;
	int flags = SDL_OPENGL;
	info = SDL_GetVideoInfo( );

	if( !info ) {
		fprintf( stderr, "Video query failed: %s\n",
				SDL_GetError( ) );
		return;

	}
	int bpp = info->vfmt->BitsPerPixel;

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	//SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	if( SDL_SetVideoMode( width, height, bpp, flags ) == 0 ) {
		fprintf( stderr, "Video mode set failed: %s\n",
				SDL_GetError( ) );
		return;

	}


	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_FLAT);
	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &gl_depth_tex);
	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenTextures(1, &gl_rgb_tex);
	glBindTexture(GL_TEXTURE_2D, gl_rgb_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glViewport(0,0,1280,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho (0, 1280, 480, 0, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();



}
OpenGLRenderer::~OpenGLRenderer()
{
	/* Delete our opengl context, destroy our window, and shutdown SDL */
	SDL_Quit();
}

bool OpenGLRenderer::Update()
{

	return true;
}

static bool isRenderingExtra = false;
void OpenGLRenderer::RenderExtra(uint8_t *rgb_front, size_t width, size_t height, size_t depth)
{
	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	if (depth == 1) {
		glTexImage2D(GL_TEXTURE_2D, 0, depth, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, rgb_front);
	}
	else if (depth == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, depth, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_front);
	}
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
	glTexCoord2f(0, 0); glVertex3f(0,0,0);
	glTexCoord2f(1, 0); glVertex3f(640,0,0);
	glTexCoord2f(1, 1); glVertex3f(640,480,0);
	glTexCoord2f(0, 1); glVertex3f(0,480,0);
	glEnd();
	isRenderingExtra = true;

}
void OpenGLRenderer::Render(uint8_t *rgb_front, uint8_t *depth_front)
{

	if (!isRenderingExtra) {
		glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, depth_front);

		glBegin(GL_TRIANGLE_FAN);
		glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
		glTexCoord2f(0, 0); glVertex3f(0,0,0);
		glTexCoord2f(1, 0); glVertex3f(640,0,0);
		glTexCoord2f(1, 1); glVertex3f(640,480,0);
		glTexCoord2f(0, 1); glVertex3f(0,480,0);
		glEnd();
	} else {
		isRenderingExtra = false;
	}


	glBindTexture(GL_TEXTURE_2D, gl_rgb_tex);
	if (GetKinectVideoFormat() == FREENECT_VIDEO_RGB || GetKinectVideoFormat() == FREENECT_VIDEO_YUV_RGB)
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_front);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, 1, 640, 480, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, rgb_front+640*4);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
	glTexCoord2f(0, 0); glVertex3f(640,0,0);
	glTexCoord2f(1, 0); glVertex3f(1280,0,0);
	glTexCoord2f(1, 1); glVertex3f(1280,480,0);
	glTexCoord2f(0, 1); glVertex3f(640,480,0);
	glEnd();


	SDL_GL_SwapBuffers();

}



