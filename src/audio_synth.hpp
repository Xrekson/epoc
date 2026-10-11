#pragma once
#include "raylib.h"
#include <cmath>
#include <vector>
#include <cstdlib>

// ============================================================================
// ZERO-DEPENDENCY PROCEDURAL SOUND SYNTHESIZER
// Generates crisp gunshot, sniper boom, hitmarker, headshot, reload & explosion SFX
// ============================================================================
struct SoundBank {
    Sound sfxRifleShot;
    Sound sfxSniperShot;
    Sound sfxSmgShot;
    Sound sfxShotgunShot;
    Sound sfxSuppressedShot;
    Sound sfxHitmarker;
    Sound sfxHeadshot;
    Sound sfxReloadClick;
    Sound sfxBoltCycle;
    Sound sfxExplosion;
    Sound sfxObjective;
    bool initialized = false;

    static Wave MakeSynthWave(float durationSec, int type) {
        int sampleRate = 44100;
        int frameCount = (int)(durationSec * sampleRate);
        short* data = (short*)MemAlloc(frameCount * sizeof(short));

        float phase = 0.0f;
        float noisePrev = 0.0f;
        for (int i = 0; i < frameCount; ++i) {
            float t = (float)i / (float)sampleRate;
            float progress = (float)i / (float)frameCount;
            float rawNoise = ((float)(std::rand() % 20000) / 10000.0f) - 1.0f;
            float sample = 0.0f;

            if (type == 0) { // Rifle Shot: Punchy low-end thump + filtered crack
                float env = std::exp(-t * 28.0f);
                float freq = 140.0f * std::exp(-t * 22.0f) + 45.0f;
                phase += freq / sampleRate;
                noisePrev = noisePrev * 0.65f + rawNoise * 0.35f;
                sample = (std::sin(phase * 6.28318f) * 0.55f + noisePrev * 0.75f) * env;
            } else if (type == 1) { // Sniper Boom: Deep cannon punch + long metallic tail
                float env = std::exp(-t * 9.5f);
                float freq = 190.0f * std::exp(-t * 18.0f) + 36.0f;
                phase += freq / sampleRate;
                noisePrev = noisePrev * 0.72f + rawNoise * 0.28f;
                sample = (std::sin(phase * 6.28318f) * 0.65f + noisePrev * 0.7f) * env;
            } else if (type == 2) { // SMG Shot: Snappy high-frequency pop
                float env = std::exp(-t * 42.0f);
                float freq = 210.0f * std::exp(-t * 30.0f) + 65.0f;
                phase += freq / sampleRate;
                sample = (std::sin(phase * 6.28318f) * 0.45f + rawNoise * 0.65f) * env;
            } else if (type == 3) { // Shotgun Blast: Wide low-mid boom
                float env = std::exp(-t * 16.0f);
                float freq = 120.0f * std::exp(-t * 15.0f) + 40.0f;
                phase += freq / sampleRate;
                noisePrev = noisePrev * 0.78f + rawNoise * 0.22f;
                sample = (std::sin(phase * 6.28318f) * 0.55f + noisePrev * 0.85f) * env;
            } else if (type == 4) { // Suppressed Pew: Soft air thwip
                float env = std::exp(-t * 48.0f);
                float freq = 320.0f * std::exp(-t * 35.0f) + 90.0f;
                phase += freq / sampleRate;
                noisePrev = noisePrev * 0.82f + rawNoise * 0.18f;
                sample = (std::sin(phase * 6.28318f) * 0.4f + noisePrev * 0.4f) * env;
            } else if (type == 5) { // PF Hitmarker Tick
                float env = std::exp(-t * 90.0f);
                sample = (std::sin(t * 2400.0f * 6.28318f) * 0.5f + rawNoise * 0.5f) * env;
            } else if (type == 6) { // PF Headshot Ding / Kill Chime
                float env = std::exp(-t * 22.0f);
                sample = (std::sin(t * 1850.0f * 6.28318f) * 0.55f + std::sin(t * 2775.0f * 6.28318f) * 0.35f) * env;
            } else if (type == 7) { // Mechanical Reload Click
                float env = (t < 0.03f) ? std::exp(-t * 120.0f) : std::exp(-(t - 0.04f) * 100.0f) * (t >= 0.04f ? 1.0f : 0.0f);
                sample = (std::sin(t * 950.0f * 6.28318f) * 0.5f + rawNoise * 0.5f) * env;
            } else if (type == 8) { // Bolt Cycle
                float env = std::sin(progress * 3.14159f);
                noisePrev = noisePrev * 0.6f + rawNoise * 0.4f;
                sample = noisePrev * env * 0.5f;
            } else if (type == 9) { // Grenade Explosion
                float env = std::exp(-t * 4.5f);
                noisePrev = noisePrev * 0.90f + rawNoise * 0.10f;
                sample = (noisePrev * 0.9f + std::sin(t * 52.0f * 6.28318f) * 0.45f) * env;
            } else { // Objective Capture Chime
                float freq = (t < 0.12f) ? 587.33f : 880.0f;
                float env = std::exp(-t * 8.0f);
                sample = std::sin(t * freq * 6.28318f) * 0.4f * env;
            }

            sample = std::clamp(sample, -1.0f, 1.0f);
            data[i] = (short)(sample * 28000.0f);
        }

        Wave w = { 0 };
        w.frameCount = frameCount;
        w.sampleRate = sampleRate;
        w.sampleSize = 16;
        w.channels = 1;
        w.data = data;
        return w;
    }

    void Init() {
        InitAudioDevice();
        if (!IsAudioDeviceReady()) return;

        auto load = [](float dur, int type, float vol) {
            Wave w = MakeSynthWave(dur, type);
            Sound s = LoadSoundFromWave(w);
            UnloadWave(w);
            SetSoundVolume(s, vol);
            return s;
        };

        sfxRifleShot      = load(0.18f, 0, 0.45f);
        sfxSniperShot     = load(0.45f, 1, 0.60f);
        sfxSmgShot        = load(0.12f, 2, 0.40f);
        sfxShotgunShot    = load(0.28f, 3, 0.55f);
        sfxSuppressedShot = load(0.11f, 4, 0.35f);
        sfxHitmarker      = load(0.05f, 5, 0.55f);
        sfxHeadshot       = load(0.20f, 6, 0.60f);
        sfxReloadClick    = load(0.09f, 7, 0.45f);
        sfxBoltCycle      = load(0.18f, 8, 0.40f);
        sfxExplosion      = load(0.85f, 9, 0.70f);
        sfxObjective      = load(0.35f, 10, 0.50f);
        initialized = true;
    }

    void Play(Sound s, float pitchJitter = 0.0f) {
        if (!initialized) return;
        if (pitchJitter > 0.0f) {
            float p = 1.0f + (((float)(std::rand() % 200) / 100.0f - 1.0f) * pitchJitter);
            SetSoundPitch(s, p);
        } else {
            SetSoundPitch(s, 1.0f);
        }
        PlaySound(s);
    }

    void Cleanup() {
        if (!initialized) return;
        UnloadSound(sfxRifleShot);
        UnloadSound(sfxSniperShot);
        UnloadSound(sfxSmgShot);
        UnloadSound(sfxShotgunShot);
        UnloadSound(sfxSuppressedShot);
        UnloadSound(sfxHitmarker);
        UnloadSound(sfxHeadshot);
        UnloadSound(sfxReloadClick);
        UnloadSound(sfxBoltCycle);
        UnloadSound(sfxExplosion);
        UnloadSound(sfxObjective);
        CloseAudioDevice();
    }
};

