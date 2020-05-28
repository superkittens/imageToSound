#pragma once

#include "ofMain.h"
#include <vector>
#include "ofxOsc.h"


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    void audioOut(ofSoundBuffer &buffer);
    void mapImageToFrequency(const ofPixels &pixels);
    void createTestFrequency();
    void sendSpectrumToAudioEngine();
    
    void drawCursor();
    
    std::vector<ofVec2f> createHilbertCurve(int m);
    std::vector<ofVec2f> hilbertHelper(int currentM, int m, std::vector<ofVec2f> &A);
    
    
    std::vector<ofVec2f> hc;
    
    constexpr static auto fftSize = 2048;
    std::array<float, fftSize> X;
    
    ofVideoGrabber vidGrabber;
    ofPixels vidPixels;
    ofTexture vidTexture;
    int camWidth;
    int camHeight;
    
    float cursorPos;
    
    constexpr static int oscPort = 13050;
    ofxOscSender sender;
    
    bool isTestSignal = false;
};
