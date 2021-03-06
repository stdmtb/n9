#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "Exporter.h"
#include "mtUtil.h"

//#define RENDER

using namespace ci;
using namespace ci::app;
using namespace std;

class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();

    void keyDown( KeyEvent event );
    void mouseMove( MouseEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void resize();
    
    Exporter mExp;
    const int mW = 4320; // 1080*4
    const int mH = 1920;
    const float mScale = 0.5;
};

void cApp::setup(){
    setWindowPos( 0, 0 );
    setWindowSize( mW*mScale, mH*mScale );
    mExp.setup( mW*mScale, mH*mScale, 2999, GL_RGB, mt::getRenderPath(), 0, true);
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){
    
    mt::timer_start();
    
    float res = 0;
    for( int i=0; i<100; i++){
        for( int j=0; j<100; j++){
            for( int k=0; k<100; k++){
                res += 1;
            }
        }
    }
    
    mt::timer_end();
}

void cApp::draw(){

    mExp.beginOrtho();
    {
        gl::clear( Colorf(1,1,1) );
        gl::color( Colorf(0,0,1) );
        gl::drawSolidRect( Rectf(100,100,300,300) );
    }
    mExp.end();
    
    mExp.draw();
}

void cApp::keyDown( KeyEvent event ){
}

void cApp::mouseDown( MouseEvent event ){
}

void cApp::mouseMove( MouseEvent event ){
}

void cApp::mouseDrag( MouseEvent event ){
}

void cApp::resize(){
}

CINDER_APP_NATIVE( cApp, RendererGl(0) )
