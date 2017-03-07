#pragma once
 
#include "mbed.h"
 
Serial pc(USBTX, USBRX);
 
class metronome
{
public:
    enum { beat_samples = 3 };
 
public:
    metronome()
    : m_timing(false), m_beat_count(0), min_bpm(999), max_bpm(0) {}
    ~metronome() {}
 
public:
              // Call when entering "learn" mode
    void start_timing()
    {
              m_beat_count = 0;
              m_timing = true;
              m_timer.start();
    }
              // Call when leaving "learn" mode
    void stop_timing()
    {
              m_timing = false;
              m_timer.stop();
              }
              // Should only record the current time when timing
              // Insert the time at the next free position of m_beats
    void tap()
    {
              //m_timer.read_ms();
              m_beat_count++;
    }
 
    bool is_timing() const { return m_timing; }
              // Calculate the BPM from the deltas between m_beats
              // Return 0 if there are not enough samples
    size_t get_bpm()
    {
              if(m_beat_count > 3)
                           return int((float)(60 * 1000 * m_beat_count)/(float)(m_timer.read_ms() * 1));
              else return 0;
    }
   
    void reset_min_max()
    {
              min_bpm = 0;
              max_bpm = 0;
              pc.printf("reset to %d and %d", min_bpm, max_bpm);
    }
   
    int get_min()
    {
              return min_bpm;
    }
   
    void set_min(int min)
    {
              min_bpm = min;
    }
             
    int get_max()
    {
              return max_bpm;
    }
   
    void set_max(int max)
    {
              max_bpm = max;
              }
             
public:
    int min_bpm, max_bpm;
   
private:
    bool m_timing;
    Timer m_timer;
 
              // Insert new samples at the end of the array, removing the oldest
    size_t m_beats[beat_samples];
    size_t m_beat_count;
};
