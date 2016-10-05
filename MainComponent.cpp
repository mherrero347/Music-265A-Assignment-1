// Music 256a / CS 476a | fall 2016
// CCRMA, Stanford University
//
// Author: Matt Herrero (mherrero@stanford.edu)
// Description: Simple JUCE sine wave additive synthesizer. Modified
// from Romain Michon's simple sine wave synthesizer.

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "Sine.h"
//this constant defines how many octave partial oscillators are available to the user
const int SINE_WAVE_COUNT = 5;

class MainContentComponent :
    public AudioAppComponent,
    private Slider::Listener,
    private ToggleButton::Listener
{
public:
    MainContentComponent() : gain (0.0), onOff (0), samplingRate(0.0)
    {
        // configuring frequency slider and adding it to the main window
        addAndMakeVisible (frequencySlider);
        frequencySlider.setRange (50.0, 5000.0);
        frequencySlider.setSkewFactorFromMidPoint (500.0);
        frequencySlider.setValue(1000); // will also set the default frequency of the sine osc
        frequencySlider.addListener (this);
        
        // configuring frequency label box and adding it to the main window
        addAndMakeVisible(frequencyLabel);
        frequencyLabel.setText ("Frequency", dontSendNotification);
        frequencyLabel.attachToComponent (&frequencySlider, true);
        
        // configuring gain slider and adding it to the main window
        addAndMakeVisible (gainSlider);
        gainSlider.setRange (0.0, 1.0);
        gainSlider.setValue(0.5); // will alsi set the default gain of the sine osc
        gainSlider.addListener (this);
        
        
        // configuring gain label and adding it to the main window
        addAndMakeVisible(gainLabel);
        gainLabel.setText ("Gain", dontSendNotification);
        gainLabel.attachToComponent (&gainSlider, true);
        
        // configuring toggle buttons for on/off of individual octave partials
        for(int i = 0; i < sine_arr_size; i++){
            addAndMakeVisible(sine_on_off_buttons[i]);
            sine_on_off_buttons[i].addListener(this);
        }
        
        // configuring label for on/off buttons and adding it to the main window
        addAndMakeVisible(onOffLabel);
        onOffLabel.setText ("Select Octaves!", dontSendNotification);
        onOffLabel.attachToComponent (&sine_on_off_buttons[0], true);
        
        setSize (600, 200);
        nChans = 1;
        setAudioChannels (0, nChans); // no inputs, one output
    }
    
    ~MainContentComponent()
    {
        shutdownAudio();
    }
    
    void resized() override
    {
        // placing the UI elements in the main window
        // getWidth has to be used in case the window is resized by the user
        const int sliderLeft = 80;
        const int toggleLeft = 120;
        frequencySlider.setBounds (sliderLeft, 10, getWidth() - sliderLeft - 20, 20);
        gainSlider.setBounds (sliderLeft, 40, getWidth() - sliderLeft - 20, 20);
        for(int i = 0; i < sine_arr_size; i++){
            sine_on_off_buttons[i].setBounds (toggleLeft+(i*40), 60, 40, 40);
        }
        
    }
    
    void sliderValueChanged (Slider* slider) override
    {
        if (samplingRate > 0.0){
            if (slider == &frequencySlider){
                //assign correct frequency for each octave partial
                for (int i = 0; i < sine_arr_size; ++i) {
                    if (i>0) {
                        //each oscillator is 2 times the frequency of the last
                        sine_freq_arr[i] = sine_freq_arr[i-1]*(2);
                        sine_arr[i].setFrequency(sine_freq_arr[i]);
                    } else {
                        //sets frequency of lowest oscillator
                        sine_freq_arr[i] = frequencySlider.getValue();
                        sine_arr[i].setFrequency(sine_freq_arr[i]);
                    }
                }
            }
            else if (slider == &gainSlider){
                gain = gainSlider.getValue();
            }
        }
    }
    
    void buttonClicked (Button* button) override
    {
        // turns audio on or off
        for(int i = 0; i < sine_arr_size; i++) {
            if(button == &sine_on_off_buttons[i] && sine_on_off_buttons[i].getToggleState()){
                if(!any_on) any_on = 1;
                sine_on_off_states[i] = 1;
            }
            else if(button == &sine_on_off_buttons[i] && !sine_on_off_buttons[i].getToggleState()){
                sine_on_off_states[i] = 0;
                if(std::find(std::begin(sine_on_off_states), std::end(sine_on_off_states), 1) ==
                   std::end(sine_on_off_states)) any_on = 0;
            }
        }
    }
    
    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        samplingRate = sampleRate;
        for (int i = 0; i < sine_arr_size; ++i) {
            sine_arr[i].setSamplingRate(sampleRate);
        }
    }
    
    void releaseResources() override
    {
    }
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // getting the audio output buffer to be filled
        float* const buffer = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        
        // computing one block
        for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            if(any_on == 1){
                //declare total sample
                float next_sample = 0;
                int waves_added = 0;
                for (int i = 0; i < sine_arr_size; ++i) {
                    if(sine_on_off_states[i]) {
                        //add sine waves together on total sample
                        next_sample += sine_arr[i].tick();
                        waves_added++;
                    } else {
                        //preserve phase of non-audible waves
                        sine_arr[i].tick();
                    }
                }
                //scale down signal to ensure it's between 1 and -1
                next_sample *= (gain/waves_added);
                buffer[sample] = next_sample;
            } else {
                buffer[sample] = 0.0;
            }
        }
    }
    
    
private:
    // UI Elements
    Slider frequencySlider;
    Slider gainSlider;
    //array of UI buttons toggling each oscillator on and off
    ToggleButton sine_on_off_buttons[SINE_WAVE_COUNT];
    //array tracking the on/off state of each oscillator
    int sine_on_off_states[SINE_WAVE_COUNT];
    
    Label frequencyLabel, gainLabel, onOffLabel;
    
    // the sine wave oscillator array
    Sine sine_arr[SINE_WAVE_COUNT];
    // the array that tracks the frequencie for the sine wave array
    float sine_freq_arr[SINE_WAVE_COUNT];
    
    // Global Variables
    float gain;
    int any_on = 0;
    int sine_arr_size = SINE_WAVE_COUNT;
    int onOff, samplingRate, nChans;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
