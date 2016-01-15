//#define RENDER

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"
#include "cinder/MayaCamUI.h"

#include "tbb/tbb.h"

#include "Exporter.h"
#include "mtUtil.h"
#include "VboSet.h"
#include "RfExporterBin.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace tbb;

class Particle{

public:
    
    Vec3f pos;
    Vec3f vel;
    Vec3f force;

    float mass;
    int id;
    int life;
    
    bool operator<( const Particle & p ) const {
        return life < p.life;
    }

    bool operator>( const Particle & p ) const {
        return life > p.life;
    }

};


class cApp : public AppNative {
  public:
	void setup();
    void update();
    void draw();
    void keyDown( KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void add_particle();
    void update_particle();
    void update_vbo();
    
    bool bOrtho = true;
    bool bTbb   = true;
    const int mW = 1080*4;
    const int mH = 1920;
    int ptclCount = 0;
    float dispScale = 0.55;
    float frame    = 0;
    fs::path assetDir = mt::getAssetPath();

    vector<Perlin> mPln;
    vector<Vec2i> absPos = { Vec2i(1640,192), Vec2i(2680,192), Vec2i(1640,1728), Vec2i(2680,1728) };

    vector<vector<Particle>> ptcls;
    vector<VboSet> vbos;
    Exporter mExp;

    vector<vector<Colorf>> picker;
    
};

void cApp::setup(){

    setWindowSize( mW*dispScale, mH*dispScale );
    printf( "win : %0.2f, %0.2f\n", mW*dispScale, mH*dispScale );

    setWindowPos( 0, 0 );
    mExp.setup( mW, mH, 0, 1000-1, GL_RGB, mt::getRenderPath(), 0);
    

    for( int i=0; i<4; i++){
        mPln.push_back( Perlin(4, mt::getSeed() + 111*i) );
        ptcls.push_back( vector<Particle>() );
        vbos.push_back( VboSet() );
    }
    
    mt::loadColorSample( "img/colorSample/n5pickCol_0.png", picker );
    
#ifdef RENDER
    mExp.startRender();
#endif
}

void cApp::update(){

    add_particle();
    update_particle();
    update_vbo();
}

void cApp::add_particle(){

    int np = -1;
    for( auto & ptcl : ptcls ){
        
        np++;
        int nAdd = 3000;
        for( int i=0; i<nAdd; i++){
            
            Particle pt;
            pt.life = randInt(100, 200);
            pt.id = ptclCount++;
            pt.mass = randFloat(0.5, 1.1);
            pt.pos.set( absPos[0].x, absPos[0].y, 0);
            
            //        Vec3f v( randFloat(-1,1),randFloat(-1,1),randFloat(-1,1) );
            float noiseScale = 0.002;
            Vec3f v = mPln[np].dfBm( pt.id*noiseScale, i+noiseScale, 10+pt.mass+0.02 );
            pt.vel = v*30;
            
            
            ptcl.push_back( pt );
        }
        
    }
}

void cApp::update_particle(){
    
    int np = -1;
    for( auto & ptcl : ptcls ){
        
        np++;
        int loadNum = ptcl.size();
        int substep = 1;
        
        int imgw = picker.size();
        int imgh = picker[0].size();
        
        for( int j=0; j<substep; j++){
            parallel_for( blocked_range<size_t>(0, loadNum), [&](const blocked_range<size_t>& r) {
                for( size_t i=r.begin(); i!=r.end(); ++i ){
                    Particle & pt = ptcl[i];
                    Vec3f & p = pt.pos;
                    Vec3f & v = pt.vel;
                    Vec3f & f = pt.force;
                    
                    Vec3f dir_n = p.safeNormalized();
                    dir_n.z = 0;
                    pt.life--;
                    
                    if( 0 ){
                        
                        int idx = (int)abs(p.x)%imgw;
                        int idy = (int)abs(p.y)%imgh;
                        Colorf & c = picker[idx][idy];
                        
                        Vec3f n = mPln[np].dfBm( c.r*0.001, c.g*0.001, c.b*0.001 + frame*0.001);
                        n *= 2.0;
                        f = n;
                        v = v*0.5 + f*0.5;
                        p += v;
                        p.z = 0;
                    }else{
                        
                        //
                        // Perlin base
                        //
                        Vec2f p2d(p.x, p.z);
                        float dist = (p2d-absPos[0]).length();
                        //float noiseScale = lmap(dist, 0.0f, 4320.0f*0.1f, 0.02f, 0.002f);
                        
                        float noiseScale = 0.002 - dist*0.00001;
                        
                        float amp = 15;
                        
                        float ox = 10;
                        float oy = 10;
                        float oz = 10;
                        
                        Vec3f n = mPln[np].dfBm( ox+p.x*noiseScale, oy+p.y*noiseScale, oz+frame*(0.003 - dist*0.000001) );
                        Vec2f n2d(n.x, n.y);
                        if( n2d.length()<0.4)
                            n = n.safeNormalized()*0.4;
                        
//                        n.x += randFloat(-1.0f,1.0f) * 0.1;
//                        n.y += randFloat(-1.0f,1.0f) * 0.1;
//                        n.z += randFloat(-1.0f,1.0f) * 0.1;
                        
                        f = f*0.5 + n*amp * 0.5;
                        v = v*0.5 + f*0.5;
                        
                        float vlen = v.length();
                        vlen = MAX(vlen, 1.0);
                        
                        v = (v.normalized()+dir_n).normalized() * vlen;
                        
                        p += v;
                        p.z = 0;
                        
                    }
                }
            });
        }
    }
}

void cApp::update_vbo(){

    int nv = -1;
    for( auto & vbo : vbos ){
        nv++;
        vbo.resetVbo();
    
        vector<Particle> & ptcl = ptcls[nv];
        
        for (int i=0; i<ptcl.size(); i++) {
            if( ptcl[i].life > 0){
                vbo.addPos( ptcl[i].pos );
                vbo.addCol( ColorAf(1,0,0,0.8) );
            }
        }
        
        if(vbo.vbo){
            vbo.updateVboPos();
            vbo.updateVboCol();
        }else{
            vbo.init( GL_POINTS );
        }
    }
}

void cApp::draw(){
    
    bOrtho ? mExp.beginOrtho() : mExp.beginPersp(58, 1, 100000); {

        gl::enableAlphaBlending();
        
        glPointSize(1);
        gl::clear( Colorf(0,0,0) );
        gl::pushMatrices();
//        glTranslatef( absPos[0].x, absPos[0].y, 0);
        
        for( auto & vbo : vbos ){
            vbo.draw();
        }

        gl::popMatrices();

    } mExp.end();
    
    mExp.draw();
    
    mt::drawScreenGuide();
    
    gl::color(0, 0, 0);
    gl::drawString( "fps : "+to_string(getAverageFps()) , Vec2i(20,20));
    gl::drawString( "frame : "+to_string(getElapsedFrames()) , Vec2i(20,40));
   // gl::drawString( "nParticle : "+to_string(vbo.getPos().size()) , Vec2i(20,60));
    
    frame++;
}

void cApp::keyDown( KeyEvent event ){
    switch (event.getChar()) {
        case 's':   mExp.snapShot();          break;
        case 'S':   mExp.startRender();       break;
        case 'T':   mExp.stopRender();        break;
        case 'f':   frame+=100;               break;
        case 'o':   bOrtho = !bOrtho;         break;
        case 'p':   bTbb = !bTbb;         break;
    }
}

void cApp::mouseDown( MouseEvent event ){
    cout << event.getPos() << endl;
}

void cApp::mouseDrag( MouseEvent event ){
}


CINDER_APP_NATIVE( cApp, RendererGl(0) )
