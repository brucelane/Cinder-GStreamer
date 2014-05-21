
#include "cinder/app/AppNative.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"


using namespace ci;
using namespace ci::app;
using namespace std;


class _TBOX_PREFIX_App : public AppNative {
public:
	void setup();
	void update();
	void draw();
    
private:


};

void _TBOX_PREFIX_App::setup()
{
 
}

void _TBOX_PREFIX_App::update()
{

}

void _TBOX_PREFIX_App::draw()
{
    gl::clear( Color( 0.39f, 0.025f, 0.0f ) ); 
    gl::disableAlphaBlending();

}

// This line tells Cinder to actually create the application
CINDER_APP_NATIVE( _TBOX_PREFIX_App, RendererGl )
