
#include <unistd.h>
#include <cv.h>

#include "Input.h"
#include "KinectDriver.h"
#include "Renderer.h"

using namespace kinecap;


static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;
static CvHaarClassifierCascade* nested_cascade = 0;
int use_nested_cascade = 0;

void detect_and_draw( IplImage* image );

const char* cascade_name =
"./data/haarcascades/haarcascade_frontalface_alt.xml";
/*    "haarcascade_profileface.xml";*/
const char* nested_cascade_name =
"./data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
//    "./data/haarcascades/haarcascade_eye.xml";
double scale = 4;
void detect_and_draw( IplImage* img );


int main(int argc, char **argv)
{
	if (!InitKinect())
		return -1;

	OpenGLRenderer *renderer = new OpenGLRenderer();


	IplImage* rgbImg;
	IplImage* depthImg;
	rgbImg = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);
	depthImg = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);
	IplImage* rgbCopy = cvCreateImage( cvSize(640,480),IPL_DEPTH_8U, 3);
	IplImage* depthCopy = cvCreateImage(cvSize(640,480), IPL_DEPTH_8U, 3);

	IplImage* rgbOut = cvCreateImage( cvSize(640,480),IPL_DEPTH_8U, 1);

	enum TrackingState
	{
		TRACK_HEAD,
		TRACKING
	};
	TrackingState state = TRACK_HEAD;
	while (true)
	{
		ProcessInput();
		if (!SynchronizeDrawingData())
			continue;


		switch (state)
		{
		case TRACK_HEAD:
		{
			cvSetData(rgbImg, GetKinectRgbData(), 640*3);
			cvSetData(depthImg, GetKinectDepthData(), 640*3);
			cvCopy( rgbImg, rgbCopy, 0 );
			//cvCopy( rgbImg, rgbOut, 0 );
			cvCopy( depthImg, depthCopy, 0 );

			if(IsKeyUp(306)) { //left ctrl

				state = TRACKING;
				printf("start tracking!\n");
			}
			const CvRect &selection = ProcessSelection();
			if ((selection.x + selection.y + selection.width + selection.height)!=0) {
				cvRectangle( rgbImg, cvPoint(selection.x, selection.y),
						cvPoint(selection.x+selection.width, selection.y+selection.height),
						cvScalar(255, 0, 0));


			}
			renderer->Render((uint8_t*)rgbCopy->imageData, (uint8_t*)depthImg->imageData);
		}
			break;
		case TRACKING:

			cvSetData(rgbImg, GetKinectRgbData(), 640*3);
			cvSetData(depthImg, GetKinectDepthData(), 640*3);

			printf("compare!\n");

			cvCmp( rgbImg, rgbCopy, rgbOut, CV_CMP_EQ  );
			printf("endcompare!\n");
			fflush(stdout);
			//ProcessSelection();
			//renderer->RenderExtra((uint8_t*)backproject->imageData, 640,480, 1);
			//renderer->RenderExtra((uint8_t*)mask->imageData, 640,480, 1);
			//renderer->RenderExtra((uint8_t*)hue->imageData, 640,480, 1);
			//renderer->RenderExtra((uint8_t*)rgbOut->imageData, 640,480, 1);
			//renderer->RenderExtra((uint8_t*)histimg->imageData, 320,200, 3);
			printf("1!\n");
			renderer->Render((uint8_t*)rgbCopy->imageData, (uint8_t*)depthImg->imageData);

			break;
		}

		if (!renderer->Update())
			return 1;

		sleep(0);
	}
	delete renderer;


	return 0;
}


void detect_and_draw( IplImage* img )
{
    static CvScalar colors[] =
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    IplImage *gray, *small_img;
    int i, j;

    gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
    small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
                         cvRound (img->height/scale)), 8, 1 );

    cvCvtColor( img, gray, CV_BGR2GRAY );
    cvResize( gray, small_img, CV_INTER_LINEAR );
    cvEqualizeHist( small_img, small_img );
    cvClearMemStorage( storage );

    if( cascade )
    {
        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
                                            1.1, 2, 0
                                            //|CV_HAAR_FIND_BIGGEST_OBJECT
                                            //|CV_HAAR_DO_ROUGH_SEARCH
                                            |CV_HAAR_DO_CANNY_PRUNING
                                            //|CV_HAAR_SCALE_IMAGE
                                            ,
                                            cvSize(30, 30) );
        t = (double)cvGetTickCount() - t;
        printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvMat small_img_roi;
            CvSeq* nested_objects;
            CvPoint center;
            CvScalar color = colors[i%8];
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            cvCircle( img, center, radius, color, 3, 8, 0 );
            if( !nested_cascade )
                continue;
            cvGetSubRect( small_img, &small_img_roi, *r );
            nested_objects = cvHaarDetectObjects( &small_img_roi, nested_cascade, storage,
                                        1.1, 2, 0
                                        //|CV_HAAR_FIND_BIGGEST_OBJECT
                                        //|CV_HAAR_DO_ROUGH_SEARCH
                                        //|CV_HAAR_DO_CANNY_PRUNING
                                        //|CV_HAAR_SCALE_IMAGE
                                        ,
                                        cvSize(0, 0) );
            for( j = 0; j < (nested_objects ? nested_objects->total : 0); j++ )
            {
                CvRect* nr = (CvRect*)cvGetSeqElem( nested_objects, j );
                center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
                center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
                radius = cvRound((nr->width + nr->height)*0.25*scale);
                cvCircle( img, center, radius, color, 3, 8, 0 );
            }
        }
    }

    //cvShowImage( "result", img );
    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );
}
