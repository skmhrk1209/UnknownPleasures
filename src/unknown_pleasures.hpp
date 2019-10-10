#pragma once

#include "ofMain.h"
#include <random>

class UnknownPleasures : public ofBaseApp
{
public:
	UnknownPleasures(int, int);

	void setup() override;
	void update() override;
	void draw() override;

	const int mWidth;
	const int mDepth;

	ofTrueTypeFont mLargeFont;
	ofTrueTypeFont mSmallFont;
	ofEasyCam mCamera;
	std::unordered_map<std::string, float> mParams;
	std::vector<ofPolyline> mStormyWaves;
	std::vector<ofPolyline> mCalmWaves;
	std::vector<std::deque<float>> mWaveQueues;
	std::random_device mSeed;
	std::mt19937 mEngine;
};

UnknownPleasures::UnknownPleasures(int width, int depth)
	: mWidth(width), mDepth(depth), mEngine(mSeed()) {}

void UnknownPleasures::setup()
{
	mParams = {{"dy", 2.0},
			   {"dx", 0.5},
			   {"largeFontSize", 50},
			   {"smallFontSize", 25},
			   {"waveOccurProb", 0.02},
			   {"bigWaveWidthMean", 10.0},
			   {"bigWaveWidthStd", 10.0},
			   {"bigWaveHeightMean", 10.0},
			   {"bigWaveHeightStd", 10.0},
			   {"smallWaveHeightMean", 0.0},
			   {"smallWaveHeightStd", 5.0},
			   {"waveMomentum", 0.01},
			   {"perlinNoiseMagnitude", 0.1},
			   {"calmWaveLevel", 3.0},
			   {"cameraPositionX", 0.0},
			   {"cameraPositionY", 200.0},
			   {"cameraPositionZ", 300.0},
			   {"cameraTargetX", 0.0},
			   {"cameraTargetY", 0.0},
			   {"cameraTargetZ", 0.0}};

	for (float z = -(mDepth >> 1); z < +(mDepth >> 1); z += mParams["dy"])
	{
		mStormyWaves.emplace_back();
		mCalmWaves.emplace_back();
		mWaveQueues.emplace_back();

		for (float x = -(mWidth >> 1); x < (mWidth >> 1); x += mParams["dx"])
		{
			mStormyWaves.back().addVertex(x, 0.0, z);
			mCalmWaves.back().addVertex(x, 0.0, z);
		}
	}

	ofBackground(0);
	mLargeFont.load("Helvetica", mParams["largeFontSize"]);
	mSmallFont.load("Helvetica", mParams["smallFontSize"]);
	mCamera.setAutoDistance(false);
	mCamera.setPosition(ofPoint(mParams["cameraPositionX"], mParams["cameraPositionY"], mParams["cameraPositionZ"]));
	mCamera.setTarget(ofPoint(mParams["cameraTargetX"], mParams["cameraTargetY"], mParams["cameraTargetZ"]));
}

void UnknownPleasures::update()
{
	auto t = ofGetElapsedTimef();

	// Wave occurrence.
	for (auto &waveQueue : mWaveQueues)
	{
		if (std::bernoulli_distribution(mParams["waveOccurProb"])(mEngine))
		{
			// Sample from truncated normal distribution
			auto waveWidthDist = std::normal_distribution<>(mParams["bigWaveWidthMean"], mParams["bigWaveWidthStd"]);
			auto waveWidth = waveWidthDist(mEngine);
			while (waveWidth < 0)
			{
				waveWidth = waveWidthDist(mEngine);
			}

			// Sample from truncated normal distribution.
			auto waveHeightDist = std::normal_distribution<>(mParams["bigWaveHeightMean"], mParams["bigWaveHeightStd"]);
			auto waveHeight = waveHeightDist(mEngine);
			while (waveHeight < 0)
			{
				waveHeight = waveHeightDist(mEngine);
			}

			auto prevWaveQueue = std::move(waveQueue);
			waveQueue.clear();

			// Push triangle wave to queue.
			for (auto i = 0; i < waveWidth; ++i)
			{
				waveQueue.emplace_back(waveHeight / waveWidth * i);
			}
			waveQueue.emplace_back(waveHeight);
			for (auto i = 0; i < waveWidth; ++i)
			{
				waveQueue.emplace_back(waveHeight / waveWidth * (waveWidth - i));
			}

			// Wave interference.
			for (auto i = 0; i < prevWaveQueue.size(); ++i)
			{
				waveQueue[i] += prevWaveQueue[i];
			}
		}
	}

	for (int i = 0; i < mStormyWaves.size(); ++i)
	{
		for (int j = mStormyWaves[i].size() - 1; j >= 0; --j)
		{
			// Wave occurs at only left edge.
			if (j == 0)
			{
				// Small wave always occurs.
				auto waveHeightDist = std::uniform_real_distribution<>(mParams["smallWaveHeightMean"], mParams["smallWaveHeightStd"]);
				mStormyWaves[i][j].y = waveHeightDist(mEngine);
				// When big wave coming.
				if (!mWaveQueues[i].empty())
				{
					mStormyWaves[i][j].y += mWaveQueues[i].front();
					mWaveQueues[i].pop_front();
				}
			}
			// Wave shifts from left to right except left edge.
			else
			{
				// Smoothing using exponential moving average.
				mStormyWaves[i][j].y = mStormyWaves[i][j].y * mParams["waveMomentum"] + mStormyWaves[i][j - 1].y * (1 - mParams["waveMomentum"]);
				// Apply perlin noise.
				mStormyWaves[i][j].y += ofSignedNoise(mStormyWaves[i][j].x, mStormyWaves[i][j].z, t) * mParams["perlinNoiseMagnitude"];
			}
			// Calm down stormy wave.
			mCalmWaves[i][j].y = mStormyWaves[i][j].y * pow((cos(mStormyWaves[i][j].x * M_PI / (mWidth >> 1)) + 1) * 0.5, mParams["calmWaveLevel"]);
		}
	}
}

void UnknownPleasures::draw()
{
	ofSetColor(255);
	ofDisableDepthTest();
	mLargeFont.drawString("JOY DIVISION", 300, 200);
	mSmallFont.drawString("UNKNOWN PLEASURES", 300, 700);
	ofEnableDepthTest();
	mCamera.begin();
	for (const auto &calmWave : mCalmWaves)
	{
		calmWave.draw();
	}
	mCamera.end();
}
