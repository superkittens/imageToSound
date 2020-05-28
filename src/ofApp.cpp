#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofBackground(54, 54, 54);
    hc = createHilbertCurve(8);
    
    camWidth = 256;
    camHeight = 256;
    
    vidGrabber.listDevices();
    
    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(15);
    vidGrabber.initGrabber(camWidth, camHeight);
    
    vidPixels.allocate(camWidth, camHeight, OF_PIXELS_RGB);
    
    sender.setup("localhost", oscPort);
}

//--------------------------------------------------------------
void ofApp::update(){
    vidGrabber.update();
    if (vidGrabber.isFrameNew())
    {
        ofPixels &p = vidGrabber.getPixels();
        p.resize(camWidth, camHeight);
        p.mirror(false, true);
        
        if (!isTestSignal)
            mapImageToFrequency(p);
        
        else
            createTestFrequency();
        
        sendSpectrumToAudioEngine();
        
        for (size_t i = 0; i < p.size(); ++i)
            vidPixels[i] = p[i];
        
        vidTexture.loadData(vidPixels);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    vidTexture.draw(20, 20);
    
    ofSetColor(245, 58, 135);
    ofDrawLine(0, 700, ofGetWidth(), 700);
    
    for (int i = 0; i < fftSize / 2; ++i)
        ofDrawLine(i, 700 - X[i] * 100, (i + 1), 700 - X[i + 1] * 100);
    
    drawCursor();
    
    ofSetColor(255, 255, 255);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

    if (key == 't')
        isTestSignal = isTestSignal ? false : true;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
    if (x < 0)
        cursorPos = 0;
    
    else if (x >= (fftSize / 2))
        cursorPos = (fftSize / 2) - 1;
    
    else
        cursorPos = x;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}


void ofApp::audioOut(ofSoundBuffer &buffer)
{

}


void ofApp::createTestFrequency()
{
    std::fill(X.begin(), X.end(), 0.0);
    
    if (cursorPos > 0 && cursorPos < fftSize / 2)
    {
        X[cursorPos] = 1;
    }
    
    else
        X[1] = 1;
}


void ofApp::sendSpectrumToAudioEngine()
{
    ofxOscMessage m;
    m.setAddress("/spectrum");
    
    for (int i = 0; i < fftSize / 2; ++i)
        m.addFloatArg(X[i]);
    
    sender.sendMessage(m);
}


void ofApp::drawCursor()
{
    //  Draw cursor on camera image
    ofSetColor(245, 58, 135);
    auto blockSize = hc.size() / (fftSize / 2);
    ofDrawCircle(hc[cursorPos * blockSize - 1].x + 20, hc[cursorPos * blockSize - 1].y + 20, 5);
    
    //  Draw cursor in frequency representation
    ofSetColor(96, 232, 140);
    ofDrawCircle(cursorPos, 700 - X[cursorPos] * 100, 5);
    
    auto freq = (48000. / fftSize) * cursorPos;
    
    ofSetColor(255, 255, 255);
    stringstream posString;
    posString << "x: " << hc[cursorPos * blockSize - 1].x << " y: " << hc[cursorPos * blockSize - 1].y << std::endl;;
    posString << "f: " << freq << " amplitude: " << X[cursorPos] << std::endl;
    ofDrawBitmapString(posString.str(), 300, 10);
}


/*
 *  Take image and map each point on the image to a specific frequency
 *  Point locations are defined by the Hilbert curve.
 *  X[0] is NOT written to as this is the DC component of the frequency spectrum
 *  The "energy" of each frequency bin is a sum of weighted RGB values at each point
 *  Energies are averaged and scaled when all pixels of the image has been processed
 */
void ofApp::mapImageToFrequency(const ofPixels &pixels)
{
    std::fill(X.begin(), X.end(), 0.0);
    
    auto blockSize = (camWidth * camHeight) / (fftSize / 2);
    auto currentBlock = 1;
    auto numPixelsInBlock = 0;
    
    for (int i = 0; i < hc.size() - blockSize; ++i)
    {
        if (numPixelsInBlock > blockSize - 1)
        {
            currentBlock++;
            numPixelsInBlock = 0;
        }
        
        X[currentBlock] += (1.f - (pixels.getColor(hc[i].x, hc[i].y).r / 255.f)) * 0.8;
        X[currentBlock] += (1.f - (pixels.getColor(hc[i].x, hc[i].y).g / 255.f)) * 0.7;
        X[currentBlock] += (1.f - (pixels.getColor(hc[i].x, hc[i].y).b / 255.f)) * 0.4;
        
        numPixelsInBlock++;
    }
    
    float gain = 1.0;
    for (int i = 0; i < X.size(); ++i)
        X[i] = (X[i] / (float)blockSize) * gain;
}


std::vector<ofVec2f> ofApp::hilbertHelper(int currentM, int m, std::vector<ofVec2f> &A)
{
    if (currentM > m) return A;
    
    size_t numPoints = A.size();
    float N = powf(2, currentM);
    float NPrev = powf(2, currentM - 1);
    
    for (int i = 0; i < numPoints; ++i)
        A.push_back(ofVec2f(A[i].x, A[i].y + (N / 2)));
    
    for (int i = 0; i < numPoints; ++i)
        A.push_back(ofVec2f(A[i].x + (N / 2), A[i].y + (N / 2)));
    
    for (int i = 0; i < numPoints; ++i)
    {
        float x = A[i].x;
        float y = A[i].y;
        A.push_back(ofVec2f((N / 2) + (NPrev - 1) - y, (NPrev - 1) - x));
    }
    
    for (int i = 0; i < numPoints; ++i)
    {
        float x = A[i].x;
        float y = A[i].y;
        A[i].x = y;
        A[i].y = x;
    }
    
    hilbertHelper(currentM + 1, m, A);
        
    return A;
}


std::vector<ofVec2f> ofApp::createHilbertCurve(int m)
{
    std::vector<ofVec2f> A;
    if (m < 1) return A;
    
    //  Base case m = 1
    A.push_back(ofVec2f(0, 0));
    A.push_back(ofVec2f(0, 1));
    A.push_back(ofVec2f(1, 1));
    A.push_back(ofVec2f(1, 0));
    
    return hilbertHelper(2, m, A);
}

