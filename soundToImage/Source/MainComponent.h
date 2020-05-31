/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent,
                        private Thread,
                        public Timer
{
public:
    
    class ReferenceCountedBuffer : public ReferenceCountedObject
    {
    public:
        typedef ReferenceCountedObjectPtr<ReferenceCountedBuffer> Ptr;
        
        ReferenceCountedBuffer(const String &nameToUse, int numChannels, int numSamples) :  name(nameToUse),
                                                                                            buffer(numChannels, numSamples)
        {
            DBG(String("Buffer named: ") + name + String("Num Channels: ") + String(numChannels) + String("Num Samples") + String(numSamples));
        }
        
        ~ReferenceCountedBuffer()
        {
            DBG(String("Buffer named: ") + name + String(" destroyed"));
        }
        
        AudioSampleBuffer *getAudioSampleBuffer()
        {
            return &buffer;
        }
        
        int position = 0;
        
    private:
        String name;
        AudioSampleBuffer buffer;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferenceCountedBuffer)
    };
    
    
    class mVec2f
    {
    public:
        float x, y;
        mVec2f(float x, float y){ this->x = x; this->y = y;}
    };
    
    
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
    
    void run() override;
    

private:
    //==============================================================================
    void checkForBuffersToFree();
    void checkForPathToOpen();
    void timerCallback() override;
    
    void openButtonClicked();
    void clearButtonClicked();
    
    void getSpectrum();
    
    std::vector<mVec2f> createHilbertCurve(size_t m);
    std::vector<mVec2f> hilbertCurveHelper(size_t currentM, size_t m, std::vector<mVec2f> &A);
    
    AudioFormatManager formatManager;
    String chosenPath;
    
    TextButton openButton;
    TextButton clearButton;
    
    ReferenceCountedArray<ReferenceCountedBuffer> buffers;
    ReferenceCountedBuffer::Ptr currentBuffer;
    
    Image img;
    std::vector<mVec2f> hc;
    constexpr static int imgWidth = 256;
    constexpr static int imgHeight = 256;
    
    dsp::FFT fft;
    dsp::WindowingFunction<float> w;
    constexpr static auto fftOrder = 16;
    constexpr static auto fftSize = 1 << fftOrder;
    std::array<float, fftSize * 2> x_left, x_right;
    std::array<float, fftSize * 2> X_left, X_right;
    bool okToMeasureSpectrum = true;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
